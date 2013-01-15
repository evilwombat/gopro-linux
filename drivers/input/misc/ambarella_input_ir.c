/*
 * drivers/input/misc/ambarella_input_ir.c
 *
 * Author: Qiao Wang <qwang@ambarella.com>
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
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/input.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/mach-types.h>

#include <mach/hardware.h>
#include <plat/ir.h>

#include <plat/ambinput.h>

/* ========================================================================= */
#define MAX_IR_BUFFER			(512)
#define HW_FIFO_BUFFER			(48)

/* ========================================================================= */
struct ambarella_ir_frame_info {
	u32				frame_head_size;
	u32				frame_data_size;
	u32				frame_end_size;
	u32				frame_repeat_head_size;
};

struct ambarella_ir_info {
	struct input_dev		*pinput_dev;
	unsigned char __iomem 		*regbase;

	u32				id;
	struct resource			*mem;
	unsigned int			irq;
	unsigned int			gpio_id;

	int (*ir_parse)(struct ambarella_ir_info *pirinfo, u32 *uid);
	int				ir_pread;
	int				ir_pwrite;
	u16				tick_buf[MAX_IR_BUFFER];
	struct ambarella_ir_frame_info 	frame_info;
	u32				frame_data_to_received;

	u32				last_ir_uid;
	u32				last_ir_flag;

	struct ambarella_key_table	*pkeymap;
	struct ambarella_ir_controller	*pcontroller_info;
};

/* ========================================================================= */
extern struct ambarella_input_board_info *ambarella_input_get_board_info(void);

/* ========================================================================= */
static u16 ambarella_ir_read_data(
	struct ambarella_ir_info *pirinfo, int pointer)
{
	BUG_ON(pointer < 0);
	BUG_ON(pointer >= MAX_IR_BUFFER);
	BUG_ON(pointer == pirinfo->ir_pwrite);

	return pirinfo->tick_buf[pointer];
}

static int ambarella_ir_get_tick_size(struct ambarella_ir_info *pirinfo)
{
	int				size = 0;

	if (pirinfo->ir_pread > pirinfo->ir_pwrite)
		size = MAX_IR_BUFFER - pirinfo->ir_pread + pirinfo->ir_pwrite;
	else
		size = pirinfo->ir_pwrite - pirinfo->ir_pread;

	return size;
}

void ambarella_ir_inc_read_ptr(struct ambarella_ir_info *pirinfo)
{
	BUG_ON(pirinfo->ir_pread == pirinfo->ir_pwrite);

	pirinfo->ir_pread++;
	if (pirinfo->ir_pread >= MAX_IR_BUFFER)
		pirinfo->ir_pread = 0;
}

void ambarella_ir_move_read_ptr(struct ambarella_ir_info *pirinfo, int offset)
{
	for (; offset > 0; offset--) {
		ambarella_ir_inc_read_ptr(pirinfo);
	}
}

/* ========================================================================= */
#include "ambarella_ir_nec.c"
#include "ambarella_ir_sony.c"
#include "ambarella_ir_philips.c"
#include "ambarella_ir_panasonic.c"

