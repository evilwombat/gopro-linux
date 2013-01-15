/*
 * arch/arm/plat-ambarella/misc/atag.c
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
#include <linux/bootmem.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>

#include <asm/page.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/tlbflush.h>
#include <asm/mach/map.h>

#include <mach/hardware.h>
#include <mach/board.h>
#include <plat/debug.h>
#include <hal/hal.h>

#if defined(CONFIG_PLAT_AMBARELLA_BOSS)

#include <linux/aipc/aipc.h>
#include <linux/aipc/ipc_slock.h>
#include <linux/aipc/ipc_shm.h>
#include <linux/aipc/ipc_mutex.h>
#include <linux/aipc/ipc_log.h>

#include <mach/boss.h>
#include <plat/ambcache.h>
#include <asm/cacheflush.h>

struct boss_s *boss = NULL;
EXPORT_SYMBOL(boss);

#endif

/* ==========================================================================*/
u32 ambarella_debug_level = AMBA_DEBUG_NULL;
EXPORT_SYMBOL(ambarella_debug_level);

u32 ambarella_debug_info = 0;
EXPORT_SYMBOL(ambarella_debug_info);

u32 ambarella_boot_splash_logo = 0;
EXPORT_SYMBOL(ambarella_boot_splash_logo);

unsigned long ambarella_debug_lookup_name(const char *name)
{
	return module_kallsyms_lookup_name(name);
}
EXPORT_SYMBOL(ambarella_debug_lookup_name);

/* ==========================================================================*/
static int __init parse_eth0_tag_mac(const struct tag *tag)
{
	u32 high, low;

	high = tag->u.serialnr.high;
	low = tag->u.serialnr.low;

	ambarella_board_generic.eth0_mac[0] = (low >> 0);
	ambarella_board_generic.eth0_mac[1] = (low >> 8);
	ambarella_board_generic.eth0_mac[2] = (low >> 16);
	ambarella_board_generic.eth0_mac[3] = (low >> 24);
	ambarella_board_generic.eth0_mac[4] = (high >> 0);
	ambarella_board_generic.eth0_mac[5] = (high >> 8);
	return 0;
}
__tagtable(ATAG_AMBARELLA_ETH0, parse_eth0_tag_mac);

static int __init parse_eth1_tag_mac(const struct tag *tag)
{
	u32 high, low;

	high = tag->u.serialnr.high;
	low = tag->u.serialnr.low;

	ambarella_board_generic.eth1_mac[0] = (low >> 0);
	ambarella_board_generic.eth1_mac[1] = (low >> 8);
	ambarella_board_generic.eth1_mac[2] = (low >> 16);
	ambarella_board_generic.eth1_mac[3] = (low >> 24);
	ambarella_board_generic.eth1_mac[4] = (high >> 0);
	ambarella_board_generic.eth1_mac[5] = (high >> 8);
	return 0;
}
__tagtable(ATAG_AMBARELLA_ETH1, parse_eth1_tag_mac);

static int __init parse_wifi_tag_mac(const struct tag *tag)
{
	u32 high, low;

	high = tag->u.serialnr.high;
	low = tag->u.serialnr.low;

	ambarella_board_generic.wifi_mac[0] = (low >> 0);
	ambarella_board_generic.wifi_mac[1] = (low >> 8);
	ambarella_board_generic.wifi_mac[2] = (low >> 16);
	ambarella_board_generic.wifi_mac[3] = (low >> 24);
	ambarella_board_generic.wifi_mac[4] = (high >> 0);
	ambarella_board_generic.wifi_mac[5] = (high >> 8);
	return 0;
}
__tagtable(ATAG_AMBARELLA_WIFI, parse_wifi_tag_mac);

static int __init parse_ueth_tag_mac(const struct tag *tag)
{
	u32 high, low;

	high = tag->u.serialnr.high;
	low = tag->u.serialnr.low;

	ambarella_board_generic.ueth_mac[0] = (low >> 0);
	ambarella_board_generic.ueth_mac[1] = (low >> 8);
	ambarella_board_generic.ueth_mac[2] = (low >> 16);
	ambarella_board_generic.ueth_mac[3] = (low >> 24);
	ambarella_board_generic.ueth_mac[4] = (high >> 0);
	ambarella_board_generic.ueth_mac[5] = (high >> 8);
	return 0;
}
__tagtable(ATAG_AMBARELLA_UETH, parse_ueth_tag_mac);

