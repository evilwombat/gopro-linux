/*
 * arch/arm/plat-ambarella/include/plat/atag.h
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

#ifndef __PLAT_AMBARELLA_ATAG_H
#define __PLAT_AMBARELLA_ATAG_H

/* ==========================================================================*/
#define MEM_MAP_CHECK_MASK		(PAGE_SIZE - 1)

#define ATAG_AMBARELLA_DSP		0x44000110
#define ATAG_AMBARELLA_BSB		0x44000044
#define ATAG_AMBARELLA_REVMEM		0x44001111
#define ATAG_AMBARELLA_HAL		0x44000722
#define ATAG_AMBARELLA_BST		0x44000927
#define ATAG_AMBARELLA_BAPI		0x44000426

#define ATAG_AMBARELLA_NAND_BST		0x44040400
#define ATAG_AMBARELLA_NAND_BLD		0x44040401
#define ATAG_AMBARELLA_NAND_PTB		0x44040402
#define ATAG_AMBARELLA_NAND_PRI		0x44040403
#define ATAG_AMBARELLA_NAND_ROM		0x44040404
#define ATAG_AMBARELLA_NAND_BAK		0x44040405
#define ATAG_AMBARELLA_NAND_PBA		0x44040406
#define ATAG_AMBARELLA_NAND_DSP		0x44040407
#define ATAG_AMBARELLA_NAND_NFTL	0x44040408
#define ATAG_AMBARELLA_NAND_RAM		0x44040409

#define ATAG_AMBARELLA_NAND_CS		0x440404F0
#define ATAG_AMBARELLA_NAND_T0		0x440404F1
#define ATAG_AMBARELLA_NAND_T1		0x440404F2
#define ATAG_AMBARELLA_NAND_T2		0x440404F3

#define ATAG_AMBARELLA_ETH0		0x44040410
#define ATAG_AMBARELLA_ETH1		0x44040411
#define ATAG_AMBARELLA_WIFI		0x44040412
#define ATAG_AMBARELLA_UETH		0x44040414

#define MAX_NAND_ERASE_BLOCK_SIZE	(2048 * 128)
#define MAX_AMBOOT_PARTITION_NR		(16)
#define MAX_AMBOOT_PARTITION_NANE_SIZE	(16)

#define AMBOOT_DEFAULT_BST_SIZE		(MAX_NAND_ERASE_BLOCK_SIZE)
#define AMBOOT_DEFAULT_PTB_SIZE		(MAX_NAND_ERASE_BLOCK_SIZE)
#define AMBOOT_DEFAULT_BLD_SIZE 	(MAX_NAND_ERASE_BLOCK_SIZE)
#define AMBOOT_DEFAULT_PRI_SIZE 	(4 * 1024 * 1024)
#define AMBOOT_DEFAULT_RXM_SIZE		(8 * 1024 * 1024)

#define AMBARELLA_BOARD_TYPE_AUTO	(0)
#define AMBARELLA_BOARD_TYPE_BUB	(1)
#define AMBARELLA_BOARD_TYPE_EVK	(2)
#define AMBARELLA_BOARD_TYPE_IPCAM	(3)
#define AMBARELLA_BOARD_TYPE_VENDOR	(4)

#define AMBARELLA_BOARD_CHIP_AUTO	(0)

#define AMBARELLA_BOARD_REV_AUTO	(0)

#define AMBARELLA_BOARD_VERSION(c,t,r)	(((c) << 16) + ((t) << 12) + (r))
#define AMBARELLA_BOARD_CHIP(v)		(((v) >> 16) & 0xFFFF)
#define AMBARELLA_BOARD_TYPE(v)		(((v) >> 12) & 0xF)
#define AMBARELLA_BOARD_REV(v)		(((v) >> 0) & 0xFFF)

#define MEMORY_RESERVE_MAX_NR		(16)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct ambarella_mem_rev_desc {
	unsigned long			physaddr;
	unsigned long			size;
};

struct ambarella_mem_rev_info {
	u32				counter;
	struct ambarella_mem_rev_desc	desc[MEMORY_RESERVE_MAX_NR];
};

/* ==========================================================================*/
extern u32 get_ambarella_mem_rev_info(struct ambarella_mem_rev_info *pinfo);

extern u32 get_ambarella_ppm_phys(void);
extern u32 get_ambarella_ppm_virt(void);
extern u32 get_ambarella_ppm_size(void);

extern u32 get_ambarella_bsbmem_phys(void);
extern u32 get_ambarella_bsbmem_virt(void);
extern u32 get_ambarella_bsbmem_size(void);

extern u32 get_ambarella_dspmem_phys(void);
extern u32 get_ambarella_dspmem_virt(void);
extern u32 get_ambarella_dspmem_size(void);

extern u32 ambarella_phys_to_virt(u32 paddr);
extern u32 ambarella_virt_to_phys(u32 vaddr);

extern u32 get_ambarella_ahb_phys(void);
extern u32 get_ambarella_ahb_virt(void);
extern u32 get_ambarella_ahb_size(void);

extern u32 get_ambarella_apb_phys(void);
extern u32 get_ambarella_apb_virt(void);
extern u32 get_ambarella_apb_size(void);

extern u32 get_ambarella_bstmem_info(u32 *bstadd, u32 *bstsize);
extern u32 *get_ambarella_bstmem_head(void);

extern u32 ambarella_boot_splash_logo;

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

