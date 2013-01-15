/*
 * include/linux/aipc/ipc_slock_def.h
 *
 * Ambarella IPC Spinlock Definitions
 *
 * History:
 *    2011/08/01 - [Henry Lin] created file
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AIPC_SLOCK_DEF_H__
#define __AIPC_SLOCK_DEF_H__

enum {
	IPC_SLOCK_ID_HAL = 0,
	IPC_SLOCK_ID_BOSS,
	IPC_SLOCK_ID_IPC,
	IPC_SLOCK_ID_IPC_CLNT_OUT,
	IPC_SLOCK_ID_IPC_CLNT_IN,
	IPC_SLOCK_ID_IPC_SVC_IN,
	IPC_SLOCK_ID_IPC_SVC_OUT,
	IPC_SLOCK_ID_MUTEX_WAKEUP_UITRON,
	IPC_SLOCK_ID_MUTEX_WAKEUP_LINUX,
	IPC_SLOCK_ID_NUM
};

enum {
	IPC_SLOCK_POS_WAKEUP_ADD = 0,
	IPC_SLOCK_POS_WAKEUP_REMOVE,
	IPC_SLOCK_POS_WAKEUP_LOCAL,
	IPC_SLOCK_POS_CONFIG,
	IPC_SLOCK_POS_LOCK,
	IPC_SLOCK_POS_UNLOCK,
	IPC_SLOCK_POS_LOCK_COUNT,
	IPC_SLOCK_POS_HAL,
	IPC_SLOCK_POS_BOSS_ENABLE_IRQ,
	IPC_SLOCK_POS_BOSS_DISABLE_IRQ,
	IPC_SLOCK_POS_BOSS_GET_IRQ_OWNER,
	IPC_SLOCK_POS_LOG_PRINT,
	IPC_SLOCK_POS_TEST,

	IPC_SLOCK_POS_I_CLNT_OUT = 32,
	IPC_SLOCK_POS_I_CLNT_IN,
	IPC_SLOCK_POS_I_SVC_IN,
	IPC_SLOCK_POS_I_SVC_OUT,
	IPC_SLOCK_POS_I_CLNT_CANCEL,

	IPC_SLOCK_POS_L_CLNT_OUT = 64,
	IPC_SLOCK_POS_L_CLNT_IN,
	IPC_SLOCK_POS_L_SVC_IN,
	IPC_SLOCK_POS_L_SVC_OUT,
	IPC_SLOCK_POS_LU_CLNT_OUT,
	IPC_SLOCK_POS_L_CLNT_CANCEL,
	IPC_SLOCK_POS_L_STATISTIC,
};

#endif

