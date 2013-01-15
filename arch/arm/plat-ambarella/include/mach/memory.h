/*
 * arch/arm/plat-ambarella/include/mach/memory.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
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

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

/*
 * MAP Sample	Phisical		Virtual			Size
 * --------------------------------------------------------------------------
 * AHB(IO)	AHB_PHYS_BASE		AHB_BASE		AHB_SIZE
 * APB(IO)	APB_PHYS_BASE		APB_BASE		APB_BASE
 * AXI(IO)	AXI_PHYS_BASE		AXI_BASE		AXI_SIZE
 * PPM		DEFAULT_MEM_START	NOLINUX_MEM_V_START	CONFIG_AMBARELLA_PPM_SIZE
 * Linux MEM	PHYS_OFFSET		CONFIG_VMSPLIT_xG	Linux MEM Size
 * BSB		BSB_START		BSB_BASE		BSB_SIZE
 * DSP		DSP_START		DSP_BASE		DSP_SIZE
 */

/* ==========================================================================*/
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW)
#define DEFAULT_MEM_START		(0x00000000)
#else
#define DEFAULT_MEM_START		(0xc0000000)
#endif
#define PHYS_OFFSET			(DEFAULT_MEM_START + CONFIG_AMBARELLA_PPM_SIZE)

/* ==========================================================================*/
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW)
#define AHB_PHYS_BASE			(0xe0000000)
#define APB_PHYS_BASE			(0xe8000000)
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW_CORTEX_EXT)
#define AXI_PHYS_BASE			(0xf0000000)
#define DDD_PHYS_BASE			(0xf0020000)
#endif
#define DRAMC_PHYS_BASE			(0xdffe0000)
#define CRYPT_PHYS_BASE			(0xfffef000)
#else
#define AHB_PHYS_BASE			(0x60000000)
#define APB_PHYS_BASE			(0x70000000)
#endif

#if defined(CONFIG_VMSPLIT_3G)
#define NOLINUX_MEM_V_START		(0xe0000000)
#define NOLINUX_MEM_V_SIZE		(0x10000000)
#define AHB_BASE			(0xf0000000)
#define APB_BASE			(0xf1000000)
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW)
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW_CORTEX_EXT)
#define AXI_BASE			(0xf2000000)
#define DDD_BASE			(0xf2020000)
#endif
#define DRAMC_BASE			(0xf2040000)
#define CRYPT_BASE			(0xf2080000)
#endif
#else
#define NOLINUX_MEM_V_START		(0xb0000000)
#define NOLINUX_MEM_V_SIZE		(0x20000000)


#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW)
#define AHB_BASE			AHB_PHYS_BASE
#define APB_BASE			APB_PHYS_BASE
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW_CORTEX_EXT)
#define AXI_BASE			AXI_PHYS_BASE
#define DDD_BASE			DDD_PHYS_BASE
#endif
#define DRAMC_BASE			DRAMC_PHYS_BASE
#define CRYPT_BASE			CRYPT_PHYS_BASE
#else
#if defined(CONFIG_PLAT_AMBARELLA_A5S_BOSS) || \
    defined(CONFIG_PLAT_AMBARELLA_A7_BOSS) || \
    defined(CONFIG_PLAT_AMBARELLA_A7L_BOSS)
#define AHB_BASE			(0xd8000000)
#define APB_BASE			(0xd9000000)
#define DRAMC_BASE			(0xfffe0000)
#else
#define AHB_BASE			(0xe0000000)
#define APB_BASE			(0xe8000000)
#define DRAMC_BASE			(0xfffe0000)
#endif
#endif
#endif

#define AHB_SIZE			(0x01000000)
#define APB_SIZE			(0x01000000)
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW)
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW_CORTEX_EXT)
#define AXI_SIZE			(0x00003000)
#define DDD_SIZE			(0x00000e00)
#endif
#define DRAMC_SIZE			(0x00020000)
#define CRYPT_SIZE			(0x00001000)
#endif

/* ==========================================================================*/
#define DEFAULT_BSB_START		(0x00000000)
#define DEFAULT_BSB_BASE		(0x00000000)
#define DEFAULT_BSB_SIZE		(0x00000000)

#define DEFAULT_DSP_START		(0x00000000)
#define DEFAULT_DSP_BASE		(0x00000000)
#define DEFAULT_DSP_SIZE		(0x00000000)

#define DEFAULT_BST_START		(DEFAULT_MEM_START + 0x00000000)
#define DEFAULT_BST_SIZE		(0x00000000)
#define AMB_BST_MAGIC			(0xffaa5500)
#define AMB_BST_INVALID			(0xdeadbeaf)
#define AMB_BST_START_COUNTER		(0xffffffff)

#define DEFAULT_DEBUG_START		(DEFAULT_MEM_START + 0x000f8000)
#define DEFAULT_DEBUG_SIZE		(0x00008000)

/* ==========================================================================*/
#define __virt_to_bus(x)		__virt_to_phys(x)
#define __bus_to_virt(x)		__phys_to_virt(x)
#define __pfn_to_bus(x)			__pfn_to_phys(x)
#define __bus_to_pfn(x)			__phys_to_pfn(x)

/* ==========================================================================*/
#define CONSISTENT_DMA_SIZE		SZ_8M

/* ==========================================================================*/
#define MAX_PHYSMEM_BITS		32
#define SECTION_SIZE_BITS		24
/* ==========================================================================*/

#endif

