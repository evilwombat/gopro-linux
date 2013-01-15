/*
 * include/linux/discharge_curve.h
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

#ifndef __LINUX_DISCHARGE_CURVE_H__
#define __LINUX_DISCHARGE_CURVE_H__

#define DISCHARGE_CURVE_SEGMENT_NUM_MAX 8

struct discharge_curve_entry {
	unsigned long uV;
	int capacity;
};

struct discharge_curve_table {
	const char *name;
	int num;
	/* The voltages need to be sorted descending */
	/* The 1st entry's capacity will always be 100 */
	/* The last entry's capacity will always be 0 */
	struct discharge_curve_entry curve[DISCHARGE_CURVE_SEGMENT_NUM_MAX];
};

extern int convert_uV_to_capacity(int uV, int * capacity);

#endif /* __LINUX_POWER_SUPPLY_H__ */
