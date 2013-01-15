/*
 * ambhw/sd.h
 *
 * History:
 *	2007/01/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2008, Ambarella, Inc.
 */

#ifndef __AMBHW__SD_H__
#define __AMBHW__SD_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

#if ((CHIP_REV == A1)  || (CHIP_REV == A5)  || (CHIP_REV == A6)  || \
     (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
     (CHIP_REV == A5S) || (CHIP_REV == A5L) || (CHIP_REV == A7))
#define SD_INSTANCES			1
#else
#define SD_INSTANCES			2
#endif

#if (CHIP_REV == A1) || (CHIP_REV == A2) || (CHIP_REV == A3) ||	\
    defined(__FPGA__)
#define SD_SUPPORT_PLL_SCALER		0
#else
#define SD_SUPPORT_PLL_SCALER     	1
#endif

#if ((CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A6) || \
     (CHIP_REV == A5S) || (CHIP_REV == A5L) || (CHIP_REV == A7) || \
     (CHIP_REV == I1) )
#define SD_HAS_INTERNAL_MUXER      	1
#else
#define SD_HAS_INTERNAL_MUXER           0
#endif

#if (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1) 	|| \
    (CHIP_REV == A7L)
/* There is no dedicated irq for 2nd CD pin. */
/* So it does not work in A5S and use GPIO instead. */
#define SD_HAS_INTERNAL_2ND_CDWP	0
#else
#define SD_HAS_INTERNAL_2ND_CDWP	0
#endif

#if (CHIP_REV == A5L) || (CHIP_REV == I1) || (CHIP_REV == A7L)
#define SD_HAS_DELAY_CTRL		1
#else
#define SD_HAS_DELAY_CTRL		0
#endif

#if ((CHIP_REV == A1) || (CHIP_REV == A3)  || (CHIP_REV == A5)  || \
     (CHIP_REV == A6) || (CHIP_REV == A2S) || (CHIP_REV == A2M) || \
     (CHIP_REV == A2Q))
#define SD_BUS_SWITCH_DLY		1
#else
#define SD_BUS_SWITCH_DLY		0
#endif

#if ((CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == A5L) || \
     (CHIP_REV == I1)  || (CHIP_REV == A7L))
#define SD_HAS_IO_DRIVE_CTRL		1
#else
#define SD_HAS_IO_DRIVE_CTRL		0
#endif

#if (CHIP_REV == I1) || (CHIP_REV == A7L)
#define SD_HOST1_SUPPORT_XC		0
#define SD_HOST2_SUPPORT_XC		1
#else
#define SD_HOST1_SUPPORT_XC		0
#define SD_HOST2_SUPPORT_XC		0
#endif

#if (CHIP_REV == I1) ||  (CHIP_REV == A7L)
#define SD_SUPPORT_ACMD23		0
#else
#define SD_SUPPORT_ACMD23		1
#endif

#if (CHIP_REV == I1)
#define SD_HAS_SDXC_CLOCK		1
#else
#define SD_HAS_SDXC_CLOCK		0
#endif

#if (CHIP_REV == A7L)
#define SD_HOST1_HOST2_HAS_MUX		1
#else
#define SD_HOST1_HOST2_HAS_MUX		0
#endif

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#define SD_DMA_ADDR_OFFSET		0x000
#define SD_BLK_SZ_OFFSET		0x004	/* Half word */
#define SD_BLK_CNT_OFFSET		0x006	/* Half word */
#define SD_ARG_OFFSET			0x008
#define SD_XFR_OFFSET			0x00c	/* Half word */
#define SD_CMD_OFFSET			0x00e	/* Half word */
#define SD_RSP0_OFFSET			0x010
#define SD_RSP1_OFFSET			0x014
#define SD_RSP2_OFFSET			0x018
#define SD_RSP3_OFFSET			0x01c
#define SD_DATA_OFFSET			0x020
#define SD_STA_OFFSET			0x024
#define SD_HOST_OFFSET			0x028	/* Byte */
#define SD_PWR_OFFSET			0x029	/* Byte */
#define SD_GAP_OFFSET			0x02a	/* Byte */
#define SD_WAK_OFFSET			0x02b	/* Byte */
#define SD_CLK_OFFSET			0x02c	/* Half word */
#define SD_TMO_OFFSET			0x02e	/* Byte */
#define SD_RESET_OFFSET			0x02f	/* Byte */
#define SD_NIS_OFFSET			0x030	/* Half word */
#define SD_EIS_OFFSET			0x032	/* Half word */
#define SD_NISEN_OFFSET			0x034	/* Half word */
#define SD_EISEN_OFFSET			0x036	/* Half word */
#define SD_NIXEN_OFFSET			0x038	/* Half word */
#define SD_EIXEN_OFFSET			0x03a	/* Half word */
#define SD_AC12ES_OFFSET		0x03c	/* Half word */
#define SD_CAP_OFFSET			0x040
#define SD_CUR_OFFSET			0x048
#define SD_XC_CTR_OFFSET		0x060
#define SD_BOOT_CTR_OFFSET		0x070
#define SD_BOOT_STA_OFFSET		0x074
#define SD_VOL_SW_OFFSET		0x07c
#define SD_SIST_OFFSET			0x0fc	/* Half word */
#define SD_VER_OFFSET			0x0fe	/* Half word */

