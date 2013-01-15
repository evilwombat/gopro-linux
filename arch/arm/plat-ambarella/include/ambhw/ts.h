/*
 * include/ambhw/ts.h
 *
 * History:
 *	2008/08/06 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2006-2008, Ambarella, Inc.
 */

#ifndef __AMBHW__TS_H__
#define __AMBHW__TS_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/
#if 	(CHIP_REV == A6)
#define TS_ESC_DEAD_LOCK	1
#else
#define TS_ESC_DEAD_LOCK	0
#endif

#define TS_TX_MAX_CHANS		2
#define TS_RX_MAX_CHANS		2
#define TS_MAX_CHANS		(TS_TX_MAX_CHANS + TS_RX_MAX_CHANS)

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/
/* Offset */
#define TS_CH0_TX_CTR_OFFSET			0x100
#define TS_CH0_TX_STS_OFFSET			0x104
#define TS_CH0_TX_PKT_INFO_OFFSET		0x108
#define TS_CH0_TX_DMA_BASE_OFFSET		0x10c
#define TS_CH0_TX_DMA_LIMIT_OFFSET		0x110
#define TS_CH0_TX_DMA_RPTR_OFFSET		0x114
#define TS_CH0_TX_DMA_STOP_OFFSET		0x118

#define TS_CH0_TX_DMA_CTRL_OFFSET		0x124
#define TS_CH0_TX_DMA_NOTIFY_OFFSET		0x128

#define TS_CH1_TX_CTR_OFFSET			0x180
#define TS_CH1_TX_STS_OFFSET			0x184
#define TS_CH1_TX_PKT_INFO_OFFSET		0x188
#define TS_CH1_TX_DMA_BASE_OFFSET		0x18c
#define TS_CH1_TX_DMA_LIMIT_OFFSET		0x190
#define TS_CH1_TX_DMA_RPTR_OFFSET		0x194
#define TS_CH1_TX_DMA_STOP_OFFSET		0x198

#define TS_CH1_TX_DMA_CTRL_OFFSET		0x1a4
#define TS_CH1_TX_DMA_NOTIFY_OFFSET		0x1a8

#define TS_CH0_RX_CTR_OFFSET			0x200
#define TS_CH0_RX_STS_OFFSET			0x204
#define TS_CH0_RX_TM_OFFSET			0x208
#define TS_CH0_RX_TEST_CTR_OFFSET		0x20c
#define TS_CH0_RX_PKT_INFO_OFFSET		0x210
#define TS_CH0_RX_DMA_BASE_OFFSET		0x214
#define TS_CH0_RX_DMA_LIMIT_OFFSET		0x218
#define TS_CH0_RX_DMA_WPTR_OFFSET		0x21c
#define TS_CH0_RX_DMA_STOP_OFFSET		0x220

#define TS_CH0_RX_DMA_CTRL_OFFSET		0x228
#define TS_CH0_RX_DMA_NOTIFY_OFFSET		0x22c

#define TS_CH1_RX_CTR_OFFSET			0x280
#define TS_CH1_RX_STS_OFFSET			0x284
#define TS_CH1_RX_TM_OFFSET			0x288
#define TS_CH1_RX_TEST_CTR_OFFSET		0x28c
#define TS_CH1_RX_PKT_INFO_OFFSET		0x290
#define TS_CH1_RX_DMA_BASE_OFFSET		0x294
#define TS_CH1_RX_DMA_LIMIT_OFFSET		0x298
#define TS_CH1_RX_DMA_WPTR_OFFSET		0x29c
#define TS_CH1_RX_DMA_STOP_OFFSET		0x2a0

#define TS_CH1_RX_DMA_CTRL_OFFSET		0x2a8
#define TS_CH1_RX_DMA_NOTIFY_OFFSET		0x2ac

/* Registers */
#define TS_CH0_TX_CTR_REG			TS_REG(0x100)
#define TS_CH0_TX_STS_REG			TS_REG(0x104)
#define TS_CH0_TX_PKT_INFO_REG			TS_REG(0x108)
#define TS_CH0_TX_DMA_BASE_REG			TS_REG(0x10c)
#define TS_CH0_TX_DMA_LIMIT_REG			TS_REG(0x110)
#define TS_CH0_TX_DMA_RPTR_REG			TS_REG(0x114)
#define TS_CH0_TX_DMA_STOP_REG			TS_REG(0x118)

#define TS_CH0_TX_DMA_CTRL_REG			TS_REG(0x124)
#define TS_CH0_TX_DMA_NOTIFY_REG		TS_REG(0x128)

