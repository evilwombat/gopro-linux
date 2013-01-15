/*
 * arch/arm/plat-ambarella/generic/gpio.c
 *
 * History:
 *	2007/11/21 - [Grady Chen] created file
 *	2008/01/08 - [Anthony Ginger] Add GENERIC_GPIO support.
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
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/delay.h>

#include <linux/proc_fs.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>

#include <linux/aipc/i_gpio.h>

#define CONFIG_AMBARELLA_GPIO_CAN_SLEEP		(0)
#define CONFIG_AMBARELLA_GPIO_ACCESS_MODE	(0)

#if 0
#define amba_dbg printk
#else
#define amba_dbg(...) //printk
#endif

/* ==========================================================================*/
static inline int ambarella_gpio_base_to_id(u32 base, u32 offset)
{
	int id=-1;

	switch(base){
	case GPIO0_BASE:
		id=0;
		break;
	case GPIO1_BASE:
		id=GPIO_BANK_SIZE;
		break;
	case GPIO2_BASE:
		id=GPIO_BANK_SIZE * 2;
		break;
	case GPIO3_BASE:
		id=GPIO_BANK_SIZE * 3;
		break;
	case GPIO4_BASE:
		id=GPIO_BANK_SIZE * 4;
		break;
	case GPIO5_BASE:
		id=GPIO_BANK_SIZE * 5;
		break;
	default:
		id=-1;
		break;
	}

	if (id < 0) {
		pr_err("%s: invalid GPIO base:0x%08x offset:0x%08x.\n",
			__func__, base, offset);
		return id;
	}

	id |= offset & 0x1f;

	return id;
}

static int ambarella_gpio_inline_config(u32 base, u32 offset, int func)
{
	int	retval = 0;
	int	id = 0;

amba_dbg("\n\n%s\n\n",__func__);
	id=ambarella_gpio_base_to_id(base, offset);
	if(id<0){
		pr_err("%s: invalid GPIO func %d for 0x%x:0x%x.\n",
			__func__, func, base, offset);
		retval = -EINVAL;
	} else {
		if(ipc_gpio_config(id, func)<0){
			retval = -EINVAL;
		}
	}

	return retval;
}

static void ambarella_gpio_inline_set(u32 base, u32 offset, int value)
{
	int	id = 0;

amba_dbg("\n\n%s\n\n",__func__);
	id=ambarella_gpio_base_to_id(base, offset);
	if(id<0){
		pr_err("%s: invalid GPIO 0x%x:0x%x.\n",
			__func__, base, offset);
		return;
	}

	if (value == GPIO_LOW) {
		ipc_gpio_clr(id);
	} else {
		ipc_gpio_set(id);
	}
}

static int ambarella_gpio_inline_get(u32 base, u32 offset)
{
	u32	val = 0;
	int	id = 0;

amba_dbg("\n\n%s\n\n",__func__);
	id=ambarella_gpio_base_to_id(base, offset);
	if(id<0){
		pr_err("%s: invalid GPIO 0x%x:0x%x.\n",
			__func__, base, offset);
		return GPIO_LOW;
	}

	val = ipc_gpio_get(id);

	if(val<=0){
		return GPIO_LOW;
	}

	return GPIO_HIGH;
}

/* ==========================================================================*/
void ambarella_gpio_config(int id, int func)
{
	amba_dbg("\n\n%s\n\n",__func__);
	ipc_gpio_config(id, func);
}
EXPORT_SYMBOL(ambarella_gpio_config);

void ambarella_gpio_set(int id, int value)
{
	amba_dbg("\n\n%s\n\n",__func__);
	if(value){
		ipc_gpio_set(id);
	} else {
		ipc_gpio_clr(id);
	}
}
EXPORT_SYMBOL(ambarella_gpio_set);

int ambarella_gpio_get(int id)
{
	int ret=0;

	amba_dbg("\n\n%s\n\n",__func__);
	ret=ipc_gpio_get(id);
	if(ret<0){
		pr_err("%s: invalid GPIO %d.(ERR:%d)\n",
			__func__, id, ret);
		ret=0;
	}

	return ret;
}
EXPORT_SYMBOL(ambarella_gpio_get);

/* ==========================================================================*/
struct ambarella_gpio_chip {
	struct gpio_chip	chip;
	u32			mem_base;
};
#define to_ambarella_gpio_chip(c) container_of(c, struct ambarella_gpio_chip, chip)

static DEFINE_MUTEX(ambarella_gpio_lock);
static unsigned long ambarella_gpio_valid[BITS_TO_LONGS(AMBGPIO_SIZE)];
static unsigned long ambarella_gpio_freeflag[BITS_TO_LONGS(AMBGPIO_SIZE)];

static int ambarella_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	int					retval = 0;
	struct ambarella_gpio_chip		*ambarella_chip;

