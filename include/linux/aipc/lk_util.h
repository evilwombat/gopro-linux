/*
 * include/linux/aipc/lk_util.h
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

#ifndef __AIPC_LK_UTIL_H__
#define __AIPC_LK_UTIL_H__

#ifdef __KERNEL__

struct fbreq_handler
{
	int (*takeover_req)(int id);
	int (*release_req)(int id);
};

extern struct fbreq_handler fbreq_handler;

extern int ambarella_udc_connect(int on);

#endif  /* __KERNEL__ */

#endif
