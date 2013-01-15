/*
 * ambhw/i2s.h
 *
 * History:
 *	2010/11/11 - [Daniel Tsai] created file
 *
 * Copyright (C) 2009-2011, Ambarella, Inc.
 */
 
#ifndef __AMBHW__SPDIF_H__
#define __AMBHW__SPDIF_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/
#define SPDIF_MODE_REG			SPDIF_REG(0x00)
#define SPDIF_CONTROL_REG		SPDIF_REG(0x04)
#define SPDIF_ENABLE_REG		SPDIF_REG(0x08)
#define SPDIF_STATUS_REG		SPDIF_REG(0x0c)
#define SPDIF_FIFO_CNT_REG	SPDIF_REG(0x10)
#define SPDIF_FIFO_TH_REG		SPDIF_REG(0x14)
#define SPDIF_DATA_A_REG		SPDIF_REG(0x18)
#define SPDIF_DATA_B_REG		SPDIF_REG(0x1c)

#define SPDIF_CS_DATA_A_REG		 SPDIF_REG(0x20)
#define SPDIF_CS_DATA_B_REG	   SPDIF_REG(0x38)
#define SPDIF_USER_DATA_A_REG	 SPDIF_REG(0x50)
#define SPDIF_USER_DATA_B_REG	 SPDIF_REG(0x68)
#define SPDIF_DMA_DI_REG		   SPDIF_REG(0x80)

#define SPDIF_PRO_MINI				(1 << 16)
#define SPDIF_DATA_SOURCE			(1 << 15)
#define SPDIF_CS_USER_SOURCE	(1 << 14)
#define SPDIF_WLEN						(1 << 9)
#define SPDIF_MCLK_DIV				(1 << 4)
#define SPDIF_ALIGNMENT				(1 << 2)
#define SPDIF_LINEAR_PCM			(1 << 1)
#define SPDIF_PRO_CNT					(1 << 0)

#define SPDIF_FIFO_RD_STEP		(1 << 11)
#define SPDIF_RESET						(1 << 10)
#define SPDIF_CS_USER_UPD	    (1 << 9)
#define SPDIF_VAL_SET					(1 << 8)
#define SPDIF_DUMMY						(1 << 6)
#define SPDIF_IRQ_CLEAR				(1 << 5)
#define SPDIF_FIFO_CLEAR			(1 << 4)
#define SPDIF_IRQ_FIFO_EMPTY	(1 << 3)
#define SPDIF_IRQ_FIFO_FULL		(1 << 2)
#define SPDIF_IRQ_FIFO_TH			(1 << 1)
#define SPDIF_IRQ_ENABLE			(1 << 0)

#define SPDIF_IDLE						(1 << 5)
#define SPDIF_FIFO_TH_VLD			(1 << 4)
#define SPDIF_FIFO_A_FULL			(1 << 3)
#define SPDIF_FIFO_B_FULL			(1 << 2)
#define SPDIF_FIFO_A_EMPTY		(1 << 1)
#define SPDIF_FIFO_B_EMPTY		(1 << 0)
#define SPDIF_FIFO_FULL			  (SPDIF_FIFO_A_FULL | SPDIF_FIFO_B_FULL)
#define SPDIF_FIFO_EMPTY			(SPDIF_FIFO_A_EMPTY | SPDIF_FIFO_B_EMPTY)

#define SPDIF_FIFO_B_CNT_MASK		0x01FF0000
#define SPDIF_FIFO_A_CNT_MASK		0x000001FF

#endif