#define TS_CH1_TX_CTR_REG			TS_REG(0x180)
#define TS_CH1_TX_STS_REG			TS_REG(0x184)
#define TS_CH1_TX_PKT_INFO_REG			TS_REG(0x188)
#define TS_CH1_TX_DMA_BASE_REG			TS_REG(0x18c)
#define TS_CH1_TX_DMA_LIMIT_REG			TS_REG(0x190)
#define TS_CH1_TX_DMA_RPTR_REG			TS_REG(0x194)
#define TS_CH1_TX_DMA_STOP_REG			TS_REG(0x198)

#define TS_CH1_TX_DMA_CTRL_REG			TS_REG(0x1a4)
#define TS_CH1_TX_DMA_NOTIFY_REG		TS_REG(0x1a8)

#define TS_CH0_RX_CTR_REG			TS_REG(0x200)
#define TS_CH0_RX_STS_REG			TS_REG(0x204)
#define TS_CH0_RX_TM_REG			TS_REG(0x208)
#define TS_CH0_RX_TEST_CTR_REG			TS_REG(0x20c)
#define TS_CH0_RX_PKT_INFO_REG			TS_REG(0x210)
#define TS_CH0_RX_DMA_BASE_REG			TS_REG(0x214)
#define TS_CH0_RX_DMA_LIMIT_REG			TS_REG(0x218)
#define TS_CH0_RX_DMA_WPTR_REG			TS_REG(0x21c)
#define TS_CH0_RX_DMA_STOP_REG			TS_REG(0x220)

#define TS_CH0_RX_DMA_CTRL_REG			TS_REG(0x228)
#define TS_CH0_RX_DMA_NOTIFY_REG		TS_REG(0x22c)

#define TS_CH1_RX_CTR_REG			TS_REG(0x280)
#define TS_CH1_RX_STS_REG			TS_REG(0x284)
#define TS_CH1_RX_TM_REG			TS_REG(0x288)
#define TS_CH1_RX_TEST_CTR_REG			TS_REG(0x28c)
#define TS_CH1_RX_PKT_INFO_REG			TS_REG(0x290)
#define TS_CH1_RX_DMA_BASE_REG			TS_REG(0x294)
#define TS_CH1_RX_DMA_LIMIT_REG			TS_REG(0x298)
#define TS_CH1_RX_DMA_WPTR_REG			TS_REG(0x29c)
#define TS_CH1_RX_DMA_STOP_REG			TS_REG(0x2a0)

#define TS_CH1_RX_DMA_CTRL_REG			TS_REG(0x2a8)
#define TS_CH1_RX_DMA_NOTIFY_REG		TS_REG(0x2ac)

/* ChanX */
#define TS_CHX_TX_CTR_REG(x)			TS_REG(0x100 + (0x80 * (x)))
#define TS_CHX_TX_STS_REG(x)			TS_REG(0x104 + (0x80 * (x)))
#define TS_CHX_TX_PKT_INFO_REG(x)		TS_REG(0x108 + (0x80 * (x)))
#define TS_CHX_TX_DMA_BASE_REG(x)		TS_REG(0x10c + (0x80 * (x)))
#define TS_CHX_TX_DMA_LIMIT_REG(x)		TS_REG(0x110 + (0x80 * (x)))
#define TS_CHX_TX_DMA_RPTR_REG(x)		TS_REG(0x114 + (0x80 * (x)))
#define TS_CHX_TX_DMA_STOP_REG(x)		TS_REG(0x118 + (0x80 * (x)))

#define TS_CHX_TX_DMA_CTRL_REG(x)		TS_REG(0x124 + (0x80 * (x)))
#define TS_CHX_TX_DMA_NOTIFY_REG(x)		TS_REG(0x128 + (0x80 * (x)))

#define TS_CHX_RX_CTR_REG(x)			TS_REG(0x200 + (0x80 * (x)))
#define TS_CHX_RX_STS_REG(x)			TS_REG(0x204 + (0x80 * (x)))
#define TS_CHX_RX_TM_REG(x)			TS_REG(0x208 + (0x80 * (x)))
#define TS_CHX_RX_TEST_CTR_REG(x)		TS_REG(0x20c + (0x80 * (x)))
#define TS_CHX_RX_PKT_INFO_REG(x)		TS_REG(0x210 + (0x80 * (x)))
#define TS_CHX_RX_DMA_BASE_REG(x)		TS_REG(0x214 + (0x80 * (x)))
#define TS_CHX_RX_DMA_LIMIT_REG(x)		TS_REG(0x218 + (0x80 * (x)))
#define TS_CHX_RX_DMA_WPTR_REG(x)		TS_REG(0x21c + (0x80 * (x)))
#define TS_CHX_RX_DMA_STOP_REG(x)		TS_REG(0x220 + (0x80 * (x)))