amba_dbg("\n\n%s\n\n",__func__);
	ambarella_chip = to_ambarella_gpio_chip(chip);

	mutex_lock(&ambarella_gpio_lock);

	if (test_bit((chip->base + offset), ambarella_gpio_valid)) {
		if (test_bit((chip->base + offset), ambarella_gpio_freeflag)) {
			__clear_bit((chip->base + offset),
				ambarella_gpio_freeflag);
		} else {
			retval = -EACCES;
		}
	} else {
		retval = -EPERM;
	}

	mutex_unlock(&ambarella_gpio_lock);

	return retval;
}

static void ambarella_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	struct ambarella_gpio_chip		*ambarella_chip;

amba_dbg("\n\n%s\n\n",__func__);
	ambarella_chip = to_ambarella_gpio_chip(chip);

	mutex_lock(&ambarella_gpio_lock);

	__set_bit((chip->base + offset), ambarella_gpio_freeflag);

	mutex_unlock(&ambarella_gpio_lock);
}

static int ambarella_gpio_chip_direction_input(struct gpio_chip *chip,
	unsigned offset)
{
	int					retval = 0;
	struct ambarella_gpio_chip		*ambarella_chip;

amba_dbg("\n\n%s\n\n",__func__);
	ambarella_chip = to_ambarella_gpio_chip(chip);

	mutex_lock(&ambarella_gpio_lock);

	retval = ambarella_gpio_inline_config(
		ambarella_chip->mem_base,
		offset,
		GPIO_FUNC_SW_INPUT);

	mutex_unlock(&ambarella_gpio_lock);

	return retval;
}

static int ambarella_gpio_chip_get(struct gpio_chip *chip,
	unsigned offset)
{
	int					retval = 0;
	struct ambarella_gpio_chip		*ambarella_chip;

amba_dbg("\n\n%s\n\n",__func__);
	ambarella_chip = to_ambarella_gpio_chip(chip);

#if (CONFIG_AMBARELLA_GPIO_CAN_SLEEP == 1)
	mutex_lock(&ambarella_gpio_lock);
#endif

	retval = ambarella_gpio_inline_get(ambarella_chip->mem_base,
		offset);

#if (CONFIG_AMBARELLA_GPIO_CAN_SLEEP == 1)
	mutex_unlock(&ambarella_gpio_lock);
#endif

	return retval;
}

static int ambarella_gpio_chip_direction_output(struct gpio_chip *chip,
	unsigned offset, int val)
{
	int					retval = 0;
	struct ambarella_gpio_chip		*ambarella_chip;

amba_dbg("\n\n%s\n\n",__func__);
	ambarella_chip = to_ambarella_gpio_chip(chip);

	mutex_lock(&ambarella_gpio_lock);

	retval = ambarella_gpio_inline_config(
		ambarella_chip->mem_base,
		offset,
		GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_inline_set(ambarella_chip->mem_base, offset, val);

	mutex_unlock(&ambarella_gpio_lock);

	return retval;
}

static void ambarella_gpio_chip_set(struct gpio_chip *chip,
	unsigned offset, int val)
{
	struct ambarella_gpio_chip		*ambarella_chip;

amba_dbg("\n\n%s\n\n",__func__);
	ambarella_chip = to_ambarella_gpio_chip(chip);

#if (CONFIG_AMBARELLA_GPIO_CAN_SLEEP == 1)
	mutex_lock(&ambarella_gpio_lock);
#endif

	ambarella_gpio_inline_set(ambarella_chip->mem_base, offset, val);

#if (CONFIG_AMBARELLA_GPIO_CAN_SLEEP == 1)
	mutex_unlock(&ambarella_gpio_lock);
#endif
}

static int ambarella_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
amba_dbg("\n\n%s\n\n",__func__);
	return gpio_to_irq(chip->base + offset);
}

#if 0
static void ambarella_gpio_chip_dbg_show(struct seq_file *s,
	struct gpio_chip *chip)
{
	int					i;
	struct ambarella_gpio_chip		*ambarella_chip;
	u32					afsel;
	u32					mask;
	u32					data;
	u32					dir;

	ambarella_chip = to_ambarella_gpio_chip(chip);

	afsel = amba_readl(ambarella_chip->mem_base + GPIO_AFSEL_OFFSET);
	mask = amba_readl(ambarella_chip->mem_base + GPIO_MASK_OFFSET);
	data = amba_readl(ambarella_chip->mem_base + GPIO_DATA_OFFSET);
	dir = amba_readl(ambarella_chip->mem_base + GPIO_DIR_OFFSET);

	for (i = 0; i < chip->ngpio; i++) {
		if ((afsel & (1 << i)) && !(mask & (1 << i)))
			seq_printf(s, "GPIO %s%d: HW\n", chip->label, i);
		else if (!(afsel & (1 << i)) && (mask & (1 << i)))
			seq_printf(s, "GPIO %s%d:\t%s\t%s\n", chip->label, i,
				   (data & (1 << i)) ? "set" : "clear",
				   (dir & (1 << i)) ? "out" : "in");
		else
			seq_printf(s, "GPIO %s%d: unknown!\n", chip->label, i);
	}
}
#else
static void ambarella_gpio_chip_dbg_show(struct seq_file *s,
	struct gpio_chip *chip)
{
	pr_err("%s: is not supported now!\n", __func__);
}
#endif

