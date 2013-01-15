/*
 * include/linux/aipc/ipc_shm.h
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
 * Copyright (C) 2009-2011, Ambarella Inc.
 */

#ifndef __AIPC_SHM_H__
#define __AIPC_SHM_H__

#include <linux/module.h>

#define ipc_inc(a)	((*(volatile unsigned int *)(a))++)

/* SHM total size */
#define IPC_SHM_MEM_SIZE		4096

/* SHM areas */
#define IPC_SHM_AREA_GLOBAL		0
#define IPC_SHM_AREA_ARM		1
#define IPC_SHM_AREA_CORTEX0		2
#define IPC_SHM_AREA_CORTEX1		3
#define IPC_SHM_AREA_NUM		4

/* SHM area: Global */
#define IPC_SHM_GLOBAL_COUNT		0
#define IPC_SHM_GLOBAL_STOP		1
#define IPC_SHM_GLOBAL_NUM		2

/* SHM area: ARM */
#define IPC_SHM_ARM_COUNT		0
#define IPC_SHM_ARM_FAIL		1
#define IPC_SHM_ARM_NUM			2

/* SHM area: Cortex0 and Cortex1 */
#define IPC_SHM_CORTEX_COUNT		0
#define IPC_SHM_CORTEX_NUM		1

void		ipc_shm_init (unsigned int addr, unsigned int size);

unsigned int	*ipc_shm_get(int aid, int i);
void		ipc_shm_write(int aid, int i, unsigned int val);
unsigned int	ipc_shm_read(int aid, int i);
void		ipc_shm_inc(int aid, int i);

#endif

