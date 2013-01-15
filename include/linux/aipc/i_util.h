/*
 * include/linux/aipc/i_util.h
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

#ifndef __AIPC_I_UTIL_H__
#define __AIPC_I_UTIL_H__

#ifdef __KERNEL__

extern int ipc_gettimeofday(struct timeval *timeval);
extern int ipc_printk(const char *s);
extern void ipc_async_ipc(int svc_dly, int clnt_dly, int clnt_tmo, int clnt_retry);

extern int ipc_pm_suspend(void);
extern int ipc_pm_resume(void);

extern int ipc_report_ready(void);
extern int ipc_report_resume(void);
extern int ipc_request_suspend_me(void);
extern int ipc_request_shutdown_me(void);

extern int ipc_get_exfb(void **mem, unsigned int *size);
extern int ipc_report_fb_owned(void);
extern int ipc_report_fb_released(void);

extern int linux_absuspend_check(void *pinfo);
extern int linux_absuspend_enter(void);
extern int linux_absuspend_exit(void);

extern int ipc_set_device_owner(int device, int owner);

#endif

#endif
