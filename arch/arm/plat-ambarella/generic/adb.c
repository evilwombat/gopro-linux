/*
 * arch/arm/plat-ambarella/generic/adb.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
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
#include <linux/delay.h>
#include <mach/hardware.h>
#include <linux/usb/android_composite.h>
#include <plat/adb.h>

#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
static char *usb_functions[] = { "usb_mass_storage" };
static char *usb_functions_adb[] = { "usb_mass_storage", "adb" };
#else
static char *usb_functions_adb[] = { "adb" };
#endif

static struct android_usb_product usb_products[] = {
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
	{
		.product_id     = AMB_FSG_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions),
		.functions      = usb_functions,
	},
#endif
	{
		.product_id	= AMB_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_adb),
		.functions	= usb_functions_adb,
	},
};

static struct android_usb_platform_data ambarella_usb_pdata = {
	.vendor_id		= AMB_ADB_VENDOR_ID,
	.product_id		= AMB_ADB_PRODUCT_ID,
	.version		= 0x0001,
	.serial_number		= "0123456789ABCDEF",
	.product_name		= "iONE",
	.manufacturer_name	= "Ambarella",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_adb),
	.functions = usb_functions_adb,
};

struct platform_device ambarella_usb_device0 = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &ambarella_usb_pdata,
	},
};


static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "Ambarella",
	.product	= "iONE",
	.release	= 0x0001,
};

struct platform_device ambarella_fsg_device0 = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &mass_storage_pdata,
	},
};

