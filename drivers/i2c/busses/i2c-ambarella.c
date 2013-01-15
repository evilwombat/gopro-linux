/*
 * drivers/i2c/busses/ambarella_i2c.c
 *
 * Anthony Ginger, <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/i2c-id.h>

#include <asm/dma.h>

#include <mach/hardware.h>
#include <plat/idc.h>

#ifndef CONFIG_I2C_AMBARELLA_RETRIES
#define CONFIG_I2C_AMBARELLA_RETRIES		(3)
#endif

#ifndef CONFIG_I2C_AMBARELLA_ACK_TIMEOUT
#define CONFIG_I2C_AMBARELLA_ACK_TIMEOUT	(1 * HZ)
#endif

#define CONFIG_I2C_AMBARELLA_BULK_RETRY_NUM	(4)

enum ambarella_i2c_state {
	AMBA_I2C_STATE_IDLE,
	AMBA_I2C_STATE_START,
	AMBA_I2C_STATE_START_TEN,
	AMBA_I2C_STATE_START_NEW,
	AMBA_I2C_STATE_READ,
	AMBA_I2C_STATE_READ_STOP,
	AMBA_I2C_STATE_WRITE,
	AMBA_I2C_STATE_WRITE_WAIT_ACK,
	AMBA_I2C_STATE_BULK_WRITE,
	AMBA_I2C_STATE_NO_ACK,
	AMBA_I2C_STATE_ERROR
};

struct ambarella_i2c_dev_info {
	unsigned char __iomem 			*regbase;

	struct device				*dev;
	struct resource				*mem;
	unsigned int				irq;
	struct i2c_adapter			adap;

	enum ambarella_i2c_state		state;

	struct i2c_msg				*msgs;
	__u16					msg_num;
	__u16					msg_addr;
	wait_queue_head_t			msg_wait;
	unsigned int				msg_index;

	struct ambarella_idc_platform_info	*platform_info;

	struct notifier_block			system_event;
	struct semaphore			system_event_sem;
};

static inline void ambarella_i2c_set_clk(struct ambarella_i2c_dev_info *pinfo)
{
	unsigned int				apb_clk;
	__u32					idc_prescale;

	apb_clk = pinfo->platform_info->get_clock();

	amba_writel(pinfo->regbase + IDC_ENR_OFFSET, IDC_ENR_REG_DISABLE);

	if ((pinfo->platform_info->clk_limit < 1000) ||
		(pinfo->platform_info->clk_limit > 400000))
		pinfo->platform_info->clk_limit = 100000;

	idc_prescale = (apb_clk / pinfo->platform_info->clk_limit) >> 2;

	dev_dbg(pinfo->dev, "apb_clk[%dHz]\n", apb_clk);
	dev_dbg(pinfo->dev, "idc_prescale[%d]\n", idc_prescale);
	dev_dbg(pinfo->dev, "clk[%dHz]\n",
		(apb_clk / ((idc_prescale + 1) << 2)));

	amba_writeb(pinfo->regbase + IDC_PSLL_OFFSET,
		(idc_prescale & 0xff));
	amba_writeb(pinfo->regbase + IDC_PSLH_OFFSET,
		((idc_prescale & 0xff00) >> 8));

	amba_writel(pinfo->regbase + IDC_ENR_OFFSET, IDC_ENR_REG_ENABLE);
}

static int ambarella_i2c_system_event(struct notifier_block *nb,
	unsigned long val, void *data)
{
	int					errorCode = NOTIFY_OK;
	struct platform_device			*pdev;
	struct ambarella_i2c_dev_info		*pinfo;

	pinfo = container_of(nb, struct ambarella_i2c_dev_info, system_event);
	pdev = to_platform_device(pinfo->dev);

	switch (val) {
	case AMBA_EVENT_PRE_CPUFREQ:
		pr_debug("%s[%d]: Pre Change\n", __func__, pdev->id);
		down(&pinfo->system_event_sem);
		break;

	case AMBA_EVENT_POST_CPUFREQ:
		pr_debug("%s[%d]: Post Change\n", __func__, pdev->id);
		ambarella_i2c_set_clk(pinfo);
		up(&pinfo->system_event_sem);
		break;

	default:
		break;
	}

	return errorCode;
}

static inline void ambarella_i2c_hw_init(struct ambarella_i2c_dev_info *pinfo)
{
	ambarella_i2c_set_clk(pinfo);

	pinfo->msgs = NULL;
	pinfo->msg_num = 0;
	pinfo->state = AMBA_I2C_STATE_IDLE;
}

static inline void ambarella_i2c_start_single_msg(
	struct ambarella_i2c_dev_info *pinfo)
{
	if (pinfo->msgs->flags & I2C_M_TEN) {
		pinfo->state = AMBA_I2C_STATE_START_TEN;
		amba_writeb(pinfo->regbase + IDC_DATA_OFFSET,
			(0xf0 | ((pinfo->msg_addr >> 8) & 0x07)));
	} else {
		pinfo->state = AMBA_I2C_STATE_START;
		amba_writeb(pinfo->regbase + IDC_DATA_OFFSET,
			(pinfo->msg_addr & 0xff));
	}

	amba_writel(pinfo->regbase + IDC_CTRL_OFFSET, IDC_CTRL_START);
}

static inline void ambarella_i2c_bulk_write(
	struct ambarella_i2c_dev_info *pinfo,
	__u32 fifosize)
{
	do {
		amba_writeb(pinfo->regbase + IDC_FMDATA_OFFSET,
			pinfo->msgs->buf[pinfo->msg_index]);
		pinfo->msg_index++;
		fifosize--;

		if (pinfo->msg_index >= pinfo->msgs->len) {
			amba_writel(pinfo->regbase + IDC_FMCTRL_OFFSET,
				IDC_FMCTRL_IF | IDC_FMCTRL_STOP);

			return;
		}
	} while (fifosize > 1);

	amba_writel(pinfo->regbase + IDC_FMCTRL_OFFSET, IDC_FMCTRL_IF);
}

static inline void ambarella_i2c_start_bulk_msg_write(
	struct ambarella_i2c_dev_info *pinfo)
{
	__u32				fifosize = IDC_FIFO_BUF_SIZE;

	pinfo->state = AMBA_I2C_STATE_BULK_WRITE;

	amba_writel(pinfo->regbase + IDC_FMCTRL_OFFSET, IDC_FMCTRL_START);

	if (pinfo->msgs->flags & I2C_M_TEN) {
		amba_writeb(pinfo->regbase + IDC_FMDATA_OFFSET,
			(0xf0 | ((pinfo->msg_addr >> 8) & 0x07)));
		fifosize--;
	}
	amba_writeb(pinfo->regbase + IDC_FMDATA_OFFSET,
		(pinfo->msg_addr & 0xff));
	fifosize -= 2;

	ambarella_i2c_bulk_write(pinfo, fifosize);
}

static inline void ambarella_i2c_start_current_msg(
	struct ambarella_i2c_dev_info *pinfo)
{
	pinfo->msg_index = 0;
	pinfo->msg_addr = (pinfo->msgs->addr << 1);

	if (pinfo->msgs->flags & I2C_M_RD)
		pinfo->msg_addr |= 1;

	if (pinfo->msgs->flags & I2C_M_REV_DIR_ADDR)
		pinfo->msg_addr ^= 1;

	if (pinfo->msgs->flags & I2C_M_RD) {
		ambarella_i2c_start_single_msg(pinfo);
	} else if (pinfo->msgs->len > pinfo->platform_info->bulk_write_num) {
		ambarella_i2c_start_bulk_msg_write(pinfo);
	} else {
		ambarella_i2c_start_single_msg(pinfo);
	}

	dev_dbg(pinfo->dev, "msg_addr[0x%x], len[0x%x]",
		pinfo->msg_addr, pinfo->msgs->len);
}

static inline void ambarella_i2c_stop(
	struct ambarella_i2c_dev_info *pinfo,
	enum ambarella_i2c_state state,
	__u32 *pack_control)
{
	if(state != AMBA_I2C_STATE_IDLE) {
		dev_warn(pinfo->dev, "ambarella_i2c_stop[%d] from %d to %d\n",
			pinfo->msg_num, pinfo->state, state);
		*pack_control |= IDC_CTRL_ACK;
	}

	pinfo->state = state;
	pinfo->msgs = NULL;
	pinfo->msg_num = 0;

	*pack_control |= IDC_CTRL_STOP;

	if(pinfo->state == AMBA_I2C_STATE_IDLE)
		wake_up(&pinfo->msg_wait);
}

static inline __u32 ambarella_i2c_check_ack(
	struct ambarella_i2c_dev_info *pinfo,
	__u32 *pack_control,
	__u32 retry_counter)
{
	__u32				retVal = IDC_CTRL_ACK;

ambarella_i2c_check_ack_enter:
	if (unlikely((*pack_control) & IDC_CTRL_ACK)) {
		if (pinfo->msgs->flags & I2C_M_IGNORE_NAK)
			goto ambarella_i2c_check_ack_exit;

		if ((pinfo->msgs->flags & I2C_M_RD) &&
			(pinfo->msgs->flags & I2C_M_NO_RD_ACK))
			goto ambarella_i2c_check_ack_exit;

		if (retry_counter--) {
			udelay(100);
			*pack_control = amba_readl(pinfo->regbase
				+ IDC_CTRL_OFFSET);
			goto ambarella_i2c_check_ack_enter;
		}
		retVal = 0;
		ambarella_i2c_stop(pinfo,
			AMBA_I2C_STATE_NO_ACK, pack_control);
	}

ambarella_i2c_check_ack_exit:
	return retVal;
}

static irqreturn_t ambarella_i2c_irq(int irqno, void *dev_id)
{
	struct ambarella_i2c_dev_info	*pinfo;
	__u32				status_reg;
	__u32				control_reg;
	__u32				ack_control = IDC_CTRL_CLS;

	pinfo = (struct ambarella_i2c_dev_info *)dev_id;

	status_reg = amba_readl(pinfo->regbase + IDC_STS_OFFSET);
	control_reg = amba_readl(pinfo->regbase + IDC_CTRL_OFFSET);

	dev_dbg(pinfo->dev, "state[0x%x]\n", pinfo->state);
	dev_dbg(pinfo->dev, "status_reg[0x%x]\n", status_reg);
	dev_dbg(pinfo->dev, "control_reg[0x%x]\n", control_reg);

	switch (pinfo->state) {
	case AMBA_I2C_STATE_START:
		if (ambarella_i2c_check_ack(pinfo, &control_reg,
			0) == IDC_CTRL_ACK) {
			if (pinfo->msgs->flags & I2C_M_RD) {
				if (pinfo->msgs->len == 1)
					ack_control |= IDC_CTRL_ACK;
				pinfo->state = AMBA_I2C_STATE_READ;
			} else {
				pinfo->state = AMBA_I2C_STATE_WRITE;
				goto amba_i2c_irq_write;
			}
		}
		break;
	case AMBA_I2C_STATE_START_TEN:
		pinfo->state = AMBA_I2C_STATE_START;
		amba_writeb(pinfo->regbase + IDC_DATA_OFFSET,
			(pinfo->msg_addr & 0xff));
		break;
	case AMBA_I2C_STATE_START_NEW:
amba_i2c_irq_start_new:
		ambarella_i2c_start_current_msg(pinfo);
		goto amba_i2c_irq_exit;
		break;
	case AMBA_I2C_STATE_READ_STOP:
		pinfo->msgs->buf[pinfo->msg_index] =
			amba_readb(pinfo->regbase + IDC_DATA_OFFSET);
		pinfo->msg_index++;
amba_i2c_irq_read_stop:
		ambarella_i2c_stop(pinfo, AMBA_I2C_STATE_IDLE, &ack_control);
		break;
	case AMBA_I2C_STATE_READ:
		pinfo->msgs->buf[pinfo->msg_index] =
			amba_readb(pinfo->regbase + IDC_DATA_OFFSET);
		pinfo->msg_index++;

		if (pinfo->msg_index >= pinfo->msgs->len - 1) {
			if (pinfo->msg_num > 1) {
				pinfo->msgs++;
				pinfo->state = AMBA_I2C_STATE_START_NEW;
				pinfo->msg_num--;
			} else {
				if (pinfo->msg_index > pinfo->msgs->len - 1) {
					goto amba_i2c_irq_read_stop;
				} else {
					pinfo->state = AMBA_I2C_STATE_READ_STOP;
					ack_control |= IDC_CTRL_ACK;
				}
			}
		}
		break;
	case AMBA_I2C_STATE_WRITE:
amba_i2c_irq_write:
		pinfo->state = AMBA_I2C_STATE_WRITE_WAIT_ACK;
		amba_writeb(pinfo->regbase + IDC_DATA_OFFSET,
			pinfo->msgs->buf[pinfo->msg_index]);
		break;
	case AMBA_I2C_STATE_WRITE_WAIT_ACK:
		if (ambarella_i2c_check_ack(pinfo, &control_reg,
			0) == IDC_CTRL_ACK) {
			pinfo->state = AMBA_I2C_STATE_WRITE;
			pinfo->msg_index++;

			if (pinfo->msg_index >= pinfo->msgs->len) {
				if (pinfo->msg_num > 1) {
					pinfo->msgs++;
					pinfo->state = AMBA_I2C_STATE_START_NEW;
					pinfo->msg_num--;
					goto amba_i2c_irq_start_new;
				}
				ambarella_i2c_stop(pinfo,
					AMBA_I2C_STATE_IDLE, &ack_control);
			} else {
				goto amba_i2c_irq_write;
			}
		}
		break;
	case AMBA_I2C_STATE_BULK_WRITE:
		if (ambarella_i2c_check_ack(pinfo, &control_reg,
			CONFIG_I2C_AMBARELLA_BULK_RETRY_NUM) ==	IDC_CTRL_ACK) {
			amba_writel(pinfo->regbase + IDC_CTRL_OFFSET,
				IDC_CTRL_ACK);
			if (pinfo->msg_index >= pinfo->msgs->len) {
				if (pinfo->msg_num > 1) {
					pinfo->msgs++;
					pinfo->state = AMBA_I2C_STATE_START_NEW;
					pinfo->msg_num--;
					goto amba_i2c_irq_start_new;
				}
				ambarella_i2c_stop(pinfo,
					AMBA_I2C_STATE_IDLE, &ack_control);
			} else {
				ambarella_i2c_bulk_write(pinfo,
					IDC_FIFO_BUF_SIZE);
			}
			goto amba_i2c_irq_exit;
		} else {
			ack_control = IDC_CTRL_ACK;
		}
		break;
	default:
		dev_err(pinfo->dev, "ambarella_i2c_irq in wrong state[0x%x]\n",
			pinfo->state);
		dev_err(pinfo->dev, "status_reg[0x%x]\n", status_reg);
		dev_err(pinfo->dev, "control_reg[0x%x]\n", control_reg);
		ack_control = IDC_CTRL_STOP | IDC_CTRL_ACK;
		pinfo->state = AMBA_I2C_STATE_ERROR;
		break;
	}

	amba_writel(pinfo->regbase + IDC_CTRL_OFFSET, ack_control);

amba_i2c_irq_exit:
	return IRQ_HANDLED;
}

static int ambarella_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
	int num)
{
	struct ambarella_i2c_dev_info	*pinfo;
	int				errorCode = -EPERM;
	int				retryCount;
	long				timeout;

	pinfo = (struct ambarella_i2c_dev_info *)i2c_get_adapdata(adap);

	down(&pinfo->system_event_sem);
#if defined(CONFIG_AMBARELLA_IPC)
	enable_irq(pinfo->irq);
#endif
	for (retryCount = 0; retryCount < adap->retries; retryCount++) {
		errorCode = 0;
		if (pinfo->state != AMBA_I2C_STATE_IDLE)
			ambarella_i2c_hw_init(pinfo);
		pinfo->msgs = msgs;
		pinfo->msg_num = num;

		ambarella_i2c_start_current_msg(pinfo);
		timeout = wait_event_timeout(pinfo->msg_wait,
			pinfo->msg_num == 0, CONFIG_I2C_AMBARELLA_ACK_TIMEOUT);
		if (timeout <= 0) {
			pinfo->state = AMBA_I2C_STATE_NO_ACK;
			dev_err(pinfo->dev,
				"No ACK from address 0x%x, %d:%d!\n",
				pinfo->msg_addr, pinfo->msg_num,
				pinfo->msg_index);
		}
		dev_dbg(pinfo->dev, "%ld jiffies left.\n", timeout);

		if (pinfo->state != AMBA_I2C_STATE_IDLE) {
			dev_err(pinfo->dev,
				"I2C state 0x%x, please check address 0x%x!\n",
				pinfo->state, pinfo->msg_addr);
			errorCode = -EBUSY;
		} else {
			break;
		}
	}
#if defined(CONFIG_AMBARELLA_IPC)
	disable_irq(pinfo->irq);
#endif
	up(&pinfo->system_event_sem);

	if (errorCode)
		return errorCode;

	return num;
}

static u32 ambarella_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm ambarella_i2c_algo = {
	.master_xfer	= ambarella_i2c_xfer,
	.functionality	= ambarella_i2c_func,
};

static int __devinit ambarella_i2c_probe(struct platform_device *pdev)
{
	int					errorCode;
	struct ambarella_i2c_dev_info		*pinfo;
	struct i2c_adapter			*adap;
	struct resource				*irq;
	struct resource				*mem;
	struct resource				*ioarea;
	struct ambarella_idc_platform_info	*platform_info;

	platform_info =
		(struct ambarella_idc_platform_info *)pdev->dev.platform_data;
	if (platform_info == NULL) {
		dev_err(&pdev->dev, "%s: Can't get platform_data!\n", __func__);
		errorCode = - EPERM;
		goto i2c_errorCode_na;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem == NULL) {
		dev_err(&pdev->dev, "Get I2C mem resource failed!\n");
		errorCode = -ENXIO;
		goto i2c_errorCode_na;
	}

	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (irq == NULL) {
		dev_err(&pdev->dev, "Get I2C irq resource failed!\n");
		errorCode = -ENXIO;
		goto i2c_errorCode_na;
	}

	ioarea = request_mem_region(mem->start,
			(mem->end - mem->start) + 1, pdev->name);
	if (ioarea == NULL) {
		dev_err(&pdev->dev, "Request I2C ioarea failed!\n");
		errorCode = -EBUSY;
		goto i2c_errorCode_na;
	}

	pinfo = kzalloc(sizeof(struct ambarella_i2c_dev_info), GFP_KERNEL);
	if (pinfo == NULL) {
		dev_err(&pdev->dev, "Out of memory!\n");
		errorCode = -ENOMEM;
		goto i2c_errorCode_ioarea;
	}

	pinfo->regbase = (unsigned char __iomem *)mem->start;
	pinfo->mem = mem;
	pinfo->dev = &pdev->dev;
	pinfo->irq = irq->start;
	init_waitqueue_head(&pinfo->msg_wait);
	sema_init(&pinfo->system_event_sem, 1);
	pinfo->platform_info = platform_info;

	ambarella_i2c_hw_init(pinfo);

	platform_set_drvdata(pdev, pinfo);

	errorCode = request_irq(pinfo->irq,
		ambarella_i2c_irq, IRQF_TRIGGER_HIGH,
		dev_name(&pdev->dev), pinfo);
	if (errorCode) {
		dev_err(&pdev->dev, "%s: Request IRQ failed!\n", __func__);
		goto i2c_errorCode_kzalloc;
	}
#if defined(CONFIG_AMBARELLA_IPC)
	disable_irq(pinfo->irq);
#endif
	adap = &pinfo->adap;
	i2c_set_adapdata(adap, pinfo);
	adap->owner = THIS_MODULE;
	adap->class = platform_info->i2c_class;
	strlcpy(adap->name, pdev->name, sizeof(adap->name));
	adap->algo = &ambarella_i2c_algo;
	adap->dev.parent = &pdev->dev;
	adap->nr = pdev->id;
	adap->retries = CONFIG_I2C_AMBARELLA_RETRIES;
#if defined(CONFIG_AMBARELLA_IPC)
	adap->ipc_mutex_id = platform_info->ipc_mutex_id;
#endif
	errorCode = i2c_add_numbered_adapter(adap);
	if (errorCode) {
		dev_err(&pdev->dev,
			"%s: Adding I2C adapter failed!\n", __func__);
		goto i2c_errorCode_free_irq;
	}

	pinfo->system_event.notifier_call = ambarella_i2c_system_event;
	ambarella_register_event_notifier(&pinfo->system_event);

	dev_notice(&pdev->dev,
		"Ambarella Media Processor I2C adapter[%s] probed!\n",
		dev_name(&pinfo->adap.dev));

	goto i2c_errorCode_na;

i2c_errorCode_free_irq:
	free_irq(pinfo->irq, pinfo);

i2c_errorCode_kzalloc:
	platform_set_drvdata(pdev, NULL);
	kfree(pinfo);

i2c_errorCode_ioarea:
	release_mem_region(mem->start, (mem->end - mem->start) + 1);

i2c_errorCode_na:
	return errorCode;
}

static int __devexit ambarella_i2c_remove(struct platform_device *pdev)
{
	struct ambarella_i2c_dev_info	*pinfo;
	int				errorCode = 0;

	pinfo = platform_get_drvdata(pdev);

	if (pinfo) {
		ambarella_unregister_event_notifier(&pinfo->system_event);

		errorCode = i2c_del_adapter(&pinfo->adap);

		free_irq(pinfo->irq, pinfo);

		platform_set_drvdata(pdev, NULL);

		release_mem_region(pinfo->mem->start,
			(pinfo->mem->end - pinfo->mem->start) + 1);

		kfree(pinfo);
	}

	dev_notice(&pdev->dev,
		"Remove Ambarella Media Processor I2C adapter[%s] [%d].\n",
		dev_name(&pinfo->adap.dev), errorCode);

	return errorCode;
}

#ifdef CONFIG_PM
static int ambarella_i2c_suspend_noirq(struct device *dev)
{
	int				errorCode = 0;
	struct platform_device		*pdev;
	struct ambarella_i2c_dev_info	*pinfo;

	pdev = to_platform_device(dev);
	pinfo = platform_get_drvdata(pdev);
	if (pinfo) {
		down(&pinfo->system_event_sem);
		disable_irq(pinfo->irq);
	}

	dev_dbg(&pdev->dev, "%s\n", __func__);

	return errorCode;
}

static int ambarella_i2c_resume_noirq(struct device *dev)
{
	int				errorCode = 0;
	struct platform_device		*pdev;
	struct ambarella_i2c_dev_info	*pinfo;

	pdev = to_platform_device(dev);
	pinfo = platform_get_drvdata(pdev);
	if (pinfo) {
		ambarella_i2c_hw_init(pinfo);
		enable_irq(pinfo->irq);
		up(&pinfo->system_event_sem);
	}

	dev_dbg(&pdev->dev, "%s\n", __func__);

	return errorCode;
}

static const struct dev_pm_ops ambarella_i2c_dev_pm_ops = {
	.suspend_noirq = ambarella_i2c_suspend_noirq,
	.resume_noirq = ambarella_i2c_resume_noirq,
};
#endif

static const char ambarella_i2c_name[] = "ambarella-i2c";

static struct platform_driver ambarella_i2c_driver = {
	.probe		= ambarella_i2c_probe,
	.remove		= __devexit_p(ambarella_i2c_remove),
	.driver		= {
		.name	= ambarella_i2c_name,
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &ambarella_i2c_dev_pm_ops,
#endif
	},
};

static int __init ambarella_i2c_init(void)
{
	return platform_driver_register(&ambarella_i2c_driver);
}

static void __exit ambarella_i2c_exit(void)
{
	platform_driver_unregister(&ambarella_i2c_driver);
}

subsys_initcall(ambarella_i2c_init);
module_exit(ambarella_i2c_exit);

MODULE_DESCRIPTION("Ambarella Media Processor I2C Bus Controller");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("GPL");

