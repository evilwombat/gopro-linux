/*
 * arch/arm/plat-ambarella/generic/pwm.c
 *
 * Author: Jay Zhang, <jzhang@ambarella.com>
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
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/dma-mapping.h>
#include <linux/pwm_backlight.h>

#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>
#include <mach/board.h>

#define PWM_1_THROUGH_4_DIVIDER	32

#define PWM_CMD_SIZE		(5)
#define PWM_MAX_INSTANCES	(4)
#define PWM_ARRAY_SIZE		(PWM_MAX_INSTANCES * PWM_CMD_SIZE)


/*================================ PWM Device ================================*/
struct reg_bit_field {
	unsigned int		addr;
	unsigned int		msb;
	unsigned int		lsb;
};

#define SET_REG_BIT_FILED(reg, val)	\
{	\
	unsigned int		_reg, _tmp, _val;	\
	\
	_tmp = 0x1 << ((reg).msb - (reg).lsb + 1);	\
	_tmp = _tmp - 1;	\
	_val = ((val) & _tmp) << (reg).lsb;	\
	_tmp = _tmp << (reg).lsb;	\
	_reg = amba_readl((reg).addr);	\
	_reg &= (~_tmp);	\
	_reg |= _val;	\
	amba_writel((reg).addr, _reg);	\
}

typedef unsigned int (*get_pwm_clock_t)(void);

struct pwm_device {
	unsigned int		pwm_id;
	unsigned int		gpio_id;
	get_pwm_clock_t		get_clock;
	struct reg_bit_field	enable;
	struct reg_bit_field	divider;
	struct reg_bit_field	high;
	struct reg_bit_field	low;
	unsigned int		use_count;
	const char		*label;
	struct list_head	node;
};

static DEFINE_MUTEX(pwm_lock);
static LIST_HEAD(pwm_list);

static void add_pwm_device(struct pwm_device *pwm)
{
	mutex_lock(&pwm_lock);
	list_add_tail(&pwm->node, &pwm_list);
	mutex_unlock(&pwm_lock);
}

int pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns)
{
	int			retval = 0;
	unsigned int		clock, on, off;

	if (!pwm->get_clock) {
		retval = -EINVAL;
		printk("%s: Can not get pwm clock!\n", __func__);
		goto pwm_config_exit;
	}
	clock = (pwm->get_clock() + 50000) / 100000;
	SET_REG_BIT_FILED(pwm->divider, PWM_1_THROUGH_4_DIVIDER - 1);

	on = (clock * duty_ns + 5000) / 10000;
	off = (clock * (period_ns - duty_ns) + 5000) / 10000;
	if (on == 0)
		on = 1;
	if (off == 0)
		off = 1;
	SET_REG_BIT_FILED(pwm->high, on - 1);
	SET_REG_BIT_FILED(pwm->low, off - 1);

pwm_config_exit:
	return retval;
}
EXPORT_SYMBOL(pwm_config);

int pwm_enable(struct pwm_device *pwm)
{
	SET_REG_BIT_FILED(pwm->enable, 1);
	ambarella_gpio_config(pwm->gpio_id, GPIO_FUNC_HW);

	return 0;
}
EXPORT_SYMBOL(pwm_enable);

