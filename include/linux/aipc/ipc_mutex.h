/*
 * include/linux/aipc/ipc_mutex.h
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

#ifndef __AIPC_MUTEX_H__
#define __AIPC_MUTEX_H__

#include <linux/completion.h>
#include <linux/aipc/ipc_mutex_def.h>
#include <linux/aipc/ipc_slock.h>

/*
 * Settings
 */
#define IPC_MUTEX_MEM_SIZE		4096

/*
 * Debug
 */
#define DEBUG_LOG_MUTEX			0
#define DEBUG_LOG_MUTEX_NUM		8
#define DEBUG_LOG_LOCK_TIME		1
#define DEBUG_LOG_LOCK_TIME_NUM		5

/*
 * Definitions
 */
#define IPC_MUTEX_TIMEOUT_INFINITE	-1
#define IPC_MUTEX_DEFAULT_POLICY	IPC_MUTEX_POLICY_ROUND_ROBIN
#define IPC_MUTEX_DEFAULT_TIMEOUT	IPC_MUTEX_TIMEOUT_INFINITE

enum {
	IPC_MUTEX_OS_UITRON = 0,
	IPC_MUTEX_OS_LINUX,
	IPC_MUTEX_OS_NUM
};

enum {
	IPC_MUTEX_TOKEN_UITRON = 0,
	IPC_MUTEX_TOKEN_LINUX,
	IPC_MUTEX_TOKEN_NONE
};

enum {
	IPC_MUTEX_POLICY_FIFO = 0,
	IPC_MUTEX_POLICY_ROUND_ROBIN,
	IPC_MUTEX_POLICY_WAIT_COUNT,
	IPC_MUTEX_POLICY_NUM
};

enum {
	IPC_MUTEX_ERR_OK = 0,
	IPC_MUTEX_ERR_FAILED,
	IPC_MUTEX_ERR_TIMEOUT
};

typedef struct ipc_mutex_os_s {
	int count;			// waiting count
	int priority;			// waiting priority
} ipc_mutex_os_t;

typedef struct ipc_mutex_lock_time_s {
	int lock_id;
	int lock_os;
	int lock_wait;
	int lock_time;
} ipc_mutex_lock_time_t;

typedef struct ipc_mutex_s {

	int id;
	int lock;

	int token;
	int next_id;

	int policy;
	int timeout;			// timeout in ms, -1 means infinite

	int lock_count;
	int unlock_count;

	int arm_lock_count;
	int arm_unlock_count;
	int cortex_lock_count;
	int cortex_unlock_count;

	int arm_owner;
	int cortex_owner;

#if DEBUG_LOG_LOCK_TIME
	int max_lock_wait_time;
	int max_lock_wait_os;
	int max_lock_wait_id;
	int uitron_lock_wait_time;
	int linux_lock_wait_time;

	int max_lock_time;
	int max_lock_id;

	int lock_time;
	ipc_mutex_lock_time_t lock_times[DEBUG_LOG_LOCK_TIME_NUM];
	int lock_time_idx;
#endif

#if DEBUG_LOG_MUTEX
	int lock_log_idx;
	int unlock_log_idx;

	int lock_task_id[DEBUG_LOG_MUTEX_NUM];
	int unlock_task_id[DEBUG_LOG_MUTEX_NUM];

	int lock_next_id[DEBUG_LOG_MUTEX_NUM];		/* next_id when lock is called */
	int lock_prev_token[DEBUG_LOG_MUTEX_NUM];	/* token when lock is called */
#endif /* DEBUG_LOG_MUTEX */

	ipc_mutex_os_t os[IPC_MUTEX_OS_NUM];

} ipc_mutex_t;

int ipc_mutex_init_map(u32 addr, u32 size);
int ipc_mutex_init(void);
int ipc_mutex_lock(int id);
int ipc_mutex_unlock(int id);

#endif

