/*
 * arch/arm/plat-ambarella/include/plat/nand.h
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
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

#ifndef __ASM_ARCH_NAND_H
#define __ASM_ARCH_NAND_H

/* ==========================================================================*/
#define EC_MDSD		0	/* check main disable and spare disable */
#define EC_MDSE		1	/* check main disable and spare enable */
#define EC_MESD		2	/* check main enable and spare disable */
#define EC_MESE		3	/* check main enable and spare enable */

#define EG_MDSD		0	/* generate main disable and spare disable */
#define EG_MDSE		1	/* generate main disable and spare enable */
#define EG_MESD		2	/* generate main enable and spare disable */
#define EG_MESE		3	/* generate main enable and spare enable */

/**
 * Define for xdhost and nand_host
 */
#define MAIN_ONLY		0
#define SPARE_ONLY		1
#define MAIN_ECC		2
#define SPARE_ECC		3
#define SPARE_ONLY_BURST	4
#define SPARE_ECC_BURST		5

/* ==========================================================================*/
#ifndef __ASSEMBLER__
/* struct ambarella_nand_set
 *
 * define an set of one or more nand chips registered with an unique mtd
 *
 * nr_chips	 = number of chips in this set
 * nr_partitions = number of partitions pointed to be partitoons (or zero)
 * name		 = name of set (optional)
 * nr_map	 = map for low-layer logical to physical chip numbers (option)
 * partitions	 = mtd partition list
*/
struct ambarella_nand_timing
{
	u32	control;		/**< Control parameter */
	u32	size;			/**< Block(NAND)/Sector(NOR) size */
	u32	timing0;		/**< Timing param. */
	u32	timing1;		/**< Timing param. */
	u32	timing2;		/**< Timing param. */
	u32	timing3;		/**< Timing param. */
	u32	timing4;		/**< Timing param. */
	u32	timing5;		/**< Timing param. */
};

struct ambarella_nand_set
{
	int				nr_chips;
	int				nr_partitions;
	char				*name;
	int				*nr_map;
};

struct ambarella_platform_nand
{
	struct ambarella_nand_timing	*timing;
	struct ambarella_nand_set	*sets;
	u32				flash_bbt;

	int				(*parse_error)(u32 reg);
	void				(*request)(void);
	void				(*release)(void);
};

/* ==========================================================================*/
extern struct platform_device		ambarella_nand;
extern struct platform_device		ambarella_nor;

/* ==========================================================================*/
extern void ambarella_init_nand_hotboot(struct ambarella_nand_timing *hot_nand_timing);

#endif //__ASSEMBLER__
/* ==========================================================================*/

#endif