#define AMBARELLA_GPIO_BANK(name, reg_base, base_gpio)			\
{									\
	.chip = {							\
		.label			= name,				\
		.owner			= THIS_MODULE,			\
		.request		= ambarella_gpio_request,	\
		.free			= ambarella_gpio_free,		\
		.direction_input	= ambarella_gpio_chip_direction_input,	\
		.get			= ambarella_gpio_chip_get,	\
		.direction_output	= ambarella_gpio_chip_direction_output, \
		.set			= ambarella_gpio_chip_set,	\
		.to_irq			= ambarella_gpio_to_irq,	\
		.dbg_show		= ambarella_gpio_chip_dbg_show,	\
		.base			= base_gpio,			\
		.ngpio			= GPIO_BANK_SIZE,		\
		.can_sleep		= CONFIG_AMBARELLA_GPIO_CAN_SLEEP,	\
		.exported		= 0,				\
	},								\
	.mem_base			= reg_base,			\
}

static struct ambarella_gpio_chip ambarella_gpio_banks[] = {
	AMBARELLA_GPIO_BANK("ambarella-gpio0", GPIO0_BASE, GPIO(0 * GPIO_BANK_SIZE)),
	AMBARELLA_GPIO_BANK("ambarella-gpio1", GPIO1_BASE, GPIO(1 * GPIO_BANK_SIZE)),
#if (GPIO_INSTANCES >= 3)
	AMBARELLA_GPIO_BANK("ambarella-gpio2", GPIO2_BASE, GPIO(2 * GPIO_BANK_SIZE)),
#endif
#if (GPIO_INSTANCES >= 4)
	AMBARELLA_GPIO_BANK("ambarella-gpio3", GPIO3_BASE, GPIO(3 * GPIO_BANK_SIZE)),
#endif
#if (GPIO_INSTANCES >= 5)
	AMBARELLA_GPIO_BANK("ambarella-gpio4", GPIO4_BASE, GPIO(4 * GPIO_BANK_SIZE)),
#endif
#if (GPIO_INSTANCES >= 6)
	AMBARELLA_GPIO_BANK("ambarella-gpio5", GPIO5_BASE, GPIO(5 * GPIO_BANK_SIZE)),
#endif
};

/* ==========================================================================*/
#ifdef CONFIG_AMBARELLA_GPIO_PROC
static unsigned char gpio_array[GPIO_MAX_LINES * 3];
static const char gpio_proc_name[] = "gpio";
static struct proc_dir_entry *gpio_file;

static int ambarella_gpio_proc_read(char *page, char **start,
	off_t off, int count, int *eof, void *data)
{
	unsigned short				i;

amba_dbg("\n\n%s\n\n",__func__);
	if (off >= GPIO_MAX_LINES) {
		*eof = 1;
		return 0;
	}

	if (off + count > GPIO_MAX_LINES) {
		*eof = 1;
		count = GPIO_MAX_LINES - off;
	}

	*start = page + off;
	for (i = off; i < (off + count); i++) {
		if (ambarella_gpio_get(i)) {
			page[i] = '1';
		} else {
			page[i] = '0';
		}
	}

	return count;
}

static int ambarella_gpio_proc_write(struct file *file,
	const char __user *buffer, unsigned long count, void *data)
{
	int					retval = 0;
	int					cmd_cnt;
	int					i;
	int					gpio_id;
	int					func;
amba_dbg("\n\n%s\n\n",__func__);
	if (count > sizeof(gpio_array)) {
		pr_err("%s: count %d out of size!\n", __func__, (u32)count);
		retval = -ENOSPC;
		goto ambarella_gpio_write_exit;
	}

	if (copy_from_user(gpio_array, buffer, count)) {
		pr_err("%s: copy_from_user fail!\n", __func__);
		retval = -EFAULT;
		goto ambarella_gpio_write_exit;
	}

	cmd_cnt = count / 3;
	for (i = 0; i < cmd_cnt; i++) {
		gpio_id = gpio_array[i * 3 + 1];
		func = gpio_array[i * 3 + 2];
		if ((gpio_array[i * 3] == 'C') || (gpio_array[i * 3] == 'c')) {
			ambarella_gpio_config(gpio_id, func);
			continue;
		}
		if ((gpio_array[i * 3] == 'W') || (gpio_array[i * 3] == 'w')) {
			ambarella_gpio_set(gpio_id, func);
			continue;
		}
	}

	retval = count;

ambarella_gpio_write_exit:
	return retval;
}
#endif

/* ==========================================================================*/
void __init ambarella_gpio_set_valid(unsigned pin, int valid)
{
amba_dbg("\n\n%s\n\n",__func__);
	if (likely(pin >=0 && pin < AMBGPIO_SIZE)) {
		if (valid)
			__set_bit(pin, ambarella_gpio_valid);
		else
			__clear_bit(pin, ambarella_gpio_valid);
	}
}

