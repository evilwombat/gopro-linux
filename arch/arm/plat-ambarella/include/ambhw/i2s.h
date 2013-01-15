/*
 * ambhw/i2s.h
 *
 * History:
 *	2007/01/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2008, Ambarella, Inc.
 */

#ifndef __AMBHW__I2S_H__
#define __AMBHW__I2S_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

#if 	(CHIP_REV == A1) || (CHIP_REV == A2) ||		\
	(CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q)
#define	I2S_SUPPORT_GATE_SHIFT	0
#define	I2S_MAX_CHANNELS	2
#define	I2S_AMBA_IP		0
#define	I2S_24BITMUX_MODE_REG_BITS	1
#elif (CHIP_REV == A3) || (CHIP_REV == A5) || (CHIP_REV == A6)
#define	I2S_SUPPORT_GATE_SHIFT	1
#define	I2S_MAX_CHANNELS	6
#define	I2S_AMBA_IP		0
#define	I2S_24BITMUX_MODE_REG_BITS	1
#elif (CHIP_REV == A5L)
#define	I2S_SUPPORT_GATE_SHIFT	1
#define	I2S_MAX_CHANNELS	2
#define	I2S_AMBA_IP		1
#define	I2S_24BITMUX_MODE_REG_BITS	4
#elif (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1) || \
      (CHIP_REV == A7L)  
#define	I2S_SUPPORT_GATE_SHIFT	1
#define	I2S_MAX_CHANNELS	6
#define	I2S_AMBA_IP		1
#define	I2S_24BITMUX_MODE_REG_BITS	4
#else
#define	I2S_SUPPORT_GATE_SHIFT	1
#define	I2S_MAX_CHANNELS	6
#define	I2S_AMBA_IP		1
#define	I2S_24BITMUX_MODE_REG_BITS	4
#endif

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#define I2S_MODE_OFFSET				0x00
#define I2S_RX_CTRL_OFFSET			0x04
#define I2S_TX_CTRL_OFFSET			0x08
#define I2S_WLEN_OFFSET				0x0c
#define I2S_WPOS_OFFSET				0x10
#define I2S_SLOT_OFFSET				0x14
#define I2S_TX_FIFO_LTH_OFFSET		0x18
#define I2S_RX_FIFO_GTH_OFFSET		0x1c
#define I2S_CLOCK_OFFSET			0x20
#define I2S_INIT_OFFSET				0x24
#define I2S_TX_STATUS_OFFSET		0x28
#define I2S_TX_LEFT_DATA_OFFSET		0x2c
#define I2S_TX_RIGHT_DATA_OFFSET	0x30
#define I2S_RX_STATUS_OFFSET		0x34
#define I2S_RX_DATA_OFFSET			0x38
#define I2S_TX_FIFO_CNTR_OFFSET		0x3c
#define I2S_RX_FIFO_CNTR_OFFSET		0x40
#define I2S_TX_INT_ENABLE_OFFSET	0x44
#define I2S_RX_INT_ENABLE_OFFSET	0x48
#define I2S_RX_ECHO_OFFSET			0x4c
#define I2S_24BITMUX_MODE_OFFSET	0x50
#define I2S_RX_DATA_DMA_OFFSET		0x80
#define I2S_TX_LEFT_DATA_DMA_OFFSET	0xc0