/* ========================================================================= */
static int ambarella_input_report_ir(struct ambarella_ir_info *pirinfo, u32 uid)
{
	int				i;

	if (!pirinfo->pkeymap)
		return -1;

	if ((pirinfo->last_ir_uid == uid) && (pirinfo->last_ir_flag)) {
		pirinfo->last_ir_flag--;
		return 0;
	}
	pirinfo->last_ir_uid = uid;

	for (i = 0; i < AMBINPUT_TABLE_SIZE; i++) {
		if (pirinfo->pkeymap[i].type == AMBINPUT_END)
			break;

		if ((pirinfo->pkeymap[i].type & AMBINPUT_SOURCE_MASK) !=
			AMBINPUT_SOURCE_IR)
			continue;

		if ((pirinfo->pkeymap[i].type == AMBINPUT_IR_KEY) &&
			(pirinfo->pkeymap[i].ir_key.raw_id == uid)) {
			input_report_key(pirinfo->pinput_dev,
				pirinfo->pkeymap[i].ir_key.key_code, 1);
			input_report_key(pirinfo->pinput_dev,
				pirinfo->pkeymap[i].ir_key.key_code, 0);
			dev_dbg(&pirinfo->pinput_dev->dev, "IR_KEY [%d]\n",
				pirinfo->pkeymap[i].ir_key.key_code);
			pirinfo->last_ir_flag =
				pirinfo->pkeymap[i].ir_key.key_flag;
			break;
		}

		if ((pirinfo->pkeymap[i].type == AMBINPUT_IR_REL) &&
			(pirinfo->pkeymap[i].ir_rel.raw_id == uid)) {
			if (pirinfo->pkeymap[i].ir_rel.key_code == REL_X) {
				input_report_rel(pirinfo->pinput_dev,
					REL_X,
					pirinfo->pkeymap[i].ir_rel.rel_step);
				input_report_rel(pirinfo->pinput_dev,
					REL_Y, 0);
				input_sync(pirinfo->pinput_dev);
				dev_dbg(&pirinfo->pinput_dev->dev,
					"IR_REL X[%d]:Y[%d]\n",
					pirinfo->pkeymap[i].ir_rel.rel_step, 0);
			} else
			if (pirinfo->pkeymap[i].ir_rel.key_code == REL_Y) {
				input_report_rel(pirinfo->pinput_dev,
					REL_X, 0);
				input_report_rel(pirinfo->pinput_dev,
					REL_Y,
					pirinfo->pkeymap[i].ir_rel.rel_step);
				input_sync(pirinfo->pinput_dev);
				dev_dbg(&pirinfo->pinput_dev->dev,
					"IR_REL X[%d]:Y[%d]\n", 0,
					pirinfo->pkeymap[i].ir_rel.rel_step);
			}
			pirinfo->last_ir_flag = 0;
			break;
		}

		if ((pirinfo->pkeymap[i].type == AMBINPUT_IR_ABS) &&
			(pirinfo->pkeymap[i].ir_abs.raw_id == uid)) {
			input_report_abs(pirinfo->pinput_dev,
				ABS_X, pirinfo->pkeymap[i].ir_abs.abs_x);
			input_report_abs(pirinfo->pinput_dev,
				ABS_Y, pirinfo->pkeymap[i].ir_abs.abs_y);
			input_sync(pirinfo->pinput_dev);
			dev_dbg(&pirinfo->pinput_dev->dev,
				"IR_ABS X[%d]:Y[%d]\n",
				pirinfo->pkeymap[i].ir_abs.abs_x,
				pirinfo->pkeymap[i].ir_abs.abs_y);
			pirinfo->last_ir_flag = 0;
			break;
		}

		if ((pirinfo->pkeymap[i].type == AMBINPUT_IR_SW) &&
			(pirinfo->pkeymap[i].ir_sw.raw_id == uid)) {
			input_report_switch(pirinfo->pinput_dev,
				pirinfo->pkeymap[i].ir_sw.key_code,
				pirinfo->pkeymap[i].ir_sw.key_value);
			dev_dbg(&pirinfo->pinput_dev->dev, "IR_SW [%d:%d]\n",
				pirinfo->pkeymap[i].ir_sw.key_code,
				pirinfo->pkeymap[i].ir_sw.key_value);
			pirinfo->last_ir_flag = 0;
			break;
		}
	}

	return 0;
}

static inline void ambarella_ir_write_data(struct ambarella_ir_info *pirinfo,
	u16 val)
{
	BUG_ON(pirinfo->ir_pwrite < 0);
	BUG_ON(pirinfo->ir_pwrite >= MAX_IR_BUFFER);

	pirinfo->tick_buf[pirinfo->ir_pwrite] = val;

	pirinfo->ir_pwrite++;

	if (pirinfo->ir_pwrite >= MAX_IR_BUFFER)
		pirinfo->ir_pwrite = 0;
}

static inline int ambarella_ir_update_buffer(struct ambarella_ir_info *pirinfo)
{
	int				count;
	int				size;

	count = amba_readl(pirinfo->regbase + IR_STATUS_OFFSET);
	dev_dbg(&pirinfo->pinput_dev->dev, "size we got is [%d]\n", count);
	for (; count > 0; count--) {
		ambarella_ir_write_data(pirinfo,
			amba_readl(pirinfo->regbase + IR_DATA_OFFSET));
	}
	size = ambarella_ir_get_tick_size(pirinfo);

	return size;
}