int __init ambarella_init_gpio(void)
{
	int					retval = 0;
	int					i;

amba_dbg("\n\n%s\n\n",__func__);
	memset(ambarella_gpio_valid, 0xff, sizeof(ambarella_gpio_valid));
	memset(ambarella_gpio_freeflag, 0xff, sizeof(ambarella_gpio_freeflag));
	for (i = GPIO_MAX_LINES + 1; i < AMBGPIO_SIZE; i++) {
		ambarella_gpio_set_valid(i, 0);
		__clear_bit(i, ambarella_gpio_freeflag);
	}

	for (i = 0; i < ARRAY_SIZE(ambarella_gpio_banks); i++) {
		retval = gpiochip_add(&ambarella_gpio_banks[i].chip);
		if (retval) {
			printk("%s: gpiochip_add %s fail %d.\n",
				__func__,
				ambarella_gpio_banks[i].chip.label,
				retval);
			retval = -EINVAL;
			break;
		}
	}

#ifdef CONFIG_AMBARELLA_GPIO_PROC
	gpio_file = create_proc_entry(gpio_proc_name, S_IRUGO | S_IWUSR,
		get_ambarella_proc_dir());
	if (gpio_file == NULL) {
		pr_err("%s: %s fail!\n", __func__, gpio_proc_name);
		retval = -ENOMEM;
	} else {
		gpio_file->read_proc = ambarella_gpio_proc_read;
		gpio_file->write_proc = ambarella_gpio_proc_write;
	}
#endif

#if 0
#if defined(CONFIG_AMBARELLA_RAW_BOOT)
	amba_writel(GPIO0_AFSEL_REG, 0xFFFFFFFF);
	amba_writel(GPIO0_DIR_REG, 0x00000000);
	amba_writel(GPIO0_MASK_REG, 0xFFFFFFFF);
	amba_writel(GPIO0_DATA_REG, 0x00000000);
	amba_writel(GPIO0_ENABLE_REG, 0xFFFFFFFF);

	amba_writel(GPIO1_AFSEL_REG, 0xFFFFFFFF);
	amba_writel(GPIO1_DIR_REG, 0x00000000);
	amba_writel(GPIO1_MASK_REG, 0xFFFFFFFF);
	amba_writel(GPIO1_DATA_REG, 0x00000000);
	amba_writel(GPIO1_ENABLE_REG, 0xFFFFFFFF);

#if (GPIO_INSTANCES >= 3)
	amba_writel(GPIO2_AFSEL_REG, 0xFFFFFFFF);
	amba_writel(GPIO2_DIR_REG, 0x00000000);
	amba_writel(GPIO2_MASK_REG, 0xFFFFFFFF);
	amba_writel(GPIO2_DATA_REG, 0x00000000);
	amba_writel(GPIO2_ENABLE_REG, 0xFFFFFFFF);
#endif
#if (GPIO_INSTANCES >= 4)
	amba_writel(GPIO3_AFSEL_REG, 0xFFFFFFFF);
	amba_writel(GPIO3_DIR_REG, 0x00000000);
	amba_writel(GPIO3_MASK_REG, 0xFFFFFFFF);
	amba_writel(GPIO3_DATA_REG, 0x00000000);
	amba_writel(GPIO3_ENABLE_REG, 0xFFFFFFFF);
#endif
#if (GPIO_INSTANCES >= 5)
	amba_writel(GPIO4_AFSEL_REG, 0xFFFFFFFF);
	amba_writel(GPIO4_DIR_REG, 0x00000000);
	amba_writel(GPIO4_MASK_REG, 0xFFFFFFFF);
	amba_writel(GPIO4_DATA_REG, 0x00000000);
	amba_writel(GPIO4_ENABLE_REG, 0xFFFFFFFF);
#endif
#if (GPIO_INSTANCES >= 6)
	amba_writel(GPIO5_AFSEL_REG, 0xFFFFFFFF);
	amba_writel(GPIO5_DIR_REG, 0x00000000);
	amba_writel(GPIO5_MASK_REG, 0xFFFFFFFF);
	amba_writel(GPIO5_DATA_REG, 0x00000000);
	amba_writel(GPIO5_ENABLE_REG, 0xFFFFFFFF);
#endif
#endif
#endif

	return retval;
}

/* ==========================================================================*/
struct ambarella_gpio_pm_info {
	u32 data_reg;
	u32 dir_reg;
	u32 is_reg;
	u32 ibe_reg;
	u32 iev_reg;
	u32 ie_reg;
	u32 afsel_reg;
	u32 mask_reg;
};