/* ==========================================================================*/
#define AMBARELLA_IO_DESC_AHB_ID	0
#define AMBARELLA_IO_DESC_APB_ID	1
#define AMBARELLA_IO_DESC_PPM_ID	2
#define AMBARELLA_IO_DESC_BSB_ID	3
#define AMBARELLA_IO_DESC_DSP_ID	4
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW)
#define AMBARELLA_IO_DESC_DRAMC_ID	(AMBARELLA_IO_DESC_DSP_ID + 1)
#define AMBARELLA_IO_DESC_CRYPT_ID	(AMBARELLA_IO_DESC_DSP_ID + 2)
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW_CORTEX_EXT)
#define AMBARELLA_IO_DESC_AXI_ID	(AMBARELLA_IO_DESC_DSP_ID + 3)
#define AMBARELLA_IO_DESC_DDD_ID	(AMBARELLA_IO_DESC_DSP_ID + 4)
#endif
#endif

struct ambarella_mem_map_desc {
	char		name[32];
	struct map_desc	io_desc;
};
static struct ambarella_mem_map_desc ambarella_io_desc[] = {
#if !defined(CONFIG_PLAT_AMBARELLA_A5S_BOSS) && \
    !defined(CONFIG_PLAT_AMBARELLA_A7_BOSS) && \
    !defined(CONFIG_PLAT_AMBARELLA_A7L_BOSS)
	[AMBARELLA_IO_DESC_AHB_ID] = {
		.name		= "AHB",
		.io_desc	= {
			.virtual= AHB_BASE,
			.pfn	= __phys_to_pfn(AHB_PHYS_BASE),
			.length	= AHB_SIZE,
			.type	= MT_DEVICE,
			},
	},
	[AMBARELLA_IO_DESC_APB_ID] = {
		.name		= "APB",
		.io_desc	= {
			.virtual= APB_BASE,
			.pfn	= __phys_to_pfn(APB_PHYS_BASE),
			.length	= APB_SIZE,
			.type	= MT_DEVICE,
			},
	},
#endif
	[AMBARELLA_IO_DESC_PPM_ID] = {
		.name		= "PPM",	/*Private Physical Memory*/
		.io_desc	= {
			.virtual= NOLINUX_MEM_V_START,
			.pfn	= __phys_to_pfn(DEFAULT_MEM_START),
			.length	= CONFIG_AMBARELLA_PPM_SIZE,
			.type	= MT_MEMORY,
			},
	},
	[AMBARELLA_IO_DESC_BSB_ID] = {
		.name		= "BSB",	/*Bit Stream Buffer*/
		.io_desc	= {
			.virtual= DEFAULT_BSB_BASE,
			.pfn	= __phys_to_pfn(DEFAULT_BSB_START),
			.length	= DEFAULT_BSB_SIZE,
			.type	= MT_MEMORY,
			},
	},
	[AMBARELLA_IO_DESC_DSP_ID] = {
		.name		= "DSP",
		.io_desc	= {
			.virtual= DEFAULT_DSP_BASE,
			.pfn	= __phys_to_pfn(DEFAULT_DSP_START),
			.length	= DEFAULT_DSP_SIZE,
			.type	= MT_MEMORY,
			},
	},
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW)
	[AMBARELLA_IO_DESC_DRAMC_ID] = {
		.name		= "DRAMC",
		.io_desc	= {
			.virtual= DRAMC_BASE,
			.pfn	= __phys_to_pfn(DRAMC_PHYS_BASE),
			.length	= DRAMC_SIZE,
			.type	= MT_DEVICE,
			},
	},
	[AMBARELLA_IO_DESC_CRYPT_ID] = {
		.name		= "CRYPT",
		.io_desc	= {
			.virtual= CRYPT_BASE,
			.pfn	= __phys_to_pfn(CRYPT_PHYS_BASE),
			.length	= CRYPT_SIZE,
			.type	= MT_DEVICE,
			},
	},
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW_CORTEX_EXT)
	[AMBARELLA_IO_DESC_AXI_ID] = {
		.name		= "AXI",
		.io_desc	= {
			.virtual= AXI_BASE,
			.pfn	= __phys_to_pfn(AXI_PHYS_BASE),
			.length	= AXI_SIZE,
			.type	= MT_DEVICE,
			},
	},
	[AMBARELLA_IO_DESC_DDD_ID] = {
		.name		= "DDD",
		.io_desc	= {
			.virtual= DDD_BASE,
			.pfn	= __phys_to_pfn(DDD_PHYS_BASE),
			.length	= DDD_SIZE,
			.type	= MT_DEVICE,
			},
	},
#endif
#endif
};

#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
static struct ambarella_mem_hal_desc ambarella_hal_info = {
	.physaddr			= DEFAULT_HAL_START,
	.size				= DEFAULT_HAL_SIZE,
	.virtual			= DEFAULT_HAL_BASE,
	.remapped			= 0,
	.inited				= 0,
};
DEFINE_SPINLOCK(ambarella_hal_info_lock);
unsigned long ambarella_hal_info_lock_flags;
#endif

