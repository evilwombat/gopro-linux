/*
 * include/linux/aipc/i_touch.h
 *
 * Authors:
 *	Keny Huang <skhuang@ambarella.com>
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
 * Copyright (C) 2009-2011, Ambarella Inc.
 */

#ifndef __AIPC_I_TOUCH_H__
#define __AIPC_I_TOUCH_H__

#ifdef __KERNEL__

typedef struct vtouch_abs_info_s
{
	unsigned int min;
	unsigned int max;
} vtouch_abs_info_t;

struct vtouch_module_info
{
	char dev_name[32];
	vtouch_abs_info_t x1;
	vtouch_abs_info_t y1;
	vtouch_abs_info_t x2;
	vtouch_abs_info_t y2;
	vtouch_abs_info_t pressure;
	vtouch_abs_info_t mt_major;
};

extern int ipc_itouch_is_module_valid(void);
extern int ipc_itouch_get_module_info(struct vtouch_module_info *info);

#endif  /* __KERNEL__ */

#endif