static irqreturn_t ambarella_ir_irq(int irq, void *devid)
{
	struct ambarella_ir_info	*pirinfo;
	int				size;
	int				rval;
	u32				uid;
	u32				edges;

	pirinfo = (struct ambarella_ir_info *)devid;

	BUG_ON(pirinfo->ir_pread < 0);
	BUG_ON(pirinfo->ir_pread >= MAX_IR_BUFFER);

	if (amba_readl(pirinfo->regbase + IR_CONTROL_OFFSET) &
		IR_CONTROL_FIFO_OV) {
		while (amba_readl(pirinfo->regbase + IR_STATUS_OFFSET) > 0) {
			amba_readl(pirinfo->regbase + IR_DATA_OFFSET);
		}
		amba_writel(pirinfo->regbase + IR_CONTROL_OFFSET,
			(amba_readl(pirinfo->regbase + IR_CONTROL_OFFSET) |
		IR_CONTROL_FIFO_OV));

		dev_err(&pirinfo->pinput_dev->dev,
			"IR_CONTROL_FIFO_OV overflow\n");

		goto ambarella_ir_irq_exit;
	}

	size = ambarella_ir_update_buffer(pirinfo);

	if(!pirinfo->ir_parse)
		goto ambarella_ir_irq_exit;

	rval = pirinfo->ir_parse(pirinfo, &uid);

	if (rval == 0) {// yes, we find the key
		if(pirinfo->pcontroller_info->debug)
			printk(KERN_NOTICE "uid = 0x%08x\n", uid);
		ambarella_input_report_ir(pirinfo, uid);
	}

	pirinfo->frame_data_to_received = pirinfo->frame_info.frame_data_size +
		pirinfo->frame_info.frame_head_size;
	pirinfo->frame_data_to_received -= ambarella_ir_get_tick_size(pirinfo);

	if (pirinfo->frame_data_to_received <= HW_FIFO_BUFFER) {
		edges = pirinfo->frame_data_to_received;
		pirinfo->frame_data_to_received = 0;
	} else {// > HW_FIFO_BUFFER
		edges = HW_FIFO_BUFFER;
		pirinfo->frame_data_to_received -= HW_FIFO_BUFFER;
	}

	dev_dbg(&pirinfo->pinput_dev->dev,
		"line[%d],frame_data_to_received[%d]\n", __LINE__, edges);
	amba_clrbitsl(pirinfo->regbase + IR_CONTROL_OFFSET,
		IR_CONTROL_INTLEV(0x3F));
	amba_setbitsl(pirinfo->regbase + IR_CONTROL_OFFSET,
		IR_CONTROL_INTLEV(edges));

ambarella_ir_irq_exit:
	amba_writel(pirinfo->regbase + IR_CONTROL_OFFSET,
		(amba_readl(pirinfo->regbase + IR_CONTROL_OFFSET) |
		IR_CONTROL_LEVINT));

	return IRQ_HANDLED;
}

void ambarella_ir_enable(struct ambarella_ir_info *pirinfo)
{
	u32 edges;

	pirinfo->frame_data_to_received = pirinfo->frame_info.frame_head_size
		+ pirinfo->frame_info.frame_data_size;

	BUG_ON(pirinfo->frame_data_to_received > MAX_IR_BUFFER);

	if (pirinfo->frame_data_to_received > HW_FIFO_BUFFER) {
		edges = HW_FIFO_BUFFER;
		pirinfo->frame_data_to_received -= HW_FIFO_BUFFER;
	} else {
		edges = pirinfo->frame_data_to_received;
		pirinfo->frame_data_to_received = 0;
	}
	amba_writel(pirinfo->regbase + IR_CONTROL_OFFSET, IR_CONTROL_RESET);
	amba_setbitsl(pirinfo->regbase + IR_CONTROL_OFFSET,
		IR_CONTROL_ENB | IR_CONTROL_INTLEV(edges) | IR_CONTROL_INTENB);

	enable_irq(pirinfo->irq);
}

