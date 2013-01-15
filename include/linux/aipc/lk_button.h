/*
 * include/linux/aipc/lk_button.h
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

#ifndef __AIPC_LK_BUTTON_H__
#define __AIPC_LK_BUTTON_H__

#ifdef __KERNEL__

#ifdef CONFIG_INPUT_AMBA_VBUTTON
extern int amba_vbutton_key_pressed(int key_code);
extern int amba_vbutton_key_released(int key_code);
#else
static inline int amba_vbutton_key_pressed(int key_code)
{
	printk("%s: %d\n",__func__,key_code);
	return 0;
}

static inline int amba_vbutton_key_released(int key_code)
{
	printk("%s: %d\n",__func__,key_code);
	return 0;
}
#endif

#endif  /* __KERNEL__ */

#endif

