/*
 * arch/arm/plat-ambarella/generic/init.c
 *
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
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <linux/memblock.h>

#include <mach/hardware.h>
#include <mach/board.h>
#include <mach/init.h>

#include <hal/hal.h>

/* ==========================================================================*/
u64 ambarella_dmamask = DMA_BIT_MASK(32);
EXPORT_SYMBOL(ambarella_dmamask);

/* ==========================================================================*/
static struct proc_dir_entry *ambarella_proc_dir = NULL;

int __init ambarella_create_proc_dir(void)
{
	int					retval = 0;

	ambarella_proc_dir = proc_mkdir("ambarella", NULL);
	if (!ambarella_proc_dir)
		retval = -ENOMEM;

	return retval;
}

struct proc_dir_entry *get_ambarella_proc_dir(void)
{
	return ambarella_proc_dir;
}
EXPORT_SYMBOL(get_ambarella_proc_dir);

/* ==========================================================================*/
void __init ambarella_memblock_reserve(void)
{
	struct ambarella_mem_rev_info		rev_info;
	int					i;
	u32					bstadd, bstsize;

	if (!get_ambarella_mem_rev_info(&rev_info)) {
		for (i = 0; i < rev_info.counter; i++) {
			memblock_reserve(rev_info.desc[i].physaddr,
				rev_info.desc[i].size);
		}
	}

	if (get_ambarella_bstmem_info(&bstadd, &bstsize) == AMB_BST_MAGIC) {
		pr_info("\t--:\t0x%08x[0x%08x]\tBST\n", bstadd, bstsize);
	}
}

/* ==========================================================================*/
int __init ambarella_init_machine(char *board_name)
{
	int					retval = 0;
	u32 					boot_from;

#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
	char					*pname;
	int					version;

	retval = amb_get_chip_name(HAL_BASE_VP, &pname);
	BUG_ON(retval != AMB_HAL_SUCCESS);
	retval = amb_get_version(HAL_BASE_VP, &version);
	BUG_ON(retval != AMB_HAL_SUCCESS);
#endif

	ambarella_board_generic.board_chip = AMBARELLA_BOARD_CHIP(system_rev);
	ambarella_board_generic.board_type = AMBARELLA_BOARD_TYPE(system_rev);
	ambarella_board_generic.board_rev = AMBARELLA_BOARD_REV(system_rev);

	pr_info("Ambarella %s:\n", board_name);
	pr_info("\tchip id:\t\t%d\n", ambarella_board_generic.board_chip);
	pr_info("\tboard type:\t\t%d\n", ambarella_board_generic.board_type);
	pr_info("\tboard revision:\t\t%d\n", ambarella_board_generic.board_rev);
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
	pr_info("\tchip name:\t\t%s\n", pname);
	pr_info("\tHAL version:\t\t%d\n", version);
	pr_info("\treference clock:\t%d\n",
		amb_get_reference_clock_frequency(HAL_BASE_VP));
	pr_info("\tsystem configuration:\t0x%08x\n",
		amb_get_system_configuration(HAL_BASE_VP));
	pr_info("\tboot type:\t\t0x%08x\n",
		amb_get_boot_type(HAL_BASE_VP));
	pr_info("\thif type:\t\t0x%08x\n",
		amb_get_hif_type(HAL_BASE_VP));
#endif

	retval = ambarella_create_proc_dir();
	BUG_ON(retval != 0);

	retval = ambarella_init_pll();
	BUG_ON(retval != 0);

	retval = ambarella_init_tm();
	BUG_ON(retval != 0);

	retval = ambarella_init_gpio();
	BUG_ON(retval != 0);

	retval = ambarella_init_fio();
	BUG_ON(retval != 0);

	retval = ambarella_init_dma();
	BUG_ON(retval != 0);

	retval = ambarella_init_adc();
	BUG_ON(retval != 0);

	retval = ambarella_init_pwm();
	BUG_ON(retval != 0);

	retval = ambarella_init_fb();
	BUG_ON(retval != 0);

	retval = ambarella_init_pm();
	BUG_ON(retval != 0);

	retval = ambarella_init_audio();
	BUG_ON(retval != 0);

	retval = ambarella_init_sd();
	BUG_ON(retval != 0);

	boot_from = rct_boot_from();

	if ((boot_from & BOOT_FROM_NOR) == BOOT_FROM_NOR) {
		platform_device_register(&ambarella_nor);
		device_set_wakeup_capable(&ambarella_nor.dev, 1);
		device_set_wakeup_enable(&ambarella_nor.dev, 0);
	} else {
		platform_device_register(&ambarella_nand);
		device_set_wakeup_capable(&ambarella_nand.dev, 1);
		device_set_wakeup_enable(&ambarella_nand.dev, 0);
	}

#if (ETH_INSTANCES >= 1)
	retval = ambarella_init_eth0(ambarella_board_generic.eth0_mac);
	BUG_ON(retval != 0);
#endif
#if (ETH_INSTANCES >= 2)
	retval = ambarella_init_eth1(ambarella_board_generic.eth1_mac);
	BUG_ON(retval != 0);
#endif

	return retval;
}
 