static struct ambarella_mem_rev_desc ambarella_bst_info = {
	.physaddr			= DEFAULT_BST_START,
	.size				= DEFAULT_BST_SIZE,
};


#if defined(CONFIG_PLAT_AMBARELLA_BOSS)

static int ipc_smem_init(void)
{
	u32 mem, size;

	mem = boss->smem_addr + BOSS_BOSS_MEM_SIZE;
	size = boss->smem_size - BOSS_BOSS_MEM_SIZE;

	/* Initialize shm buffer */
	if (size < IPC_SHM_MEM_SIZE) {
		return -1;
	}
	ipc_shm_init(mem, IPC_SHM_MEM_SIZE);
	mem += IPC_SHM_MEM_SIZE;
	size -= IPC_SHM_MEM_SIZE;

	/* Initialize spinlock buffer */
	if (size < IPC_SPINLOCK_MEM_SIZE) {
		return -1;
	}
	ipc_slock_init(mem, IPC_SPINLOCK_MEM_SIZE);
	mem += IPC_SPINLOCK_MEM_SIZE;
	size -= IPC_SPINLOCK_MEM_SIZE;

	/* Initialize mutex buffer */
	if (size < IPC_MUTEX_MEM_SIZE) {
		return -1;
	}
	ipc_mutex_init_map(mem, IPC_MUTEX_MEM_SIZE);
	mem += IPC_MUTEX_MEM_SIZE;
	size -= IPC_MUTEX_MEM_SIZE;

	/* Initialize log buffer */
	if (size < IPC_LOG_MEM_SIZE) {
		return -1;
	}
	ipc_log_init(mem, IPC_LOG_MEM_SIZE);
	mem += IPC_LOG_MEM_SIZE;
	size -= IPC_LOG_MEM_SIZE;

	return 0;
}
#endif

void __init ambarella_map_io(void)
{
	int					i;
	u32					iop, ios, iov;
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
	u32					halp, hals, halv;
	unsigned int				hal_type = 0;
	int					bhal_mapped = 0;
	struct map_desc				hal_desc;

	spin_lock_irqsave(&ambarella_hal_info_lock,
		ambarella_hal_info_lock_flags);
	halp = ambarella_hal_info.physaddr;
	hals = ambarella_hal_info.size;
	halv = ambarella_hal_info.virtual;
	spin_unlock_irqrestore(&ambarella_hal_info_lock,
		ambarella_hal_info_lock_flags);
#endif

	for (i = 0; i < ARRAY_SIZE(ambarella_io_desc); i++) {
		iop = __pfn_to_phys(ambarella_io_desc[i].io_desc.pfn);
		ios = ambarella_io_desc[i].io_desc.length;
		iov = ambarella_io_desc[i].io_desc.virtual;
		if (ios > 0) {
			iotable_init(&(ambarella_io_desc[i].io_desc), 1);
			pr_info("Ambarella: %s\t= 0x%08x[0x%08x],0x%08x %d\n",
				ambarella_io_desc[i].name, iop, iov, ios,
				ambarella_io_desc[i].io_desc.type);
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
			if ((halv >= iov) && ((halv + hals) <= (iov + ios))) {
				bhal_mapped = 1;
				hal_type = ambarella_io_desc[i].io_desc.type;
			}
#endif
		}
	}

	if (ambarella_io_desc[AMBARELLA_IO_DESC_PPM_ID].io_desc.length == 0) {
		iov = __phys_to_virt(DEFAULT_MEM_START);
		ios = SZ_1M;
		ambarella_io_desc[AMBARELLA_IO_DESC_PPM_ID].io_desc.virtual =
			iov;
		ambarella_io_desc[AMBARELLA_IO_DESC_PPM_ID].io_desc.length =
			ios;
		ambarella_io_desc[AMBARELLA_IO_DESC_PPM_ID].io_desc.pfn =
			__phys_to_pfn(DEFAULT_MEM_START);
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
		if ((halv >= iov) && ((halv + hals) <= (iov + ios))) {
			bhal_mapped = 1;
			hal_type = MT_MEMORY;
		}
#endif
	}

#if defined(CONFIG_PLAT_AMBARELLA_A5S_BOSS) || \
    defined(CONFIG_PLAT_AMBARELLA_A7_BOSS) || \
    defined(CONFIG_PLAT_AMBARELLA_A7L_BOSS)
	bhal_mapped = 1;
	ambarella_hal_info.remapped = 1;
#endif

#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
	spin_lock_irqsave(&ambarella_hal_info_lock,
		ambarella_hal_info_lock_flags);
	if (!bhal_mapped) {
		if (!ambarella_hal_info.remapped) {
			hal_desc.virtual = halv;
			hal_desc.pfn = __phys_to_pfn(halp);
			hal_desc.length = hals;
#if defined(CONFIG_PLAT_AMBARELLA_BOSS)
			hal_desc.type = MT_MEMORY_NONCACHED;
#else
			hal_desc.type = MT_MEMORY;
#endif
			iotable_init(&hal_desc, 1);
			bhal_mapped = 1;
			hal_type = hal_desc.type;
			ambarella_hal_info.remapped = bhal_mapped;
		} else {
#if defined(CONFIG_PLAT_AMBARELLA_BOSS)
			hal_type = MT_MEMORY_NONCACHED;
#else
			hal_type = MT_MEMORY;
#endif
		}
	} else {
		ambarella_hal_info.remapped = bhal_mapped;
	}
	if (ambarella_hal_info.remapped)
		pr_info("Ambarella: HAL\t= 0x%08x[0x%08x],0x%08x %d\n",
			halp, halv, hals, hal_type);
	else
		pr_info("Ambarella: HAL\t= 0x%08x[0x%08x],0x%08x Map Fail!\n",
			halp, halv, hals);
	spin_unlock_irqrestore(&ambarella_hal_info_lock,
		ambarella_hal_info_lock_flags);
#endif

#if defined(CONFIG_PLAT_AMBARELLA_BOSS)
	if (boss != NULL) {
		u32 virt, phys;
		int rval;
#if defined(CONFIG_BOSS_MULTIPLE_CORE)
		pgd_t *pgd;
		pmd_t *pmd;
#endif
		pr_notice ("aipc: smem: %08x, %08x\n", boss->smem_addr, boss->smem_size);
		phys = boss->smem_addr;
		virt = ipc_phys_to_virt (phys);

#if defined(CONFIG_BOSS_MULTIPLE_CORE)
		pgd = pgd_offset_k(virt);
		pmd = pmd_offset(pgd, virt);
		if (virt & PGDIR_MASK) {
			pmd++;
		}
		*pmd = __pmd(phys | PMD_TYPE_SECT | PMD_SECT_AP_WRITE | PMD_SECT_S);
		flush_pmd_entry(pmd);
#endif
		boss->smem_addr = virt;

		rval = ipc_smem_init();
		K_ASSERT(rval == 0);

		boss->log_buf_ptr = ipc_virt_to_phys (boss_log_buf_ptr);
		boss->log_buf_len_ptr = ipc_virt_to_phys (boss_log_buf_len_ptr);
		boss->log_buf_last_ptr = ipc_virt_to_phys (boss_log_buf_last_ptr);
	}
#endif
}

