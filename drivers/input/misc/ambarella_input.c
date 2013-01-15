/*
 * drivers/input/misc/ambarella_input.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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
 * adc update: Qiao Wang 2009/10/28
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/input.h>
#include <linux/platform_device.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/mach-types.h>

#include <mach/hardware.h>
#include <plat/ambinput.h>

/* ========================================================================= */
#define AMBVI_BUFFER_SIZE			(32)
#define AMBVI_NAME				"ambvi"

/* ========================================================================= */
extern int platform_driver_register_ir(void);
extern void platform_driver_unregister_ir(void);

extern int platform_driver_register_adc(void);
extern void platform_driver_unregister_adc(void);

/* ========================================================================= */
static int abx_active_pressure = 0;

/* ========================================================================= */
static DEFINE_MUTEX(pboard_info_lock);
static struct ambarella_input_board_info *pboard_info = NULL;

struct ambarella_input_board_info *ambarella_input_get_board_info(void)
{
	mutex_lock(&pboard_info_lock);
	mutex_unlock(&pboard_info_lock);

	return pboard_info;
}

/* ========================================================================= */
irqreturn_t ambarella_gpio_irq(int irq, void *devid)
{
	int					i;
	int					gpio_id;
	int					level;
	struct ambarella_input_board_info	*pbinfo;

	pbinfo = (struct ambarella_input_board_info *)devid;

	gpio_id = irq_to_gpio(irq);
	if (gpio_id < 0)
		goto ambarella_gpio_irq_exit;

	level = ambarella_gpio_get(gpio_id);

	if (!pbinfo->pkeymap)
		goto ambarella_gpio_irq_exit;

	for (i = 0; i < AMBINPUT_TABLE_SIZE; i++) {
		if (pbinfo->pkeymap[i].type == AMBINPUT_END)
			break;

		if ((pbinfo->pkeymap[i].type & AMBINPUT_SOURCE_MASK) !=
			AMBINPUT_SOURCE_GPIO)
			continue;

		if (pbinfo->pkeymap[i].gpio_key.id != gpio_id)
			continue;

		if (pbinfo->pkeymap[i].type == AMBINPUT_GPIO_KEY) {
			input_report_key(pbinfo->pinput_dev,
				pbinfo->pkeymap[i].gpio_key.key_code,
				(level == pbinfo->pkeymap[i].gpio_key.active_val) ? 1 : 0);
			dev_dbg(&pbinfo->pinput_dev->dev,
				"GPIO %d is @ %d:%d\n",
				pbinfo->pkeymap[i].gpio_key.key_code,
				gpio_id, level);
			break;
		}

		if (pbinfo->pkeymap[i].type == AMBINPUT_GPIO_SW) {
			input_report_switch(pbinfo->pinput_dev,
				pbinfo->pkeymap[i].gpio_sw.key_code,
				(level == pbinfo->pkeymap[i].gpio_key.active_val) ? 1 : 0);
			dev_dbg(&pbinfo->pinput_dev->dev,
				"GPIO %d is @ %d:%d\n",
				pbinfo->pkeymap[i].gpio_sw.key_code,
				gpio_id, level);
			break;
		}

		if (pbinfo->pkeymap[i].type == AMBINPUT_GPIO_REL) {
			if (pbinfo->pkeymap[i].gpio_rel.key_code == REL_X) {
				input_report_rel(pbinfo->pinput_dev,
					REL_X,
					pbinfo->pkeymap[i].gpio_rel.rel_step);
				input_report_rel(pbinfo->pinput_dev,
					REL_Y, 0);
				input_sync(pbinfo->pinput_dev);
				dev_dbg(&pbinfo->pinput_dev->dev,
					"report REL_X %d @ %d:%d\n",
					pbinfo->pkeymap[i].gpio_rel.rel_step,
					gpio_id, level);
			} else
			if (pbinfo->pkeymap[i].gpio_rel.key_code == REL_Y) {
				input_report_rel(pbinfo->pinput_dev,
					REL_X, 0);
				input_report_rel(pbinfo->pinput_dev,
					REL_Y,
					pbinfo->pkeymap[i].gpio_rel.rel_step);
				input_sync(pbinfo->pinput_dev);
				dev_dbg(&pbinfo->pinput_dev->dev,
					"report REL_Y %d @ %d:%d\n",
					pbinfo->pkeymap[i].gpio_rel.rel_step,
					gpio_id, level);
			}
			break;
		}

		if (pbinfo->pkeymap[i].type == AMBINPUT_GPIO_ABS) {
			input_report_abs(pbinfo->pinput_dev,
				ABS_X, pbinfo->pkeymap[i].gpio_abs.abs_x);
			input_report_abs(pbinfo->pinput_dev,
				ABS_Y, pbinfo->pkeymap[i].gpio_abs.abs_y);
			input_sync(pbinfo->pinput_dev);
			dev_dbg(&pbinfo->pinput_dev->dev,
				"report ABS %d:%d @ %d:%d\n",
				pbinfo->pkeymap[i].gpio_abs.abs_x,
				pbinfo->pkeymap[i].gpio_abs.abs_y,
				gpio_id, level);
			break;
		}
	}

ambarella_gpio_irq_exit:
	return IRQ_HANDLED;
}