struct ambarella_gpio_pm_info ambarella_gpio0_pm;
struct ambarella_gpio_pm_info ambarella_gpio1_pm;
#if (GPIO_INSTANCES >= 3)
struct ambarella_gpio_pm_info ambarella_gpio2_pm;
#endif
#if (GPIO_INSTANCES >= 4)
struct ambarella_gpio_pm_info ambarella_gpio3_pm;
#endif
#if (GPIO_INSTANCES >= 5)
struct ambarella_gpio_pm_info ambarella_gpio4_pm;
#endif
#if (GPIO_INSTANCES >= 6)
struct ambarella_gpio_pm_info ambarella_gpio5_pm;
#endif

#if 0
u32 ambarella_gpio_suspend(u32 level)
{
	ambarella_gpio0_pm.data_reg   = amba_readl(GPIO0_DATA_REG);
	ambarella_gpio0_pm.dir_reg    = amba_readl(GPIO0_DIR_REG);
	ambarella_gpio0_pm.is_reg     = amba_readl(GPIO0_IS_REG);
	ambarella_gpio0_pm.ibe_reg    = amba_readl(GPIO0_IBE_REG);
	ambarella_gpio0_pm.iev_reg    = amba_readl(GPIO0_IEV_REG);
	ambarella_gpio0_pm.ie_reg     = amba_readl(GPIO0_IE_REG);
	ambarella_gpio0_pm.afsel_reg  = amba_readl(GPIO0_AFSEL_REG);
	ambarella_gpio0_pm.mask_reg   = amba_readl(GPIO0_MASK_REG);

	ambarella_gpio1_pm.data_reg   = amba_readl(GPIO1_DATA_REG);
	ambarella_gpio1_pm.dir_reg    = amba_readl(GPIO1_DIR_REG);
	ambarella_gpio1_pm.is_reg     = amba_readl(GPIO1_IS_REG);
	ambarella_gpio1_pm.ibe_reg    = amba_readl(GPIO1_IBE_REG);
	ambarella_gpio1_pm.iev_reg    = amba_readl(GPIO1_IEV_REG);
	ambarella_gpio1_pm.ie_reg     = amba_readl(GPIO1_IE_REG);
	ambarella_gpio1_pm.afsel_reg  = amba_readl(GPIO1_AFSEL_REG);
	ambarella_gpio1_pm.mask_reg   = amba_readl(GPIO1_MASK_REG);

#if (GPIO_INSTANCES >= 3)
	ambarella_gpio2_pm.data_reg   = amba_readl(GPIO2_DATA_REG);
	ambarella_gpio2_pm.dir_reg    = amba_readl(GPIO2_DIR_REG);
	ambarella_gpio2_pm.is_reg     = amba_readl(GPIO2_IS_REG);
	ambarella_gpio2_pm.ibe_reg    = amba_readl(GPIO2_IBE_REG);
	ambarella_gpio2_pm.iev_reg    = amba_readl(GPIO2_IEV_REG);
	ambarella_gpio2_pm.ie_reg     = amba_readl(GPIO2_IE_REG);
	ambarella_gpio2_pm.afsel_reg  = amba_readl(GPIO2_AFSEL_REG);
	ambarella_gpio2_pm.mask_reg   = amba_readl(GPIO2_MASK_REG);
#endif

#if (GPIO_INSTANCES >= 4)
	ambarella_gpio3_pm.data_reg   = amba_readl(GPIO3_DATA_REG);
	ambarella_gpio3_pm.dir_reg    = amba_readl(GPIO3_DIR_REG);
	ambarella_gpio3_pm.is_reg     = amba_readl(GPIO3_IS_REG);
	ambarella_gpio3_pm.ibe_reg    = amba_readl(GPIO3_IBE_REG);
	ambarella_gpio3_pm.iev_reg    = amba_readl(GPIO3_IEV_REG);
	ambarella_gpio3_pm.ie_reg     = amba_readl(GPIO3_IE_REG);
	ambarella_gpio3_pm.afsel_reg  = amba_readl(GPIO3_AFSEL_REG);
	ambarella_gpio3_pm.mask_reg   = amba_readl(GPIO3_MASK_REG);
#endif

#if (GPIO_INSTANCES >= 5)
	ambarella_gpio4_pm.data_reg   = amba_readl(GPIO4_DATA_REG);
	ambarella_gpio4_pm.dir_reg    = amba_readl(GPIO4_DIR_REG);
	ambarella_gpio4_pm.is_reg     = amba_readl(GPIO4_IS_REG);
	ambarella_gpio4_pm.ibe_reg    = amba_readl(GPIO4_IBE_REG);
	ambarella_gpio4_pm.iev_reg    = amba_readl(GPIO4_IEV_REG);
	ambarella_gpio4_pm.ie_reg     = amba_readl(GPIO4_IE_REG);
	ambarella_gpio4_pm.afsel_reg  = amba_readl(GPIO4_AFSEL_REG);
	ambarella_gpio4_pm.mask_reg   = amba_readl(GPIO4_MASK_REG);
#endif

#if (GPIO_INSTANCES >= 6)
	ambarella_gpio5_pm.data_reg   = amba_readl(GPIO5_DATA_REG);
	ambarella_gpio5_pm.dir_reg    = amba_readl(GPIO5_DIR_REG);
	ambarella_gpio5_pm.is_reg     = amba_readl(GPIO5_IS_REG);
	ambarella_gpio5_pm.ibe_reg    = amba_readl(GPIO5_IBE_REG);
	ambarella_gpio5_pm.iev_reg    = amba_readl(GPIO5_IEV_REG);
	ambarella_gpio5_pm.ie_reg     = amba_readl(GPIO5_IE_REG);
	ambarella_gpio5_pm.afsel_reg  = amba_readl(GPIO5_AFSEL_REG);
	ambarella_gpio5_pm.mask_reg   = amba_readl(GPIO5_MASK_REG);
#endif

	return 0;
}