/* ==========================================================================*/
static int __init dsp_mem_check(u32 start, u32 size)
{
	u32					vstart;
	u32					tmp;

	if ((start & MEM_MAP_CHECK_MASK) || (start < DEFAULT_MEM_START)) {
		pr_err("Ambarella: Bad DSP mem start 0x%08x\n", start);
		return -EINVAL;
	}

	if (size & MEM_MAP_CHECK_MASK) {
		pr_err("Ambarella: Bad DSP mem size 0x%08x\n", size);
		return -EINVAL;
	}

	vstart = (start - DEFAULT_MEM_START) + NOLINUX_MEM_V_START;
	if (vstart < VMALLOC_END) {
		pr_err("Ambarella: Bad DSP virtual 0x%08x\n", vstart);
		return -EINVAL;
	}
	if (vstart >= (NOLINUX_MEM_V_START + NOLINUX_MEM_V_SIZE)) {
		tmp = vstart - NOLINUX_MEM_V_SIZE;
		if (tmp < NOLINUX_MEM_V_START) {
			tmp = NOLINUX_MEM_V_START;
		}
		pr_info("%s: Change DSP start form 0x%08x to 0x%08x\n",
			__func__, vstart, tmp);
		vstart = tmp;
	}
	if ((vstart + size) > (NOLINUX_MEM_V_START + NOLINUX_MEM_V_SIZE)) {
		tmp = (NOLINUX_MEM_V_START + NOLINUX_MEM_V_SIZE) - vstart;
		pr_info("%s: Change DSP size form 0x%08x to 0x%08x\n",
			__func__, size, tmp);
		size = tmp;
	}

	ambarella_io_desc[AMBARELLA_IO_DESC_DSP_ID].io_desc.virtual = vstart;
	ambarella_io_desc[AMBARELLA_IO_DESC_DSP_ID].io_desc.pfn =
		__phys_to_pfn(start);
	ambarella_io_desc[AMBARELLA_IO_DESC_DSP_ID].io_desc.length = size;

	return 0;
}