void ambarella_ir_disable(struct ambarella_ir_info *pirinfo)
{
	disable_irq(pirinfo->irq);
}

void ambarella_ir_set_protocol(struct ambarella_ir_info *pirinfo,
	enum ambarella_ir_protocol protocol_id)
{
	memset(pirinfo->tick_buf, 0x0, sizeof(pirinfo->tick_buf));
	pirinfo->ir_pread  = 0;
	pirinfo->ir_pwrite = 0;

	switch (protocol_id) {
	case AMBA_IR_PROTOCOL_NEC:
		dev_notice(&pirinfo->pinput_dev->dev,
			"Protocol NEC[%d]\n", protocol_id);
		pirinfo->ir_parse = ambarella_ir_nec_parse;
		ambarella_ir_get_nec_info(&pirinfo->frame_info);
		break;
	case AMBA_IR_PROTOCOL_PANASONIC:
		dev_notice(&pirinfo->pinput_dev->dev,
			"Protocol PANASONIC[%d]\n", protocol_id);
		pirinfo->ir_parse = ambarella_ir_panasonic_parse;
		ambarella_ir_get_panasonic_info(&pirinfo->frame_info);
		break;
	case AMBA_IR_PROTOCOL_SONY:
		dev_notice(&pirinfo->pinput_dev->dev,
			"Protocol SONY[%d]\n", protocol_id);
		pirinfo->ir_parse = ambarella_ir_sony_parse;
		ambarella_ir_get_sony_info(&pirinfo->frame_info);
		break;
	case AMBA_IR_PROTOCOL_PHILIPS:
		dev_notice(&pirinfo->pinput_dev->dev,
			"Protocol PHILIPS[%d]\n", protocol_id);
		pirinfo->ir_parse = ambarella_ir_philips_parse;
		ambarella_ir_get_philips_info(&pirinfo->frame_info);
		break;
	default:
		dev_notice(&pirinfo->pinput_dev->dev,
			"Protocol default NEC[%d]\n", protocol_id);
		pirinfo->ir_parse = ambarella_ir_nec_parse;
		ambarella_ir_get_nec_info(&pirinfo->frame_info);
		break;
	}
}
void ambarella_ir_init(struct ambarella_ir_info *pirinfo)
{
	ambarella_ir_disable(pirinfo);

	pirinfo->pcontroller_info->set_pll();

	ambarella_gpio_config(pirinfo->gpio_id, GPIO_FUNC_HW);

	if (pirinfo->pcontroller_info->protocol >= AMBA_IR_PROTOCOL_END)
		pirinfo->pcontroller_info->protocol = AMBA_IR_PROTOCOL_NEC;
	ambarella_ir_set_protocol(pirinfo, pirinfo->pcontroller_info->protocol);

	ambarella_ir_enable(pirinfo);
}

