/*
 * arch/arm/plat-ambarella/boss/pm.c
 * Power Management Routines
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cpu.h>

#include <asm/cacheflush.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/suspend.h>

#include <mach/board.h>
#include <mach/hardware.h>
#include <mach/init.h>
#include <mach/system.h>
#include <mach/boss.h>

#include <hal/hal.h>
#include <linux/aipc/i_util.h>
#include <linux/aipc/i_status.h>

#include <plat/ambcache.h>

#include <generated/ipcgen/i_util.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

typedef unsigned int (*ambnation_aoss_call_t)(u32, u32, u32, u32);
extern int in_suspend;
extern char in_suspend_ipc_svc;

/* ==========================================================================*/

/* ==========================================================================*/
static struct ambernation_check_info pm_abcheck_info;
static u32 pm_abcopy_page = 0;

/* ==========================================================================*/
void ambarella_power_off(void)
{
	pr_err("%s@%d: TBD\n", __func__, __LINE__);
}

void ambarella_power_off_prepare(void)
{
}

/* ==========================================================================*/
static int ambarella_pm_pre(unsigned long *irqflag, u32 bsuspend,
	u32 tm_level, u32 bnotifier)
{
	int					retval = 0;

	if (bnotifier) {
		retval = notifier_to_errno(
			ambarella_set_event(AMBA_EVENT_PRE_PM, NULL));
		if (retval) {
			pr_err("%s@%d: AMBA_EVENT_PRE_PM failed(%d)\n",
				__func__, __LINE__, retval);
		}
	}
	if (irqflag)
		local_irq_save(*irqflag);

	if (bsuspend) {
		ambarella_timer_suspend(tm_level);
		ambarella_irq_suspend(0);
		ambarella_pll_suspend(0);
	}

	if (bnotifier && irqflag) {
		retval = notifier_to_errno(
			ambarella_set_raw_event(AMBA_EVENT_PRE_PM, NULL));
		if (retval) {
			pr_err("%s@%d: AMBA_EVENT_PRE_PM failed(%d)\n",
				__func__, __LINE__, retval);
		}
	}

	return retval;
}

static int ambarella_pm_post(unsigned long *irqflag, u32 bresume,
	u32 tm_level, u32 bnotifier)
{
	int					retval = 0;

	if (bnotifier && irqflag) {
		retval = notifier_to_errno(
			ambarella_set_raw_event(AMBA_EVENT_POST_PM, NULL));
		if (retval) {
			pr_err("%s: AMBA_EVENT_PRE_PM failed(%d)\n",
				__func__, retval);
		}
	}

	if (bresume) {
		ambarella_pll_resume(0);
		ambarella_irq_resume(0);
		ambarella_timer_resume(tm_level);
	}

	if (irqflag)
		local_irq_restore(*irqflag);

	if (bnotifier) {
		retval = notifier_to_errno(
			ambarella_set_event(AMBA_EVENT_POST_PM, NULL));
		if (retval) {
			pr_err("%s: AMBA_EVENT_PRE_PM failed(%d)\n",
				__func__, retval);
		}
	}

	return retval;
}

static int ambarella_pm_check(suspend_state_t state)
{
	int					retval = 0;

	retval = notifier_to_errno(
		ambarella_set_raw_event(AMBA_EVENT_CHECK_PM, &state));
	if (retval) {
		pr_err("%s: AMBA_EVENT_CHECK_PM failed(%d)\n",
			__func__, retval);
		goto ambarella_pm_check_exit;
	}

	retval = notifier_to_errno(
		ambarella_set_event(AMBA_EVENT_CHECK_PM, &state));
	if (retval) {
		pr_err("%s: AMBA_EVENT_CHECK_PM failed(%d)\n",
			__func__, retval);
	}

ambarella_pm_check_exit:
	return retval;
}