#define SD_DMA_ADDR_REG			SD_REG(0x000)
#define SD_BLK_SZ_REG			SD_REG(0x004)	/* Half word */
#define SD_BLK_CNT_REG			SD_REG(0x006)	/* Half word */
#define SD_ARG_REG			SD_REG(0x008)
#define SD_XFR_REG			SD_REG(0x00c)	/* Half word */
#define SD_CMD_REG			SD_REG(0x00e)	/* Half word */
#define SD_RSP0_REG			SD_REG(0x010)
#define SD_RSP1_REG			SD_REG(0x014)
#define SD_RSP2_REG			SD_REG(0x018)
#define SD_RSP3_REG			SD_REG(0x01c)
#define SD_DATA_REG			SD_REG(0x020)
#define SD_STA_REG			SD_REG(0x024)
#define SD_HOST_REG			SD_REG(0x028)	/* Byte */
#define SD_PWR_REG			SD_REG(0x029)	/* Byte */
#define SD_GAP_REG			SD_REG(0x02a)	/* Byte */
#define SD_WAK_REG			SD_REG(0x02b)	/* Byte */
#define SD_CLK_REG			SD_REG(0x02c)	/* Half word */
#define SD_TMO_REG			SD_REG(0x02e)	/* Byte */
#define SD_RESET_REG			SD_REG(0x02f)	/* Byte */
#define SD_NIS_REG			SD_REG(0x030)	/* Half word */
#define SD_EIS_REG			SD_REG(0x032)	/* Half word */
#define SD_NISEN_REG			SD_REG(0x034)	/* Half word */
#define SD_EISEN_REG			SD_REG(0x036)	/* Half word */
#define SD_NIXEN_REG			SD_REG(0x038)	/* Half word */
#define SD_EIXEN_REG			SD_REG(0x03a)	/* Half word */
#define SD_AC12ES_REG			SD_REG(0x03c)	/* Half word */
#define SD_CAP_REG			SD_REG(0x040)
#define SD_CUR_REG			SD_REG(0x048)
#define SD_XC_CTR_REG			SD_REG(0x060)
#define SD_BOOT_CTR_REG			SD_REG(0x070)
#define SD_BOOT_STA_REG			SD_REG(0x074)
#define SD_VOL_SW_REG			SD_REG(0x07c)
#define SD_SIST_REG			SD_REG(0x0fc)	/* Half word */
#define SD_VER_REG			SD_REG(0x0fe)	/* Half word */

#if (SD_INSTANCES >= 2)

