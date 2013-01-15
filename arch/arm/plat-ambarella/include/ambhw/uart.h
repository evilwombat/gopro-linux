/*
 * ambhw/uart.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2007, Ambarella, Inc.
 */

#ifndef __AMBHW__UART_H__
#define __AMBHW__UART_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>


#define _UART_0				0
#define _UART_1				1
#define _UART_2				2
#define _UART_3				3


/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/
#if (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == A7L) 
#define	UART_INSTANCES			2
#elif (CHIP_REV == I1)  
#define	UART_INSTANCES			4
#else
#define	UART_INSTANCES			1
#endif

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/
#define PORTMAX				2

#define UART0_BASE			UART_BASE

#define UART_RB_OFFSET			0x00
#define UART_TH_OFFSET			0x00
#define UART_DLL_OFFSET			0x00
#define UART_IE_OFFSET			0x04
#define UART_DLH_OFFSET			0x04
#define UART_II_OFFSET			0x08
#define UART_FC_OFFSET			0x08
#define UART_LC_OFFSET			0x0c
#define UART_MC_OFFSET			0x10
#define UART_LS_OFFSET			0x14
#define UART_MS_OFFSET			0x18
#define UART_SC_OFFSET			0x1c	/* Byte */
#define UART_SRR_OFFSET			0x88

#define UART0_RB_REG			UART0_REG(UART_RB_OFFSET)
#define UART0_TH_REG			UART0_REG(UART_TH_OFFSET)
#define UART0_DLL_REG			UART0_REG(UART_DLL_OFFSET)
#define UART0_IE_REG			UART0_REG(UART_IE_OFFSET)
#define UART0_DLH_REG			UART0_REG(UART_DLH_OFFSET)
#define UART0_II_REG			UART0_REG(UART_II_OFFSET)
#define UART0_FC_REG			UART0_REG(UART_FC_OFFSET)
#define UART0_LC_REG			UART0_REG(UART_LC_OFFSET)
#define UART0_MC_REG			UART0_REG(UART_MC_OFFSET)
#define UART0_LS_REG			UART0_REG(UART_LS_OFFSET)
#define UART0_MS_REG			UART0_REG(UART_MS_OFFSET)
#define UART0_SC_REG			UART0_REG(UART_SC_OFFSET)	/* Byte */

#if (UART_INSTANCES >= 2)

#define UART1_RB_REG			UART1_REG(UART_RB_OFFSET)
#define UART1_TH_REG			UART1_REG(UART_TH_OFFSET)
#define UART1_DLL_REG			UART1_REG(UART_DLL_OFFSET)
#define UART1_IE_REG			UART1_REG(UART_IE_OFFSET)
#define UART1_DLH_REG			UART1_REG(UART_DLH_OFFSET)
#define UART1_II_REG			UART1_REG(UART_II_OFFSET)
#define UART1_FC_REG			UART1_REG(UART_FC_OFFSET)
#define UART1_LC_REG			UART1_REG(UART_LC_OFFSET)
#define UART1_MC_REG			UART1_REG(UART_MC_OFFSET)
#define UART1_LS_REG			UART1_REG(UART_LS_OFFSET)
#define UART1_MS_REG			UART1_REG(UART_MS_OFFSET)
#define UART1_SC_REG			UART1_REG(UART_SC_OFFSET)	/* Byte */

#endif  /* UART_INSTANCES >= 2 */

#if (UART_INSTANCES >= 3)

#define UART2_RB_REG			UART2_REG(UART_RB_OFFSET)
#define UART2_TH_REG			UART2_REG(UART_TH_OFFSET)
#define UART2_DLL_REG			UART2_REG(UART_DLL_OFFSET)
#define UART2_IE_REG			UART2_REG(UART_IE_OFFSET)
#define UART2_DLH_REG			UART2_REG(UART_DLH_OFFSET)
#define UART2_II_REG			UART2_REG(UART_II_OFFSET)
#define UART2_FC_REG			UART2_REG(UART_FC_OFFSET)
#define UART2_LC_REG			UART2_REG(UART_LC_OFFSET)
#define UART2_MC_REG			UART2_REG(UART_MC_OFFSET)
#define UART2_LS_REG			UART2_REG(UART_LS_OFFSET)
#define UART2_MS_REG			UART2_REG(UART_MS_OFFSET)
#define UART2_SC_REG			UART2_REG(UART_SC_OFFSET)	/* Byte */

#endif  /* UART_INSTANCES >= 3 */

#if (UART_INSTANCES >= 4)

#define UART3_RB_REG			UART3_REG(UART_RB_OFFSET)
#define UART3_TH_REG			UART3_REG(UART_TH_OFFSET)
#define UART3_DLL_REG			UART3_REG(UART_DLL_OFFSET)
#define UART3_IE_REG			UART3_REG(UART_IE_OFFSET)
#define UART3_DLH_REG			UART3_REG(UART_DLH_OFFSET)
#define UART3_II_REG			UART3_REG(UART_II_OFFSET)
#define UART3_FC_REG			UART3_REG(UART_FC_OFFSET)
#define UART3_LC_REG			UART3_REG(UART_LC_OFFSET)
#define UART3_MC_REG			UART3_REG(UART_MC_OFFSET)
#define UART3_LS_REG			UART3_REG(UART_LS_OFFSET)
#define UART3_MS_REG			UART3_REG(UART_MS_OFFSET)
#define UART3_SC_REG			UART3_REG(UART_SC_OFFSET)	/* Byte */