u32 ambarella_gpio_resume(u32 level)
{
	amba_writel(GPIO0_AFSEL_REG,  ambarella_gpio0_pm.afsel_reg);
	amba_writel(GPIO0_MASK_REG,   ambarella_gpio0_pm.mask_reg);
	amba_writel(GPIO0_DIR_REG,    ambarella_gpio0_pm.dir_reg);
	amba_writel(GPIO0_DATA_REG,   ambarella_gpio0_pm.data_reg);
	amba_writel(GPIO0_IS_REG,     ambarella_gpio0_pm.is_reg);
	amba_writel(GPIO0_IBE_REG,    ambarella_gpio0_pm.ibe_reg);
	amba_writel(GPIO0_IEV_REG,    ambarella_gpio0_pm.iev_reg);
	amba_writel(GPIO0_IE_REG,     ambarella_gpio0_pm.ie_reg);

	amba_writel(GPIO1_AFSEL_REG,  ambarella_gpio1_pm.afsel_reg);
	amba_writel(GPIO1_MASK_REG,   ambarella_gpio1_pm.mask_reg);
	amba_writel(GPIO1_DIR_REG,    ambarella_gpio1_pm.dir_reg);
	amba_writel(GPIO1_DATA_REG,   ambarella_gpio1_pm.data_reg);
	amba_writel(GPIO1_IS_REG,     ambarella_gpio1_pm.is_reg);
	amba_writel(GPIO1_IBE_REG,    ambarella_gpio1_pm.ibe_reg);
	amba_writel(GPIO1_IEV_REG,    ambarella_gpio1_pm.iev_reg);
	amba_writel(GPIO1_IE_REG,     ambarella_gpio1_pm.ie_reg);

#if (GPIO_INSTANCES >= 3)
	amba_writel(GPIO2_AFSEL_REG,  ambarella_gpio2_pm.afsel_reg);
	amba_writel(GPIO2_MASK_REG,   ambarella_gpio2_pm.mask_reg);
	amba_writel(GPIO2_DIR_REG,    ambarella_gpio2_pm.dir_reg);
	amba_writel(GPIO2_DATA_REG,   ambarella_gpio2_pm.data_reg);
	amba_writel(GPIO2_IS_REG,     ambarella_gpio2_pm.is_reg);
	amba_writel(GPIO2_IBE_REG,    ambarella_gpio2_pm.ibe_reg);
	amba_writel(GPIO2_IEV_REG,    ambarella_gpio2_pm.iev_reg);
	amba_writel(GPIO2_IE_REG,     ambarella_gpio2_pm.ie_reg);
#endif

#if (GPIO_INSTANCES >= 4)
	amba_writel(GPIO3_AFSEL_REG,  ambarella_gpio3_pm.afsel_reg);
	amba_writel(GPIO3_MASK_REG,   ambarella_gpio3_pm.mask_reg);
	amba_writel(GPIO3_DIR_REG,    ambarella_gpio3_pm.dir_reg);
	amba_writel(GPIO3_DATA_REG,   ambarella_gpio3_pm.data_reg);
	amba_writel(GPIO3_IS_REG,     ambarella_gpio3_pm.is_reg);
	amba_writel(GPIO3_IBE_REG,    ambarella_gpio3_pm.ibe_reg);
	amba_writel(GPIO3_IEV_REG,    ambarella_gpio3_pm.iev_reg);
	amba_writel(GPIO3_IE_REG,     ambarella_gpio3_pm.ie_reg);
#endif

#if (GPIO_INSTANCES >= 5)
	amba_writel(GPIO4_AFSEL_REG,  ambarella_gpio4_pm.afsel_reg);
	amba_writel(GPIO4_MASK_REG,   ambarella_gpio4_pm.mask_reg);
	amba_writel(GPIO4_DIR_REG,    ambarella_gpio4_pm.dir_reg);
	amba_writel(GPIO4_DATA_REG,   ambarella_gpio4_pm.data_reg);
	amba_writel(GPIO4_IS_REG,     ambarella_gpio4_pm.is_reg);
	amba_writel(GPIO4_IBE_REG,    ambarella_gpio4_pm.ibe_reg);
	amba_writel(GPIO4_IEV_REG,    ambarella_gpio4_pm.iev_reg);
	amba_writel(GPIO4_IE_REG,     ambarella_gpio4_pm.ie_reg);
#endif

#if (GPIO_INSTANCES >= 6)
	amba_writel(GPIO5_AFSEL_REG,  ambarella_gpio5_pm.afsel_reg);
	amba_writel(GPIO5_MASK_REG,   ambarella_gpio5_pm.mask_reg);
	amba_writel(GPIO5_DIR_REG,    ambarella_gpio5_pm.dir_reg);
	amba_writel(GPIO5_DATA_REG,   ambarella_gpio5_pm.data_reg);
	amba_writel(GPIO5_IS_REG,     ambarella_gpio5_pm.is_reg);
	amba_writel(GPIO5_IBE_REG,    ambarella_gpio5_pm.ibe_reg);
	amba_writel(GPIO5_IEV_REG,    ambarella_gpio5_pm.iev_reg);
	amba_writel(GPIO5_IE_REG,     ambarella_gpio5_pm.ie_reg);
#endif

	return 0;
}
#else
u32 ambarella_gpio_suspend(u32 level)
{
amba_dbg("\n\n%s\n\n",__func__);
	return 0;
}

