/*
 * include/linux/aipc/i_flpart.h
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

#ifndef __AIPC_I_FLPART_H__
#define __AIPC_I_FLPART_H__

#ifdef __KERNEL__

struct ipc_flpart
{
	/* partition info */
	unsigned int crc32;
	unsigned int ver_num;
	unsigned int ver_date;
	unsigned int img_len;
	unsigned int mem_addr;
	unsigned int flag;
	unsigned int magic;
	/* meta data */
	unsigned int sblk;
	unsigned int nblk;
	char name[32];
};

extern int ipc_flpart_num_parts(void);
extern int ipc_flpart_get(int index, struct ipc_flpart *part);

#endif  /* __KERNEL__ */

#endif