static int ambarella_pm_enter_standby(void)
{
	int					retval = 0;
	unsigned long				flags;

	if (ambarella_pm_pre(&flags, 1, 1, 1))
		BUG();

#if defined(CONFIG_BOSS_SINGLE_CORE)
	boss->state = BOSS_STATE_SUSPENDED;
	arch_idle();
#else
	pr_err("%s@%d: TBD\n", __func__, __LINE__);
#endif

	if (ambarella_pm_post(&flags, 1, 1, 1))
		BUG();

	return retval;
}

static int ambarella_pm_enter_mem(void)
{
	int					retval = 0;
	unsigned long				flags;

	if (ambarella_pm_pre(&flags, 1, 1, 1))
		BUG();

	pr_err("%s@%d: TBD\n", __func__, __LINE__);

	if (ambarella_pm_post(&flags, 1, 1, 1))
		BUG();

	return retval;
}

static int ambarella_pm_suspend_enter(suspend_state_t state)
{
	int					retval = 0;

	pr_debug("%s: enter with state[%d]\n", __func__, state);

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

	pr_debug("%s: exit state[%d] with %d\n", __func__, state, retval);

	return retval;
}

static int ambarella_pm_suspend_valid(suspend_state_t state)
{
	int					retval = 0;
	int					valid = 0;

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
		//valid = 1;
		break;

	default:
		break;
	}

ambarella_pm_valid_exit:
	pr_debug("%s: state[%d]=%d\n", __func__, state, valid);

	return valid;
}

static struct platform_suspend_ops ambarella_pm_suspend_ops = {
	.valid		= ambarella_pm_suspend_valid,
	.enter		= ambarella_pm_suspend_enter,
};

static int ambarella_pm_hibernation_begin(void)
{
	int					retval = 0;

	if (pm_abcheck_info.aoss_info == NULL) {
		pm_abcopy_page = 0;
		linux_absuspend_check((void *)&pm_abcheck_info);
	}

	disable_irq(BOSS_VIRT_H2G_INT_REQ_VEC);
	disable_irq(BOSS_VIRT_H2G_MTX_VEC);

	ipc_status_report_ready(0);

	disable_irq(BOSS_VIRT_H2G_INT_RLY_VEC);

	return retval;
}

static void ambarella_pm_hibernation_end(void)
{
	int					retval = 0;

	enable_irq(BOSS_VIRT_H2G_INT_RLY_VEC);

	ipc_status_report_ready(1);

	enable_irq(BOSS_VIRT_H2G_INT_REQ_VEC);
	enable_irq(BOSS_VIRT_H2G_MTX_VEC);

	retval = ambarella_pm_post(NULL, 0, 0, 1);

	linux_absuspend_exit();
}

