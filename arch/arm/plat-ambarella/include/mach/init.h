/*
 * arch/arm/plat-ambarella/include/mach/init.h
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

#ifndef __ASM_ARCH_INIT_H
#define __ASM_ARCH_INIT_H

/* ==========================================================================*/
#include <plat/adc.h>
#include <plat/atag.h>
#include <plat/audio.h>
#include <plat/crypto.h>
#include <plat/dma.h>
#include <plat/eth.h>
#include <plat/event.h>
#include <plat/fio.h>
#include <plat/idc.h>
#include <plat/ir.h>
#include <plat/irq.h>
#include <plat/nand.h>
#include <plat/clk.h>
#include <plat/pll.h>
#include <plat/pm.h>
#include <plat/pwm.h>
#include <plat/rct.h>
#include <plat/rtc.h>
#include <plat/sd.h>
#include <plat/spi.h>
#include <plat/timer.h>
#include <plat/uart.h>
#include <plat/udc.h>
#include <plat/uhc.h>
#include <plat/ahci.h>
#include <plat/wdt.h>
#include <plat/adb.h>

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/
extern void ambarella_memblock_reserve(void);
extern int ambarella_init_machine(char *board_name);
extern void ambarella_map_io(void);

/* ==========================================================================*/
extern struct platform_device ambarella_fb0;
extern struct platform_device ambarella_fb1;
extern int ambarella_init_fb(void);

/* ==========================================================================*/
extern struct ambarella_eth_platform_info ambarella_eth0_platform_info;
extern struct ambarella_eth_platform_info ambarella_eth1_platform_info;

extern struct ambarella_idc_platform_info ambarella_idc0_platform_info;
extern struct ambarella_idc_platform_info ambarella_idc1_platform_info;

extern struct ambarella_rtc_controller ambarella_platform_rtc_controller0;

extern struct ambarella_sd_controller ambarella_platform_sd_controller0;
extern struct ambarella_sd_controller ambarella_platform_sd_controller1;

extern int ambarella_spi0_cs_pins[];
extern int ambarella_spi1_cs_pins[];
extern int ambarella_spi2_cs_pins[];
extern int ambarella_spi3_cs_pins[];
extern int ambarella_spi4_cs_pins[];

extern int fio_default_owner;

extern struct ambarella_ir_controller ambarella_platform_ir_controller0;

extern struct ambarella_adc_controller ambarella_platform_adc_controller0;

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

