/*
 * ambhw/hif.h
 *
 * History:
 *	2007/01/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2008, Ambarella, Inc.
 */

#ifndef __AMBHW__HIF_H__
#define __AMBHW__HIF_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

#if (CHIP_REV == A1) || (CHIP_REV == A5L) || (CHIP_REV == A7L)
#define HOST_SUPPORT_AHB_CLK_EN		0
#define HOST_USE_APB_BUS_CLK		0
#else
#define HOST_SUPPORT_AHB_CLK_EN		1
#define HOST_USE_APB_BUS_CLK		1
#endif

#if (CHIP_REV == A1) || (CHIP_REV == A2)
#define HOST_MAX_AHB_CLK_EN_BITS	8
#define HOST_MAX_MODES			5
#elif  (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
	(CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1) 
#define HOST_MAX_AHB_CLK_EN_BITS	0
#define HOST_MAX_MODES			5
#else
#define HOST_MAX_AHB_CLK_EN_BITS	10
#define HOST_MAX_MODES			6
#endif

#if (CHIP_REV == A5L) || (CHIP_REV == A7L)
#define HOST_INSTANCES			0
#else
#define HOST_INSTANCES			1
#endif

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#define HOST_MODE_OFFSET			0x00
#define HOST_RX_CTRL_OFFSET			0x04
#define HOST_TX_CTRL_OFFSET			0x08
#define HOST_WLEN_OFFSET			0x0c
#define HOST_WPOS_OFFSET			0x10
#define HOST_SLOT_OFFSET			0x14
#define HOST_TX_FIFO_LTH_OFFSET			0x18
#define HOST_RX_FIFO_GTH_OFFSET			0x1c
#define HOST_CLOCK_OFFSET			0x20
#define HOST_INIT_OFFSET			0x24
#define HOST_TX_FIFO_FLAG_OFFSET		0x28
#define HOST_TX_LEFT_DATA_OFFSET		0x2c
#define HOST_TX_RIGHT_DATA_OFFSET		0x30
#define HOST_RX_FIFO_FLAG_OFFSET		0x34
#define HOST_RX_DATA_OFFSET			0x38
#define HOST_TX_FIFO_CNTR_OFFSET		0x3c
#define HOST_RX_FIFO_CNTR_OFFSET		0x40
#define HOST_TX_INT_ENABLE_OFFSET		0x44
#define HOST_RX_INT_ENABLE_OFFSET		0x48
#define HOST_ENABLE_OFFSET			0x4c
#define HOST_24BITMUX_MODE_OFFSET		0x50
#define HOST_AHB_CLK_ENABLE_OFFSET		0x54
#define HOST_AHB_EARLY_TERMINATION_OFFSET	0x58
#define HOST_RX_DATA_DMA_OFFSET			0x80
#define HOST_TX_LEFT_DATA_DMA_OFFSET		0xc0

#define HOST_MODE_REG			HOST_REG(0x00)
#define HOST_RX_CTRL_REG		HOST_REG(0x04)
#define HOST_TX_CTRL_REG		HOST_REG(0x08)
#define HOST_WLEN_REG			HOST_REG(0x0c)
#define HOST_WPOS_REG			HOST_REG(0x10)
#define HOST_SLOT_REG			HOST_REG(0x14)
#define HOST_TX_FIFO_LTH_REG		HOST_REG(0x18)
#define HOST_RX_FIFO_GTH_REG		HOST_REG(0x1c)
#define HOST_CLOCK_REG			HOST_REG(0x20)
#define HOST_INIT_REG			HOST_REG(0x24)
#define HOST_TX_FIFO_FLAG_REG		HOST_REG(0x28)
#define HOST_TX_LEFT_DATA_REG		HOST_REG(0x2c)
#define HOST_TX_RIGHT_DATA_REG		HOST_REG(0x30)
#define HOST_RX_FIFO_FLAG_REG		HOST_REG(0x34)
#define HOST_RX_DATA_REG		HOST_REG(0x38)
#define HOST_TX_FIFO_CNTR_REG		HOST_REG(0x3c)
#define HOST_RX_FIFO_CNTR_REG		HOST_REG(0x40)
#define HOST_TX_INT_ENABLE_REG		HOST_REG(0x44)
#define HOST_RX_INT_ENABLE_REG		HOST_REG(0x48)
#define HOST_ENABLE_REG			HOST_REG(0x4c)
#define HOST_24BITMUX_MODE_REG		HOST_REG(0x50)
#define HOST_AHB_CLK_ENABLE_REG		HOST_REG(0x54)
#define HOST_AHB_EARLY_TERMINATION_INFO	HOST_REG(0x58)
#define HOST_RX_DATA_DMA_REG		HOST_REG(0x80)
#define HOST_TX_LEFT_DATA_DMA_REG	HOST_REG(0xc0)