#ifdef CONFIG_AMBARELLA_IPC

static struct ambarella_input_board_info *ipc_pbinfo = NULL;
int ambarella_vi_press_abs_sync(int x, int y)
{
	if(ipc_pbinfo==NULL){
		return -1;
	}

	input_report_key(ipc_pbinfo->pinput_dev, BTN_TOUCH, 1);
	input_report_abs(ipc_pbinfo->pinput_dev, ABS_X, x);
	input_report_abs(ipc_pbinfo->pinput_dev, ABS_Y, y);
	if (abx_active_pressure == 0){
		abx_active_pressure = ipc_pbinfo->abx_max_pressure / 2;
	}
	input_report_abs(ipc_pbinfo->pinput_dev, ABS_PRESSURE, abx_active_pressure);
	input_sync(ipc_pbinfo->pinput_dev);

	return 0;
}
EXPORT_SYMBOL(ambarella_vi_press_abs_sync);

int ambarella_vi_release_abs_sync(int x, int y)
{
	if(ipc_pbinfo==NULL){
		return -1;
	}
	
	input_report_key(ipc_pbinfo->pinput_dev, BTN_TOUCH, 0);
	input_report_abs(ipc_pbinfo->pinput_dev, ABS_PRESSURE, 0);
	input_sync(ipc_pbinfo->pinput_dev);

	return 0;
}
EXPORT_SYMBOL(ambarella_vi_release_abs_sync);

int ambarella_vi_send_key(int key, int pressed)
{
	if(ipc_pbinfo==NULL){
		return -1;
	}
	
	input_report_key(ipc_pbinfo->pinput_dev, key, pressed);

	return 0;
}
EXPORT_SYMBOL(ambarella_vi_send_key);

#endif

int ambarella_vi_proc_write(struct file *file,
	const char __user *buffer, unsigned long count, void *data)
{
	struct ambarella_input_board_info	*pbinfo = data;
	u32					key_num;
	char					cmd_buffer[AMBVI_BUFFER_SIZE];
	char					key_buffer[AMBVI_BUFFER_SIZE];
	u32					value1;
	u32					value2;

	memset(key_buffer, 0, AMBVI_BUFFER_SIZE);
	if (count < AMBVI_BUFFER_SIZE) {
		if (copy_from_user(cmd_buffer, buffer, count))
			return -EINVAL;

		key_num = sscanf(cmd_buffer, "%s %d:%d",
			key_buffer, &value1, &value2);
		if (key_num != 3) {
			dev_err(&pbinfo->pdev->dev,
				"Get %d data[%s %d:%d]\n",
				key_num, key_buffer, value1, value2);
			return -EINVAL;
		}

		if (memcmp(key_buffer, "key", 3) == 0) 	{
			input_report_key(pbinfo->pinput_dev, value1, value2);
		}else
		if (memcmp(key_buffer, "rel", 3) == 0) 	{
			input_report_rel(pbinfo->pinput_dev, REL_X, value1);
			input_report_rel(pbinfo->pinput_dev, REL_Y, value2);
			input_sync(pbinfo->pinput_dev);
		}else
		if (memcmp(key_buffer, "abs", 3) == 0) 	{
			input_report_abs(pbinfo->pinput_dev, ABS_X, value1);
			input_report_abs(pbinfo->pinput_dev, ABS_Y, value2);
			input_sync(pbinfo->pinput_dev);
		}else
		if (memcmp(key_buffer, "ton", 3) == 0) 	{
			input_report_key(pbinfo->pinput_dev, BTN_TOUCH, 1);
			input_report_abs(pbinfo->pinput_dev, ABS_X, value1);
			input_report_abs(pbinfo->pinput_dev, ABS_Y, value2);
			if (abx_active_pressure == 0)
				abx_active_pressure =
					pbinfo->abx_max_pressure / 2;
			input_report_abs(pbinfo->pinput_dev, ABS_PRESSURE,
				abx_active_pressure);
			input_sync(pbinfo->pinput_dev);
		}else
		if (memcmp(key_buffer, "tof", 3) == 0) 	{
			input_report_key(pbinfo->pinput_dev, BTN_TOUCH, 0);
			input_report_abs(pbinfo->pinput_dev, ABS_PRESSURE, 0);
			input_sync(pbinfo->pinput_dev);
		}else
		if (memcmp(key_buffer, "pre", 3) == 0) 	{
			abx_active_pressure = value1;
			input_report_abs(pbinfo->pinput_dev,
				ABS_PRESSURE, value1);
			input_sync(pbinfo->pinput_dev);
		}else
		if (memcmp(key_buffer, "wid", 3) == 0) 	{
			input_report_abs(pbinfo->pinput_dev,
				ABS_TOOL_WIDTH, value1);
			input_sync(pbinfo->pinput_dev);
		}else
		if (memcmp(key_buffer, "swt", 3) == 0) {
			input_report_switch(pbinfo->pinput_dev, value1, value2);
			input_sync(pbinfo->pinput_dev);
		}
	}

	return count;
}