/* SD2 registers */
#define SD2_DMA_ADDR_REG		SD2_REG(0x000)
#define SD2_BLK_SZ_REG			SD2_REG(0x004)	/* Half word */
#define SD2_BLK_CNT_REG			SD2_REG(0x006)	/* Half word */
#define SD2_ARG_REG			SD2_REG(0x008)
#define SD2_XFR_REG			SD2_REG(0x00c)	/* Half word */
#define SD2_CMD_REG			SD2_REG(0x00e)	/* Half word */
#define SD2_RSP0_REG			SD2_REG(0x010)
#define SD2_RSP1_REG			SD2_REG(0x014)
#define SD2_RSP2_REG			SD2_REG(0x018)
#define SD2_RSP3_REG			SD2_REG(0x01c)
#define SD2_DATA_REG			SD2_REG(0x020)
#define SD2_STA_REG			SD2_REG(0x024)
#define SD2_HOST_REG			SD2_REG(0x028)	/* Byte */
#define SD2_PWR_REG			SD2_REG(0x029)	/* Byte */
#define SD2_GAP_REG			SD2_REG(0x02a)	/* Byte */
#define SD2_WAK_REG			SD2_REG(0x02b)	/* Byte */
#define SD2_CLK_REG			SD2_REG(0x02c)	/* Half word */
#define SD2_TMO_REG			SD2_REG(0x02e)	/* Byte */
#define SD2_RESET_REG			SD2_REG(0x02f)	/* Byte */
#define SD2_NIS_REG			SD2_REG(0x030)	/* Half word */
#define SD2_EIS_REG			SD2_REG(0x032)	/* Half word */
#define SD2_NISEN_REG			SD2_REG(0x034)	/* Half word */
#define SD2_EISEN_REG			SD2_REG(0x036)	/* Half word */
#define SD2_NIXEN_REG			SD2_REG(0x038)	/* Half word */
#define SD2_EIXEN_REG			SD2_REG(0x03a)	/* Half word */
#define SD2_AC12ES_REG			SD2_REG(0x03c)	/* Half word */
#define SD2_CAP_REG			SD2_REG(0x040)
#define SD2_CUR_REG			SD2_REG(0x048)
#define SD2_SIST_REG			SD2_REG(0x0fc)	/* Half word */
#define SD2_VER_REG			SD2_REG(0x0fe)	/* Half word */
#endif
/* SD_BLK_SZ_REG */
#define SD_BLK_SZ_4KB			0x0000
#define SD_BLK_SZ_8KB			0x1000
#define SD_BLK_SZ_16KB			0x2000
#define SD_BLK_SZ_32KB			0x3000
#define SD_BLK_SZ_64KB			0x4000
#define SD_BLK_SZ_128KB			0x5000
#define SD_BLK_SZ_256KB			0x6000
#define SD_BLK_SZ_512KB			0x7000

/* SD_XFR_REG */
#define SD_XFR_MUL_SEL			0x0020
#define SD_XFR_SGL_SEL			0x0000
#define SD_XFR_CTH_SEL			0x0010
#define SD_XFR_HTC_SEL			0x0000
#define SD_XFR_AC12_EN			0x0004
#define SD_XFR_BLKCNT_EN		0x0002
#define SD_XFR_DMA_EN			0x0001

/* SD_CMD_REG */
#define SD_CMD_IDX(x)			((x) << 8)
#define SD_CMD_NORMAL			0x00000000
#define SD_CMD_SUSPEND			0x00000040
#define SD_CMD_RESUME			0x00000080
#define SD_CMD_ABORT			0x000000C0
#define SD_CMD_DATA			0x00000020
#define SD_CMD_CHKIDX			0x00000010
#define SD_CMD_CHKCRC			0x00000008
#define SD_CMD_RSP_NONE			0x00000000
#define SD_CMD_RSP_136			0x00000001
#define SD_CMD_RSP_48			0x00000002
#define SD_CMD_RSP_48BUSY		0x00000003

/* SD_STA_REG */
#define SD_STA_DAT_LSL(x)		((((x) & 0x1e000000) >> 25) |	\
					 (((x) & 0x00f00000) >> 20))
#define SD_STA_CMD_LSL(x)		(((x)  & 0x01000000) >> 24)
#define SD_STA_WPS_PL			0x00080000
#define SD_STA_CDP_L			0x00040000
#define SD_STA_CSS			0x00020000
#define SD_STA_CARD_INSERTED		0x00010000
#define SD_STA_BUFFER_READ_EN		0x00000800
#define SD_STA_BUFFER_WRITE_EN		0x00000400
#define SD_STA_READ_XFR_ACTIVE		0x00000200
#define SD_STA_WRITE_XFR_ACTIVE		0x00000100
#define SD_STA_DAT_ACTIVE		0x00000004
#define SD_STA_CMD_INHIBIT_DAT		0x00000002
#define SD_STA_CMD_INHIBIT_CMD		0x00000001