static int __init early_dsp(char *p)
{
	unsigned long				start = 0;
	unsigned long				size = 0;
	char					*endp;

	start = memparse(p, &endp);
	if (*endp == ',')
		size = memparse(endp + 1, NULL);

	return dsp_mem_check(start, size);
}
early_param("dsp", early_dsp);

static int __init parse_mem_tag_dsp(const struct tag *tag)
{
	return dsp_mem_check(tag->u.mem.start, tag->u.mem.size);
}
__tagtable(ATAG_AMBARELLA_DSP, parse_mem_tag_dsp);

/* ==========================================================================*/
static int __init bsb_mem_check(u32 start, u32 size)
{
	u32					vstart;
	u32					tmp;

	if ((start & MEM_MAP_CHECK_MASK) || (start < DEFAULT_MEM_START)) {
		pr_err("Ambarella: Bad BSB mem start 0x%08x\n", start);
		return -EINVAL;
	}

	if (size & MEM_MAP_CHECK_MASK) {
		pr_err("Ambarella: Bad BSB mem size 0x%08x\n", size);
		return -EINVAL;
	}

	vstart = (start - DEFAULT_MEM_START) + NOLINUX_MEM_V_START;
	if (vstart < VMALLOC_END) {
		pr_err("Ambarella: Bad BSB virtual 0x%08x\n", vstart);
		return -EINVAL;
	}
	if (vstart >= (NOLINUX_MEM_V_START + NOLINUX_MEM_V_SIZE)) {
		tmp = vstart - NOLINUX_MEM_V_SIZE;
		if (tmp < NOLINUX_MEM_V_START) {
			tmp = NOLINUX_MEM_V_START;
		}
		pr_info("%s: Change DSP start form 0x%08x to 0x%08x\n",
			__func__, vstart, tmp);
		vstart = tmp;
	}
	if ((vstart + size) > (NOLINUX_MEM_V_START + NOLINUX_MEM_V_SIZE)) {
		tmp = (NOLINUX_MEM_V_START + NOLINUX_MEM_V_SIZE) - vstart;
		pr_info("%s: Change DSP size form 0x%08x to 0x%08x\n",
			__func__, size, tmp);
		size = tmp;
	}

	ambarella_io_desc[AMBARELLA_IO_DESC_BSB_ID].io_desc.virtual = vstart;
	ambarella_io_desc[AMBARELLA_IO_DESC_BSB_ID].io_desc.pfn =
		__phys_to_pfn(start);
	ambarella_io_desc[AMBARELLA_IO_DESC_BSB_ID].io_desc.length = size;

	high_memory = __va(__pfn_to_phys(__phys_to_pfn(start)));

	return 0;
}

static int __init early_bsb(char *p)
{
	unsigned long				start = 0;
	unsigned long				size = 0;
	char					*endp;

	start = memparse(p, &endp);
	if (*endp == ',')
		size = memparse(endp + 1, NULL);

	return bsb_mem_check(start, size);
}
early_param("bsb", early_bsb);

/* ==========================================================================*/
static int __init parse_mem_tag_bsb(const struct tag *tag)
{
	return bsb_mem_check(tag->u.mem.start, tag->u.mem.size);
}
__tagtable(ATAG_AMBARELLA_BSB, parse_mem_tag_bsb);

static int __init parse_mem_tag_bst(const struct tag *tag)
{
	ambarella_bst_info.physaddr = tag->u.mem.start;
	ambarella_bst_info.size = tag->u.mem.size;

	return 0;
}
__tagtable(ATAG_AMBARELLA_BST, parse_mem_tag_bst);

