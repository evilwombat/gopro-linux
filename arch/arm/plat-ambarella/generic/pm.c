/*
 * arch/arm/plat-ambarella/generic/pm.c
 * Power Management Routines
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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
#include <linux/platform_device.h>
#include <linux/suspend.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cpu.h>
#include <linux/power_supply.h>

#include <asm/cacheflush.h>
#include <asm/io.h>
#include <asm/system.h>

#include <mach/board.h>
#include <mach/hardware.h>
#include <mach/init.h>
#include <mach/system.h>

#include <hal/hal.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

/* ==========================================================================*/
static int pm_debug_enable_timer_irq = 0;
module_param(pm_debug_enable_timer_irq, int, 0644);

#ifdef CONFIG_GPIO_WM831X
extern int wm831x_config_poweroff(void);
#endif
static int pm_check_power_supply = 1;
module_param(pm_check_power_supply, int, 0644);

/* ==========================================================================*/
void ambarella_power_off(void)
{
	if (ambarella_board_generic.power_control.gpio_id >= 0) {
		ambarella_set_gpio_output(
			&ambarella_board_generic.power_control, 0);
	} else {
		rct_power_down();
	}
}

void ambarella_power_off_prepare(void)
{
#ifdef CONFIG_GPIO_WM831X
	wm831x_config_poweroff();
#endif
}
/* ==========================================================================*/
static int ambarella_pm_notify(unsigned long val)
{
	int					retval = 0;

	retval = notifier_to_errno(ambarella_set_event(val, NULL));
	if (retval)
		pr_err("%s@%d: %ld fail(%d)\n",__func__, __LINE__, val, retval);

	return retval;
}

static int ambarella_pm_notify_raw(unsigned long val)
{
	int					retval = 0;

	retval = notifier_to_errno(ambarella_set_raw_event(val, NULL));
	if (retval)
		pr_err("%s@%d: %ld fail(%d)\n",__func__, __LINE__, val, retval);

	return retval;
}

static int ambarella_pm_pre(unsigned long *irqflag, u32 bsuspend, u32 tm_level)
{
	int					retval = 0;

	if (irqflag)
		local_irq_save(*irqflag);

	if (bsuspend) {
		ambarella_adc_suspend(0);
		ambarella_timer_suspend(tm_level);
		ambarella_irq_suspend(0);
		ambarella_gpio_suspend(0);
		ambarella_pll_suspend(0);
	}

	retval = ambarella_pm_notify_raw(AMBA_EVENT_PRE_PM);

	return retval;
}

static int ambarella_pm_post(unsigned long *irqflag, u32 bresume, u32 tm_level)
{
	int					retval = 0;

	retval = ambarella_pm_notify_raw(AMBA_EVENT_POST_PM);

	if (bresume) {
		ambarella_pll_resume(0);
		ambarella_gpio_resume(0);
		ambarella_irq_resume(0);
		ambarella_timer_resume(tm_level);
		ambarella_adc_resume(0);
	}

	if (irqflag)
		local_irq_restore(*irqflag);

	return retval;
}

static int ambarella_pm_check(suspend_state_t state)
{
	int					retval = 0;

	retval = ambarella_pm_notify_raw(AMBA_EVENT_CHECK_PM);
	if (retval)
		goto ambarella_pm_check_exit;

	retval = ambarella_pm_notify(AMBA_EVENT_CHECK_PM);
	if (retval)
		goto ambarella_pm_check_exit;

ambarella_pm_check_exit:
	return retval;
}

/* ==========================================================================*/
static int ambarella_pm_enter_standby(void)
{
	int					retval = 0;
	struct irq_desc				*pm_desc = NULL;
	struct irq_chip				*pm_chip = NULL;
	unsigned long				flags;

	if (ambarella_pm_pre(&flags, 1, 1))
		BUG();

	if (pm_debug_enable_timer_irq) {
		pm_desc = irq_to_desc(TIMER1_IRQ);
		if (pm_desc)
			pm_chip = get_irq_desc_chip(pm_desc);
		if (pm_chip && pm_chip->irq_shutdown)
			pm_chip->irq_shutdown(&pm_desc->irq_data);

		amba_clrbitsl(TIMER_CTR_REG, TIMER_CTR_EN1);
		amba_writel(TIMER1_STATUS_REG, 0x800000);
		amba_writel(TIMER1_RELOAD_REG, 0x800000);
		amba_writel(TIMER1_MATCH1_REG, 0x0);
		amba_writel(TIMER1_MATCH2_REG, 0x0);
		amba_setbitsl(TIMER_CTR_REG, TIMER_CTR_OF1);
		amba_clrbitsl(TIMER_CTR_REG, TIMER_CTR_CSL1);
	}

	if (pm_debug_enable_timer_irq) {
		if (pm_chip && pm_chip->irq_startup)
			pm_chip->irq_startup(&pm_desc->irq_data);
		amba_setbitsl(TIMER_CTR_REG, TIMER_CTR_EN1);
	}

	cpu_do_idle();

	if (pm_debug_enable_timer_irq) {
		amba_clrbitsl(TIMER_CTR_REG, TIMER_CTR_EN1);
		if (pm_chip && pm_chip->irq_shutdown)
			pm_chip->irq_shutdown(&pm_desc->irq_data);
	}

	if (ambarella_pm_post(&flags, 1, 1))
		BUG();

	return retval;
}