static int ambarella_pm_hibernation_pre_snapshot(void)
{
	int					retval = 0;

	retval = ambarella_pm_pre(NULL, 1, 0, 1);

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
	ambarella_pm_post(NULL, 1, 0, 0);
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
	ambarella_pm_post(NULL, 0, 0, 1);
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

int __init ambarella_init_pm(void)
{
	pm_power_off = ambarella_power_off;
	pm_power_off_prepare = ambarella_power_off_prepare;

	suspend_set_ops(&ambarella_pm_suspend_ops);
	hibernation_set_ops(&ambarella_pm_hibernation_ops);

	pm_abcheck_info.aoss_info = NULL;

	return 0;
}

/* ==========================================================================*/
int arch_swsusp_write(unsigned int flags)
{
	int					retval = 0;
	int					i;
#ifdef CONFIG_OUTER_CACHE
	int					l2_mode = 0;
#endif
	ambnation_aoss_call_t			pm_abaoss_entry = NULL;
	u32					pm_abaoss_arg[4];
	u32					pm_fn_pri;

	if (pm_abcheck_info.aoss_info) {
		for (i = 0; i < 4; i++) {
			pm_abaoss_arg[i] = ambarella_phys_to_virt(
				pm_abcheck_info.aoss_info->fn_pri[i]);
		}
		pm_abaoss_entry = (ambnation_aoss_call_t)pm_abaoss_arg[0];
		pm_fn_pri = (u32)pm_abcheck_info.aoss_info->fn_pri;

#if defined(CONFIG_PLAT_AMBARELLA_CORTEX) && defined(CONFIG_SMP)
		arch_smp_suspend(0);
#endif
#ifdef CONFIG_OUTER_CACHE
		l2_mode = outer_is_enabled();
		if (l2_mode)
			ambcache_l2_disable_raw();
#endif
		flush_cache_all();
		retval = pm_abaoss_entry(pm_fn_pri, pm_abaoss_arg[1],
			pm_abaoss_arg[2], pm_abaoss_arg[3]);
		printk("pm_abaoss_entry returned 0x%x\n",retval);

		if (retval != 0x01) {
			linux_absuspend_enter();
#if defined(CONFIG_BOSS_SINGLE_CORE)
			boss->state = BOSS_STATE_SUSPENDED;
			arch_idle();
#endif
			while(1) {};
		}
#if defined(CONFIG_SMP)
		arch_smp_resume(0);
#endif

#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
		set_ambarella_hal_invalid();
#endif
#ifdef CONFIG_OUTER_CACHE
		if (l2_mode)
			ambcache_l2_enable_raw();
#endif
		pm_abcopy_page = 0;
		pm_abcheck_info.aoss_info->copy_pages = 0;
		in_suspend = 0;
		in_suspend_ipc_svc = 0;
		swsusp_arch_restore_cpu();
	}

	return retval;
}

static int ambernation_increase_page_info(
	struct ambernation_aoss_info *aoss_info,
	struct ambernation_page_info *src_page_info)
{
	int						retval = 0;
	struct ambernation_page_info			*page_info;

	if (pm_abcopy_page == 0) {
		aoss_info->copy_pages = 0;
	}
	if (aoss_info->copy_pages >= aoss_info->total_pages) {
		pr_err("%s: copy_pages[%d] >= total_pages[%d].\n", __func__,
			aoss_info->copy_pages, aoss_info->total_pages);
		retval = -EPERM;
		goto ambernation_increase_page_info_exit;
	}

	page_info = (struct ambernation_page_info *)
		ambarella_phys_to_virt((u32)aoss_info->page_info);
	pr_debug("%s: page_info %p offset %d, cur %p \n", __func__,
		page_info, aoss_info->copy_pages,
		&page_info[aoss_info->copy_pages]);
	if (pm_abcopy_page == 0) {
		pm_abcopy_page = 1;
		aoss_info->copy_pages = 0;
		page_info[0].src = src_page_info->src;
		page_info[0].dst = src_page_info->dst;
		page_info[0].size = src_page_info->size;
	} else {
		if ((src_page_info->src == (page_info[aoss_info->copy_pages].src + page_info[aoss_info->copy_pages].size)) &&
			(src_page_info->dst == (page_info[aoss_info->copy_pages].dst + page_info[aoss_info->copy_pages].size))) {
			page_info[aoss_info->copy_pages].size += src_page_info->size;
		} else {
			aoss_info->copy_pages++;
			page_info[aoss_info->copy_pages].src = src_page_info->src;
			page_info[aoss_info->copy_pages].dst = src_page_info->dst;
			page_info[aoss_info->copy_pages].size = src_page_info->size;
		}
	}
	pr_debug("%s: copy [0x%08x] to [0x%08x], size [0x%08x] %d\n", __func__,
		page_info[aoss_info->copy_pages].src,
		page_info[aoss_info->copy_pages].dst,
		page_info[aoss_info->copy_pages].size,
		aoss_info->copy_pages);

ambernation_increase_page_info_exit:
	return retval;
}

void arch_copy_data_page(unsigned long dst_pfn, unsigned long src_pfn)
{
	struct ambernation_page_info		src_page_info;

	if (pm_abcheck_info.aoss_info) {
		src_page_info.src = __pfn_to_phys(src_pfn);
		src_page_info.dst = __pfn_to_phys(dst_pfn);
		src_page_info.size = PAGE_SIZE;
		ambernation_increase_page_info(pm_abcheck_info.aoss_info,
			&src_page_info);
	}
}

