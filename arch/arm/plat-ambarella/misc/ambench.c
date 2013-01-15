/*
 * arch/arm/plat-ambarella/misc/ambench.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/io.h>
#include <linux/cpu.h>

#include <asm/uaccess.h>

#include <mach/hardware.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX			"ambarella_config."

#define AMBENCH_MAX_CMD_LENGTH			(32)

/* ==========================================================================*/
typedef void (*ambarella_ambench_fn_t)(void);

struct ambarella_ambench_info {
	char *name;
	ambarella_ambench_fn_t fn;
};

/* ==========================================================================*/
static void ambench_apbread(void);

/* ==========================================================================*/
static const char ambench_proc_name[] = "ambench";
static struct proc_dir_entry *ambench_file;

static struct ambarella_ambench_info ambench_list[] = {
	{"APBRead", ambench_apbread},
	{NULL, NULL},
};

/* ==========================================================================*/

static int ambarella_ambench_proc_write(struct file *file,
	const char __user *buffer, unsigned long count, void *data)
{
	int					retval = 0;
	char					str[AMBENCH_MAX_CMD_LENGTH];
	int					i;

	i = (count < AMBENCH_MAX_CMD_LENGTH) ? count : AMBENCH_MAX_CMD_LENGTH;
	if (copy_from_user(str, buffer, i)) {
		pr_err("%s: copy_from_user fail!\n", __func__);
		retval = -EFAULT;
		goto ambarella_ambench_proc_write_exit;
	}
	str[i - 1] = 0;

	for (i = 0; i < ARRAY_SIZE(ambench_list); i++) {
		if ((ambench_list[i].name == NULL) ||
			(ambench_list[i].fn == NULL))
			break;
		if (strlen(str) == strlen(ambench_list[i].name)
			&& strcmp(str, ambench_list[i].name) == 0) {
			ambench_list[i].fn();
			break;
		}
	}

	if (strcmp(str, "all") == 0) {
		for (i = 0; i < ARRAY_SIZE(ambench_list); i++) {
			if ((ambench_list[i].name == NULL) ||
				(ambench_list[i].fn == NULL))
				break;
			ambench_list[i].fn();
		}
	}

	if (!retval)
		retval = count;

ambarella_ambench_proc_write_exit:
	return retval;
}

static int ambarella_ambench_proc_read(char *page, char **start,
	off_t off, int count, int *eof, void *data)
{
	int					retlen;
	int					i;

	if (off != 0) {
		retlen = 0;
		goto ambarella_ambench_proc_read_exit;
	}

	retlen = scnprintf(page, count, "\nPossible Benchmark:\n");
	for (i = 0; i < ARRAY_SIZE(ambench_list); i++) {
		if (count <= retlen)
			break;
		if ((ambench_list[i].name == NULL) ||
			(ambench_list[i].fn == NULL))
			break;
		retlen += scnprintf((page + retlen), (count - retlen),
				"\t%s\n", ambench_list[i].name);
	}

ambarella_ambench_proc_read_exit:
	*eof = 1;
	return retlen;
}

/* ==========================================================================*/
static int __init ambarella_init_ambench(void)
{
	int					retval = 0;

	ambench_file = create_proc_entry(ambench_proc_name, S_IRUGO | S_IWUSR,
		get_ambarella_proc_dir());
	if (ambench_file == NULL) {
		retval = -ENOMEM;
		pr_err("%s: %s fail!\n", __func__, ambench_proc_name);
	} else {
		ambench_file->read_proc = ambarella_ambench_proc_read;
		ambench_file->write_proc = ambarella_ambench_proc_write;
	}

	return retval;
}
late_initcall(ambarella_init_ambench);

/* ==========================================================================*/
#define APBREAD_RELOAD_NUM			(0x10000000)
static void ambench_apbread(void)
{
	u64					raw_counter = 0;
	u64					amba_counter = 0;
	unsigned long				flags;

	disable_nonboot_cpus();
	local_irq_save(flags);

	amba_clrbitsl(TIMER_CTR_REG, TIMER_CTR_EN1);
	amba_writel(TIMER1_STATUS_REG, APBREAD_RELOAD_NUM);
	amba_writel(TIMER1_RELOAD_REG, 0x0);
	amba_writel(TIMER1_MATCH1_REG, 0x0);
	amba_writel(TIMER1_MATCH2_REG, 0x0);
	amba_clrbitsl(TIMER_CTR_REG, TIMER_CTR_OF1);
	amba_clrbitsl(TIMER_CTR_REG, TIMER_CTR_CSL1);

	amba_setbitsl(TIMER_CTR_REG, TIMER_CTR_EN1);
	do {
		raw_counter++;
	} while(__raw_readl(TIMER1_STATUS_REG));

	amba_clrbitsl(TIMER_CTR_REG, TIMER_CTR_EN1);
	amba_writel(TIMER1_STATUS_REG, APBREAD_RELOAD_NUM);
	amba_writel(TIMER1_RELOAD_REG, 0x0);
	amba_writel(TIMER1_MATCH1_REG, 0x0);
	amba_writel(TIMER1_MATCH2_REG, 0x0);
	amba_clrbitsl(TIMER_CTR_REG, TIMER_CTR_OF1);
	amba_clrbitsl(TIMER_CTR_REG, TIMER_CTR_CSL1);

	amba_setbitsl(TIMER_CTR_REG, TIMER_CTR_EN1);
	do {
		amba_counter++;
	} while(amba_readl(TIMER1_STATUS_REG));
	amba_clrbitsl(TIMER_CTR_REG, TIMER_CTR_EN1);

	local_irq_restore(flags);
	enable_nonboot_cpus();

	raw_counter *= get_apb_bus_freq_hz();
	do_div(raw_counter, APBREAD_RELOAD_NUM);
	amba_counter *= get_apb_bus_freq_hz();
	do_div(amba_counter, APBREAD_RELOAD_NUM);
	pr_info("CPU[0x%x] APBRead: raw speed %llu/s!\n",
		cpu_architecture(), raw_counter);
	pr_info("CPU[0x%x] APBRead: amba speed %llu/s!\n",
		cpu_architecture(), amba_counter);
}

