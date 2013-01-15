/*
 * include/linux/aipc/ipc_mutex_def.h
 *
 * Ambarella IPC Mutex Definitions
 *
 * History:
 *    2011/04/12 - [Henry Lin] created file
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AIPC_MUTEX_DEF_H__
#define __AIPC_MUTEX_DEF_H__

enum {
	IPC_MUTEX_ID_FIO = 0,
	IPC_MUTEX_ID_SD2,
	IPC_MUTEX_ID_SD,

	/* IDC master instances */
	IPC_MUTEX_ID_IDC_MASTER_1,
	IPC_MUTEX_ID_IDC_MASTER_2,

	/* IDC slave instances */
	IPC_MUTEX_ID_IDC_SLAVE_1,

	/* SPI master instances on APB */
	IPC_MUTEX_ID_SPI_APB_MASTER_1,
	IPC_MUTEX_ID_SPI_APB_MASTER_2,
	IPC_MUTEX_ID_SPI_APB_MASTER_3,
	IPC_MUTEX_ID_SPI_APB_MASTER_4,

	/* SPI master instances on AHB */
	IPC_MUTEX_ID_SPI_AHB_MASTER_1,

	/* SPI slave instances on APB */
	IPC_MUTEX_ID_SPI_APB_SLAVE_1,

	IPC_MUTEX_ID_NUM
};

#endif

