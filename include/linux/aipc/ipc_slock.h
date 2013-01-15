/*
 * include/linux/aipc/ipc_slock.h
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

#ifndef __AIPC_SPINLOCK_H__
#define __AIPC_SPINLOCK_H__

#include <linux/module.h>
#include <linux/aipc/ipc_slock_def.h>

#define DEBUG_SPINLOCK			0
#define DEBUG_LOCK_TIME			0
#define DEBUG_LOCK_MAX_MS		2
#define DEBUG_LOG_SPINLOCK		0
#define DEBUG_LOG_SPINLOCK_NUM		8

#if __LINUX_ARM_ARCH__ < 6
#error SMP not supported on pre-ARMv6 CPUs
#endif

#define IPC_SPINLOCK_MEM_SIZE		4096

#define IPC_SPINLOCK_CPU_ARM		1
#define IPC_SPINLOCK_CPU_CORTEX		2

typedef struct {
	volatile unsigned int lock;
	volatile unsigned int flags;
	volatile unsigned int count;
	volatile unsigned int arm;		/* the count of ARM's lock */
	volatile unsigned int cortex;		/* the count of Cortex's lock */
#if DEBUG_LOCK_TIME
	volatile unsigned int bound;
	volatile unsigned int arm_lock_time;
	volatile unsigned int arm_long_wait;
	volatile unsigned int arm_long_lock;
	volatile unsigned int cortex_lock_time;
	volatile unsigned int cortex_long_wait;
	volatile unsigned int cortex_long_lock;
#endif
#if DEBUG_SPINLOCK
	volatile unsigned int lock_count;	/* increment after get mutex lock */
	volatile unsigned int unlock_count;	/* increment before release mutex lock */
	volatile unsigned int cpu;		/* last cpu got the mutex lock: ARM or Cortex */
#if DEBUG_LOG_SPINLOCK
	volatile int lock_log_idx;
	volatile int unlock_log_idx;
	volatile int lock_pos[DEBUG_LOG_SPINLOCK_NUM];		/* who called lock */
	volatile int unlock_pos[DEBUG_LOG_SPINLOCK_NUM];	/* who called unlock */
	volatile int lock_task_id[DEBUG_LOG_SPINLOCK_NUM];	/* which task called lock */
#endif /* DEBUG_LOG_SPINLOCK */
#endif /* DEBUG_LOG_SPINLOCK */
} ipc_slock_t;

void	ipc_slock_init(unsigned int addr, unsigned int size);

void	ipc_spin_lock(int i, int pos);
void	ipc_spin_unlock(int i, int pos);

int	ipc_spin_trylock(int i);

void	ipc_spin_test(void);

#endif	/* __AIPC_SPINLOCK_H__ */