void pwm_disable(struct pwm_device *pwm)
{
	SET_REG_BIT_FILED(pwm->enable, 0);
	ambarella_gpio_config(pwm->gpio_id, GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_set(pwm->gpio_id, GPIO_LOW);
}
EXPORT_SYMBOL(pwm_disable);

struct pwm_device *pwm_request(int pwm_id, const char *label)
{
	struct pwm_device *pwm;
	int found = 0;

	mutex_lock(&pwm_lock);

	list_for_each_entry(pwm, &pwm_list, node) {
		if (pwm->pwm_id == pwm_id) {
			found = 1;
			break;
		}
	}

	if (found) {
		if (pwm->use_count == 0) {
			pwm->use_count++;
			pwm->label = label;
		} else
			pwm = ERR_PTR(-EBUSY);
	} else
		pwm = ERR_PTR(-ENOENT);

	mutex_unlock(&pwm_lock);
	return pwm;
}
EXPORT_SYMBOL(pwm_request);

void pwm_free(struct pwm_device *pwm)
{
	mutex_lock(&pwm_lock);

	if (pwm->use_count) {
		pwm->use_count--;
		pwm->label = NULL;
	} else
		pr_warning("PWM device already freed\n");

	mutex_unlock(&pwm_lock);
}
EXPORT_SYMBOL(pwm_free);

static unsigned int get_pwm_1_through_4_clock_hz(void)
{
	return 13 * get_so_freq_hz() / (PWM_1_THROUGH_4_DIVIDER << 3);
}

static struct pwm_device ambarella_pwm0 = {
	.pwm_id		= 0,
	.gpio_id	= GPIO(16),
	.get_clock	= get_pwm_freq_hz,
	.enable		= {
		.addr	= PWM_ENABLE_REG,
		.msb	= 0,
		.lsb	= 0,
	},
	.divider	= {
		.addr	= PWM_MODE_REG,
		.msb	= 10,
		.lsb	= 1,
	},
	.high		= {
		.addr	= PWM_CONTROL_REG,
		.msb	= 31,
		.lsb	= 16,
	},
	.low		= {
		.addr	= PWM_CONTROL_REG,
		.msb	= 15,
		.lsb	= 0,
	},
	.use_count = 0,
	.label = NULL,
};

static struct pwm_device ambarella_pwm1 = {
	.pwm_id		= 1,
	.gpio_id	= GPIO(45),
	.get_clock	= get_pwm_1_through_4_clock_hz,
	.enable		= {
		.addr	= PWM_B0_ENABLE_REG,
		.msb	= 0,
		.lsb	= 0,
	},
	.divider	= {
		.addr	= PWM_B0_ENABLE_REG,
		.msb	= 10,
		.lsb	= 1,
	},
	.high		= {
		.addr	= PWM_B0_DATA1_REG,
		.msb	= 19,
		.lsb	= 10,
	},
	.low		= {
		.addr	= PWM_B0_DATA1_REG,
		.msb	= 9,
		.lsb	= 0,
	},
	.use_count = 0,
	.label = NULL,
};

static struct pwm_device ambarella_pwm2 = {
	.pwm_id		= 2,
	.gpio_id	= GPIO(46),
	.get_clock	= get_pwm_1_through_4_clock_hz,
	.enable		= {
		.addr	= PWM_B1_ENABLE_REG,
		.msb	= 0,
		.lsb	= 0,
	},
	.divider	= {
		.addr	= PWM_B1_ENABLE_REG,
		.msb	= 10,
		.lsb	= 1,
	},
	.high		= {
		.addr	= PWM_B1_DATA1_REG,
		.msb	= 19,
		.lsb	= 10,
	},
	.low		= {
		.addr	= PWM_B1_DATA1_REG,
		.msb	= 9,
		.lsb	= 0,
	},
	.use_count = 0,
	.label = NULL,
};

static struct pwm_device ambarella_pwm3 = {
	.pwm_id		= 3,
	.gpio_id	= GPIO(50),
	.get_clock	= get_pwm_1_through_4_clock_hz,
	.enable		= {
		.addr	= PWM_C0_ENABLE_REG,
		.msb	= 0,
		.lsb	= 0,
	},
	.divider	= {
		.addr	= PWM_C0_ENABLE_REG,
		.msb	= 10,
		.lsb	= 1,
	},
	.high		= {
		.addr	= PWM_C0_DATA1_REG,
		.msb	= 19,
		.lsb	= 10,
	},
	.low		= {
		.addr	= PWM_C0_DATA1_REG,
		.msb	= 9,
		.lsb	= 0,
	},
	.use_count = 0,
	.label = NULL,
};

static struct pwm_device ambarella_pwm4 = {
	.pwm_id		= 4,
	.gpio_id	= GPIO(51),
	.get_clock	= get_pwm_1_through_4_clock_hz,
	.enable		= {
		.addr	= PWM_C1_ENABLE_REG,
		.msb	= 0,
		.lsb	= 0,
	},
	.divider	= {
		.addr	= PWM_C1_ENABLE_REG,
		.msb	= 10,
		.lsb	= 1,
	},
	.high		= {
		.addr	= PWM_C1_DATA1_REG,
		.msb	= 19,
		.lsb	= 10,
	},
	.low		= {
		.addr	= PWM_C1_DATA1_REG,
		.msb	= 9,
		.lsb	= 0,
	},
	.use_count = 0,
	.label = NULL,
};

/*============================= PWM Backlight Device =========================*/
static int pwm_backlight_init(struct device *dev)
{
	int					retval = 0;
	struct platform_device			*pdev;
	struct platform_pwm_backlight_data	*data;

	pdev = container_of(dev, struct platform_device, dev);
	data = pdev->dev.platform_data;

	switch (pdev->id) {
	case 0:
		data->max_brightness	=
			ambarella_board_generic.pwm0_config.max_duty;
		data->dft_brightness	=
			0;
		data->pwm_period_ns	=
			ambarella_board_generic.pwm0_config.period_ns;
		break;

	case 1:
		data->max_brightness	=
			ambarella_board_generic.pwm1_config.max_duty;
		data->dft_brightness	=
			ambarella_board_generic.pwm1_config.max_duty;
		data->pwm_period_ns	=
			ambarella_board_generic.pwm1_config.period_ns;
		break;

	case 2:
		data->max_brightness	=
			ambarella_board_generic.pwm2_config.max_duty;
		data->dft_brightness	=
			ambarella_board_generic.pwm2_config.max_duty;
		data->pwm_period_ns	=
			ambarella_board_generic.pwm2_config.period_ns;
		break;

	case 3:
		data->max_brightness	=
			ambarella_board_generic.pwm3_config.max_duty;
		data->dft_brightness	=
			ambarella_board_generic.pwm3_config.max_duty;
		data->pwm_period_ns	=
			ambarella_board_generic.pwm3_config.period_ns;
		break;

	case 4:
		data->max_brightness	=
			ambarella_board_generic.pwm4_config.max_duty;
		data->dft_brightness	=
			ambarella_board_generic.pwm4_config.max_duty;
		data->pwm_period_ns	=
			ambarella_board_generic.pwm4_config.period_ns;
		break;

	default:
		retval = -EINVAL;
		break;
	}

	return retval;
}

static struct platform_pwm_backlight_data amb_pwm0_pdata = {
	.pwm_id		= 0,
	.max_brightness	= 255,
	.dft_brightness	= 255,
	.pwm_period_ns	= 40000,
	.init		= pwm_backlight_init,
	.notify		= NULL,
	.exit		= NULL,
};

struct platform_device ambarella_pwm_platform_device0 = {
	.name		= "pwm-backlight",
	.id		= 0,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &amb_pwm0_pdata,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

static struct platform_pwm_backlight_data amb_pwm1_pdata = {
	.pwm_id		= 1,
	.max_brightness	= 100,
	.dft_brightness	= 100,
	.pwm_period_ns	= 10000,
	.init		= pwm_backlight_init,
	.notify		= NULL,
	.exit		= NULL,
};

struct platform_device ambarella_pwm_platform_device1 = {
	.name		= "pwm-backlight",
	.id		= 1,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &amb_pwm1_pdata,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

static struct platform_pwm_backlight_data amb_pwm2_pdata = {
	.pwm_id		= 2,
	.max_brightness	= 100,
	.dft_brightness	= 100,
	.pwm_period_ns	= 10000,
	.init		= pwm_backlight_init,
	.notify		= NULL,
	.exit		= NULL,
};

struct platform_device ambarella_pwm_platform_device2 = {
	.name		= "pwm-backlight",
	.id		= 2,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &amb_pwm2_pdata,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

static struct platform_pwm_backlight_data amb_pwm3_pdata = {
	.pwm_id		= 3,
	.max_brightness	= 100,
	.dft_brightness	= 100,
	.pwm_period_ns	= 10000,
	.init		= pwm_backlight_init,
	.notify		= NULL,
	.exit		= NULL,
};

struct platform_device ambarella_pwm_platform_device3 = {
	.name		= "pwm-backlight",
	.id		= 3,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &amb_pwm3_pdata,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

static struct platform_pwm_backlight_data amb_pwm4_pdata = {
	.pwm_id		= 4,
	.max_brightness	= 100,
	.dft_brightness	= 100,
	.pwm_period_ns	= 10000,
	.init		= pwm_backlight_init,
	.notify		= NULL,
	.exit		= NULL,
};

struct platform_device ambarella_pwm_platform_device4 = {
	.name		= "pwm-backlight",
	.id		= 4,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &amb_pwm4_pdata,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

#ifdef CONFIG_AMBARELLA_PWM_PROC
static u32 pwm_array[PWM_ARRAY_SIZE];
static const char pwm_proc_name[] = "pwm";
static struct proc_dir_entry *pwm_file;

static int ambarella_pwm_proc_write(struct file *file,
	const char __user *buffer, unsigned long count, void *data)
{
	int					retval = 0;
	int					cmd_cnt;
	int					i;
	u32					pwm_ch;
	u32					enable;
	u32					xon;
	u32					xoff;
	u32					div;
	u32					data_reg;

	if (count > sizeof(pwm_array)) {
		pr_err("%s: count %d out of size!\n", __func__, (u32)count);
		retval = -ENOSPC;
		goto ambarella_pwm_proc_write_exit;
	}

	if (copy_from_user(pwm_array, buffer, count)) {
		pr_err("%s: copy_from_user fail!\n", __func__);
		retval = -EFAULT;
		goto ambarella_pwm_proc_write_exit;
	}

	cmd_cnt = count / (PWM_CMD_SIZE * sizeof(pwm_array[0]));
	for (i = 0; i < cmd_cnt; i++) {
		pwm_ch = pwm_array[i * PWM_CMD_SIZE];
		enable = pwm_array[i * PWM_CMD_SIZE + 1];
		xon = pwm_array[i * PWM_CMD_SIZE + 2];
		xoff = pwm_array[i * PWM_CMD_SIZE + 3];
		div = pwm_array[i * PWM_CMD_SIZE + 4];

		if (pwm_ch == 0)
			data_reg = ((xon - 1) << 16) + (xoff - 1);
		else
			data_reg = ((xon - 1) << 10) + (xoff - 1);

		switch (pwm_ch) {
		case 0:
			amba_writel(PWM_CONTROL_REG, data_reg);
			amba_writel(PWM_ENABLE_REG, enable);
			amba_writel(PWM_MODE_REG, div << 1);
			break;

		case 1:
			amba_writel(PWM_B0_DATA1_REG, data_reg);
			amba_writel(PWM_B0_ENABLE_REG, enable + (div << 1));
			break;

		case 2:
			amba_writel(PWM_B1_DATA1_REG, data_reg);
			amba_writel(PWM_B1_ENABLE_REG, enable + (div << 1));
			break;

		case 3:
			amba_writel(PWM_C0_DATA1_REG, data_reg);
			amba_writel(PWM_C0_ENABLE_REG, enable + (div << 1));
			break;

		case 4:
			amba_writel(PWM_C1_DATA1_REG, data_reg);
			amba_writel(PWM_C1_ENABLE_REG, enable + (div << 1));
			break;

		default:
			pr_warning("%s: invalid pwm channel id %d!\n",
				__func__, pwm_ch);
			break;
		}
	}

	retval = count;

ambarella_pwm_proc_write_exit:
	return retval;
}
#endif

int __init ambarella_init_pwm(void)
{
	int					retval = 0;

#ifdef CONFIG_AMBARELLA_PWM_PROC
	pwm_file = create_proc_entry(pwm_proc_name, S_IRUGO | S_IWUSR,
		get_ambarella_proc_dir());
	if (pwm_file == NULL) {
		retval = -ENOMEM;
		pr_err("%s: %s fail!\n", __func__, pwm_proc_name);
	} else {
		pwm_file->read_proc = NULL;
		pwm_file->write_proc = ambarella_pwm_proc_write;
	}
#endif

	add_pwm_device(&ambarella_pwm0);
	if (AMBARELLA_BOARD_TYPE(system_rev) != AMBARELLA_BOARD_TYPE_BUB)
		add_pwm_device(&ambarella_pwm1);
	add_pwm_device(&ambarella_pwm2);
	add_pwm_device(&ambarella_pwm3);
	add_pwm_device(&ambarella_pwm4);

	return retval;
}

