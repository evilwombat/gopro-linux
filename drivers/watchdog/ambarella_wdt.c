/*
 * linux/drivers/mmc/host/ambarella_sd.c
 *
 * Copyright (C) 2006-2007, Ambarella, Inc.
 *  Anthony Ginger, <hfjiang@ambarella.com>
 *
 * Ambarella Media Processor Watch Dog Timer
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/slab.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#include <plat/wdt.h>

#define CONFIG_WDT_AMBARELLA_TIMEOUT		(15)

static int init_tmo = CONFIG_WDT_AMBARELLA_TIMEOUT;
module_param(init_tmo, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init_tmo, "Watchdog timeout in seconds. default=" \
__MODULE_STRING(CONFIG_WDT_AMBARELLA_TIMEOUT) ")");

static int init_mode = WDOG_CTR_RST_EN;	//WDOG_CTR_INT_EN | WDOG_CTR_RST_EN
module_param(init_mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init_mode, "Watchdog mode: 0x2=reset, 0x4=irq");

enum ambarella_wdt_state {
	AMBA_WDT_CLOSE_STATE_DISABLE,
	AMBA_WDT_CLOSE_STATE_ALLOW,
};
struct ambarella_wdt_info {
	unsigned char __iomem 		*regbase;

	struct device			*dev;
	struct resource			*mem;
	unsigned int			irq;

	struct semaphore		wdt_mutex;

	enum ambarella_wdt_state	state;

	u32				tmo;
	u32				boot_tmo;

	struct miscdevice		wdt_dev;
	struct ambarella_wdt_controller	*pcontroller;

	u32				ctl_reg;
	u32				init_mode;
	u32				act_timeout;
};
static struct ambarella_wdt_info *pwdtinfo = NULL;

static void ambarella_wdt_keepalive(struct ambarella_wdt_info *pinfo)
{
	amba_writel(pinfo->regbase + WDOG_RELOAD_OFFSET, pinfo->tmo);
	amba_writel(pinfo->regbase + WDOG_RESTART_OFFSET, WDT_RESTART_VAL);
}

static void ambarella_wdt_stop(struct ambarella_wdt_info *pinfo)
{
	amba_writel(pinfo->regbase + WDOG_CONTROL_OFFSET, 0);
	while(amba_readl(pinfo->regbase + WDOG_CONTROL_OFFSET) != 0);
}

static void ambarella_wdt_start(struct ambarella_wdt_info *pinfo, u32 ctl_reg)
{
	if (!ctl_reg)
		ctl_reg = pinfo->init_mode | WDOG_CTR_EN;
	pinfo->pcontroller->start(ctl_reg);
}

static int ambarella_wdt_set_heartbeat(struct ambarella_wdt_info *pinfo,
	u32 timeout)
{
	int					errorCode = 0;
	u32					freq;
	u32					max_tmo;

	freq = pinfo->pcontroller->get_pll();
	if (freq)
		max_tmo = 0xFFFFFFFF / freq;
	else {
		dev_err(pinfo->dev, "freq == 0 !\n");
		errorCode = -EPERM;
		goto ambarella_wdt_set_heartbeat_exit;
	}

	if (timeout > max_tmo) {
		dev_err(pinfo->dev, "max_tmo is %d, not %d.\n",
			max_tmo, timeout);
		errorCode = -EINVAL;
		goto ambarella_wdt_set_heartbeat_exit;
	}

	pinfo->tmo = timeout * freq;
	pinfo->act_timeout = timeout;
	ambarella_wdt_keepalive(pinfo);

ambarella_wdt_set_heartbeat_exit:
	return 0;
}

static int ambarella_wdt_open(struct inode *inode, struct file *file)
{
	int					errorCode = -EBUSY;
	struct ambarella_wdt_info		*pinfo;

	pinfo = pwdtinfo;

	if (pinfo) {
		file->private_data = pinfo;

		if(down_trylock(&pinfo->wdt_mutex)) {
			errorCode = -EBUSY;
			goto ambarella_wdt_open_exit;
		}

		pinfo->state = AMBA_WDT_CLOSE_STATE_DISABLE;

		ambarella_wdt_stop(pinfo);
		ambarella_wdt_keepalive(pinfo);
		ambarella_wdt_start(pinfo, 0);
		errorCode = nonseekable_open(inode, file);
	}

ambarella_wdt_open_exit:
	return errorCode;
}

static int ambarella_wdt_release(struct inode *inode, struct file *file)
{
	int					errorCode = -EBUSY;
	struct ambarella_wdt_info		*pinfo;

	pinfo = (struct ambarella_wdt_info *)file->private_data;

	if (pinfo) {
		if (pinfo->state == AMBA_WDT_CLOSE_STATE_ALLOW) {
			ambarella_wdt_stop(pinfo);
		} else {
			dev_notice(pinfo->dev,
				"Not stopping watchdog, V first!\n");
			ambarella_wdt_keepalive(pinfo);
		}

		pinfo->state = AMBA_WDT_CLOSE_STATE_DISABLE;
		up(&pinfo->wdt_mutex);
		errorCode = 0;
	}

	return errorCode;
}

static ssize_t ambarella_wdt_write(struct file *file, const char __user *data,
	size_t len, loff_t *ppos)
{
	int					errorCode = -EBUSY;
	struct ambarella_wdt_info		*pinfo;
	size_t					i;
	char					c;

	pinfo = (struct ambarella_wdt_info *)file->private_data;

	if (pinfo && len) {
		for (i = 0; i < len; i++) {
			if (get_user(c, data + i)) {
				errorCode = -EFAULT;
				goto ambarella_wdt_write_exit;
			}

			if (c == 'V')
				pinfo->state = AMBA_WDT_CLOSE_STATE_ALLOW;
		}

		ambarella_wdt_keepalive(pinfo);
		errorCode = len;
	}

ambarella_wdt_write_exit:
	return errorCode;
}

static const struct watchdog_info ambarella_wdt_ident = {
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
	.firmware_version = 0,
	.identity = "ambarella-wdt",
};

static long ambarella_wdt_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg)
{
	int					errorCode = -EBUSY;
	struct ambarella_wdt_info		*pinfo;
	void __user				*argp = (void __user *)arg;
	u32 __user				*p = argp;
	u32					new_tmo;
	int					options;

	pinfo = (struct ambarella_wdt_info *)file->private_data;

	if (pinfo) {
		switch (cmd) {
		case WDIOC_GETSUPPORT:
			errorCode = copy_to_user(argp, &ambarella_wdt_ident,
				sizeof(ambarella_wdt_ident)) ? -EFAULT : 0;
			break;

		case WDIOC_GETSTATUS:
			errorCode = put_user(0, p);
			break;

		case WDIOC_GETBOOTSTATUS:
			errorCode = put_user(pinfo->boot_tmo, p);
			break;

		case WDIOC_KEEPALIVE:
			ambarella_wdt_keepalive(pinfo);
			errorCode = 0;
			break;

		case WDIOC_SETTIMEOUT:
			if (get_user(new_tmo, p)) {
				errorCode = -EFAULT;
				break;
			}
			errorCode = ambarella_wdt_set_heartbeat(pinfo, new_tmo);
			if (errorCode)
				break;
			ambarella_wdt_keepalive(pinfo);
			errorCode = put_user(pinfo->tmo, p);
			break;

		case WDIOC_GETTIMEOUT:
			errorCode = put_user(pinfo->tmo, p);
			break;

		case WDIOC_SETOPTIONS:
			if (get_user(options, p)) {
				errorCode = -EFAULT;
				break;
			}
			if (options & WDIOS_DISABLECARD) {
				ambarella_wdt_stop(pinfo);
				errorCode = 0;
			}
			if (options & WDIOS_ENABLECARD) {
				ambarella_wdt_keepalive(pinfo);
				errorCode = 0;
			}
			break;

		default:
			errorCode = -ENOTTY;
			break;
		}
	}

	return errorCode;
}

static const struct file_operations ambarella_wdt_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.write		= ambarella_wdt_write,
	.unlocked_ioctl	= ambarella_wdt_ioctl,
	.open		= ambarella_wdt_open,
	.release	= ambarella_wdt_release,
};

static irqreturn_t ambarella_wdt_irq(int irq, void *devid)
{
	struct ambarella_wdt_info		*pinfo;

	pinfo = (struct ambarella_wdt_info *)devid;

	amba_writel(pinfo->regbase + WDOG_CLR_TMO_OFFSET, 0x01);

	dev_info(pinfo->dev, "Watchdog timer expired!\n");

	return IRQ_HANDLED;
}

static int __devinit ambarella_wdt_probe(struct platform_device *pdev)
{
	int					errorCode = 0;
	struct resource 			*irq;
	struct resource 			*mem;
	struct resource 			*ioarea;
	struct ambarella_wdt_info		*pinfo;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem == NULL) {
		dev_err(&pdev->dev, "Get WDT mem resource failed!\n");
		errorCode = -ENXIO;
		goto ambarella_wdt_na;
	}

	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (irq == NULL) {
		dev_err(&pdev->dev, "Get WDT irq resource failed!\n");
		errorCode = -ENXIO;
		goto ambarella_wdt_na;
	}

	ioarea = request_mem_region(mem->start,
			(mem->end - mem->start) + 1, pdev->name);
	if (ioarea == NULL) {
		dev_err(&pdev->dev, "Request WDT ioarea failed!\n");
		errorCode = -EBUSY;
		goto ambarella_wdt_na;
	}

	pinfo = kzalloc(sizeof(struct ambarella_wdt_info),
		GFP_KERNEL);
	if (pinfo == NULL) {
		dev_err(&pdev->dev, "Out of memory!\n");
		errorCode = -ENOMEM;
		goto ambarella_wdt_ioarea;
	}

	pinfo->pcontroller =
		(struct ambarella_wdt_controller *)pdev->dev.platform_data;
	if ((pinfo->pcontroller == NULL) ||
		(pinfo->pcontroller->get_pll == NULL) ||
		(pinfo->pcontroller->start == NULL)) {
		dev_err(&pdev->dev, "Need WDT controller info!\n");
		errorCode = -EPERM;
		goto ambarella_wdt_free_pinfo;
	}

	pinfo->regbase = (unsigned char __iomem *)mem->start;
	pinfo->mem = mem;
	pinfo->dev = &pdev->dev;
	pinfo->irq = irq->start;
	sema_init(&pinfo->wdt_mutex, 1);
	pinfo->state = AMBA_WDT_CLOSE_STATE_DISABLE;
	pinfo->wdt_dev.minor = WATCHDOG_MINOR,
	pinfo->wdt_dev.name = "watchdog",
	pinfo->wdt_dev.fops = &ambarella_wdt_fops,
	pinfo->boot_tmo = amba_readl(pinfo->regbase + WDOG_TIMEOUT_OFFSET);
	pinfo->init_mode = (init_mode & (WDOG_CTR_INT_EN | WDOG_CTR_RST_EN));
	platform_set_drvdata(pdev, pinfo);
	pwdtinfo = pinfo;

	errorCode = ambarella_wdt_set_heartbeat(pinfo, init_tmo);
	if (errorCode)
		ambarella_wdt_set_heartbeat(pinfo,
			CONFIG_WDT_AMBARELLA_TIMEOUT);

	errorCode = misc_register(&pinfo->wdt_dev);
	if (errorCode) {
		dev_err(&pdev->dev, "cannot register miscdev minor=%d (%d)\n",
			WATCHDOG_MINOR, errorCode);
		goto ambarella_wdt_free_pinfo;
	}

	ambarella_wdt_stop(pinfo);

	errorCode = request_irq(pinfo->irq, ambarella_wdt_irq,
		IRQF_TRIGGER_RISING, dev_name(&pdev->dev), pinfo);
	if (errorCode) {
		dev_err(&pdev->dev, "Request IRQ failed!\n");
		goto ambarella_wdt_deregister;
	}

	dev_notice(&pdev->dev,
		"Ambarella Media Processor Watch Dog Timer[%s].\n",
		dev_name(&pdev->dev));

	goto ambarella_wdt_na;

ambarella_wdt_deregister:
	errorCode = misc_deregister(&pinfo->wdt_dev);

ambarella_wdt_free_pinfo:
	kfree(pinfo);

ambarella_wdt_ioarea:
	release_mem_region(mem->start, (mem->end - mem->start) + 1);

ambarella_wdt_na:
	return errorCode;
}

static int __devexit ambarella_wdt_remove(struct platform_device *pdev)
{
	struct ambarella_wdt_info		*pinfo;
	int					errorCode = 0;

	pinfo = platform_get_drvdata(pdev);

	if (pinfo) {
		down(&pinfo->wdt_mutex);
		errorCode = misc_deregister(&pinfo->wdt_dev);
		ambarella_wdt_stop(pinfo);
		free_irq(pinfo->irq, pinfo);
		platform_set_drvdata(pdev, NULL);
		pwdtinfo = NULL;
		release_mem_region(pinfo->mem->start,
			(pinfo->mem->end - pinfo->mem->start) + 1);
		kfree(pinfo);
	}

	dev_notice(&pdev->dev,
		"Remove Ambarella Media Processor Watch Dog Timer[%s] [%d].\n",
		dev_name(&pdev->dev), errorCode);

	return errorCode;
}

static void ambarella_wdt_shutdown(struct platform_device *pdev)
{
	struct ambarella_wdt_info		*pinfo;

	pinfo = platform_get_drvdata(pdev);

	if (pinfo)
		ambarella_wdt_stop(pinfo);
	else
		dev_err(&pdev->dev, "Cannot find valid pinfo\n");

	dev_dbg(&pdev->dev, "%s exit.\n", __func__);
}

#ifdef CONFIG_PM
static int ambarella_wdt_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	struct ambarella_wdt_info		*pinfo;
	int					errorCode = 0;

	pinfo = platform_get_drvdata(pdev);

	if (pinfo) {
		disable_irq(pinfo->irq);
		pinfo->ctl_reg = amba_readl(pinfo->regbase +
			WDOG_CONTROL_OFFSET);
		ambarella_wdt_stop(pinfo);
	} else {
		dev_err(&pdev->dev, "Cannot find valid pinfo\n");
		errorCode = -ENXIO;
	}

	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, errorCode, state.event);

	return errorCode;
}

static int ambarella_wdt_resume(struct platform_device *pdev)
{
	struct ambarella_wdt_info		*pinfo;
	int					errorCode = 0;

	pinfo = platform_get_drvdata(pdev);

	if (pinfo) {
		if (pinfo->ctl_reg) {
			ambarella_wdt_set_heartbeat(pinfo, pinfo->act_timeout);
			ambarella_wdt_start(pinfo, pinfo->ctl_reg);
		}
		enable_irq(pinfo->irq);
	} else {
		dev_err(&pdev->dev, "Cannot find valid pinfo\n");
		errorCode = -ENXIO;
	}

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, errorCode);

	return errorCode;
}
#endif

static struct platform_driver ambarella_wdt_driver = {
	.probe		= ambarella_wdt_probe,
	.remove		= __devexit_p(ambarella_wdt_remove),
	.shutdown	= ambarella_wdt_shutdown,
#ifdef CONFIG_PM
	.suspend	= ambarella_wdt_suspend,
	.resume		= ambarella_wdt_resume,
#endif
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "ambarella-wdt",
	},
};

static int __init ambarella_wdt_init(void)
{
	return platform_driver_register(&ambarella_wdt_driver);
}

static void __exit ambarella_wdt_exit(void)
{
	platform_driver_unregister(&ambarella_wdt_driver);
}

module_init(ambarella_wdt_init);
module_exit(ambarella_wdt_exit);

MODULE_DESCRIPTION("Ambarella Media Processor Watch Dog Timer");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("GPL");