u32 get_ambarella_ppm_phys(void)
{
	return __pfn_to_phys(
		ambarella_io_desc[AMBARELLA_IO_DESC_PPM_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_ambarella_ppm_phys);

u32 get_ambarella_ppm_virt(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_PPM_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_ambarella_ppm_virt);

u32 get_ambarella_ppm_size(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_PPM_ID].io_desc.length;
}
EXPORT_SYMBOL(get_ambarella_ppm_size);

u32 get_ambarella_bsbmem_phys(void)
{
	return __pfn_to_phys(
		ambarella_io_desc[AMBARELLA_IO_DESC_BSB_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_ambarella_bsbmem_phys);

u32 get_ambarella_bsbmem_virt(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_BSB_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_ambarella_bsbmem_virt);

u32 get_ambarella_bsbmem_size(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_BSB_ID].io_desc.length;
}
EXPORT_SYMBOL(get_ambarella_bsbmem_size);

u32 get_ambarella_dspmem_phys(void)
{
	return __pfn_to_phys(
		ambarella_io_desc[AMBARELLA_IO_DESC_DSP_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_ambarella_dspmem_phys);

u32 get_ambarella_dspmem_virt(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_DSP_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_ambarella_dspmem_virt);

u32 get_ambarella_dspmem_size(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_DSP_ID].io_desc.length;
}
EXPORT_SYMBOL(get_ambarella_dspmem_size);

u32 get_ambarella_bstmem_info(u32 *bstadd, u32 *bstsize)
{
#if !defined(CONFIG_PLAT_AMBARELLA_BOSS)
	if ((ambarella_bst_info.size == 0) ||
		(ambarella_bst_info.physaddr < get_ambarella_ppm_phys()) ||
		((ambarella_bst_info.physaddr + ambarella_bst_info.size) >
		(get_ambarella_ppm_phys() + get_ambarella_ppm_size())))
		return AMB_BST_INVALID;
#endif
	*bstadd = ambarella_bst_info.physaddr;
	*bstsize = ambarella_bst_info.size;

	return AMB_BST_MAGIC;
}
EXPORT_SYMBOL(get_ambarella_bstmem_info);

u32 *get_ambarella_bstmem_head(void)
{
	u32 *pstart_address = NULL;
	u32 *phead_address = NULL;
	u32 offset = 0;
	u32 valid = 0;

	if (ambarella_bst_info.size == 0)
		return (u32 *)AMB_BST_INVALID;

	pstart_address = (u32 *)ambarella_phys_to_virt(
		ambarella_bst_info.physaddr);
	for (phead_address =
		&pstart_address[(ambarella_bst_info.size >> 2) - 1];
		phead_address > pstart_address; phead_address--) {
		if (*phead_address == AMB_BST_MAGIC) {
			valid = 1;
			break;
		}
	}
	for (; phead_address > pstart_address; phead_address--) {
		if (*phead_address != AMB_BST_MAGIC) {
			offset = *phead_address;
			break;
		}
	}
	if ((offset == 0) || (valid == 0))
		return (u32 *)AMB_BST_INVALID;

	phead_address -= (offset >> 2);

	return phead_address;
}
EXPORT_SYMBOL(get_ambarella_bstmem_head);

u32 ambarella_phys_to_virt(u32 paddr)
{
	int					i;
	u32					phystart;
	u32					phylength;
	u32					phyoffset;
	u32					vstart;

	for (i = 0; i < ARRAY_SIZE(ambarella_io_desc); i++) {
		phystart = __pfn_to_phys(ambarella_io_desc[i].io_desc.pfn);
		phylength = ambarella_io_desc[i].io_desc.length;
		vstart = ambarella_io_desc[i].io_desc.virtual;
		if ((paddr >= phystart) && (paddr < (phystart + phylength))) {
			phyoffset = paddr - phystart;
			return (vstart + phyoffset);
		}
	}

	return __phys_to_virt(paddr);
}
EXPORT_SYMBOL(ambarella_phys_to_virt);

u32 ambarella_virt_to_phys(u32 vaddr)
{
	int					i;
	u32					phystart;
	u32					vlength;
	u32					voffset;
	u32					vstart;

	for (i = 0; i < ARRAY_SIZE(ambarella_io_desc); i++) {
		phystart = __pfn_to_phys(ambarella_io_desc[i].io_desc.pfn);
		vlength = ambarella_io_desc[i].io_desc.length;
		vstart = ambarella_io_desc[i].io_desc.virtual;
		if ((vaddr >= vstart) && (vaddr < (vstart + vlength))) {
			voffset = vaddr - vstart;
			return (phystart + voffset);
		}
	}

	return __virt_to_phys(vaddr);
}
EXPORT_SYMBOL(ambarella_virt_to_phys);

u32 get_ambarella_ahb_phys(void)
{
	return __pfn_to_phys(
		ambarella_io_desc[AMBARELLA_IO_DESC_AHB_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_ambarella_ahb_phys);

u32 get_ambarella_ahb_virt(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_AHB_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_ambarella_ahb_virt);

u32 get_ambarella_ahb_size(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_AHB_ID].io_desc.length;
}
EXPORT_SYMBOL(get_ambarella_ahb_size);

u32 get_ambarella_apb_phys(void)
{
	return __pfn_to_phys(
		ambarella_io_desc[AMBARELLA_IO_DESC_APB_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_ambarella_apb_phys);

u32 get_ambarella_apb_virt(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_APB_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_ambarella_apb_virt);

u32 get_ambarella_apb_size(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_APB_ID].io_desc.length;
}
EXPORT_SYMBOL(get_ambarella_apb_size);

/* ==========================================================================*/
static struct ambarella_mem_rev_info ambarella_reserve_mem_info = {
	.counter			= 0,
};

static int __init reserve_mem_check(u32 start, u32 size)
{
	u32					index;

	if ((start & MEM_MAP_CHECK_MASK) || (start < PHYS_OFFSET)) {
		pr_err("Ambarella: Bad revmem start 0x%08x\n", start);
		return -EINVAL;
	}

	if (size & MEM_MAP_CHECK_MASK) {
		pr_err("Ambarella: Bad revmem size 0x%08x\n", size);
		return -EINVAL;
	}

	index = ambarella_reserve_mem_info.counter;
	if (index < MEMORY_RESERVE_MAX_NR) {
		ambarella_reserve_mem_info.desc[index].physaddr = start;
		ambarella_reserve_mem_info.desc[index].size = size;
		ambarella_reserve_mem_info.counter++;
	} else {
		pr_err("Ambarella: too much revmem %d!\n", index);
		return -EINVAL;
	}

	return 0;
}

static int __init early_revmem(char *p)
{
	unsigned long				start = 0;
	unsigned long				size = 0;
	char					*endp;

	start = memparse(p, &endp);
	if (*endp == ',')
		size = memparse(endp + 1, NULL);

	return reserve_mem_check(start, size);
}
early_param("revmem", early_revmem);

static int __init parse_mem_tag_revmem(const struct tag *tag)
{
	return reserve_mem_check(tag->u.mem.start, tag->u.mem.size);
}
__tagtable(ATAG_AMBARELLA_REVMEM, parse_mem_tag_revmem);

u32 get_ambarella_mem_rev_info(struct ambarella_mem_rev_info *pinfo)
{
	int					i;
	u32					revp, revs;

	if (ambarella_reserve_mem_info.counter)
		pr_info("Ambarella Reserve Memory:\n");

	for (i = 0; i < ambarella_reserve_mem_info.counter; i++) {
		revp = ambarella_reserve_mem_info.desc[i].physaddr;
		revs = ambarella_reserve_mem_info.desc[i].size;
		pr_info("\t%02d:\t0x%08x[0x%08x]\tNormal\n", i, revp, revs);
	}

	*pinfo = ambarella_reserve_mem_info;

	return 0;
}
EXPORT_SYMBOL(get_ambarella_mem_rev_info);

/* ==========================================================================*/
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)

static int __init hal_mem_check(u32 pstart, u32 size, u32 vstart)
{
	if ((pstart & MEM_MAP_CHECK_MASK) || (pstart < DEFAULT_MEM_START)) {
		pr_err("Ambarella: Bad HAL pstart 0x%08x\n", pstart);
		return -EINVAL;
	}

	if (vstart & MEM_MAP_CHECK_MASK) {
		pr_err("Ambarella: Bad HAL vstart 0x%08x\n", vstart);
		return -EINVAL;
	}

	spin_lock_irqsave(&ambarella_hal_info_lock,
		ambarella_hal_info_lock_flags);
	ambarella_hal_info.physaddr = pstart;
	ambarella_hal_info.size = size;
	ambarella_hal_info.virtual = vstart;
	spin_unlock_irqrestore(&ambarella_hal_info_lock,
		ambarella_hal_info_lock_flags);

	return 0;
}

static int __init early_hal(char *p)
{
	unsigned long				pstart = 0;
	unsigned long				size = 0;
	unsigned long				vstart = 0;
	char					*endp;

	pstart = memparse(p, &endp);
	if (*endp == ',')
		vstart = memparse(endp + 1, &endp);
	if (*endp == ',')
		size = memparse(endp + 1, NULL);

	return hal_mem_check(pstart, size, vstart);
}
early_param("hal", early_hal);

static int __init parse_mem_tag_hal(const struct tag *tag)
{
	return hal_mem_check(tag->u.ramdisk.start,
		tag->u.ramdisk.size, tag->u.ramdisk.flags);
}
__tagtable(ATAG_AMBARELLA_HAL, parse_mem_tag_hal);

void *get_ambarella_hal_vp(void)
{
	u32					hal_vp;

	spin_lock_irqsave(&ambarella_hal_info_lock,
		ambarella_hal_info_lock_flags);
	if (unlikely((!ambarella_hal_info.remapped))) {
		pr_err("%s: remap HAL first!\n", __func__);
		BUG();
	}

	if (unlikely((!ambarella_hal_info.inited))) {
		amb_hal_success_t		retval;

#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW)
#if defined(CONFIG_AMBARELLA_RAW_BOOT)
		pr_err("%s: %d!\n", __func__, __LINE__);
		retval = amb_hal_init(
			(void *)ambarella_hal_info.virtual,
			(void *)APB_BASE, (void *)AHB_BASE,
			(void *)DRAMC_BASE);
#else
		retval = amb_set_peripherals_base_address(
			(void *)ambarella_hal_info.virtual,
			(void *)APB_BASE, (void *)AHB_BASE,
			(void *)DRAMC_BASE);
#endif
#else
#if defined(CONFIG_AMBARELLA_RAW_BOOT)
		retval = amb_hal_init(
			(void *)ambarella_hal_info.virtual,
			(void *)APB_BASE, (void *)AHB_BASE,
			(void *)DRAMC_BASE);
#else
#if defined(CONFIG_PLAT_AMBARELLA_A7L_BOSS) || \
    defined(CONFIG_PLAT_AMBARELLA_A7L)
		retval = amb_set_peripherals_base_address(
			(void *)ambarella_hal_info.virtual,
			(void *)APB_BASE, (void *)AHB_BASE,
			(void *)DRAMC_BASE);
#else
		retval = amb_set_peripherals_base_address(
			(void *)ambarella_hal_info.virtual,
			(void *)APB_BASE, (void *)AHB_BASE);
#endif
#endif
#endif
		BUG_ON(retval != AMB_HAL_SUCCESS);
		ambarella_hal_info.inited = 1;
	}

	hal_vp = ambarella_hal_info.virtual;
	spin_unlock_irqrestore(&ambarella_hal_info_lock,
		ambarella_hal_info_lock_flags);

	return (void *)hal_vp;
}
EXPORT_SYMBOL(get_ambarella_hal_vp);

void set_ambarella_hal_invalid(void)
{
	ambarella_hal_info.inited = 0;
}
u32 ambarella_hal_get_size(void)
{
	u32					hal_size;

	spin_lock_irqsave(&ambarella_hal_info_lock,
		ambarella_hal_info_lock_flags);
	hal_size = ambarella_hal_info.size;
	spin_unlock_irqrestore(&ambarella_hal_info_lock,
		ambarella_hal_info_lock_flags);
	return hal_size;
}
EXPORT_SYMBOL(ambarella_hal_get_size);

void ambarella_hal_set_invalid(void)
{
	spin_lock_irqsave(&ambarella_hal_info_lock,
		ambarella_hal_info_lock_flags);
	ambarella_hal_info.inited = 0;
	spin_unlock_irqrestore(&ambarella_hal_info_lock,
		ambarella_hal_info_lock_flags);
}
#endif

int arch_pfn_is_nosave(unsigned long pfn)
{
	int					i;
	unsigned long				nosave_begin_pfn;
	unsigned long				nosave_end_pfn;

	nosave_begin_pfn =
		ambarella_io_desc[AMBARELLA_IO_DESC_PPM_ID].io_desc.pfn;
	nosave_end_pfn = PFN_PHYS(nosave_begin_pfn) +
		ambarella_io_desc[AMBARELLA_IO_DESC_PPM_ID].io_desc.length;
	nosave_end_pfn = PFN_UP(nosave_end_pfn);

	if ((pfn >= nosave_begin_pfn) && (pfn < nosave_end_pfn))
		return 1;

	for (i = 0; i < ambarella_reserve_mem_info.counter; i++) {
		nosave_begin_pfn = __phys_to_pfn(
			ambarella_reserve_mem_info.desc[i].physaddr);
		nosave_end_pfn = __phys_to_pfn(
			ambarella_reserve_mem_info.desc[i].physaddr +
			ambarella_reserve_mem_info.desc[i].size);
		if ((pfn >= nosave_begin_pfn) && (pfn < nosave_end_pfn))
			return 1;
	}

	return 0;
}


#if defined(CONFIG_PLAT_AMBARELLA_BOSS)

static int __init early_boss(char *p)
{
	extern unsigned int boss_log_buf_ptr;
	extern unsigned int boss_log_buf_len_ptr;
	extern unsigned int boss_log_buf_last_ptr;
	unsigned long paddr, vaddr;

	char *endp;

	paddr = memparse(p, &endp);
	vaddr = ipc_phys_to_virt(paddr);

	printk (KERN_NOTICE "boss: %08lx [%08lx]\n", vaddr, paddr);
	printk (KERN_NOTICE "boss: printk: %08x [%08x], %08x [%08x], %08x [%08x]\n",
		(u32) boss_log_buf_ptr, ipc_virt_to_phys (boss_log_buf_ptr),
		(u32) boss_log_buf_len_ptr, ipc_virt_to_phys (boss_log_buf_len_ptr),
		(u32) boss_log_buf_last_ptr, ipc_virt_to_phys (boss_log_buf_last_ptr));

	boss = (struct boss_s *) vaddr;

	return 0;
}

early_param("boss", early_boss);

#endif

/* ==========================================================================*/
static int __init early_boot_splash_logo(char *p)
{
	ambarella_boot_splash_logo = 1;
	return 0;
}
early_param("boot_splash_logo_on", early_boot_splash_logo);