/* SD_HOST_REG */
#define SD_HOST_8BIT			0x08
#define SD_HOST_HIGH_SPEED		0x04
#define SD_HOST_4BIT			0x02
#define SD_HOST_LED_ON			0x01

/* SD_PWR_REG */
#if ((CHIP_REV == A2) || (CHIP_REV == A2S) || (CHIP_REV == A2M) ||	\
     (CHIP_REV == A2Q) || (CHIP_REV == A5) || (CHIP_REV == A5S) ||	\
     (CHIP_REV == A5L) || (CHIP_REV == A7))
#define SD_PWR_3_3V			0x0e
#define SD_PWR_3_0V			0x0c
#define SD_PWR_1_8V			0x0a
#elif ((CHIP_REV == I1) || (CHIP_REV == A7L))
/* SD_PWR_REG only care about bit[3] */
#define SD_PWR_3_3V			0x08
#define SD_PWR_3_0V			0x08
#define SD_PWR_1_8V			0x00
#endif

#define SD_PWR_ON			0x01
#define SD_PWR_OFF			0x00

/* SD_GAP_REG */
#define SD_GAP_INT_AT_GAP		0x08
#define SD_GAP_READ_WAIT		0x04
#define SD_GAP_CONT_REQ			0x02
#define SD_GAP_STOP_AT_GAP		0x01

/* SD_WAK_REG */
#define SD_WAK_ON_CARD_RMV		0x04
#define SD_WAK_ON_CARD_IST		0x02
#define SD_WAK_ON_CARD_INT		0x01

/* SD_CLK_REG */
#define SD_CLK_DIV_256			0x8000
#define SD_CLK_DIV_128			0x4000
#define SD_CLK_DIV_64			0x2000
#define SD_CLK_DIV_32			0x1000
#define SD_CLK_DIV_16			0x0800
#define SD_CLK_DIV_8			0x0400
#define SD_CLK_DIV_4			0x0200
#define SD_CLK_DIV_2			0x0100
#define SD_CLK_DIV_1			0x0000
#define SD_CLK_EN			0x0004
#define SD_CLK_ICLK_STABLE		0x0002
#define SD_CLK_ICLK_EN			0x0001

/* SD_TMO_REG */

/* SD_RESET_REG */
#define SD_RESET_DAT			0x04
#define SD_RESET_CMD			0x02
#define SD_RESET_ALL			0x01

/* SD_NIS_REG */
#define SD_NIS_ERROR			0x8000
#define SD_NIS_CARD			0x0100
#define SD_NIS_REMOVAL			0x0080
#define SD_NIS_INSERT			0x0040
#define SD_NIS_READ_READY		0x0020
#define SD_NIS_WRITE_READY		0x0010
#define SD_NIS_DMA			0x0008
#define SD_NIS_BLOCK_GAP		0x0004
#define SD_NIS_XFR_DONE			0x0002
#define SD_NIS_CMD_DONE			0x0001

/* SD_EIS_REG */
#define SD_EIS_ACMD12_ERR		0x0100
#define SD_EIS_CURRENT_ERR		0x0080
#define SD_EIS_DATA_BIT_ERR		0x0040
#define SD_EIS_DATA_CRC_ERR		0x0020
#define SD_EIS_DATA_TMOUT_ERR		0x0010
#define SD_EIS_CMD_IDX_ERR		0x0008
#define SD_EIS_CMD_BIT_ERR		0x0004
#define SD_EIS_CMD_CRC_ERR		0x0002
#define SD_EIS_CMD_TMOUT_ERR		0x0001

/* SD_NISEN_REG */
#define SD_NISEN_CARD			0x0100
#define SD_NISEN_REMOVAL		0x0080
#define SD_NISEN_INSERT			0x0040
#define SD_NISEN_READ_READY		0x0020
#define SD_NISEN_WRITE_READY		0x0010
#define SD_NISEN_DMA			0x0008
#define SD_NISEN_BLOCK_GAP		0x0004
#define SD_NISEN_XFR_DONE		0x0002
#define SD_NISEN_CMD_DONE		0x0001

