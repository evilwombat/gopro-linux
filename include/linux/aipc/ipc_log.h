/*
 * include/linux/aipc/ipc_log.h
 *
 * Ambarella IPC Log
 *
 * History:
 *    2011/05/18 - [Henry Lin] created file
 *
 * Copyright (C) 2011-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AIPC_LOG_H__
#define __AIPC_LOG_H__

#define IPC_LOG_ENABLE			0

enum {
	IPC_LOG_LEVEL_ERROR = 0,
	IPC_LOG_LEVEL_WARNING,
	IPC_LOG_LEVEL_INFO,
	IPC_LOG_LEVEL_DEBUG,
	IPC_LOG_LEVEL_NUM
};

/*
 * Settings
 */
#define IPC_LOG_MEM_SIZE_BIT		17
#define IPC_LOG_MEM_SIZE		(1 << IPC_LOG_MEM_SIZE_BIT)
#define IPC_LOG_MEM_SIZE_MASK		(IPC_LOG_MEM_SIZE - 1)
 #define IPC_LOG_LEVEL_OUTPUT		0

/* IPC Log API */
int ipc_log_init(u32 addr, u32 size);
void ipc_log_print(int level, const char *fmt, ...);
void ipc_log_set_level(int level);
void ipc_log_set_output(int output);

#endif