u32 ambarella_gpio_resume(u32 level)
{
amba_dbg("\n\n%s\n\n",__func__);
	return 0;
}
#endif
/* ==========================================================================*/
static inline int ambarella_gpio_check(struct ambarella_gpio_io_info *pinfo)
{
	int					retval = 0;

	if (pinfo == NULL) {
		pr_err("%s: pinfo is NULL.\n", __func__);
		retval = -1;
		goto ambarella_gpio_check_exit;
	}

	if ((pinfo->gpio_id < 0 ) || (pinfo->gpio_id >= ARCH_NR_GPIOS)) {
		amba_dbg("%s: wrong gpio id %d.\n", __func__, pinfo->gpio_id);
		retval = -1;
		goto ambarella_gpio_check_exit;
	}

ambarella_gpio_check_exit:
	return retval;
}

static inline void ambarella_gpio_delay(int delay)
{
	if (delay > 0) {
#if (CONFIG_AMBARELLA_GPIO_CAN_SLEEP == 1)
		msleep(delay);
#else
		mdelay(delay);
#endif
	}
}

int ambarella_set_gpio_output(struct ambarella_gpio_io_info *pinfo, u32 on)
{
	int retval = 0;

amba_dbg("\n\n%s\n\n",__func__);

	retval = ambarella_gpio_check(pinfo);
	if (retval){
		goto ambarella_set_gpio_output_exit;
	}

	amba_dbg("%s: Gpio[%d] %s, level[%s], delay[%dms].\n",
		__func__,
		pinfo->gpio_id,
		on ? "ON" : "OFF",
		pinfo->active_level ? "HIGH" : "LOW",
		pinfo->active_delay);

#if (CONFIG_AMBARELLA_GPIO_ACCESS_MODE == 1)
	retval = 1;
	if (gpio_request(pinfo->gpio_id, __func__)) {
		printk("%s: Cannot request gpio%d!\n",
			__func__, pinfo->gpio_id);
		retval = 0;
	}

	if (on){
amba_dbg("\nSet GPIO Pin\n");
		gpio_direction_output(pinfo->gpio_id, pinfo->active_level);
	} else {
amba_dbg("\nClr GPIO Pin\n");
		gpio_direction_output(pinfo->gpio_id, !pinfo->active_level);
	}

	if (retval) {
		gpio_free(pinfo->gpio_id);
		retval = 0;
	}
#else
	ambarella_gpio_config(pinfo->gpio_id, GPIO_FUNC_SW_OUTPUT);
	if (on){
		ambarella_gpio_set(pinfo->gpio_id, pinfo->active_level);
	} else {
		ambarella_gpio_set(pinfo->gpio_id, !pinfo->active_level);
	}
#endif

	ambarella_gpio_delay(pinfo->active_delay);

ambarella_set_gpio_output_exit:
	return retval;
}
EXPORT_SYMBOL(ambarella_set_gpio_output);

u32 ambarella_get_gpio_input(struct ambarella_gpio_io_info *pinfo)
{
	int retval = 0;
	u32 gpio_value = 0;

amba_dbg("\n\n%s\n\n",__func__);

	retval = ambarella_gpio_check(pinfo);
	if (retval){
		goto ambarella_get_gpio_input_exit;
	}

#if (CONFIG_AMBARELLA_GPIO_ACCESS_MODE == 1)
	retval = 1;
	if (gpio_request(pinfo->gpio_id, __func__)) {
		printk("%s: Cannot request gpio%d!\n",
			__func__, pinfo->gpio_id);
		retval = 0;
	}

	gpio_direction_input(pinfo->gpio_id);
#else
	ambarella_gpio_config(pinfo->gpio_id, GPIO_FUNC_SW_INPUT);
#endif

	ambarella_gpio_delay(pinfo->active_delay);

#if (CONFIG_AMBARELLA_GPIO_ACCESS_MODE == 1)
	gpio_value = gpio_get_value(pinfo->gpio_id);
	if (retval) {
		gpio_free(pinfo->gpio_id);
		retval = 0;
	}
#else
	gpio_value = ambarella_gpio_get(pinfo->gpio_id);
#endif

	amba_dbg("%s: {gpio[%d], level[%s], delay[%dms]} get[%d].\n",
		__func__,
		pinfo->gpio_id,
		pinfo->active_level ? "HIGH" : "LOW",
		pinfo->active_delay,
		gpio_value);

ambarella_get_gpio_input_exit:
	return (gpio_value == pinfo->active_level) ? 1 : 0;
}
EXPORT_SYMBOL(ambarella_get_gpio_input);

