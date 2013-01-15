/*
 * arch/arm/plat-ambarella/generic/pll_rct.c
 *
 * Author: Cao Rongrong, <rrcao@ambarella.com>
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
#include <linux/proc_fs.h>

#include <asm/uaccess.h>
#include <mach/hardware.h>

#include <plat/pll.h>
#include <plat/timer.h>

/* ==========================================================================*/
struct ambarella_pll_info {
	unsigned int armfreq;
};

/* ==========================================================================*/
static struct proc_dir_entry *freq_file = NULL;
static struct ambarella_pll_info pll_info;

/* ==========================================================================*/
static int ambarella_pll_rct_read(char *page, char **start,
	off_t off, int count, int *eof, void *data)
{
	int					retlen = 0;

	if (off != 0)
		return 0;

	retlen = scnprintf(page, count,
			"\nPLL Information:\n"
			"\tCore:\t%d Hz\n"
			"\tDram:\t%d Hz\n"
			"\tiDSP:\t%d Hz\n"
			"\tAHB:\t%d Hz\n"
			"\tAPB:\t%d Hz\n\n",
			get_core_bus_freq_hz(),
			get_dram_freq_hz(),
			get_idsp_freq_hz(),
			get_ahb_bus_freq_hz(),
			get_apb_bus_freq_hz());

	*eof = 1;

	return retlen;
}

static void ambarella_freq_set_pll(unsigned int new_freq_cpu)
{
	unsigned int				cur_freq_cpu;

#if ((CHIP_REV == A2) || (CHIP_REV == A3))
	do {
		cur_freq_cpu = amba_readl(PLL_CORE_CTRL_REG) & 0xfff00000;
		if (new_freq_cpu > pll_info.armfreq) {
			cur_freq_cpu += 0x01000000;
		} else {
			cur_freq_cpu -= 0x01000000;
		}
		cur_freq_cpu |= (amba_readl(PLL_CORE_CTRL_REG) & 0x000fffff);
		amba_writel(PLL_CORE_CTRL_REG, cur_freq_cpu);
		mdelay(20);
		cur_freq_cpu = get_core_bus_freq_hz();
	} while ((new_freq_cpu > pll_info.armfreq &&
		cur_freq_cpu < new_freq_cpu) ||
		(new_freq_cpu < pll_info.armfreq &&
		cur_freq_cpu > new_freq_cpu));
#elif ((CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
	(CHIP_REV == A5) || (CHIP_REV == A6))
	do {
		u32 reg, intprog, sdiv, sout, valwe;

		reg = amba_readl(PLL_CORE_CTRL_REG);
		intprog = PLL_CTRL_INTPROG(reg);
		sout = PLL_CTRL_SOUT(reg);
		sdiv = PLL_CTRL_SDIV(reg);
		valwe = PLL_CTRL_VALWE(reg);
		valwe &= 0xffe;
		if (new_freq_cpu > pll_info.armfreq)
			intprog++;
		else
			intprog--;

		reg = PLL_CTRL_VAL(intprog, sout, sdiv, valwe);
		amba_writel(PLL_CORE_CTRL_REG, reg);

		/* PLL write enable */
		reg |= 0x1;
		amba_writel(PLL_CORE_CTRL_REG, reg);

		/* FIXME: wait a while */
		mdelay(20);
		cur_freq_cpu = get_core_bus_freq_hz();
	} while ((new_freq_cpu > pll_info.armfreq &&
		cur_freq_cpu < new_freq_cpu) ||
		(new_freq_cpu < pll_info.armfreq &&
		cur_freq_cpu > new_freq_cpu));
#endif
}

