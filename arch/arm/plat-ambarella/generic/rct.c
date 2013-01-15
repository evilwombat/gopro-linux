/*
 * arch/arm/plat-ambarella/generic/rct.c
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *	2008/01/08 - [Anthony Ginger] Port to 2.6.28
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

#include <linux/device.h>
#include <linux/delay.h>

#include <asm/io.h>

#include <mach/hardware.h>
#include <plat/audio.h>
#include <hal/hal.h>

#undef readb
#undef readw
#undef readl
#undef writeb
#undef writew
#undef writel
#define readb(v)		amba_readb(v)
#define readw(v)		amba_readw(v)
#define readl(v)		amba_readl(v)
#define writeb(v,d)		amba_writeb(v,d)
#define writew(v,d)		amba_writew(v,d)
#define writel(v,d)		amba_writel(v,d)
#define dly_tsk(x)		mdelay(10 * (x))

#define K_ASSERT(x)		BUG_ON(!(x))

#if (CHIP_REV == A2)
#define A2_METAL_REV_A1
#include "rct/a2.c"
#elif (CHIP_REV == A2M) || (CHIP_REV == A2S)
#include "rct/a2s.c"
#elif (CHIP_REV == A3)
#include "rct/a3.c"
#elif (CHIP_REV == A5)
#include "rct/a5.c"
#elif (CHIP_REV == A5S)
#include "rct/a5s.c"
#elif (CHIP_REV == A7)
#include "rct/a7.c"
#elif (CHIP_REV == A7L)
#include "rct/a7l.c"
#elif (CHIP_REV == I1)
#include "rct/i1.c"
#endif
#include "rct/audio.c"

EXPORT_SYMBOL(get_stepping_info);

EXPORT_SYMBOL(get_apb_bus_freq_hz);
EXPORT_SYMBOL(get_ahb_bus_freq_hz);
EXPORT_SYMBOL(get_core_bus_freq_hz);
#if (CHIP_REV == A5S)
EXPORT_SYMBOL(get_arm_bus_freq_hz);
#endif
EXPORT_SYMBOL(get_dram_freq_hz);
EXPORT_SYMBOL(get_idsp_freq_hz);

EXPORT_SYMBOL(get_so_freq_hz);
EXPORT_SYMBOL(rct_set_so_clk_src);
EXPORT_SYMBOL(rct_set_so_freq_hz);
EXPORT_SYMBOL(rct_rescale_so_pclk_freq_hz);
EXPORT_SYMBOL(rct_set_so_pclk_freq_hz);
EXPORT_SYMBOL(rct_set_vin_lvds_pad);

EXPORT_SYMBOL(get_vout_freq_hz);
EXPORT_SYMBOL(get_vout_clk_rescale_value);
EXPORT_SYMBOL(rct_set_vout_freq_hz);
EXPORT_SYMBOL(rct_set_vout_clk_src);
EXPORT_SYMBOL(rct_rescale_vout_clk_freq_hz);
EXPORT_SYMBOL(get_vout2_freq_hz);
EXPORT_SYMBOL(get_vout2_clk_rescale_value);
EXPORT_SYMBOL(rct_set_vout2_freq_hz);
EXPORT_SYMBOL(rct_set_vout2_clk_src);
EXPORT_SYMBOL(rct_rescale_vout2_clk_freq_hz);

#if (VOUT_SUPPORT_ONCHIP_HDMI != 0)
EXPORT_SYMBOL(rct_set_hdmi_clk_src);
#endif

EXPORT_SYMBOL(ambarella_audio_notify_transition);
EXPORT_SYMBOL(ambarella_audio_register_notifier);
EXPORT_SYMBOL(ambarella_audio_unregister_notifier);

