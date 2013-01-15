/*
 * drivers/video/ambarella/ambarella_cc.c
 *
 * Authors:
 *	Charles Chiou <cchiou@ambarella.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * Copyright (C) 2009-2010, Ambarella Inc.
 */

#include <linux/module.h>
#include <linux/kernel.h>

static u16 rgb565_to_yuv565_tab[65536];

static void setup_rgb565_to_yuv565_tab(void)
{
	static int rgb565_to_yuv565_tab_init = 0;
	unsigned int i;
	u8 r, g, b, y, u, v;
	u16 *tab = rgb565_to_yuv565_tab;

	if (rgb565_to_yuv565_tab_init)
		return;
	else
		rgb565_to_yuv565_tab_init = 1;

	for (i = 0; i < 65536; i++) {
		r = (i & 0xf800) >> 8;
		g = (i & 0x07e0) >> 3;
		b = (i & 0x001f) << 3;

		y = (( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
		u = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
		v = ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;

		y >>= 2;
		u >>= 3;
		v >>= 3;

		tab[i] = ((u << 11) | (y << 5) | (v));
	}
}

void cc_rgb565_yuv565(u32 *src, u32 *dst, int pitch, int xres, int yres)
{
	int i, size;
	u16 *tab;
	u32 rgb32, uyv32;

	int j;
	u16 *src_p, *dst_p;
		

	setup_rgb565_to_yuv565_tab();

	tab = rgb565_to_yuv565_tab;

	size = (pitch * yres) >> 4;

	for (i = 0; i < size; i++) {
		rgb32 = *src++;
		uyv32 = tab[rgb32 >> 16] << 16;
		uyv32 |= tab[(rgb32 << 16) >> 16];
		*dst++ = uyv32;

		rgb32 = *src++;
		uyv32 = tab[rgb32 >> 16] << 16;
		uyv32 |= tab[(rgb32 << 16) >> 16];
		*dst++ = uyv32;

		rgb32 = *src++;
		uyv32 = tab[rgb32 >> 16] << 16;
		uyv32 |= tab[(rgb32 << 16) >> 16];
		*dst++ = uyv32;

		rgb32 = *src++;
		uyv32 = tab[rgb32 >> 16] << 16;
		uyv32 |= tab[(rgb32 << 16) >> 16];
		*dst++ = uyv32;
	}
}
EXPORT_SYMBOL(cc_rgb565_yuv565);
