/*
 * include/linux/aipc/i_rtc.h
 *
 * Authors:
 *	Josh Wang <chwang@ambarella.com>
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
 * Copyright (C) 2011-2012, Ambarella Inc.
 */

#ifndef __AIPC_I_GPIO_H__
#define __AIPC_I_GPIO_H__

#ifdef __KERNEL__

extern int ipc_rtc_set_time(u32 *tm);
extern int ipc_rtc_set_alarm(u32 *tm);
extern int ipc_rtc_set_pos(void);

#endif  /* __KERNEL__ */

#endif

