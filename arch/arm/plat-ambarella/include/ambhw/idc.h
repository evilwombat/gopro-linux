/*
 * ambhw/idc.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2008, Ambarella, Inc.
 */

#ifndef __AMBHW__IDC_H__
#define __AMBHW__IDC_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

#if (CHIP_REV == A1) || (CHIP_REV == A2) || 		\
    (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
    (CHIP_REV == A5L)  
#define IDC_INSTANCES		1
#else
#define IDC_INSTANCES		2
#endif

#if (CHIP_REV == A5L)
#define IDCS_INSTANCES			1
#else
#define IDCS_INSTANCES			0
#endif

#if (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == A5L) || \
    (CHIP_REV == I1)  || (CHIP_REV == A7L)
#define IDC_INTERNAL_DELAY_CLK		2
#else
#define IDC_INTERNAL_DELAY_CLK		0
#endif

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#define IDC_ENR_OFFSET			0x00
#define IDC_CTRL_OFFSET			0x04
#define IDC_DATA_OFFSET			0x08
#define IDC_STS_OFFSET			0x0c
#define IDC_PSLL_OFFSET			0x10
#define IDC_PSLH_OFFSET			0x14
#define IDC_FMCTRL_OFFSET		0x18
#define IDC_FMDATA_OFFSET		0x1c

#if	(IDCS_INSTANCES >= 1)
#define IDCS_ENR_OFFSET			0x00
#define IDCS_CTRL_OFFSET		0x04
#define IDCS_DATA_OFFSET		0x08
#define IDCS_STS_OFFSET			0x0c
#define IDCS_FIFO_CNT_OFFSET		0x10
#define IDCS_RX_CNT_OFFSET		0x14
#define IDCS_TX_CNT_OFFSET		0x18
#define IDCS_HOLD_TIME_OFFSET		0x1c
#define IDCS_SLAVE_ADDR_OFFSET		0x20
#define	IDCS_SCL_TIMER_OFFSET		0x24
#define IDCS_TIMEOUT_STS_OFFSET		0x28
#endif

#define IDC_ENR_REG			IDC_REG(IDC_ENR_OFFSET)
#define IDC_CTRL_REG			IDC_REG(IDC_CTRL_OFFSET)
#define IDC_DATA_REG			IDC_REG(IDC_DATA_OFFSET)
#define IDC_STS_REG			IDC_REG(IDC_STS_OFFSET)
#define IDC_PSLL_REG			IDC_REG(IDC_PSLL_OFFSET)
#define IDC_PSLH_REG			IDC_REG(IDC_PSLH_OFFSET)
#define IDC_FMCTRL_REG			IDC_REG(IDC_FMCTRL_OFFSET)
#define IDC_FMDATA_REG			IDC_REG(IDC_FMDATA_OFFSET)

#if 	(IDC_INSTANCES >= 2)
#define IDC2_ENR_REG			IDC2_REG(IDC_ENR_OFFSET)
#define IDC2_CTRL_REG			IDC2_REG(IDC_CTRL_OFFSET)
#define IDC2_DATA_REG			IDC2_REG(IDC_DATA_OFFSET)
#define IDC2_STS_REG			IDC2_REG(IDC_STS_OFFSET)
#define IDC2_PSLL_REG			IDC2_REG(IDC_PSLL_OFFSET)
#define IDC2_PSLH_REG			IDC2_REG(IDC_PSLH_OFFSET)
#define IDC2_FMCTRL_REG			IDC2_REG(IDC_FMCTRL_OFFSET)
#define IDC2_FMDATA_REG			IDC2_REG(IDC_FMDATA_OFFSET)
#endif

#if	(IDCS_INSTANCES >= 1)
#define IDCS_ENR_REG			IDCS_REG(IDCS_ENR_OFFSET)
#define IDCS_CTRL_REG			IDCS_REG(IDCS_CTRL_OFFSET)
#define IDCS_DATA_REG			IDCS_REG(IDCS_DATA_OFFSET)
#define IDCS_STS_REG			IDCS_REG(IDCS_STS_OFFSET)
#define IDCS_FIFO_CNT_REG		IDCS_REG(IDCS_FIFO_CNT_OFFSET)
#define IDCS_RX_CNT_REG			IDCS_REG(IDCS_RX_CNT_OFFSET)
#define IDCS_TX_CNT_REG			IDCS_REG(IDCS_TX_CNT_OFFSET)
#define IDCS_HOLD_TIME_REG		IDCS_REG(IDCS_HOLD_TIME_OFFSET)
#define IDCS_SLAVE_ADDR_REG		IDCS_REG(IDCS_SLAVE_ADDR_OFFSET)
#define	IDCS_SCL_TIMER_REG		IDCS_REG(IDCS_SCL_TIMER_OFFSET)
#define IDCS_TIMEOUT_STS_REG		IDCS_REG(IDCS_TIMEOUT_STS_OFFSET)


/* Bit/format definition */

/* IDCS_CTRL_REG */ 
#define IDCS_IRQ_EN		0x02
#define IDCS_IRQ_P_SR_EN	0x04
#define IDCS_IRQ_FIFO_TH_EN	0x08
#define IDCS_IRQ_TIME_OUT_EN	0x10

/* IDCS_STS_REG */
#define IDCS_TIMEOUT		0x100
#define IDCS_STOP		0x080
#define IDCS_REPEATED_START	0x040
#define IDCS_FIFO_TH_VLD	0x020
#define IDCS_SEL		0x010
#define IDCS_GENERAL_CALL	0x008
#define IDCS_FIFO_FULL		0x004
#define IDCS_FIFO_EMPTY		0x002
#define IDCS_RX_TX_STATE	0x001

#define IDCS_TX_MODE		0x0
#define IDCS_RX_MODE		0x1

#endif

#endif
