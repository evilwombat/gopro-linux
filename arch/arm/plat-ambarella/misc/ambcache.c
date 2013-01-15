/*
 * arch/arm/plat-ambarella/misc/ambcache.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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

#include <asm/cacheflush.h>
#ifdef CONFIG_CACHE_PL310
#include <asm/hardware/cache-l2x0.h>
#endif
#include <asm/io.h>

#include <mach/hardware.h>
#include <plat/ambcache.h>
#include <plat/atag.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

/* ==========================================================================*/
#define CACHE_LINE_SIZE		32
#define CACHE_LINE_MASK		~(CACHE_LINE_SIZE - 1)

/* ==========================================================================*/
#ifdef CONFIG_OUTER_CACHE
static u32 cache_l2_status = 0;
#ifdef CONFIG_CACHE_PL310
static void __iomem *ambcache_l2_base = __io(AMBARELLA_VA_L2CC_BASE);
#endif
#endif

/* ==========================================================================*/
void ambcache_clean_range(void *addr, unsigned int size)
{
	u32					vstart;
	u32					vend;
#ifdef CONFIG_OUTER_CACHE
	u32					pstart;
#endif
	u32					addr_tmp;

	vstart = (u32)addr & CACHE_LINE_MASK;
	vend = ((u32)addr + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;
#ifdef CONFIG_OUTER_CACHE
	pstart = ambarella_virt_to_phys(vstart);
#endif

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c10, 1" : : "r" (addr_tmp));
	}
	dsb();

#ifdef CONFIG_OUTER_CACHE
	outer_clean_range(pstart, (pstart + size));
	dsb();
#endif
}
EXPORT_SYMBOL(ambcache_clean_range);

void ambcache_inv_range(void *addr, unsigned int size)
{
	u32					vstart;
	u32					vend;
#ifdef CONFIG_OUTER_CACHE
	u32					pstart;
#endif
	u32					addr_tmp;

	vstart = (u32)addr & CACHE_LINE_MASK;
	vend = ((u32)addr + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;

#ifdef CONFIG_OUTER_CACHE
	pstart = ambarella_virt_to_phys(vstart);
	outer_inv_range(pstart, (pstart + size));
	dsb();
#endif

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c6, 1" : : "r" (addr_tmp));
	}
	dsb();

#ifdef CONFIG_OUTER_CACHE
	outer_inv_range(pstart, (pstart + size));

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c6, 1" : : "r" (addr_tmp));
	}
	dsb();
#endif
}
EXPORT_SYMBOL(ambcache_inv_range);

void ambcache_flush_range(void *addr, unsigned int size)
{
	u32					vstart;
	u32					vend;
#ifdef CONFIG_OUTER_CACHE
	u32					pstart;
#endif
	u32					addr_tmp;

#ifdef CONFIG_OUTER_CACHE
	vstart = (u32)addr & CACHE_LINE_MASK;
	vend = ((u32)addr + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;
	pstart = ambarella_virt_to_phys(vstart);

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c10, 1" : : "r" (addr_tmp));
	}
	dsb();

	outer_flush_range(pstart, (pstart + size));

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c6, 1" : : "r" (addr_tmp));
	}
	dsb();
#else
	vstart = (u32)addr & CACHE_LINE_MASK;
	vend = ((u32)addr + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c14, 1" : : "r" (addr_tmp));
	}
	dsb();
#endif
}
EXPORT_SYMBOL(ambcache_flush_range);

void ambcache_l2_enable_raw()
{
#ifdef CONFIG_OUTER_CACHE
	if (!outer_is_enabled()) {
#ifdef CONFIG_CACHE_PL310
		if (readl(ambcache_l2_base + L2X0_DATA_LATENCY_CTRL) !=
			0x00000120) {
			writel(0x00000120, (ambcache_l2_base +
				L2X0_DATA_LATENCY_CTRL));
			l2x0_init(ambcache_l2_base, 0x00000000, 0xffffffff);
		} else
#endif
			outer_enable();
	}
#endif
}
EXPORT_SYMBOL(ambcache_l2_enable_raw);

void ambcache_l2_disable_raw()
{
#ifdef CONFIG_OUTER_CACHE
	flush_cache_all();
	outer_flush_all();
	outer_disable();
	outer_inv_all();
	flush_cache_all();
#endif
}
EXPORT_SYMBOL(ambcache_l2_disable_raw);

int ambcache_l2_enable()
{
	ambcache_l2_enable_raw();
	return outer_is_enabled() ? 0 : -1;
}
EXPORT_SYMBOL(ambcache_l2_enable);

int ambcache_l2_disable()
{
#ifdef CONFIG_OUTER_CACHE
	unsigned long flags;

	if (outer_is_enabled()) {
		disable_nonboot_cpus();
		local_irq_save(flags);
		ambcache_l2_disable_raw();
		local_irq_restore(flags);
		enable_nonboot_cpus();
	}
#endif
	return outer_is_enabled() ? -1 : 0;
}
EXPORT_SYMBOL(ambcache_l2_disable);

#ifdef CONFIG_OUTER_CACHE
/* =========================Debug Only========================================*/
int cache_l2_set_status(const char *val, const struct kernel_param *kp)
{
	int ret;

	ret = param_set_uint(val, kp);
	if (!ret) {
		if (cache_l2_status) {
			ret = ambcache_l2_enable();
		} else {
			ret = ambcache_l2_disable();
		}
	}

	return ret;
}

static int cache_l2_get_status(char *buffer, const struct kernel_param *kp)
{
	cache_l2_status = outer_is_enabled();

	return param_get_uint(buffer, kp);
}

static struct kernel_param_ops param_ops_cache_l2 = {
	.set = cache_l2_set_status,
	.get = cache_l2_get_status,
};
module_param_cb(cache_l2, &param_ops_cache_l2, &cache_l2_status, 0644);
#endif

