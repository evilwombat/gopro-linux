/*
 * include/linux/aipc/lk_touch.h
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

#ifndef __AIPC_LK_TOUCH_H__
#define __AIPC_LK_TOUCH_H__

#ifdef __KERNEL__

struct amba_vtouch_finger {
	int x;
	int y;
	int z;
	int press;
};

#define MAX_VTOUCH_FINGER	10
struct amba_vtouch_data
{
	struct amba_vtouch_finger finger[MAX_VTOUCH_FINGER];
	int curr_;
	int prev_;
	int max_finger;
};

#ifdef CONFIG_TOUCHSCREEN_AMBA_VTOUCH
extern int amba_vtouch_press_abs_mt_sync(struct amba_vtouch_data *data);
extern int amba_vtouch_release_abs_mt_sync(struct amba_vtouch_data *data);
#else
int amba_vtouch_press_abs_mt_sync(struct amba_vtouch_data *data)
{
	printk("%s\n",__func__);
	return 0;
}

int amba_vtouch_release_abs_mt_sync(struct amba_vtouch_data *data)
{
	printk("%s\n",__func__);
	return 0;
}
#endif

#endif  /* __KERNEL__ */

#endif