static int __devinit ambarella_ir_probe(struct platform_device *pdev)
{
	int					retval;
	struct resource 			*irq;
	struct resource 			*mem;
	struct resource 			*io;
	struct resource 			*ioarea;
	struct ambarella_ir_info		*pirinfo;
	struct ambarella_input_board_info	*pboard_info;

	pboard_info = ambarella_input_get_board_info();
	if (pboard_info == NULL){
		dev_err(&pdev->dev, "pboard_info is NULL!\n");
		retval = -ENOMEM;
		goto ir_errorCode_na;
	}

	pirinfo = kzalloc(sizeof(struct ambarella_ir_info), GFP_KERNEL);
	if (!pirinfo) {
		dev_err(&pdev->dev, "Failed to allocate pirinfo!\n");
		retval = -ENOMEM;
		goto ir_errorCode_na;
	}

	pirinfo->pcontroller_info =
		(struct ambarella_ir_controller *)pdev->dev.platform_data;
	if ((pirinfo->pcontroller_info == NULL) ||
		(pirinfo->pcontroller_info->set_pll == NULL) ||
		(pirinfo->pcontroller_info->get_pll == NULL) ) {
		dev_err(&pdev->dev, "Platform data is NULL!\n");
		retval = -ENXIO;
		goto ir_errorCode_pinfo;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem == NULL) {
		dev_err(&pdev->dev, "Get mem resource failed!\n");
		retval = -ENXIO;
		goto ir_errorCode_pinfo;
	}

	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (irq == NULL) {
		dev_err(&pdev->dev, "Get irq resource failed!\n");
		retval = -ENXIO;
		goto ir_errorCode_pinfo;
	}

	io = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (io == NULL) {
		dev_err(&pdev->dev, "Get GPIO resource failed!\n");
		retval = -ENXIO;
		goto ir_errorCode_pinfo;
	}

	ioarea = request_mem_region(mem->start,
			(mem->end - mem->start) + 1, pdev->name);
	if (ioarea == NULL) {
		dev_err(&pdev->dev, "Request ioarea failed!\n");
		retval = -EBUSY;
		goto ir_errorCode_pinfo;
	}

	pirinfo->regbase = (unsigned char __iomem *)mem->start;
	pirinfo->id = pdev->id;
	pirinfo->mem = mem;
	pirinfo->irq = irq->start;
	pirinfo->gpio_id = io->start;
	pirinfo->last_ir_uid = 0;
	pirinfo->last_ir_flag = 0;
	pirinfo->pinput_dev = pboard_info->pinput_dev;
	pirinfo->pkeymap = pboard_info->pkeymap;

	platform_set_drvdata(pdev, pirinfo);

	retval = request_irq(pirinfo->irq,
		ambarella_ir_irq, IRQF_TRIGGER_HIGH,
		dev_name(&pdev->dev), pirinfo);
	if (retval) {
		dev_err(&pdev->dev, "Request IRQ failed!\n");
		goto ir_errorCode_free_platform;
	}

	ambarella_ir_init(pirinfo);

	dev_notice(&pdev->dev, "IR Host Controller probed!\n");

	goto ir_errorCode_na;

ir_errorCode_free_platform:
	platform_set_drvdata(pdev, NULL);
	release_mem_region(mem->start, (mem->end - mem->start) + 1);
ir_errorCode_pinfo:
	kfree(pirinfo);

ir_errorCode_na:
	return retval;
}

static int __devexit ambarella_ir_remove(struct platform_device *pdev)
{
	struct ambarella_ir_info	*pirinfo;
	int				retval = 0;

	pirinfo = platform_get_drvdata(pdev);

	if (pirinfo) {
		free_irq(pirinfo->irq, pirinfo);
		platform_set_drvdata(pdev, NULL);
		release_mem_region(pirinfo->mem->start,
			(pirinfo->mem->end - pirinfo->mem->start) + 1);
		kfree(pirinfo);
	}

	dev_notice(&pdev->dev,
		"Remove Ambarella Media Processor IR Host Controller.\n");

	return retval;
}

#if (defined CONFIG_PM)
static int ambarella_ir_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	int					retval = 0;
	struct ambarella_ir_info		*pirinfo;

	pirinfo = platform_get_drvdata(pdev);

	disable_irq(pirinfo->irq);
	amba_clrbitsl(pirinfo->regbase + IR_CONTROL_OFFSET, IR_CONTROL_INTENB);

	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, retval, state.event);
	return retval;
}

static int ambarella_ir_resume(struct platform_device *pdev)
{
	int					retval = 0;
	struct ambarella_ir_info		*pirinfo;

	pirinfo = platform_get_drvdata(pdev);

	ambarella_ir_enable(pirinfo);

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, retval);

	return retval;
}
#endif

static struct platform_driver ambarella_ir_driver = {
	.probe		= ambarella_ir_probe,
	.remove		= __devexit_p(ambarella_ir_remove),
#if (defined CONFIG_PM)
	.suspend	= ambarella_ir_suspend,
	.resume		= ambarella_ir_resume,
#endif
	.driver		= {
		.name	= "ambarella-ir",
		.owner	= THIS_MODULE,
	},
};

int platform_driver_register_ir(void)
{
	return platform_driver_register(&ambarella_ir_driver);
}

void platform_driver_unregister_ir(void)
{
	platform_driver_unregister(&ambarella_ir_driver);
}