int ambarella_set_gpio_reset(struct ambarella_gpio_io_info *pinfo)
{
	int					retval = 0;

amba_dbg("\n\n%s\n\n",__func__);

	retval = ambarella_gpio_check(pinfo);
	if (retval){
		goto ambarella_set_gpio_reset_exit;
	}

	amba_dbg("%s: Reset gpio[%d], level[%s], delay[%dms].\n",
		__func__,
		pinfo->gpio_id,
		pinfo->active_level ? "HIGH" : "LOW",
		pinfo->active_delay);

#if (CONFIG_AMBARELLA_GPIO_ACCESS_MODE == 1)
	retval = 1;
	if (gpio_request(pinfo->gpio_id, __func__)) {
		printk("%s: Cannot request gpio%d!\n",
			__func__, pinfo->gpio_id);
		retval = 0;
	}

	gpio_direction_output(pinfo->gpio_id, pinfo->active_level);
#else
	ambarella_gpio_config(pinfo->gpio_id, GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_set(pinfo->gpio_id, pinfo->active_level);
#endif

	ambarella_gpio_delay(pinfo->active_delay);

#if (CONFIG_AMBARELLA_GPIO_ACCESS_MODE == 1)
	gpio_direction_output(pinfo->gpio_id, !pinfo->active_level);
#else
	ambarella_gpio_set(pinfo->gpio_id, !pinfo->active_level);
#endif

	ambarella_gpio_delay(pinfo->active_delay);

#if (CONFIG_AMBARELLA_GPIO_ACCESS_MODE == 1)
	if (retval) {
		gpio_free(pinfo->gpio_id);
		retval = 0;
	}
#endif

ambarella_set_gpio_reset_exit:
	return retval;
}
EXPORT_SYMBOL(ambarella_set_gpio_reset);

int ambarella_is_valid_gpio_irq(struct ambarella_gpio_irq_info *pinfo)
{
	int					bvalid = 0;

amba_dbg("\n\n%s\n\n",__func__);

	if (pinfo == NULL) {
		printk("%s: pinfo is NULL.\n", __func__);
		goto ambarella_is_valid_gpio_irq_exit;
	}

	if ((pinfo->irq_gpio < 0 ) || (pinfo->irq_gpio >= ARCH_NR_GPIOS)){
//		printk("\n%s, %d\n",__func__,__LINE__);
		goto ambarella_is_valid_gpio_irq_exit;
	}

	if ((pinfo->irq_type != IRQ_TYPE_EDGE_RISING) &&
		(pinfo->irq_type != IRQ_TYPE_EDGE_FALLING) &&
		(pinfo->irq_type != IRQ_TYPE_EDGE_BOTH) &&
		(pinfo->irq_type != IRQ_TYPE_LEVEL_HIGH) &&
		(pinfo->irq_type != IRQ_TYPE_LEVEL_LOW)){
		printk("\n%s, %d\n",__func__,__LINE__);
		goto ambarella_is_valid_gpio_irq_exit;
	}

	if ((pinfo->irq_gpio_val != GPIO_HIGH) &&
		(pinfo->irq_gpio_val != GPIO_LOW)){
		printk("\n%s, %d\n",__func__,__LINE__);
		goto ambarella_is_valid_gpio_irq_exit;
	}

	if ((pinfo->irq_gpio_mode != GPIO_FUNC_SW_INPUT) &&
		(pinfo->irq_gpio_mode != GPIO_FUNC_SW_OUTPUT) &&
		(pinfo->irq_gpio_mode != GPIO_FUNC_HW)){
		printk("\n%s, %d\n",__func__,__LINE__);
		goto ambarella_is_valid_gpio_irq_exit;
	}

	if ((pinfo->irq_gpio_mode != GPIO_FUNC_HW) &&
		((pinfo->irq_line < GPIO_INT_VEC(0)) ||
		(pinfo->irq_line >= NR_IRQS))){
		printk("\n%s, %d\n",__func__,__LINE__);
		goto ambarella_is_valid_gpio_irq_exit;
	}

	bvalid = 1;

ambarella_is_valid_gpio_irq_exit:
	return bvalid;
}
EXPORT_SYMBOL(ambarella_is_valid_gpio_irq);