static int ambarella_pm_enter_mem(void)
{
	int					retval = 0;
	unsigned long				flags;
#if defined(CONFIG_AMBARELLA_SUPPORT_BAPI)
	struct ambarella_bapi_reboot_info_s	reboot_info;
#endif

	if (ambarella_pm_pre(&flags, 1, 1))
		BUG();

#if defined(CONFIG_AMBARELLA_SUPPORT_BAPI)
	reboot_info.magic = DEFAULT_BAPI_REBOOT_MAGIC;
	reboot_info.mode = AMBARELLA_BAPI_CMD_REBOOT_SELFREFERESH;
	retval = ambarella_bapi_cmd(AMBARELLA_BAPI_CMD_SET_REBOOT_INFO,
		&reboot_info);
	if (retval)
		goto ambarella_pm_enter_mem_exit_bapi;
	retval = ambarella_bapi_cmd(AMBARELLA_BAPI_CMD_AOSS_SAVE, NULL);
ambarella_pm_enter_mem_exit_bapi:
#endif

	if (ambarella_pm_post(&flags, 1, 1))
		BUG();

	return retval;
}

static int ambarella_pm_suspend_valid(suspend_state_t state)
{
	int					retval = 0;
	int					valid = 0;
#if defined(CONFIG_AMBARELLA_SUPPORT_BAPI)
	int					mode;
#endif

	retval = ambarella_pm_check(state);
	if (retval)
		goto ambarella_pm_valid_exit;

	switch (state) {
	case PM_SUSPEND_ON:
		valid = 1;
		break;

	case PM_SUSPEND_STANDBY:
		valid = 1;
		break;

	case PM_SUSPEND_MEM:
#if defined(CONFIG_AMBARELLA_SUPPORT_BAPI)
		mode = AMBARELLA_BAPI_REBOOT_SELFREFERESH;
		if (ambarella_bapi_cmd(AMBARELLA_BAPI_CMD_CHECK_REBOOT,
			&mode) == 1) {
			valid = 1;
		}
#endif
		if (pm_check_power_supply &&
			(power_supply_is_system_supplied() > 0)) {
			valid = 0;
		}
		break;

	default:
		break;
	}

ambarella_pm_valid_exit:
	pr_debug("%s: state[%d]=%d\n", __func__, state, valid);

	return valid;
}

static int ambarella_pm_suspend_begin(suspend_state_t state)
{
	int					retval = 0;

	switch (state) {
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		retval = ambarella_pm_notify(AMBA_EVENT_PRE_PM);
		break;

	default:
		break;
	}

	return retval;
}

static int ambarella_pm_suspend_enter(suspend_state_t state)
{
	int					retval = 0;

	switch (state) {
	case PM_SUSPEND_ON:
		break;

	case PM_SUSPEND_STANDBY:
		retval = ambarella_pm_enter_standby();
		break;

	case PM_SUSPEND_MEM:
		retval = ambarella_pm_enter_mem();
		break;

	default:
		break;
	}

	return retval;
}

static void ambarella_pm_suspend_end(void)
{
	ambarella_pm_notify(AMBA_EVENT_POST_PM);
}

static struct platform_suspend_ops ambarella_pm_suspend_ops = {
	.valid		= ambarella_pm_suspend_valid,
	.begin		= ambarella_pm_suspend_begin,
	.enter		= ambarella_pm_suspend_enter,
	.end		= ambarella_pm_suspend_end,
};

/* ==========================================================================*/
static int ambarella_pm_hibernation_begin(void)
{
	int					retval;
#if defined(CONFIG_AMBARELLA_SUPPORT_BAPI)
	int					mode;

	retval = -1;
	mode = AMBARELLA_BAPI_REBOOT_HIBERNATE;
	if (ambarella_bapi_cmd(AMBARELLA_BAPI_CMD_CHECK_REBOOT, &mode) == 1) {
		retval = 1;
	}
#else
	retval = 1;
#endif

	if (retval == 1)
		retval = ambarella_pm_notify(AMBA_EVENT_PRE_PM);

	return retval;
}

static void ambarella_pm_hibernation_end(void)
{
	ambarella_pm_notify(AMBA_EVENT_POST_PM);
}

static int ambarella_pm_hibernation_pre_snapshot(void)
{
	int					retval = 0;

	retval = ambarella_pm_pre(NULL, 1, 0);

	return retval;
}

static void ambarella_pm_hibernation_finish(void)
{
}

static int ambarella_pm_hibernation_prepare(void)
{
	int					retval = 0;

	return retval;
}

static int ambarella_pm_hibernation_enter(void)
{
	int					retval = 0;

	ambarella_power_off();

	return retval;
}

static void ambarella_pm_hibernation_leave(void)
{
	ambarella_pm_post(NULL, 1, 0);
}

static int ambarella_pm_hibernation_pre_restore(void)
{
	int					retval = 0;

	return retval;
}

static void ambarella_pm_hibernation_restore_cleanup(void)
{
}

static void ambarella_pm_hibernation_restore_recover(void)
{
}

static struct platform_hibernation_ops ambarella_pm_hibernation_ops = {
	.begin = ambarella_pm_hibernation_begin,
	.end = ambarella_pm_hibernation_end,
	.pre_snapshot = ambarella_pm_hibernation_pre_snapshot,
	.finish = ambarella_pm_hibernation_finish,
	.prepare = ambarella_pm_hibernation_prepare,
	.enter = ambarella_pm_hibernation_enter,
	.leave = ambarella_pm_hibernation_leave,
	.pre_restore = ambarella_pm_hibernation_pre_restore,
	.restore_cleanup = ambarella_pm_hibernation_restore_cleanup,
	.recover = ambarella_pm_hibernation_restore_recover,
};

/* ==========================================================================*/
int __init ambarella_init_pm(void)
{
	pm_power_off = ambarella_power_off;
	pm_power_off_prepare = ambarella_power_off_prepare;

	suspend_set_ops(&ambarella_pm_suspend_ops);
	hibernation_set_ops(&ambarella_pm_hibernation_ops);

	return 0;
}

