/*
 * include/linux/aipc/i_example_util.h
 *
 * Authors:
 *	Henry Lin <hllin@ambarella.com>
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

#ifndef __AIPC_I_EXAMPLE_UTIL_H__
#define __AIPC_I_EXAMPLE_UTIL_H__

#ifdef __KERNEL__

extern int ic_example_gettimeofday(struct timeval *timeval);
extern int ic_example_printk(const char *s);

#endif

#endif
