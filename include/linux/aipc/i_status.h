/*
 * include/linux/aipc/i_status.h
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
 * Copyright (C) 2009-2010, Ambarella Inc.
 */

#ifndef __AIPC_I_STATUS_H__
#define __AIPC_I_STATUS_H__

#ifdef __KERNEL__

#include <linux/time.h>

extern int i_status_init(void);
extern void i_status_cleanup(void);

extern int ipc_status_report_ready(int status);
extern int ipc_boss_ver_report(int ver);
extern int ipc_boss_time_event(int id, char *tag, struct timeval *time);

#endif

#endif