static int ambarella_pll_rct_write(struct file *file,
	const char __user *buffer, unsigned long count, void *data)
{
	char					str[AMBPLL_MAX_CMD_LENGTH];
	int					retval = 0;
	unsigned int				i;
	unsigned long				flags;
	unsigned int				temp_freq;

	i = (count < AMBPLL_MAX_CMD_LENGTH) ? count : AMBPLL_MAX_CMD_LENGTH;
	if (copy_from_user(str, buffer, i)) {
		pr_err("%s: copy_from_user fail!\n", __func__);
		retval = -EFAULT;
		goto ambarella_pll_proc_write_exit;
	}
	str[AMBPLL_MAX_CMD_LENGTH - 1] = 0;

	retval = sscanf(str, "%d", &i);
	if (retval != 1) {
		pr_err("%s: convert sting fail %d!\n", __func__, retval);
		retval = -EINVAL;
		goto ambarella_pll_proc_write_exit;
	}
	if (i > 243000000 || i < 135000000) {
		pr_err("%s:\n\tinvalid frequency (%d)\n",
			__func__, i);
		pr_info("\tfrequency should be 135000 ~ 243000 (in KHz)\n");
		retval = -EINVAL;
		goto ambarella_pll_proc_write_exit;
	}
	pr_debug("%s: %ld %d\n", __func__, count, i);

	if(i == pll_info.armfreq)
		goto ambarella_pll_proc_write_exit;

	retval = notifier_to_errno(
		ambarella_set_event(AMBA_EVENT_PRE_CPUFREQ, NULL));
	if (retval) {
		pr_err("%s: AMBA_EVENT_PRE_CPUFREQ failed(%d)\n",
			__func__, retval);
	}

	local_irq_save(flags);

	retval = notifier_to_errno(
		ambarella_set_raw_event(AMBA_EVENT_PRE_CPUFREQ, NULL));
	if (retval) {
		pr_err("%s: AMBA_EVENT_PRE_CPUFREQ failed(%d)\n",
			__func__, retval);
	}

	temp_freq = ambarella_adjust_jiffies(AMBA_EVENT_PRE_CPUFREQ,
		pll_info.armfreq, i);

	ambarella_timer_suspend(1);
	ambarella_freq_set_pll(i);
	ambarella_timer_resume(1);

	pll_info.armfreq = ambarella_adjust_jiffies(AMBA_EVENT_POST_CPUFREQ,
		temp_freq, get_core_bus_freq_hz());

	retval = notifier_to_errno(
		ambarella_set_raw_event(AMBA_EVENT_POST_CPUFREQ, NULL));
	if (retval) {
		pr_err("%s: AMBA_EVENT_POST_CPUFREQ failed(%d)\n",
			__func__, retval);
	}

	local_irq_restore(flags);

	retval = notifier_to_errno(
		ambarella_set_event(AMBA_EVENT_POST_CPUFREQ, NULL));
	if (retval) {
		pr_err("%s: AMBA_EVENT_POST_CPUFREQ failed(%d)\n",
			__func__, retval);
	}

	if (!retval)
		retval = count;

ambarella_pll_proc_write_exit:
	return retval;
}

static int __init ambarella_init_pll_rct(void)
{
	int					retval = 0;

	pll_info.armfreq = get_core_bus_freq_hz();

	freq_file = create_proc_entry("corepll", S_IRUGO | S_IWUSR,
		get_ambarella_proc_dir());
	if (freq_file == NULL) {
		retval = -ENOMEM;
		pr_err("%s: create proc file (freq) fail!\n", __func__);
		goto ambarella_init_pll_rct_exit;
	} else {
		freq_file->read_proc = ambarella_pll_rct_read;
		freq_file->write_proc = ambarella_pll_rct_write;
	}

ambarella_init_pll_rct_exit:
	return retval;
}

/* ==========================================================================*/
int __init ambarella_init_pll(void)
{
	int					retval = 0;

	retval = ambarella_init_pll_rct();

	return retval;
}

/* ==========================================================================*/
u32 ambarella_pll_suspend(u32 level)
{
	return 0;
}

u32 ambarella_pll_resume(u32 level)
{
	ambarella_freq_set_pll(pll_info.armfreq);

	return 0;
}

