/*
 * arch/arm/plat-ambarella/generic/idc.c
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
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>
#include <linux/ambarella-i2cmux.h>

#include <mach/hardware.h>
#include <plat/idc.h>
#if defined(CONFIG_AMBARELLA_IPC)
#include <linux/aipc/ipc_mutex.h>
#endif
/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

/* ==========================================================================*/
#define DEFAULT_I2C_CLASS	(I2C_CLASS_HWMON | I2C_CLASS_SPD)

struct resource ambarella_idc0_resources[] = {
	[0] = {
		.start	= IDC_BASE,
		.end	= IDC_BASE + 0x0FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IDC_IRQ,
		.end	= IDC_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

struct ambarella_idc_platform_info ambarella_idc0_platform_info = {
	.clk_limit	= 100000,
	.bulk_write_num	= 60,
#if (IDC_SUPPORT_PIN_MUXING_FOR_HDMI == 1)
	.i2c_class	= DEFAULT_I2C_CLASS | I2C_CLASS_DDC,
#elif (IDC_SUPPORT_INTERNAL_MUX == 1)
	.i2c_class	= DEFAULT_I2C_CLASS,
#else
	.i2c_class	= DEFAULT_I2C_CLASS,
#endif
#if defined(CONFIG_AMBARELLA_IPC)
	.ipc_mutex_id = IPC_MUTEX_ID_IDC_MASTER_1,
#endif
	.get_clock	= get_apb_bus_freq_hz,
};
AMBA_IDC_PARAM_CALL(0, ambarella_idc0_platform_info, 0644);

struct platform_device ambarella_idc0 = {
	.name		= "ambarella-i2c",
	.id		= 0,
	.resource	= ambarella_idc0_resources,
	.num_resources	= ARRAY_SIZE(ambarella_idc0_resources),
	.dev		= {
		.platform_data		= &ambarella_idc0_platform_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

static struct ambarella_i2cmux_platform_data ambarella_i2cmux_info = {
	.parent		= 0,
	.number		= 2,
#if (IDC_SUPPORT_PIN_MUXING_FOR_HDMI == 1)
	.gpio		= IDC_BUS_HDMI,
#elif (IDC_SUPPORT_INTERNAL_MUX == 1)
	.gpio		= IDC3_BUS_MUX,
#endif
	.select_function	= GPIO_FUNC_HW,
	.deselect_function	= GPIO_FUNC_SW_INPUT,
};

struct platform_device ambarella_i2cmux = {
	.name		= "ambarella-i2cmux",
	.id		= 0,
	.dev		= {
		.platform_data		= &ambarella_i2cmux_info,
	}
};

#if (IDC_INSTANCES >= 2)
struct resource ambarella_idc1_resources[] = {
	[0] = {
		.start	= IDC2_BASE,
		.end	= IDC2_BASE + 0x0FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IDC2_IRQ,
		.end	= IDC2_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

struct ambarella_idc_platform_info ambarella_idc1_platform_info = {
	.clk_limit	= 100000,
	.bulk_write_num	= 60,
	.i2c_class	= I2C_CLASS_DDC,
#if defined(CONFIG_AMBARELLA_IPC)
	.ipc_mutex_id = IPC_MUTEX_ID_IDC_MASTER_2,
#endif
	.get_clock	= get_apb_bus_freq_hz,
};
AMBA_IDC_PARAM_CALL(1, ambarella_idc1_platform_info, 0644);

struct platform_device ambarella_idc1 = {
	.name		= "ambarella-i2c",
	.id		= 1,
	.resource	= ambarella_idc1_resources,
	.num_resources	= ARRAY_SIZE(ambarella_idc1_resources),
	.dev		= {
		.platform_data		= &ambarella_idc1_platform_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif

