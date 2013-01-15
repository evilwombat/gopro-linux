/*
 * arch/arm/plat-ambarella/misc/bapi.c
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/suspend.h>
#include <linux/fb.h>

#include <asm/setup.h>
#include <asm/suspend.h>
#include <asm/io.h>
#include <asm/unified.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

#include <mach/hardware.h>
#include <plat/bapi.h>
#include <plat/ambcache.h>
#include <plat/debug.h>
#include <plat/fb.h>

#include <hal/hal.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

/* ==========================================================================*/
static struct ambarella_bapi_tag_s bapi_tag;
static struct ambarella_bapi_s *bapi_info = NULL;
static ambarella_bapi_aoss_call_t bapi_aoss_entry = NULL;
static u32 bapi_aoss_arg[4];
static u32 aoss_copy_page = 0;
extern int in_suspend;

/* ==========================================================================*/
static u32 sys_bapi_head = 0;
module_param_cb(bapi_head, &param_ops_uint, &sys_bapi_head, 0444);
static u32 sys_bapi_reboot = 0;
module_param_cb(bapi_reboot, &param_ops_uint, &sys_bapi_reboot, 0444);
static u32 sys_bapi_debug = 0;
module_param_cb(bapi_debug, &param_ops_uint, &sys_bapi_debug, 0444);
static u32 sys_bapi_aoss = 0;
module_param_cb(bapi_aoss, &param_ops_uint, &sys_bapi_aoss, 0444);

/* ==========================================================================*/
static int __init parse_mem_tag_bapi(const struct tag *tag)
{
	bapi_tag.magic = tag->u.mem.size;
	bapi_tag.pbapi_info = tag->u.mem.start;

	pr_debug("%s: magic=0x%08x pbapi_info=0x%08x\n", __func__,
		bapi_tag.magic, bapi_tag.pbapi_info);
	return 0;
}
__tagtable(ATAG_AMBARELLA_BAPI, parse_mem_tag_bapi);

/* ==========================================================================*/
static int ambarella_bapi_check_bapi_info(enum ambarella_bapi_cmd_e cmd)
{
	int				retval = 0;

	if (bapi_info == NULL) {
		pr_debug("CMD[%d]: bapi_info is NULL.\n", cmd);
		retval = -EPERM;
		goto ambarella_bapi_check_bapi_info_exit;
	}

	if (bapi_info->magic != DEFAULT_BAPI_MAGIC) {
		pr_debug("CMD[%d]: Wrong magic %d!\n",
			cmd, bapi_info->magic);
		retval = -EPERM;
		goto ambarella_bapi_check_bapi_info_exit;
	}

	if (bapi_info->version != DEFAULT_BAPI_VERSION) {
		pr_debug("CMD[%d]: Wrong version %d!\n",
			cmd, bapi_info->version);
		retval = -EPERM;
		goto ambarella_bapi_check_bapi_info_exit;
	}

ambarella_bapi_check_bapi_info_exit:
	return retval;
}

static int ambarella_bapi_aoss_increase_page_info(
	struct ambarella_bapi_aoss_s *sysainfo,
	struct ambarella_bapi_aoss_page_info_s *src_page_info)
{
	int						retval = 0;
	struct ambarella_bapi_aoss_page_info_s		*syspinfo = NULL;

	if (sysainfo->copy_pages >= sysainfo->total_pages) {
		pr_err("%s: copy_pages[%d] >= total_pages[%d].\n", __func__,
			sysainfo->copy_pages, sysainfo->total_pages);
		retval = -EPERM;
		goto ambarella_bapi_aoss_increase_page_info_exit;
	}

	syspinfo = (struct ambarella_bapi_aoss_page_info_s *)
		ambarella_phys_to_virt(sysainfo->page_info);
	pr_debug("%s: syspinfo %p offset %d, cur %p \n", __func__,
		syspinfo, sysainfo->copy_pages,
		&syspinfo[sysainfo->copy_pages]);
	if (aoss_copy_page == 0) {
		aoss_copy_page = 1;
		sysainfo->copy_pages = 0;
		syspinfo[0].src = src_page_info->src;
		syspinfo[0].dst = src_page_info->dst;
		syspinfo[0].size = PAGE_SIZE;
	} else {
		if ((src_page_info->src == (syspinfo[sysainfo->copy_pages].src + syspinfo[sysainfo->copy_pages].size)) &&
			(src_page_info->dst == (syspinfo[sysainfo->copy_pages].dst + syspinfo[sysainfo->copy_pages].size))) {
			syspinfo[sysainfo->copy_pages].size += PAGE_SIZE;
		} else {
			sysainfo->copy_pages++;
			syspinfo[sysainfo->copy_pages].src = src_page_info->src;
			syspinfo[sysainfo->copy_pages].dst = src_page_info->dst;
			syspinfo[sysainfo->copy_pages].size = PAGE_SIZE;
		}
	}
	pr_debug("%s: copy [0x%08x] to [0x%08x], size [0x%08x] %d\n", __func__,
		syspinfo[sysainfo->copy_pages].src,
		syspinfo[sysainfo->copy_pages].dst,
		syspinfo[sysainfo->copy_pages].size,
		sysainfo->copy_pages);

ambarella_bapi_aoss_increase_page_info_exit:
	return retval;
}

