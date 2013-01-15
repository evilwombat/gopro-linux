/*
 * ambhw/usbdc.h
 *
 * History:
 *	2007/01/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2007, Ambarella, Inc.
 */

#ifndef __AMBHW__USBDC_H__
#define __AMBHW__USBDC_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

/* None so far */

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/
/************************************************************/
/* USB Device Controller IN/OUT Endpoint-Specific Registers */
/************************************************************/

#define UDC_EP_IN_BASE			USBDC_BASE

#define UDC_EP_IN_CTRL_REG(n)		(UDC_EP_IN_BASE + 0x0000 + 0x0020*(n))
#define UDC_EP_IN_STS_REG(n)		(UDC_EP_IN_BASE + 0x0004 + 0x0020*(n))
#define UDC_EP_IN_BUF_SZ_REG(n)		(UDC_EP_IN_BASE + 0x0008 + 0x0020*(n))
#define UDC_EP_IN_MAX_PKT_SZ_REG(n)	(UDC_EP_IN_BASE + 0x000c + 0x0020*(n))
#define UDC_EP_IN_DAT_DESC_PTR_REG(n)	(UDC_EP_IN_BASE + 0x0014 + 0x0020*(n))
#define UDC_EP_IN_WR_CFM_REG(n)		(UDC_EP_IN_BASE + 0x001c + 0x0020*(n))

#define UDC_EP_OUT_BASE			(USBDC_BASE + 0x0200)

#define UDC_EP_OUT_CTRL_REG(n)		(UDC_EP_OUT_BASE + 0x0000 + 0x0020*(n))
#define UDC_EP_OUT_STS_REG(n)		(UDC_EP_OUT_BASE + 0x0004 + 0x0020*(n))
#define UDC_EP_OUT_PKT_FRM_NUM_REG(n)	(UDC_EP_OUT_BASE + 0x0008 + 0x0020*(n))
#define UDC_EP_OUT_MAX_PKT_SZ_REG(n)	(UDC_EP_OUT_BASE + 0x000c + 0x0020*(n))
#define UDC_EP_OUT_SETUP_BUF_PTR_REG(n)	(UDC_EP_OUT_BASE + 0x0010 + 0x0020*(n))
#define UDC_EP_OUT_DAT_DESC_PTR_REG(n)	(UDC_EP_OUT_BASE + 0x0014 + 0x0020*(n))
#define UDC_EP_OUT_RD_CFM_ZO_REG(n)	(UDC_EP_OUT_BASE + 0x001c + 0x0020*(n))

/* UDC_EP_IN_CTRL_REG(n) and UDC_EP_OUT_CTRL_REG(n) */

#define UDC_EP_CTRL_RRDY		0x00000200
#define UDC_EP_CTRL_CNAK		0x00000100
#define UDC_EP_CTRL_SNAK		0x00000080
#define UDC_EP_CTRL_NAK			0x00000040
#define UDC_EP_CTRL_ET			0x00000030
#define UDC_EP_CTRL_P			0x00000008
#define UDC_EP_CTRL_SN			0x00000004
#define UDC_EP_CTRL_F			0x00000002
#define UDC_EP_CTRL_S			0x00000001

/* UDC_EP_IN_STS_REG(n) and UDC_EP_OUT_STS_REG(n) */
#define UDC_EP_STS_RX_PKT_SZ		0x007ff800
#define UDC_EP_STS_TDC			0x00000400
#define UDC_EP_STS_HE			0x00000200
#define UDC_EP_STS_BNA			0x00000080
#define UDC_EP_STS_IN			0x00000040
#define UDC_EP_STS_OUT			0x00000030

/* UDC_EP_IN_BUF_SZ_REG(n) and UDC_EP_OUT_PKT_FRM_NUM_REG(n) */
#define UDC_EP_IN_BUF_SZ		0x0000ffff
#define UDC_EP_OUT_PKT_FRM_NUM		0x0000ffff

/* UDC_EP_IN_MAX_PKT_SZ_REG(n) and UDC_EP_OUT_MAX_PKT_SZ_REG(n) */
#define UDC_EP_MAX_PKT_BUF_SZ		0xffff0000
#define UDC_EP_MAX_PKT_FRM_NUM		0x0000ffff

/* UDC_EP_OUT_SETUP_BUF_PTR_REG(n) */
/* UDC_EP_IN_DAT_DESC_PTR_REG(n) and UDC_EP_OUT_DAT_DESC_PTR_REG(n) */
/* UDC_EP_IN_WR_CFM_REG(n) and UDC_EP_OUT_RD_CFM_ZO_REG(n) */