#define HOST_LEFT_JUSTIFY_MODE	       	0
#define HOST_RIGHT_JUSTIFY_MODE       	1
#define HOST_MSB_EXTEND_MODE	       	2
#define HOST_I2S_MODE	               	4
#define HOST_DSP_MODE	               	6
#if (HOST_MAX_MODES >= 6)
#define HOST_FIOS_DMA_ENABLE_MODE	15
#endif
#define	HOST_FIFO_RST	            	0x01
#define	HOST_RX_EN	            	0x02
#define	HOST_TX_EN	            	0x04

#define HOST_IDLE			(1 << 4)
#define HOST_VALID_THRESHOLD		(1 << 4)
#define HOST_OVERFLOW			(1 << 2)
#define HOST_FULL			(1 << 1)
#define HOST_EMPTY			(1 << 0)

#define HOST_RX_LOOPBACK		(1 << 3)
#define HOST_RX_NORMAL			(0 << 3)
#define HOST_RX_ORDER_LSB		(1 << 2)
#define HOST_RX_ORDER_MSB		(0 << 2)
#define HOST_RX_WS_MST			(1 << 1)
#define HOST_RX_WS_SLV			(0 << 1)
#define HOST_RX_WS_INV			(1 << 0)
#define HOST_RX_WS_NO_INV		(0 << 0)

#define HOST_TX_ORDER_LSB		(1 << 6)
#define HOST_TX_ORDER_MSB		(0 << 6)
#define HOST_TX_MST			(1 << 5)
#define HOST_TX_SLV			(0 << 5)
#define HOST_TX_WS_INV			(1 << 4)
#define HOST_TX_WS_NO_INV		(0 << 4)
#define HOST_TX_UNISON_16		(1 << 3)
#define HOST_TX_UNISON_24		(0 << 3)
#define HOST_TX_MUTE			(1 << 2)
#define HOST_TX_NORMAL			(0 << 2)
#define HOST_TX_STEREO			(0 << 0)
#define HOST_TX_MONO_RIGHT		(2 << 0)
#define HOST_TX_MONO_LEFT		(3 << 0)

#define HOST_CLK_WOE			(1 << 9)
#define HOST_CLK_SOE			(1 << 8)
#define HOST_CLK_MST			(1 << 7)
#define HOST_CLK_SLV			(0 << 7)
#define HOST_CLK_TSP_FALLING		(1 << 6)
#define HOST_CLK_TSP_RISING		(0 << 6)
#define HOST_CLK_RSP_FALLING		(1 << 5)
#define HOST_CLK_RSP_RISING		(0 << 5)

#define HOST_LEFT_JUSTIFY_MODE		0
#define HOST_RIGHT_JUSTIFY_MODE		1

/* HOST_AHB_CLK_ENABLE_REG */
#if (HOST_MAX_AHB_CLK_EN_BITS == 10)
#define HOST_AHB_FDMA_BURST_DIS		0x200
#define HOST_AHB_BOOT_SEL		0x100
#endif

#define HOST_AHB_DMA_CHAN0_RST		0x80
#define HOST_AHB_SDIO_SEL		0x40
#define HOST_AHB_HANG_BREAK_ENB		0x20
#define HOST_AHB_RETRY_DEBUG_ENB	0x10
#define HOST_AHB_HOST_DMA_CLK_ENB	0x08
#define HOST_AHB_FLASH_CLK_ENB		0x04
#define HOST_AHB_CF_CLK_ENB		0x02
#define HOST_AHB_XD_CLK_ENB		0x01

#endif