int ambarella_bapi_cmd(enum ambarella_bapi_cmd_e cmd, void *args)
{
	int						retval = 0;

	switch(cmd) {
	case AMBARELLA_BAPI_CMD_INIT:
		if ((bapi_tag.magic == DEFAULT_BAPI_TAG_MAGIC) &&
			(bapi_tag.pbapi_info != 0)) {
			bapi_info = (struct ambarella_bapi_s *)
				ambarella_phys_to_virt(bapi_tag.pbapi_info);
			pr_debug("%s: bapi_info & 0x%p\n", __func__, bapi_info);
		} else {
			bapi_info = NULL;
			retval = -ENXIO;
		}
		retval = ambarella_bapi_check_bapi_info(cmd);
		if (!retval) {
			sys_bapi_head = ambarella_virt_to_phys((u32)bapi_info);
			ambarella_debug_info = (u32)bapi_info->debug;
			sys_bapi_reboot = ambarella_virt_to_phys(
				(u32)&bapi_info->reboot_info);
			sys_bapi_debug = ambarella_virt_to_phys(
				ambarella_debug_info);
			sys_bapi_aoss = ambarella_virt_to_phys(
				(u32)&bapi_info->aoss_info);
		}

		break;

	case AMBARELLA_BAPI_CMD_AOSS_INIT:
		retval = ambarella_bapi_check_bapi_info(cmd);
		if (retval == 0) {
			bapi_info->aoss_info.magic = DEFAULT_BAPI_AOSS_MAGIC;
			bapi_info->aoss_info.copy_pages = 0;
		}
		break;

	case AMBARELLA_BAPI_CMD_AOSS_COPY_PAGE:
		retval = ambarella_bapi_check_bapi_info(cmd);
		if ((retval != 0) || (args == NULL))
			break;
		retval = ambarella_bapi_aoss_increase_page_info(
			&bapi_info->aoss_info,
			(struct ambarella_bapi_aoss_page_info_s *)args);
		break;

	case AMBARELLA_BAPI_CMD_AOSS_SAVE:
	{
		int					i;
		ambarella_bapi_aoss_return_t		return_fn;
#ifdef CONFIG_OUTER_CACHE
		int					l2_mode = 0;
#endif

		return_fn = (ambarella_bapi_aoss_return_t)args;
		retval = ambarella_bapi_check_bapi_info(cmd);
		if (retval == 0) {
			for (i = 0; i < 4; i++) {
				bapi_aoss_arg[i] = ambarella_phys_to_virt(
					bapi_info->aoss_info.fn_pri[i]);
			}
			pr_debug("%s: %p for 0x%08x[0x%08x], 0x%08x[0x%08x], "
			"0x%08x[0x%08x], 0x%08x[0x%08x].\n",
			__func__, bapi_info->aoss_info.fn_pri,
			bapi_aoss_arg[0], bapi_info->aoss_info.fn_pri[0],
			bapi_aoss_arg[1], bapi_info->aoss_info.fn_pri[1],
			bapi_aoss_arg[2], bapi_info->aoss_info.fn_pri[2],
			bapi_aoss_arg[3], bapi_info->aoss_info.fn_pri[3]);
			ambcache_clean_range(bapi_info, bapi_info->size);
			if ((bapi_info->aoss_info.fn_pri[0] == 0) ||
				(bapi_info->aoss_info.fn_pri[1] == 0)) {
				pr_info("Wrong BAPI AOSS info\n");
				retval = -EPERM;
				break;
			}

#if defined(CONFIG_PLAT_AMBARELLA_CORTEX) && defined(CONFIG_SMP)
			arch_smp_suspend(0);
#endif
			bapi_aoss_entry =
				(ambarella_bapi_aoss_call_t)bapi_aoss_arg[0];
#ifdef CONFIG_OUTER_CACHE
			l2_mode = outer_is_enabled();
			if (l2_mode)
				ambcache_l2_disable_raw();
#endif
			flush_cache_all();
			retval = bapi_aoss_entry((u32)bapi_info->aoss_info.fn_pri,
				bapi_aoss_arg[1], bapi_aoss_arg[2], bapi_aoss_arg[3]);
#if defined(CONFIG_PLAT_AMBARELLA_CORTEX)
			if (retval != 0x01) {
				ambarella_swvic_set(AXI_SOFT_IRQ(0));
				while(1) {};
			}
#if defined(CONFIG_SMP)
			arch_smp_resume(0);
#endif
#endif
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
			set_ambarella_hal_invalid();
#endif
#ifdef CONFIG_OUTER_CACHE
#if 0
			if (l2_mode)
				ambcache_l2_enable_raw();
#endif
#endif
			aoss_copy_page = 0;
			bapi_info->aoss_info.copy_pages = 0;
			if (return_fn)
				return_fn();
		}
	}
		break;

	case AMBARELLA_BAPI_CMD_SET_REBOOT_INFO:
	{
		struct ambarella_bapi_reboot_info_s	*preboot_info;

		retval = ambarella_bapi_check_bapi_info(cmd);
		if ((retval != 0) || (args == NULL))
			break;

		preboot_info = (struct ambarella_bapi_reboot_info_s *)args;
		ambcache_inv_range(&bapi_info->reboot_info,
			sizeof(struct ambarella_bapi_reboot_info_s));
		bapi_info->reboot_info.magic = preboot_info->magic;
		bapi_info->reboot_info.mode = preboot_info->mode;
		ambcache_clean_range(&bapi_info->reboot_info,
			sizeof(struct ambarella_bapi_reboot_info_s));
	}
		break;

	case AMBARELLA_BAPI_CMD_CHECK_REBOOT:
	{
		retval = ambarella_bapi_check_bapi_info(cmd);
		if ((retval != 0) || (args == NULL))
			break;

		ambcache_inv_range(&bapi_info->reboot_info,
			sizeof(struct ambarella_bapi_reboot_info_s));
		if (bapi_info->reboot_info.flag & *(u32 *)args)
			retval = 1;
	}
		break;

	case AMBARELLA_BAPI_CMD_UPDATE_FB_INFO:
	{
		retval = ambarella_bapi_check_bapi_info(cmd);
		if (retval != 0)
			break;

		if (bapi_info->fb0_info.xres && bapi_info->fb0_info.yres &&
			bapi_info->fb0_info.xvirtual &&
			bapi_info->fb0_info.yvirtual &&
			bapi_info->fb0_info.fb_start &&
			bapi_info->fb0_info.fb_length) {
			ambarella_fb_update_info(0, bapi_info->fb0_info.xres,
				bapi_info->fb0_info.yres,
				bapi_info->fb0_info.xvirtual,
				bapi_info->fb0_info.yvirtual,
				bapi_info->fb0_info.format,
				bapi_info->fb0_info.bits_per_pixel,
				bapi_info->fb0_info.fb_start,
				bapi_info->fb0_info.fb_length);
		}

		if (bapi_info->fb1_info.xres && bapi_info->fb1_info.yres &&
			bapi_info->fb1_info.xvirtual &&
			bapi_info->fb1_info.yvirtual &&
			bapi_info->fb1_info.fb_start &&
			bapi_info->fb1_info.fb_length) {
			ambarella_fb_update_info(1, bapi_info->fb1_info.xres,
				bapi_info->fb1_info.yres,
				bapi_info->fb1_info.xvirtual,
				bapi_info->fb1_info.yvirtual,
				bapi_info->fb1_info.format,
				bapi_info->fb1_info.bits_per_pixel,
				bapi_info->fb1_info.fb_start,
				bapi_info->fb1_info.fb_length);
		}
	}
		break;

	default:
		pr_err("%s: Unknown CMD[%d].\n", __func__, cmd);
		break;
	}

	return retval;
}