static int __devinit ambarella_setup_keymap(
	struct ambarella_input_board_info *pbinfo)
{
	int					retval = 0;
	int					i, j;
	int					vi_enabled = 0;
	int					vi_key_set = 0;
	int					vi_sw_set = 0;

	if (!pbinfo->pkeymap) {
		retval = -EINVAL;
		goto ambarella_setup_keymap_exit;
	}

	for (i = 0; i < AMBINPUT_TABLE_SIZE; i++) {
		if (pbinfo->pkeymap[i].type == AMBINPUT_END)
			break;

		switch (pbinfo->pkeymap[i].type & AMBINPUT_SOURCE_MASK) {
		case AMBINPUT_SOURCE_IR:
			break;

		case AMBINPUT_SOURCE_ADC:
			break;

		case AMBINPUT_SOURCE_GPIO:
			ambarella_gpio_config(pbinfo->pkeymap[i].gpio_key.id,
				GPIO_FUNC_SW_INPUT);
			retval = request_irq(
				GPIO_INT_VEC(pbinfo->pkeymap[i].gpio_key.id),
				ambarella_gpio_irq,
				pbinfo->pkeymap[i].gpio_key.irq_mode,
				pbinfo->pinput_dev->name, pbinfo);
			if (retval)
				dev_err(&pbinfo->pdev->dev,
					"Request GPIO%d IRQ failed!\n",
					pbinfo->pkeymap[i].gpio_key.id);
			if ((retval == 0) &&
				(pbinfo->pkeymap[i].gpio_key.can_wakeup)) {
				retval = set_irq_wake(GPIO_INT_VEC(
					pbinfo->pkeymap[i].gpio_key.id), 1);
				if (retval)
					dev_err(&pbinfo->pdev->dev,
						"set_irq_wake %d failed!\n",
						pbinfo->pkeymap[i].gpio_key.id);
			}
			retval = 0;	//Continue with error...
			break;

		case AMBINPUT_SOURCE_VI:
			vi_enabled = 1;
			break;

		default:
			dev_warn(&pbinfo->pdev->dev,
				"Unknown AMBINPUT_SOURCE %d\n",
				(pbinfo->pkeymap[i].type &
				AMBINPUT_SOURCE_MASK));
			break;
		}

		switch (pbinfo->pkeymap[i].type & AMBINPUT_TYPE_MASK) {
		case AMBINPUT_TYPE_KEY:
			set_bit(EV_KEY, pbinfo->pinput_dev->evbit);
			set_bit(pbinfo->pkeymap[i].ir_key.key_code,
				pbinfo->pinput_dev->keybit);
			if (vi_enabled && !vi_key_set) {
				for (j = 0; j < KEY_CNT; j++) {
					set_bit(j, pbinfo->pinput_dev->keybit);
				}
				vi_key_set = 1;
			}
			break;

		case AMBINPUT_TYPE_REL:
			set_bit(EV_KEY, pbinfo->pinput_dev->evbit);
			set_bit(EV_REL, pbinfo->pinput_dev->evbit);
			set_bit(BTN_LEFT, pbinfo->pinput_dev->keybit);
			set_bit(BTN_RIGHT, pbinfo->pinput_dev->keybit);
			set_bit(REL_X, pbinfo->pinput_dev->relbit);
			set_bit(REL_Y, pbinfo->pinput_dev->relbit);
			break;

		case AMBINPUT_TYPE_ABS:
			set_bit(EV_KEY, pbinfo->pinput_dev->evbit);
			set_bit(EV_ABS, pbinfo->pinput_dev->evbit);
			set_bit(BTN_LEFT, pbinfo->pinput_dev->keybit);
			set_bit(BTN_RIGHT, pbinfo->pinput_dev->keybit);
			set_bit(BTN_TOUCH, pbinfo->pinput_dev->keybit);
			set_bit(ABS_X, pbinfo->pinput_dev->absbit);
			set_bit(ABS_Y, pbinfo->pinput_dev->absbit);
			set_bit(ABS_PRESSURE, pbinfo->pinput_dev->absbit);
			set_bit(ABS_TOOL_WIDTH, pbinfo->pinput_dev->absbit);
			input_set_abs_params(pbinfo->pinput_dev, ABS_X,
				0, pbinfo->abx_max_x, 0, 0);
			input_set_abs_params(pbinfo->pinput_dev, ABS_Y,
				0, pbinfo->abx_max_y, 0, 0);
			input_set_abs_params(pbinfo->pinput_dev, ABS_PRESSURE,
				0, pbinfo->abx_max_pressure, 0, 0);
			input_set_abs_params(pbinfo->pinput_dev, ABS_TOOL_WIDTH,
				0, pbinfo->abx_max_width, 0, 0);
			break;

		case AMBINPUT_TYPE_SW:
			set_bit(EV_SW, pbinfo->pinput_dev->evbit);
			set_bit(pbinfo->pkeymap[i].ir_key.key_code,
				pbinfo->pinput_dev->swbit);
			if (vi_enabled && !vi_sw_set) {
				for (j = 0; j < SW_CNT; j++) {
					set_bit(j, pbinfo->pinput_dev->swbit);
				}
				vi_sw_set = 1;
			}
			break;

		default:
			dev_warn(&pbinfo->pdev->dev,
				"Unknown AMBINPUT_TYPE %d\n",
				(pbinfo->pkeymap[i].type & AMBINPUT_TYPE_MASK));
			break;
		}
	}

ambarella_setup_keymap_exit:
	return retval;
}