#endif  /* UART_INSTANCES >= 4 */


#define UARTX_RB_REG(x)			(UART_BASE + ((x) << 16) + UART_RB_OFFSET)
#define UARTX_TH_REG(x)			(UART_BASE + ((x) << 16) + UART_TH_OFFSET)
#define UARTX_DLL_REG(x)		(UART_BASE + ((x) << 16) + UART_DLL_OFFSET)
#define UARTX_IE_REG(x)			(UART_BASE + ((x) << 16) + UART_IE_OFFSET)
#define UARTX_DLH_REG(x)		(UART_BASE + ((x) << 16) + UART_DLH_OFFSET)
#define UARTX_II_REG(x)			(UART_BASE + ((x) << 16) + UART_II_OFFSET)
#define UARTX_FC_REG(x)			(UART_BASE + ((x) << 16) + UART_FC_OFFSET)
#define UARTX_LC_REG(x)			(UART_BASE + ((x) << 16) + UART_LC_OFFSET)
#define UARTX_MC_REG(x)			(UART_BASE + ((x) << 16) + UART_MC_OFFSET)
#define UARTX_LS_REG(x)			(UART_BASE + ((x) << 16) + UART_LS_OFFSET)
#define UARTX_MS_REG(x)			(UART_BASE + ((x) << 16) + UART_MS_OFFSET)
#define UARTX_SC_REG(x)			(UART_BASE + ((x) << 16) + UART_SC_OFFSET)	/* Byte */

/* UART[x]_IE_REG */
#define UART_IE_PTIME			0x80
#define UART_IE_ETOI			0x20
#define UART_IE_EBDI			0x10
#define UART_IE_EDSSI			0x08
#define UART_IE_ELSI			0x04
#define UART_IE_ETBEI			0x02
#define UART_IE_ERBFI			0x01

/* UART[x]_II_REG */
#define UART_II_MODEM_STATUS_CHANGED	0x00
#define UART_II_NO_INT_PENDING		0x01
#define UART_II_THR_EMPTY		0x02
#define UART_II_RCV_DATA_AVAIL		0x04
#define UART_II_RCV_STATUS		0x06
#define UART_II_CHAR_TIMEOUT		0x0c

/* UART[x]_FC_REG */
#define UART_FC_RX_ONECHAR		0x00
#define UART_FC_RX_QUARTER_FULL		0x40
#define UART_FC_RX_HALF_FULL		0x80
#define UART_FC_RX_2_TO_FULL		0xc0
#define UART_FC_TX_EMPTY		0x00
#define UART_FC_TX_2_IN_FIFO		0x10
#define UART_FC_TX_QUATER_IN_FIFO	0x20
#define UART_FC_TX_HALF_IN_FIFO		0x30
#define UART_FC_XMITR			0x04
#define UART_FC_RCVRR			0x02
#define UART_FC_FIFOE			0x01

/* UART[x]_LC_REG */
#define UART_LC_DLAB			0x80
#define UART_LC_BRK			0x40
#define UART_LC_EPS			0x10
#define UART_LC_EVEN_PARITY		0x08
#define UART_LC_ODD_PARITY		0x00
#define UART_LC_STOP_2BIT		0x04
#define UART_LC_STOP_1BIT		0x00
#define UART_LC_CLS_8_BITS		0x03
#define UART_LC_CLS_7_BITS		0x02
#define UART_LC_CLS_6_BITS		0x01
#define UART_LC_CLS_5_BITS		0x00
/*	quick defs */
#define	UART_LC_8N1			0x03
#define	UART_LC_7E1			0x0a

/* UART[x]_MC_REG */
#define UART_MC_SIRE			0x40
#define UART_MC_AFCE			0x20
#define UART_MC_LB			0x10
#define UART_MC_OUT2			0x08
#define UART_MC_OUT1			0x04
#define UART_MC_RTS			0x02
#define UART_MC_DTR			0x01

/* UART[x]_LS_REG */
#define UART_LS_FERR			0x80
#define UART_LS_TEMT			0x40
#define UART_LS_THRE			0x20
#define UART_LS_BI			0x10
#define UART_LS_FE			0x08
#define UART_LS_PE			0x04
#define UART_LS_OE			0x02
#define UART_LS_DR			0x01

/* UART[x]_MS_REG */
#define UART_MS_DCD			0x80
#define UART_MS_RI			0x40
#define UART_MS_DSR			0x20
#define UART_MS_CTS			0x10
#define UART_MS_DDCD			0x08
#define UART_MS_TERI			0x04
#define UART_MS_DDSR			0x02
#define UART_MS_DCTS			0x01

/* Other defs for UART */
#define RECV_BUF_SIZ			1500
#define SEND_BUF_SIZ			1500

#endif