/* ==========================================================================*/
void arch_copy_data_page(unsigned long dst_pfn, unsigned long src_pfn)
{
	struct ambarella_bapi_aoss_page_info_s	page_info;

	page_info.src = __pfn_to_phys(src_pfn);
	page_info.dst = __pfn_to_phys(dst_pfn);

	ambarella_bapi_cmd(AMBARELLA_BAPI_CMD_AOSS_COPY_PAGE, &page_info);
}

static void ambarella_arch_swsusp_return(void)
{
	in_suspend = 0;
	swsusp_arch_restore_cpu();
}

int arch_swsusp_write(unsigned int flags)
{
	int					retval = 0;
	struct ambarella_bapi_reboot_info_s	reboot_info;

	reboot_info.magic = DEFAULT_BAPI_REBOOT_MAGIC;
	reboot_info.mode = AMBARELLA_BAPI_CMD_REBOOT_HIBERNATE;
	retval = ambarella_bapi_cmd(AMBARELLA_BAPI_CMD_SET_REBOOT_INFO,
		&reboot_info);
	if (retval)
		goto arch_swsusp_write_exit;

	retval = ambarella_bapi_cmd(AMBARELLA_BAPI_CMD_AOSS_SAVE,
		ambarella_arch_swsusp_return);

arch_swsusp_write_exit:
	return retval;
}

/* ==========================================================================*/
static int __init ambarella_bapi_init(void)
{
	int					retval = 0;

	ambarella_bapi_cmd(AMBARELLA_BAPI_CMD_INIT, NULL);
	ambarella_bapi_cmd(AMBARELLA_BAPI_CMD_AOSS_INIT, NULL);
	ambarella_bapi_cmd(AMBARELLA_BAPI_CMD_UPDATE_FB_INFO, NULL);

	return retval;
}
arch_initcall(ambarella_bapi_init);