static void __devexit ambarella_free_keymap(
	struct ambarella_input_board_info *pbinfo)
{
	int					i;

	if (!pbinfo->pkeymap)
		return;

	for (i = 0; i < AMBINPUT_TABLE_SIZE; i++) {
		if (pbinfo->pkeymap[i].type == AMBINPUT_END)
			break;

		switch (pbinfo->pkeymap[i].type & AMBINPUT_SOURCE_MASK) {
		case AMBINPUT_SOURCE_IR:
			break;

		case AMBINPUT_SOURCE_ADC:
			break;

		case AMBINPUT_SOURCE_VI:
			break;

		case AMBINPUT_SOURCE_GPIO:
			if (pbinfo->pkeymap[i].gpio_key.can_wakeup)
				set_irq_wake(GPIO_INT_VEC(
					pbinfo->pkeymap[i].gpio_key.id), 0);
			free_irq(GPIO_INT_VEC(
				pbinfo->pkeymap[i].gpio_key.id), pbinfo);
			break;

		default:
			dev_warn(&pbinfo->pdev->dev,
				"Unknown AMBINPUT_SOURCE %d\n",
				(pbinfo->pkeymap[i].type &
				AMBINPUT_SOURCE_MASK));
			break;
		}
	}
}

static int __devinit ambarella_input_probe(struct platform_device *pdev)
{
	int					retval;
	struct proc_dir_entry			*input_file;
	struct input_dev			*pinput_dev;
	struct ambarella_input_board_info 	*pbinfo;

	pbinfo =
		(struct ambarella_input_board_info *)pdev->dev.platform_data;
	if (pbinfo == NULL) {
		dev_err(&pdev->dev, "Need board info!\n");
		retval = -EPERM;
		goto ambarella_input_probe_exit;
	}

	pinput_dev = input_allocate_device();
	if (!pinput_dev) {
		dev_err(&pdev->dev, "input_allocate_device fail!\n");
		retval = -ENOMEM;
		goto ambarella_input_probe_exit;
	}
	pinput_dev->name = "AmbInput";
	pinput_dev->phys = "ambarella/input0";
	pinput_dev->id.bustype = BUS_HOST;

	retval = input_register_device(pinput_dev);
	if (retval) {
		dev_err(&pdev->dev, "Register input_dev failed!\n");
		goto ambarella_input_probe_free_input_dev;
	}
	pbinfo->pdev = pdev;
	pbinfo->pinput_dev = pinput_dev;

	input_file = create_proc_entry(AMBVI_NAME,
		S_IRUGO | S_IWUSR, get_ambarella_proc_dir());
	if (input_file == NULL) {
		dev_err(&pdev->dev, "Register %s failed!\n",
			dev_name(&pinput_dev->dev));
		retval = -ENOMEM;
		goto ambarella_input_probe_unregister_device;
	} else {
		input_file->write_proc = ambarella_vi_proc_write;
		input_file->data = pbinfo;
	}

#ifdef CONFIG_AMBARELLA_IPC
	//Keep for IPC
	ipc_pbinfo = pbinfo;
#endif
	
	retval = ambarella_setup_keymap(pbinfo);
	if (retval)
		goto ambarella_input_probe_unregister_device;

	pboard_info = pbinfo;
	mutex_unlock(&pboard_info_lock);

	dev_notice(&pdev->dev, "AmbInput probed!\n");

	goto ambarella_input_probe_exit;

ambarella_input_probe_unregister_device:
	input_unregister_device(pinput_dev);

ambarella_input_probe_free_input_dev:
	input_free_device(pinput_dev);
	pbinfo->pdev = NULL;
	pbinfo->pinput_dev = NULL;

ambarella_input_probe_exit:
	return retval;
}