#define TS_CHX_RX_DMA_CTRL_REG(x)		TS_REG(0x228 + (0x80 * (x)))
#define TS_CHX_RX_DMA_NOTIFY_REG(x)		TS_REG(0x22c + (0x80 * (x)))

/***************************/
/* TX bitfileds definition */
/***************************/
/* TS_CHX_TX_CTR_REG */
#define TS_TX_CTR_INVALID_CLK_DIS		0x00020000
#define TS_TX_CTR_DMA_NOTIFY_EN			0x00010000
#define TS_TX_CTR_STOP				0x00008000
#define TS_TX_CTR_CLK_EN			0x00004000
#define TS_TX_CTR_SRC_EXT			0x00001000
#define TS_TX_CTR_SRC_CORE			0x00000000
#define TS_TX_CTR_DIV(x)			(((x) & 0x1f) << 7)
#define TS_TX_CTR_CLK_POL_NE			0x00000040
#define TS_TX_CTR_CLK_POL_PE			0x00000000
#define TS_TX_CTR_START				0x00000020
#define TS_TX_CTR_IDLE				0x00000000
#define TS_TX_CTR_PKT_SSI			0x00000008
#define TS_TX_CTR_PKT_SPI			0x00000000
#define TS_TX_CTR_RST				0x00000004
#define TS_TX_CTR_FINISH_INT_EN			0x00000002
#define TS_TX_CTR_FIFO_UNDER_ERR_EN		0x00000001

/* TS_CHX_TX_STS_REG */
#define TS_TX_STS_DMA_NOTIFY			0x00000400
#define TS_TX_STS_FIFO_WM(x)			(((x) & 0x3f3) >> 2)
#define TS_TX_STS_FINISH_INT			0x00000002
#define TS_TX_STS_FIFO_UNDER_ERR		0x00000001

/* TS_CHX_TX_PKT_INFO_REG */
#define TS_TX_PKT_INFO_LEN(x)			(((x) & 0x3ff) << 16)
#define TS_TX_PKT_INFO_VALID_LEN(x)		((x) & 0x3ff)

/* TS_CHX_TX_DMA_CTRL_REG */
#define TS_TX_DMA_CTRL_ADDR_MODE(x)		(((x) & 0x1) << 4)
#define TS_TX_DMA_CTRL_FIFO_TH(x)		((x) & 0x3)

/***************************/
/* RX bitfileds definition */
/***************************/
/* TS_CHX_RX_CTR_REG */
#define TS_TX_CTR_STRICT_PKT_ERR		0x20000000
#define TS_RX_CTR_CLK_SEL_RX			0x00000000
#define TS_RX_CTR_CLK_SEL_HCLK			0x10000000
#define TS_RX_CTR_DMA_NOTIFY_EN			0x08000000
#define TS_RX_CTR_STOP				0x04000000
#define TS_RX_CTR_CLK_EN			0x02000000
#define TS_RX_CTR_TIMEOUT(x)			(((x) & 0xffff) << 8)
#define TS_RX_CTR_PKT_LEN_ERR_EN		0x00000080
#define TS_RX_CTR_CLK_POL_NE			0x00000040
#define TS_RX_CTR_CLK_POL_PE			0x00000000
#define TS_RX_CTR_START				0x00000020
#define TS_RX_CTR_IDLE				0x00000000
#define TS_RX_CTR_DATA_RAW			0x00000010
#define TS_RX_CTR_DATA_TS			0x00000000
#define TS_RX_CTR_PKT_SSI			0x00000008
#define TS_RX_CTR_PKT_SPI			0x00000000
#define TS_RX_CTR_RST				0x00000004
#define TS_RX_CTR_FINISH_INT_EN			0x00000002
#define TS_RX_CTR_FIFO_OVER_ERR_EN		0x00000001

/* TS_CHX_RX_STS_REG */
#define TS_RX_STS_DMA_NOTIFY			0x00010000
#define TS_RX_STS_FIFO_WM(x)			(((x) & 0xff00) >> 8)
#define TS_RX_STS_PKT_LEN_ERR			0x00000080
#define TS_RX_STS_FINISH_INT			0x00000002
#define TS_RX_STS_FIFO_OVER_ERR			0x00000001

/* TS_CHX_RX_TM_REG */
/* Read only */

/* TS_CHX_RX_TEST_CTR_REG */
#define TS_RX_TEST_CTR_LOOPBACK_EN		0x00000001

/* TS_CHX_RX_PKT_INFO_REG */
#define TS_RX_PKT_INFO_LEN(x)			(((x) & 0x3ff) << 16)
#define TS_RX_PKT_INFO_VALID_LEN(x)		((x) & 0x3ff)

/* TS_CHX_RX_DMA_CTRL_REG */
#define TS_RX_DMA_CTRL_ADDR_MODE(x)		(((x) & 0x1) << 4)

#endif