/* SD_EISEN_REG */
#define SD_EISEN_ACMD12_ERR		0x0100
#define SD_EISEN_CURRENT_ERR		0x0080
#define SD_EISEN_DATA_BIT_ERR		0x0040
#define SD_EISEN_DATA_CRC_ERR		0x0020
#define SD_EISEN_DATA_TMOUT_ERR		0x0010
#define SD_EISEN_CMD_IDX_ERR		0x0008
#define SD_EISEN_CMD_BIT_ERR		0x0004
#define SD_EISEN_CMD_CRC_ERR		0x0002
#define SD_EISEN_CMD_TMOUT_ERR		0x0001

/* SD_NIXEN_REG */
#define SD_NIXEN_CARD			0x0100
#define SD_NIXEN_REMOVAL		0x0080
#define SD_NIXEN_INSERT			0x0040
#define SD_NIXEN_READ_READY		0x0020
#define SD_NIXEN_WRITE_READY		0x0010
#define SD_NIXEN_DMA			0x0008
#define SD_NIXEN_BLOCK_GAP		0x0004
#define SD_NIXEN_XFR_DONE		0x0002
#define SD_NIXEN_CMD_DONE		0x0001

/* SD_EIXEN_REG */
#define SD_EIXEN_ACMD12_ERR		0x0100
#define SD_EIXEN_CURRENT_ERR		0x0080
#define SD_EIXEN_DATA_BIT_ERR		0x0040
#define SD_EIXEN_DATA_CRC_ERR		0x0020
#define SD_EIXEN_DATA_TMOUT_ERR		0x0010
#define SD_EIXEN_CMD_IDX_ERR		0x0008
#define SD_EIXEN_CMD_BIT_ERR		0x0004
#define SD_EIXEN_CMD_CRC_ERR		0x0002
#define SD_EIXEN_CMD_TMOUT_ERR		0x0001

/* SD_AC12ES_REG */
#if ((CHIP_REV == A2) || (CHIP_REV == A2S) || (CHIP_REV == A2M) || \
     (CHIP_REV == A2Q) || (CHIP_REV == A5L) || (CHIP_REV == A7) || \
     (CHIP_REV == I1) || (CHIP_REV == A7L))
#define SD_AC12ES_NOT_ISSUED		0x0080
#define SD_AC12ES_INDEX			0x0010
#define SD_AC12ES_END_BIT		0x0008
#else
#define SD_AC12ES_NOT_ISSUED		0x0040
#define SD_AC12ES_INDEX			0x0020
#define SD_AC12ES_END_BIT		0x0010
#endif

#define SD_AC12ES_CRC_ERROR		0x0004
#define SD_AC12ES_TMOUT_ERROR		0x0002
#define SD_AC12ES_NOT_EXECED		0x0001

/* SD_CAP_REG */
#define SD_CAP_INTMODE			0x08000000
#define SD_CAP_VOL_1_8V			0x04000000
#define SD_CAP_VOL_3_0V			0x02000000
#define SD_CAP_VOL_3_3V			0x01000000
#define SD_CAP_SUS_RES			0x00800000
#define SD_CAP_DMA			0x00400000
#define SD_CAP_HIGH_SPEED		0x00200000
#define SD_CAP_MAX_512B_BLK		0x00000000
#define SD_CAP_MAX_1KB_BLK		0x00010000
#define SD_CAP_MAX_2KB_BLK		0x00020000
#define SD_CAP_BASE_FREQ(x)		(((x) & 0x3f00) >> 8)
#define SD_CAP_TOCLK_KHZ		0x00000000
#define SD_CAP_TOCLK_MHZ		0x00000080
#define SD_CAP_TOCLK_FREQ(x)		(((x) & 0x3f))

/* SD_XC_CTR_REG */
#define SD_XC_CTR_DDR_EN		0x00008000
#define SD_XC_CTR_VOL_1_8V		0x00000001
#define SD_XC_CTR_VOL_3_3V		0x00000000

/* SD_BOOT_CTR_REG */
#define SD_BOOT_CTR_RST_EN		0x00010000

/* SD_BOOT_STA_REG */
#define SD_BOOT_STA_END_ALT		0x01010000
#define SD_BOOT_STA_BOOT_RDY		0x00000001

/* SD_VOL_SW_REG */
#define SD_VOL_SW_CMD_STAT_H		0x00010000
#define SD_VOL_SW_DAT_STAT_H		0x00000007

/* SD_VER_REG */
#define SD_VER_VENDOR(x)		((x) >> 8)
#define SD_VER_SPEC(x)			((x) & 0xf)

#endif