#define I2S_MODE_REG				I2S_REG(0x00)
#define I2S_RX_CTRL_REG				I2S_REG(0x04)
#define I2S_TX_CTRL_REG				I2S_REG(0x08)
#define I2S_WLEN_REG				I2S_REG(0x0c)
#define I2S_WPOS_REG				I2S_REG(0x10)
#define I2S_SLOT_REG				I2S_REG(0x14)
#define I2S_TX_FIFO_LTH_REG			I2S_REG(0x18)
#define I2S_RX_FIFO_GTH_REG			I2S_REG(0x1c)
#define I2S_CLOCK_REG				I2S_REG(0x20)
#define I2S_INIT_REG				I2S_REG(0x24)
#define I2S_TX_STATUS_REG			I2S_REG(0x28)
#define I2S_TX_LEFT_DATA_REG		I2S_REG(0x2c)
#define I2S_TX_RIGHT_DATA_REG		I2S_REG(0x30)
#define I2S_RX_STATUS_REG			I2S_REG(0x34)
#define I2S_RX_DATA_REG				I2S_REG(0x38)
#define I2S_TX_FIFO_CNTR_REG		I2S_REG(0x3c)
#define I2S_RX_FIFO_CNTR_REG		I2S_REG(0x40)
#define I2S_TX_INT_ENABLE_REG		I2S_REG(0x44)
#define I2S_RX_INT_ENABLE_REG		I2S_REG(0x48)
#define I2S_RX_ECHO_REG				I2S_REG(0x4c)
#define I2S_24BITMUX_MODE_REG		I2S_REG(0x50)
#define I2S_GATEOFF_REG				I2S_REG(0x54)
#define I2S_CHANNEL_SELECT_REG		I2S_REG(0x58)
#define I2S_RX_DATA_DMA_REG			I2S_REG_PHYS(0x80)
#define I2S_TX_LEFT_DATA_DMA_REG	I2S_REG_PHYS(0xc0)

#if	(I2S_AMBA_IP == 1)
#define I2S_TX_FIFO_RESET_BIT	(1 << 4)
#define I2S_RX_FIFO_RESET_BIT	(1 << 3)
#endif
#define I2S_TX_ENABLE_BIT		(1 << 2)
#define I2S_RX_ENABLE_BIT		(1 << 1)
#define I2S_FIFO_RESET_BIT		(1 << 0)

#define I2S_RX_LOOPBACK_BIT		(1 << 3)
#define I2S_RX_ORDER_BIT		(1 << 2)
#define I2S_RX_WS_MST_BIT		(1 << 1)
#define I2S_RX_WS_INV_BIT		(1 << 0)

#define I2S_TX_LOOPBACK_BIT		(1 << 7)
#define I2S_TX_ORDER_BIT		(1 << 6)
#define I2S_TX_WS_MST_BIT		(1 << 5)
#define I2S_TX_WS_INV_BIT		(1 << 4)
#define I2S_TX_UNISON_BIT		(1 << 3)
#define I2S_TX_MUTE_BIT			(1 << 2)
#define I2S_TX_MONO_MASK		0xfffffffc
#define I2S_TX_MONO_RIGHT		(1 << 1)
#define I2S_TX_MONO_LEFT		(1 << 0)

#define I2S_CLK_WS_OUT_EN		(1 << 9)
#define I2S_CLK_BCLK_OUT_EN		(1 << 8)
#define I2S_CLK_BCLK_OUTPUT		(1 << 7)
#define I2S_CLK_MASTER_MODE		(I2S_CLK_WS_OUT_EN	|	\
					 I2S_CLK_BCLK_OUT_EN	|	\
					 I2S_CLK_BCLK_OUTPUT)
#define I2S_CLK_TX_PO_FALL		(1 << 6)
#define I2S_CLK_RX_PO_FALL		(1 << 5)
#define I2S_CLK_DIV_MASK		0xffffffe0

#if	(I2S_AMBA_IP == 0)
#define I2S_RX_SHIFT_ENB		(1 << 2)
#define I2S_TX_SHIFT_ENB		(1 << 1)
#else
#define I2S_RX_SHIFT_ENB		(1 << 1)
#define I2S_TX_SHIFT_ENB		(1 << 0)
#endif

#define I2S_2CHANNELS_ENB		0x00
#define I2S_4CHANNELS_ENB		0x01
#define I2S_6CHANNELS_ENB		0x02

#define I2S_FIFO_THRESHOLD_INTRPT	0x08
#define I2S_FIFO_FULL_INTRPT		0x02
#define I2S_FIFO_EMPTY_INTRPT		0x01

/* I2S_24BITMUX_MODE_REG */
#define I2S_24BITMUX_MODE_ENABLE		0x1
#if	(I2S_24BITMUX_MODE_REG_BITS == 4)
#define I2S_24BITMUX_MODE_FDMA_BURST_DIS	0x2
#define I2S_24BITMUX_MODE_RST_CHAN0		0x4
#define I2S_24BITMUX_MODE_DMA_BOOTSEL		0x8
#endif

#endif
