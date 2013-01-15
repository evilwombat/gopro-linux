/*
 * driver/power/discharge_curve.c
 *
 * Author: Xiagen Feng <xgfeng@ambarella.com>
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
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
 */

#include <linux/discharge_curve.h>
#include <linux/errno.h>

static struct discharge_curve_table dc_table = {
#ifdef CONFIG_DC_SANYO_18650
	.name = "sanyo 18650",
	.num = 3,
	.curve = {
		[0] = {4170000, 100},
		[1] = {3630000, 20},
		[2] = {3350000, 0}
	},
#elif defined (CONFIG_DC_TRUST_FIRE_18650)
	.name = "trust fire 18650",
	.num = 4,
	.curve = {
		[0] = {4120000, 100},
		[1] = {3800000, 80},
		[2] = {3400000,20},
		[3] = {3100000,0},
	},
#else
	.name = "generic lithium based",
	.num = 3,
	.curve = {
		[0] = {4200000, 100},
		[1] = {3600000, 20},
		[2] = {3400000, 0}
	},
#endif
};

int convert_uV_to_capacity(int uV, int * capacity) {
	int i, ret = 0;
	unsigned long capacity_diff, uV_diff;
	if ((dc_table.num < 3) ||
		(dc_table.num > DISCHARGE_CURVE_SEGMENT_NUM_MAX) ||
		(dc_table.curve[0].capacity != 100) ||
		(dc_table.curve[dc_table.num - 1].capacity != 0)) {
		/* Use fixed genericlion */
		if (uV >= 0) {
			if (uV > 4200000) {
				*capacity = 100;
			} else if ((uV <= 4200000) && (uV > 3600000)) {
				*capacity = 20 + 80*(uV - 3600000)/600000;
			} else if ((uV <= 3600000) && (uV > 3400000)) {
				*capacity = 20*(uV - 3400000)/200000;
			} else {
				*capacity = 0;
			}
		} else {
			ret = -EINVAL;
		}
	} else {
		/* Use dc_table to calculate capacity */
		if (uV >= 0) {
			if (uV > dc_table.curve[0].uV) {
				*capacity = 100;
				goto out;
			};
			for (i = 1 ; i < dc_table.num  ; i++) {
				if ((uV > dc_table.curve[i].uV) &&
					(uV <= dc_table.curve[i - 1].uV)) {
					capacity_diff = dc_table.curve[i - 1].capacity - dc_table.curve[i].capacity;
					uV_diff = dc_table.curve[i - 1].uV - dc_table.curve[i].uV;
					*capacity = dc_table.curve[i].capacity +
						(uV - dc_table.curve[i].uV)*capacity_diff/uV_diff;
					goto out;
				}
			}
			*capacity = 0;
		} else {
			ret = -EINVAL;
		}
	}
out:
	return ret;
}