/* ---------------------------------------------------------------------- */

/******************************************/
/* USB Device Controller Global Registers */
/******************************************/
#if (CHIP_REV == 1)
#define UDC_CFG				0x400
#define UDC_CTRL			0x404
#define UDC_STS				0x408
#define UDC_INTR			0x40c
#define UDC_INTR_MSK			0x410
#define UDC_EP_INTR			0x414
#define UDC_EP_INTR_MSK			0x418
#define UDC_TEST_MODE			0x41c
#endif

#define UDC_REG(x)			(USBDC_BASE + (x))
#define UDC_CFG_REG			UDC_REG(0x400)
#define UDC_CTRL_REG			UDC_REG(0x404)
#define UDC_STS_REG			UDC_REG(0x408)
#define UDC_INTR_REG			UDC_REG(0x40c)
#define UDC_INTR_MSK_REG		UDC_REG(0x410)
#define UDC_EP_INTR_REG			UDC_REG(0x414)
#define UDC_EP_INTR_MSK_REG		UDC_REG(0x418)
#define UDC_TEST_MODE_REG		UDC_REG(0x41c)

/* UDC_CFG_REG */
#define UDC_CFG_DDR			0x00080000
#define UDC_CFG_SET_DESC		0x00040000
#define UDC_CFG_CSR_PRG			0x00020000
#define UDC_CFG_HALT			0x00010000
#define UDC_CFG_HS_TIMEOUT		0x0000e000
#define UDC_CFG_PS_TIMEOUT		0x00001c00
#define UDC_CFG_PHY_ERROR		0x00000200
#define UDC_CFG_STS_i			0x00000100
#define UDC_CFG_STS			0x00000080
#define UDC_CFG_DIR			0x00000040
#define UDC_CFG_P1			0x00000020
#define UDC_CFG_SS			0x00000010
#define UDC_CFG_SP			0x00000008
#define UDC_CFG_RWKP			0x00000004
#define UDC_CFG_SPD			0x00000003

/* UDC_CTRL_REG */
#define UDC_CTRL_THLEN			0xff000000
#define UDC_CTRL_BRLEN			0x00ff0000
#define UDC_CTRL_CSR_DONE		0x00002000
#define UDC_CTRL_DEVNAK			0x00001000
#define UDC_CTRL_SCALE			0x00000800
#define UDC_CTRL_SD			0x00000400
#define UDC_CTRL_MODE			0x00000200
#define UDC_CTRL_BREN			0x00000100
#define UDC_CTRL_THE			0x00000080
#define UDC_CTRL_BF			0x00000040
#define UDC_CTRL_BE			0x00000020
#define UDC_CTRL_DU			0x00000010
#define UDC_CTRL_TDE			0x00000008
#define UDC_CTRL_RDE			0x00000004
#define UDC_CTRL_RES			0x00000001

/* UDC_STS_REG */
#define UDC_STS_TS			0xfffc0000
#define UDC_STS_PHY_ERROR		0x00010000
#define UDC_STS_RXFIFO_EMPTY		0x00008000
#define UDC_STS_ENUM_SPD		0x00006000
#define UDC_STS_SUSP			0x00001000
#define UDC_STS_ALT			0x00000f00
#define UDC_STS_INTF			0x000000f0
#define UDC_STS_CFG			0x0000000f

/* UDC_INTR_REG */
#define UDC_INTR_ENUM			0x00000040
#define UDC_INTR_SOF			0x00000020
#define UDC_INTR_US			0x00000010
#define UDC_INTR_UR			0x00000008
#define UDC_INTR_ES			0x00000004
#define UDC_INTR_SI			0x00000002
#define UDC_INTR_SC			0x00000001

/* UDC_INTR_MSK_REG */
#define UDC_INTR_MSK_ENUM		0x00000040
#define UDC_INTR_MSK_SOF		0x00000020
#define UDC_INTR_MSK_US			0x00000010
#define UDC_INTR_MSK_UR			0x00000008
#define UDC_INTR_MSK_ES			0x00000004
#define UDC_INTR_MSK_SI			0x00000002
#define UDC_INTR_MSK_SC			0x00000001

/* UDC_EP_INTR_REG */
/* UDC_EP_INTR_MSK_REG */

/* UDC_TEST_MODE_REG */
#define UDC_TEST_MODE_TSTMODE		0x00000001


#endif