static int __devexit ambarella_input_remove(struct platform_device *pdev)
{
	int					retval = 0;
	struct ambarella_input_board_info 	*pbinfo;

	pbinfo =
		(struct ambarella_input_board_info *)pdev->dev.platform_data;
	if (pbinfo != NULL) {
		ambarella_free_keymap(pbinfo);
		remove_proc_entry(AMBVI_NAME, get_ambarella_proc_dir());
		input_unregister_device(pbinfo->pinput_dev);
		input_free_device(pbinfo->pinput_dev);
		pbinfo->pdev = NULL;
		pbinfo->pinput_dev = NULL;
	}

	dev_notice(&pdev->dev, "AmbInput Removed!\n" );

	return retval;
}

#ifdef CONFIG_PM
static int ambarella_input_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	int					errorCode = 0;

	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, errorCode, state.event);
	return errorCode;
}

static int ambarella_input_resume(struct platform_device *pdev)
{
	int					errorCode = 0;

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, errorCode);

	return errorCode;
}
#endif

static struct platform_driver ambarella_input_driver = {
	.probe		= ambarella_input_probe,
	.remove		= __devexit_p(ambarella_input_remove),
#ifdef CONFIG_PM
	.suspend	= ambarella_input_suspend,
	.resume		= ambarella_input_resume,
#endif
	.driver		= {
		.name	= "ambarella-input",
		.owner	= THIS_MODULE,
	},
};

static int __init ambarella_input_init(void)
{
	int				retval = 0;

	mutex_lock(&pboard_info_lock);

	retval = platform_driver_register(&ambarella_input_driver);
	if (retval)
		printk(KERN_ERR "Register ambarella_input_driver failed %d!\n",
			retval);

#ifdef CONFIG_INPUT_AMBARELLA_IR
	if (platform_driver_register_ir())
		printk(KERN_ERR "Register ambarella_ir_driver failed!\n");
#endif
#ifdef CONFIG_INPUT_AMBARELLA_ADC
	if (platform_driver_register_adc())
		printk(KERN_ERR "Register ambarella_adc_driver failed!\n");
#endif

	return retval;
}

static void __exit ambarella_input_exit(void)
{
#ifdef CONFIG_INPUT_AMBARELLA_ADC
	platform_driver_unregister_adc();
#endif
#ifdef CONFIG_INPUT_AMBARELLA_IR
	platform_driver_unregister_ir();
#endif
	platform_driver_unregister(&ambarella_input_driver);
}

module_init(ambarella_input_init);
module_exit(ambarella_input_exit);

MODULE_DESCRIPTION("Ambarella Media Processor Generic Input Driver");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ambinput");

