/*
 * ambhw/fio.h
 *
 * History:
 *	2007/01/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2008, Ambarella, Inc.
 */

#ifndef __AMBHW__FIO_H__
#define __AMBHW__FIO_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

#if !defined(__KERNEL__)
#include <ambhw/nor_onenand.h>
#endif

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

#if 	(CHIP_REV == A1)

#define CF_NAND_XD_CONCURR_WA			1
#define CF_SUPPORT_ATAPI			0
#define CF_SUPPORT_XFER_UTIL			0
#define CF_SUPPORT_ERR_RECOVERY			0
#define CF_SUPPORT_UDMA				0
#define CF_SUPPORT_80X_SI			0
#define CF_SUPPORT_CD2_INT			1
#define CF_SUPPORT_INTRQ			0
#define NAND_BOOT_WITH_XD_WA			1
#define NAND_XD_SUPPORT_CONT_SPARE_ACCESS	0
#define SD_WRITE_PROTECT_TYPE 			0

#else

#define CF_NAND_XD_CONCURR_WA			0
#define CF_SUPPORT_ATAPI			1
#define CF_SUPPORT_XFER_UTIL			1
#define CF_SUPPORT_ERR_RECOVERY			1
#define CF_SUPPORT_UDMA				1
#define CF_SUPPORT_80X_SI			1
#define CF_SUPPORT_CD2_INT			0
#define CF_SUPPORT_INTRQ			1
#define NAND_BOOT_WITH_XD_WA			0
#define NAND_XD_SUPPORT_CONT_SPARE_ACCESS	1
#define SD_WRITE_PROTECT_TYPE 			1

#endif

#if 	(CHIP_REV == A1) || \
	(CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
	(CHIP_REV == A5S) || (CHIP_REV == A5L) || (CHIP_REV == A7)  || \
	(CHIP_REV == I1) || (CHIP_REV == A7L)
#define FIO_SUPPORT_AHB_CLK_ENA			0
#else
#define FIO_SUPPORT_AHB_CLK_ENA			1
#endif

#if 	(CHIP_REV == A5S) || (CHIP_REV == A5L) || (CHIP_REV == A7) || \
	(CHIP_REV == A7M) || (CHIP_REV == A7L)
#define	FIO_USE_2X_FREQ				1
#else
#define	FIO_USE_2X_FREQ				0
#endif

#if 	(CHIP_REV == A1) || (CHIP_REV == I1)
#define NAND_XD_SUPPORT_WAS			0
#elif 	(CHIP_REV == A2) || (CHIP_REV == A3) ||		\
	(CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) ||	\
	(CHIP_REV == A5S) || (CHIP_REV == A5L) || (CHIP_REV == A7)  ||	\
	(CHIP_REV == A7L)
#define NAND_XD_SUPPORT_WAS			1
#else
#define NAND_XD_SUPPORT_WAS			2
#endif

#if 	(CHIP_REV == A2)  || (CHIP_REV == A2S) || (CHIP_REV == A2M) || 	\
 	(CHIP_REV == A2Q) || (CHIP_REV == A5L)
#define CF_PULL_CTL_POLARITY			1
#else
#define CF_PULL_CTL_POLARITY			0
#endif

#if 	(CHIP_REV == A6) || (CHIP_REV == A5S) || (CHIP_REV == A7) || 	\
	(CHIP_REV == I1) || (CHIP_REV == A7L)
#define CFC_SUPPORT_HW_LBA48			1
#define NAND_SUPPORT_INTLVE			0
#define NAND_SUPPORT_SPARE_BURST_READ		0
#define NAND_SUPPORT_EDC_STATUS_READ		0
#else
#define CFC_SUPPORT_HW_LBA48			0
#define NAND_SUPPORT_INTLVE			0
#define NAND_SUPPORT_SPARE_BURST_READ		0
#define NAND_SUPPORT_EDC_STATUS_READ		0
#endif

#if 	(CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1) || \
	(CHIP_REV == A7L)
#define	NAND_DUMMY_XFER				0
#else
#define	NAND_DUMMY_XFER				0
#endif

#if 	(CHIP_REV == A7M) || (CHIP_REV == I1)
#define	ONENAND_NOR_SUPPORT			1
#else
#define	ONENAND_NOR_SUPPORT			0
#endif

#if 	(CHIP_REV == A7L)
#define	NAND_PROGRAMMABLE_CMDWORD		1
#else
#define	NAND_PROGRAMMABLE_CMDWORD		0
#endif

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

/* ---------------------------------------------------------------------- */

/******************************/
/* Flash I/O Global Registers */
/******************************/

#define FIO_CTR_OFFSET			0x000
#define FIO_STA_OFFSET			0x004
#define FIO_DMACTR_OFFSET		0x080
#define FIO_DMAADR_OFFSET		0x084
#define FIO_DMASTA_OFFSET		0x08c

#define FIO_CTR_REG			FIO_REG(0x000)
#define FIO_STA_REG			FIO_REG(0x004)
#define FIO_DMACTR_REG			FIO_REG(0x080)
#define FIO_DMAADR_REG			FIO_REG(0x084)
#define FIO_DMASTA_REG			FIO_REG(0x08c)

/* FIO_CTR_REG */
#define FIO_CTR_DA			0x00020000
#define FIO_CTR_DR			0x00010000
#define FIO_CTR_SX			0x00000100
#define FIO_CTR_RS			0x00000010
#define FIO_CTR_SE			0x00000008
#define FIO_CTR_CO			0x00000004
#define FIO_CTR_RR			0x00000002
#define FIO_CTR_XD			0x00000001

/* FIO_STA_REG */
#define FIO_STA_SI			0x00000008
#define FIO_STA_CI			0x00000004
#define FIO_STA_XI			0x00000002
#define FIO_STA_FI			0x00000001

/* FIO_DMACTR_REG */
#define FIO_DMACTR_EN			0x80000000
#define FIO_DMACTR_RM			0x40000000
#define FIO_DMACTR_SD			0x30000000
#define FIO_DMACTR_CF			0x20000000
#define FIO_DMACTR_XD			0x10000000
#define FIO_DMACTR_FL			0x00000000
#define FIO_DMACTR_BLK_32768B		0x0c000000
#define FIO_DMACTR_BLK_16384B		0x0b000000
#define FIO_DMACTR_BLK_8192B		0x0a000000
#define FIO_DMACTR_BLK_4096B		0x09000000
#define FIO_DMACTR_BLK_2048B		0x08000000
#define FIO_DMACTR_BLK_1024B		0x07000000
#define FIO_DMACTR_BLK_256B		0x05000000
#define FIO_DMACTR_BLK_512B		0x06000000
#define FIO_DMACTR_BLK_128B		0x04000000
#define FIO_DMACTR_BLK_64B		0x03000000
#define FIO_DMACTR_BLK_32B		0x02000000
#define FIO_DMACTR_BLK_16B		0x01000000
#define FIO_DMACTR_BLK_8B		0x00000000
#define FIO_DMACTR_TS8B			0x00c00000
#define FIO_DMACTR_TS4B			0x00800000
#define FIO_DMACTR_TS2B			0x00400000
#define FIO_DMACTR_TS1B			0x00000000

/* FIO_DMASTA_REG */
#define FIO_DMASTA_RE			0x04000000
#define FIO_DMASTA_AE			0x02000000
#define FIO_DMASTA_DN			0x01000000

/* ---------------------------------------------------------------------- */

/******************************/
/* Flash Controller Registers */
/******************************/

#define FLASH_CTR_OFFSET		0x120
#define FLASH_CMD_OFFSET		0x124
#define FLASH_TIM0_OFFSET		0x128
#define FLASH_TIM1_OFFSET		0x12c
#define FLASH_TIM2_OFFSET		0x130
#define FLASH_TIM3_OFFSET		0x134
#define FLASH_TIM4_OFFSET		0x138
#define FLASH_TIM5_OFFSET		0x13c
#define FLASH_STA_OFFSET		0x140
#define FLASH_ID_OFFSET			0x144
#define FLASH_CFI_OFFSET		0x148
#define FLASH_LEN_OFFSET		0x14c
#define FLASH_INT_OFFSET		0x150

#define FLASH_CTR_REG			FIO_REG(0x120)
#define FLASH_CMD_REG			FIO_REG(0x124)
#define FLASH_TIM0_REG			FIO_REG(0x128)
#define FLASH_TIM1_REG			FIO_REG(0x12c)
#define FLASH_TIM2_REG			FIO_REG(0x130)
#define FLASH_TIM3_REG			FIO_REG(0x134)
#define FLASH_TIM4_REG			FIO_REG(0x138)
#define FLASH_TIM5_REG			FIO_REG(0x13c)
#define FLASH_STA_REG			FIO_REG(0x140)
#define FLASH_ID_REG			FIO_REG(0x144)
#define FLASH_CFI_REG			FIO_REG(0x148)
#define FLASH_LEN_REG			FIO_REG(0x14c)
#define FLASH_INT_REG			FIO_REG(0x150)

#define NAND_CTR_REG			FLASH_CTR_REG
#define NAND_CMD_REG			FLASH_CMD_REG
#define NAND_TIM0_REG			FLASH_TIM0_REG
#define NAND_TIM1_REG			FLASH_TIM1_REG
#define NAND_TIM2_REG			FLASH_TIM2_REG
#define NAND_TIM3_REG			FLASH_TIM3_REG
#define NAND_TIM4_REG			FLASH_TIM4_REG
#define NAND_TIM5_REG			FLASH_TIM5_REG
#define NAND_STA_REG			FLASH_STA_REG
#define NAND_ID_REG			FLASH_ID_REG
#define NAND_COPY_ADDR_REG		FLASH_CFI_REG
#define NAND_LEN_REG			FLASH_LEN_REG
#define NAND_INT_REG			FLASH_INT_REG

/* For chips to support programable command word. */
#define NAND_READ_CMDWORD_REG		FIO_REG(0x154)
#define NAND_PROG_CMDWORD_REG		FIO_REG(0x158)
#define NAND_CMDWORD1(x)		((x) << 16)
#define NAND_CMDWORD2(x)		((x) & 0xff)


#define NAND_CPS1_ADDR_REG		FIO_REG(0x154)
#define NAND_CPD1_ADDR_REG		FIO_REG(0x158)
#define NAND_CPS2_ADDR_REG		FIO_REG(0x15c)
#define NAND_CPD2_ADDR_REG		FIO_REG(0x160)
#define NAND_CPS3_ADDR_REG		FIO_REG(0x164)
#define NAND_CPD3_ADDR_REG		FIO_REG(0x168)
#define NAND_NBR_SPA_REG		FIO_REG(0x16c)

#define NOR_CTR_REG			FLASH_CTR_REG
#define NOR_CMD_REG			FLASH_CMD_REG
#define NOR_TIM0_REG			FLASH_TIM0_REG
#define NOR_TIM1_REG			FLASH_TIM1_REG
#define NOR_TIM2_REG			FLASH_TIM2_REG
#define NOR_TIM3_REG			FLASH_TIM3_REG
#define NOR_TIM4_REG			FLASH_TIM4_REG
#define NOR_TIM5_REG			FLASH_TIM5_REG
#define NOR_STA_REG			FLASH_STA_REG
#define NOR_ID_REG			FLASH_ID_REG
#define NOR_CFI_REG			FLASH_CFI_REG
#define NOR_LEN_REG			FLASH_LEN_REG
#define NOR_INT_REG			FLASH_INT_REG

/* FLASH_CTR_REG (NAND mode) */
#define NAND_CTR_A(x)			((x) << 28)
#define NAND_CTR_SA			0x08000000
#define NAND_CTR_SE			0x04000000
#define NAND_CTR_C2			0x02000000
#define NAND_CTR_P3			0x01000000
#define NAND_CTR_I4			0x00800000
#define NAND_CTR_RC			0x00400000
#define NAND_CTR_CC			0x00200000
#define NAND_CTR_CE			0x00100000
#define NAND_CTR_EC_MAIN		0x00080000
#define NAND_CTR_EC_SPARE		0x00040000
#define NAND_CTR_EG_MAIN		0x00020000
#define NAND_CTR_EG_SPARE		0x00010000
#define NAND_CTR_WP			0x00000200
#define NAND_CTR_IE			0x00000100
#define NAND_CTR_XS			0x00000080
#define NAND_CTR_SZ_8G			0x00000070
#define NAND_CTR_SZ_4G			0x00000060
#define NAND_CTR_SZ_2G			0x00000050
#define NAND_CTR_SZ_1G			0x00000040
#define NAND_CTR_SZ_512M		0x00000030
#define NAND_CTR_SZ_256M		0x00000020
#define NAND_CTR_SZ_128M		0x00000010
#define NAND_CTR_SZ_64M			0x00000000
#define NAND_CTR_4BANK			0x00000008
#define NAND_CTR_2BANK			0x00000004
#define NAND_CTR_1BANK			0x00000000
#define NAND_CTR_WD_64BIT		0x00000003
#define NAND_CTR_WD_32BIT		0x00000002
#define NAND_CTR_WD_16BIT		0x00000001
#define NAND_CTR_WD_8BIT		0x00000000

#define NAND_CTR_INTLVE			0x80000000
#define NAND_CTR_SPRBURST		0x40000000
#define NAND_CFG_STAT_ENB		0x00002000
#define NAND_CTR_2INTL			0x00001000
#define NAND_CTR_K9			0x00000800

#define NAND_CTR_WAS			0x00000400

/* FLASH_CMD_REG (NAND mode) */
#if defined(__KERNEL__)
#define NAND_AMB_CMD_ADDR(x)		((x) << 4)
#define NAND_AMB_CMD_NOP		0x0
#define NAND_AMB_CMD_DMA		0x1
#define NAND_AMB_CMD_RESET		0x2
#define NAND_AMB_CMD_NOP2		0x3
#define NAND_AMB_CMD_NOP3		0x4
#define NAND_AMB_CMD_NOP4		0x5
#define NAND_AMB_CMD_NOP5		0x6
#define NAND_AMB_CMD_COPYBACK		0x7
#define NAND_AMB_CMD_NOP6		0x8
#define NAND_AMB_CMD_ERASE		0x9
#define NAND_AMB_CMD_READID		0xa
#define NAND_AMB_CMD_NOP7		0xb
#define NAND_AMB_CMD_READSTATUS		0xc
#define NAND_AMB_CMD_NOP8		0xd
#define NAND_AMB_CMD_READ		0xe
#define NAND_AMB_CMD_PROGRAM		0xf
#else
#define NAND_CMD_ADDR(x)		((x) << 4)
#define NAND_CMD_NOP			0x0
#define NAND_CMD_DMA			0x1
#define NAND_CMD_RESET			0x2
#define NAND_CMD_NOP2			0x3
#define NAND_CMD_NOP3			0x4
#define NAND_CMD_NOP4			0x5
#define NAND_CMD_NOP5			0x6
#define NAND_CMD_COPYBACK		0x7
#define NAND_CMD_NOP6			0x8
#define NAND_CMD_ERASE			0x9
#define NAND_CMD_READID			0xa
#define NAND_CMD_NOP7			0xb
#define NAND_CMD_READSTATUS		0xc
#define NAND_CMD_NOP8			0xd
#define NAND_CMD_READ			0xe
#define NAND_CMD_PROGRAM		0xf
#endif

/* FLASH_TIM0_REG (NAND mode) */
#define NAND_TIM0_TCLS(x)		((x) << 24)
#define NAND_TIM0_TALS(x)		((x) << 16)
#define NAND_TIM0_TCS(x)		((x) << 8)
#define NAND_TIM0_TDS(x)		(x)

/* FLASH_TIM1_REG (NAND mode) */
#define NAND_TIM1_TCLH(x)		((x) << 24)
#define NAND_TIM1_TALH(x)		((x) << 16)
#define NAND_TIM1_TCH(x)		((x) << 8)
#define NAND_TIM1_TDH(x)		(x)

/* FLASH_TIM2_REG (NAND mode) */
#define NAND_TIM2_TWP(x)		((x) << 24)
#define NAND_TIM2_TWH(x)		((x) << 16)
#define NAND_TIM2_TWB(x)		((x) << 8)
#define NAND_TIM2_TRR(x)		(x)

/* FLASH_TIM3_REG (NAND mode) */
#define NAND_TIM3_TRP(x)		((x) << 24)
#define NAND_TIM3_TREH(x)		((x) << 16)
#define NAND_TIM3_TRB(x)		((x) << 8)
#define NAND_TIM3_TCEH(x)		(x)

/* FLASH_TIM4_REG (NAND mode) */
#define NAND_TIM4_TRDELAY(x)		((x) << 24)
#define NAND_TIM4_TCLR(x)		((x) << 16)
#define NAND_TIM4_TWHR(x)		((x) << 8)
#define NAND_TIM4_TIR(x)		(x)

#if (NAND_SUPPORT_INTLVE == 0)
/* FLASH_TIM5_REG (NAND mode) */
#define NAND_TIM5_TWW(x)		((x) << 16)
#define NAND_TIM5_TRHZ(x)		((x) << 8)
#define NAND_TIM5_TAR(x)		(x)
#else
/* FLASH_TIM5_REG (NAND mode) */
#define NAND_TIM5_TDBSY(x)		((x & 0x3ff) << 22)
#define NAND_TIM5_TWW(x)		((x & 0x3f) << 16)
#define NAND_TIM5_TRHZ(x)		((x) << 8)
#define NAND_TIM5_TAR(x)		(x)
#endif

/* FLASH_INT_REG (NAND mode) */
#define NAND_INT_DI			0x1

/* NOR mode definitions */

/* FLASH_CTR_REG (NOR mode) */
#define NOR_CTR_BRTY(x)			((x) << 28)
#define NOR_CTR_BS			0x02000000
#define NOR_CTR_BW			0x01000000
#define NOR_CTR_CE			0x00200000
#define NOR_CTR_BR			0x00100000
#define NOR_CTR_LE			0x00040000
#define NOR_CTR_LD			0x00020000
#define NOR_CTR_WP			0x00010000
#define NOR_CTR_BZ_32KB			0x00008000
#define NOR_CTR_BZ_16KB			0x00004000
#define NOR_CTR_BZ_8KB			0x00000000
#define NOR_CTR_TB			0x00002000
#define NOR_CTR_BB			0x00001000
#define NOR_CTR_KZ_64KB			0x00000000
#define NOR_CTR_KZ_128KB		0x00000800
#define NOR_CTR_IE			0x00000100
#define NOR_CTR_XS			0x00000080
#define NOR_CTR_SZ_512M			0x00000070
#define NOR_CTR_SZ_256M			0x00000060
#define NOR_CTR_SZ_128M			0x00000050
#define NOR_CTR_SZ_64M			0x00000040
#define NOR_CTR_SZ_32M			0x00000030
#define NOR_CTR_SZ_16M			0x00000020
#define NOR_CTR_SZ_8M			0x00000010
#define NOR_CTR_SZ_4M			0x00000000
#define NOR_CTR_4BANK			0x00000008
#define NOR_CTR_2BANK			0x00000004
#define NOR_CTR_1BANK			0x00000000
#define NOR_CTR_WD_64BIT		0x00000003
#define NOR_CTR_WD_32BIT		0x00000002
#define NOR_CTR_WD_16BIT		0x00000001
#define NOR_CTR_WD_8BIT			0x00000000

/* FLASH_CMD_REG (NOR mode) */
#define NOR_CMD_ADDR(x)			((x) << 4)
#define NOR_CMD_NOP			0x0
#define NOR_CMD_DMA			0x1
#define NOR_CMD_RESET			0x2
#define NOR_CMD_NOP2			0x3
#define NOR_CMD_LOCK			0x4
#define NOR_CMD_UNLOCK			0x5
#define NOR_CMD_LOCKDOWN		0x6
#define NOR_CMD_NOP9			0x7
#define NOR_CMD_READCFI			0x8
#define NOR_CMD_ERASE			0x9
#define NOR_CMD_READID			0xa
#define NOR_CMD_NOP7			0xb
#define NOR_CMD_READSTATUS		0xc
#define NOR_CMD_CLEARSTATUS		0xd
#define NOR_CMD_READ			0xe
#define NOR_CMD_PROGRAM			0xf

/* FLASH_TIM0_REG (NOR mode) */
#define NOR_TIM0_TAS(x)			((x) << 16)
#define NOR_TIM0_TCS(x)			((x) << 8)
#define NOR_TIM0_TDS(x)			(x)

/* FLASH_TIM1_REG (NOR mode) */
#define NOR_TIM1_TAH(x)			((x) << 16)
#define NOR_TIM1_TCH(x)			((x) << 8)
#define NOR_TIM1_TDH(x)			(x)

/* FLASH_TIM2_REG (NOR mode) */
#define NOR_TIM2_TWP(x)			((x) << 24)
#define NOR_TIM2_TWH(x)			((x) << 16)

/* FLASH_TIM3_REG (NOR mode) */
#define NOR_TIM3_TRC(x)			((x) << 24)
#define NOR_TIM3_TADELAY(x)		((x) << 16)
#define NOR_TIM3_TCDELAY(x)		((x) << 8)
#define NOR_TIM3_TRPDELAY(x)		(x)

/* FLASH_TIM4_REG (NOR mode) */
#define NOR_TIM4_TRDELAY(x)		((x) << 24)
#define NOR_TIM4_TABDELAY(x)		((x) << 16)
#define NOR_TIM4_TWHR(x)		((x) << 8)
#define NOR_TIM4_TIR(x)			(x)

/* FLASH_TIM5_REG (NOR mode) */
#define NOR_TIM5_TRHZ(x)		((x) << 8)
#define NOR_TIM5_TRPH(x)		(x)

/* FLASH_INT_REG (NOR mode) */
#define NOR_INT_DI			0x1

/* ---------------------------------------------------------------------- */
/***************************/
/* XD Controller Registers */
/***************************/

#define XD_CTR_OFFSET			0x1a0
#define XD_CMD_OFFSET			0x1a4
#define XD_TIM0_OFFSET			0x1a8
#define XD_TIM1_OFFSET			0x1ac
#define XD_TIM2_OFFSET			0x1b0
#define XD_TIM3_OFFSET			0x1b4
#define XD_TIM4_OFFSET			0x1b8
#define XD_TIM5_OFFSET			0x1bc
#define XD_STA_OFFSET			0x1c0
#define XD_ID_OFFSET			0x1c4
#define XD_ID2_OFFSET			0x1c8
#define XD_LEN_OFFSET			0x1cc
#define XD_INT_OFFSET			0x1d0

#define XD_CTR_REG			XD_REG(0x1a0)
#define XD_CMD_REG			XD_REG(0x1a4)
#define XD_TIM0_REG			XD_REG(0x1a8)
#define XD_TIM1_REG			XD_REG(0x1ac)
#define XD_TIM2_REG			XD_REG(0x1b0)
#define XD_TIM3_REG			XD_REG(0x1b4)
#define XD_TIM4_REG			XD_REG(0x1b8)
#define XD_TIM5_REG			XD_REG(0x1bc)
#define XD_STA_REG			XD_REG(0x1c0)
#define XD_ID_REG			XD_REG(0x1c4)
#define XD_ID2_REG			XD_REG(0x1c8)
#define XD_LEN_REG			XD_REG(0x1cc)
#define XD_INT_REG			XD_REG(0x1d0)

/* XD_CTL_REG */
#define XD_CTR_ADDR(x)			((x) << 24)
#define XD_CTR_SA			0x00800000
#define XD_CTR_SE			0x00400000
#define XD_CTR_C2			0x00200000
#define XD_CTR_P3			0x00100000
#define XD_CTR_EC			0x00080000
#define XD_CTR_EG			0x00020000
#define XD_CTR_IE			0x00000100
#define XD_CTR_SZ_8GB			0x00000090
#define XD_CTR_SZ_4GB			0x00000080
#define XD_CTR_SZ_2GB			0x00000070
#define XD_CTR_SZ_1GB			0x00000060
#define XD_CTR_SZ_512MB			0x00000050
#define XD_CTR_SZ_256MB			0x00000040
#define XD_CTR_SZ_128MB			0x00000030
#define XD_CTR_SZ_64MB			0x00000020
#define XD_CTR_SZ_32MB			0x00000010
#define XD_CTR_SZ_16MB			0x00000000
#define XD_CTR_4BANK			0x00000080
#define XD_CTR_2BANK			0x00000040
#define XD_CTR_1BANK			0x00000000
#define XD_CTR_WD_32BIT			0x00000002
#define XD_CTR_WD_16BIT			0x00000001
#define XD_CTR_WD_8BIT			0x00000000

#define XD_CTR_WAS			0x00000400

/* XD_CMD_REG */
#define XD_CMD_VAL(cmd,addr)		(((cmd) & 0xf)| (addr) << 4)
#define XD_CMD_NOP			0x0
#define XD_CMD_DMA			0x1
#define XD_CMD_RESET			0x2
#define XD_CMD_READ_ID2			0x3
#define XD_CMD_READ_ID3			0x4
#define XD_CMD_READ_STA2		0x5
#define XD_CMD_NOP2			0x6
#define XD_CMD_NOP3			0x7
#define XD_CMD_NOP4			0x8
#define XD_CMD_ERASE			0x9
#define XD_CMD_READ_ID			0xa
#define XD_CMD_NOP5			0xb
#define XD_CMD_READ_STATUS		0xc
#define XD_CMD_NOP6			0xd
#define XD_CMD_READ			0xe
#define XD_CMD_PROGRAM			0xf

/* XD_TIM0_REG */
#define XD_TIM0_TCLS(x)			((x) << 24)
#define XD_TIM0_TALS(x)			((x) << 16)
#define XD_TIM0_TCS(x)			((x) << 8)
#define XD_TIM0_TDS(x)			(x)

/* XD_TIM1_REG */
#define XD_TIM1_TCLH(x)			((x) << 24)
#define XD_TIM1_TALH(x)			((x) << 16)
#define XD_TIM1_TCH(x)			((x) << 8)
#define XD_TIM1_TDH(x)			(x)

/* XD_TIM2_REG */
#define XD_TIM2_TWP(x)			((x) << 24)
#define XD_TIM2_TWH(x)			((x) << 16)
#define XD_TIM2_TWB(x)			((x) << 8)
#define XD_TIM2_TRR(x)			(x)

/* XD_TIM3_REG */
#define XD_TIM3_TRP(x)			((x) << 24)
#define XD_TIM3_TREH(x)			((x) << 16)
#define XD_TIM3_TRB(x)			((x) << 8)
#define XD_TIM3_TCEH(x)			(x)

/* XD_TIM4_REG */
#define XD_TIM4_TRDELAY(x)		((x) << 24)
#define XD_TIM4_TCLR(x)			((x) << 16)
#define XD_TIM4_TWHR(x)			((x) << 8)
#define XD_TIM4_TIR(x)			(x)

/* XD_TIM5_REG */
#define XD_TIM5_TWW(x)			((x) << 16)
#define XD_TIM5_TRHZ(x)			((x) << 8)
#define XD_TIM5_TAR(x)			(x)

/* XD_INT_REG */
#define XD_INT_DI			0x1

/* ---------------------------------------------------------------------- */

/***************************/
/* CF Controller Registers */
/***************************/

#define CF_CTR_OFFSET			0x200
#define CF_CMD_OFFSET			0x204
#define CF_RD_TIM0_OFFSET		0x208
#define CF_RD_TIM1_OFFSET		0x20c
#define CF_RD_TIM2_OFFSET		0x210
#define CF_RD_TIM3_OFFSET		0x214
#define CF_RD_TIM4_OFFSET		0x218
#define CF_RD_TIM5_OFFSET		0x21c
#define CF_WR_TIM0_OFFSET		0x220
#define CF_WR_TIM1_OFFSET		0x224
#define CF_WR_TIM2_OFFSET		0x228
#define CF_WR_TIM3_OFFSET		0x22c
#define CF_WR_TIM4_OFFSET		0x230
#define CF_WR_TIM5_OFFSET		0x234
#define CF_CM_TIM0_OFFSET		0x238
#define CF_CM_TIM1_OFFSET		0x23c
#define CF_CM_TIM2_OFFSET		0x240
#define CF_CM_TIM3_OFFSET		0x244
#define CF_CM_TIM4_OFFSET		0x248
#define CF_STA_OFFSET			0x24c

#define CF_TRERR_OFFSET			0x250
#define CF_STPLP_OFFSET			0x254
#define CF_PKTCTL_OFFSET		0x260
#define CF_PKTTLEN_OFFSET		0x264
#define CF_TUTMOUT_OFFSET		0x270
#define CF_TUDMA_OFFSET			0x274

#define CF_CFC_SEC_CNT_HB_OFFSET	0x27c

#define CF_CFC_DAT_OFFSET		0x280
#define CF_CFC_FEA_OFFSET		0x284
#define CF_CFC_SEC_CNT_OFFSET		0x288
#define CF_CFC_SEC_NUM_OFFSET		0x28c
#define CF_CFC_CYL_LO_OFFSET		0x290
#define CF_CFC_CYL_HI_OFFSET		0x294
#define CF_CFC_DRV_HEAD_OFFSET		0x298
#define CF_CFC_CMD_OFFSET		0x29c
#define CF_CFC_DEV_CTR_OFFSET		0x2a0

#define CF_CFC_SEC_NUM_HB_OFFSET	0x2a4
#define CF_CFC_CYL_LO_HB_OFFSET		0x2a8
#define CF_CFC_CYL_HI_HB_OFFSET		0x2ac

#define CF_PKTCMD0_OFFSET		0x2b0
#define CF_PKTCMD1_OFFSET		0x2b4
#define CF_PKTCMD2_OFFSET		0x2b8
#define CF_PKTCMD3_OFFSET		0x2bc

#define CF_CFC_DAT_OUT_OFFSET		0x2c0
#define CF_CFC_ERR_OUT_OFFSET		0x2c4
#define CF_CFC_SEC_CNT_OUT_OFFSET	0x2c8
#define CF_CFC_SEC_NUM_OUT_OFFSET	0x2cc
#define CF_CFC_CYL_LO_OUT_OFFSET	0x2d0
#define CF_CFC_CYL_HI_OUT_OFFSET	0x2d4
#define CF_CFC_DRV_HEAD_OUT_OFFSET	0x2d8
#define CF_CFC_STA_OUT_OFFSET		0x2dc
#define CF_CFC_ALT_STA_OUT_OFFSET	0x2e0
#define CF_CFC_DRV_ADD_OUT_OFFSET	0x2e4

#define CF_CFSTAT_OFFSET		0x2f0
#define CF_CFLNTH_OFFSET		0x2f4
#define CF_RSVOPT_OFFSET		0x2fc

#define CF_CTR_REG			CF_REG(0x200)
#define CF_CMD_REG			CF_REG(0x204)
#define CF_RD_TIM0_REG			CF_REG(0x208)
#define CF_RD_TIM1_REG			CF_REG(0x20c)
#define CF_RD_TIM2_REG			CF_REG(0x210)
#define CF_RD_TIM3_REG			CF_REG(0x214)
#define CF_RD_TIM4_REG			CF_REG(0x218)
#define CF_RD_TIM5_REG			CF_REG(0x21c)
#define CF_WR_TIM0_REG			CF_REG(0x220)
#define CF_WR_TIM1_REG			CF_REG(0x224)
#define CF_WR_TIM2_REG			CF_REG(0x228)
#define CF_WR_TIM3_REG			CF_REG(0x22c)
#define CF_WR_TIM4_REG			CF_REG(0x230)
#define CF_WR_TIM5_REG			CF_REG(0x234)
#define CF_CM_TIM0_REG			CF_REG(0x238)
#define CF_CM_TIM1_REG			CF_REG(0x23c)
#define CF_CM_TIM2_REG			CF_REG(0x240)
#define CF_CM_TIM3_REG			CF_REG(0x244)
#define CF_CM_TIM4_REG			CF_REG(0x248)
#define CF_STA_REG			CF_REG(0x24c)

#define CF_TRERR_REG			CF_REG(0x250)
#define CF_STPLP_REG			CF_REG(0x254)
#define CF_PKTCTL_REG			CF_REG(0x260)
#define CF_PKTTLEN_REG			CF_REG(0x264)
#define CF_TUTMOUT_REG			CF_REG(0x270)
#define CF_TUDMA_REG			CF_REG(0x274)

#define CF_CFC_SEC_CNT_HB_REG		CF_REG(0x27c)

#define CF_CFC_DAT_REG			CF_REG(0x280)
#define CF_CFC_FEA_REG			CF_REG(0x284)
#define CF_CFC_SEC_CNT_REG		CF_REG(0x288)
#define CF_CFC_SEC_NUM_REG		CF_REG(0x28c)
#define CF_CFC_CYL_LO_REG		CF_REG(0x290)
#define CF_CFC_CYL_HI_REG		CF_REG(0x294)
#define CF_CFC_DRV_HEAD_REG		CF_REG(0x298)
#define CF_CFC_CMD_REG			CF_REG(0x29c)
#define CF_CFC_DEV_CTR_REG		CF_REG(0x2a0)

#define CF_CFC_SEC_NUM_HB_REG		CF_REG(0x2a4)
#define CF_CFC_CYL_LO_HB_REG		CF_REG(0x2a8)
#define CF_CFC_CYL_HI_HB_REG		CF_REG(0x2ac)

#define CF_PKTCMD0_REG			CF_REG(0x2b0)
#define CF_PKTCMD1_REG			CF_REG(0x2b4)
#define CF_PKTCMD2_REG			CF_REG(0x2b8)
#define CF_PKTCMD3_REG			CF_REG(0x2bc)

#define CF_CFC_DAT_OUT_REG		CF_REG(0x2c0)
#define CF_CFC_ERR_OUT_REG		CF_REG(0x2c4)
#define CF_CFC_SEC_CNT_OUT_REG		CF_REG(0x2c8)
#define CF_CFC_SEC_NUM_OUT_REG		CF_REG(0x2cc)
#define CF_CFC_CYL_LO_OUT_REG		CF_REG(0x2d0)
#define CF_CFC_CYL_HI_OUT_REG		CF_REG(0x2d4)
#define CF_CFC_DRV_HEAD_OUT_REG		CF_REG(0x2d8)
#define CF_CFC_STA_OUT_REG		CF_REG(0x2dc)
#define CF_CFC_ALT_STA_OUT_REG		CF_REG(0x2e0)
#define CF_CFC_DRV_ADD_OUT_REG		CF_REG(0x2e4)

#define CF_CFSTAT_REG			CF_REG(0x2f0)
#define CF_CFLNTH_REG			CF_REG(0x2f4)
#define CF_RSVOPT_REG			CF_REG(0x2fc)

/* CF_CTR_REG */
#define CF_CTR_CE			0x00200000
#define CF_CTR_DE			0x00100000
#define CF_CTR_RI			0x00020000
#define CF_CTR_SI			0x00010000
#define CF_CTR_WC			0x00008000
#define CF_CTR_RC			0x00004000
#define CF_CTR_PC_SEC_IO		0x00003000
#define CF_CTR_PC_PRI_IO		0x00002000
#define CF_CTR_PC_CONT_IO		0x00001000
#define CF_CTR_PC_MEM			0x00000000
#define CF_CTR_16b_IDE_MODE		0x00000500
#define CF_CTR_8b_IDE_MODE		0x00000400
#define CF_CTR_16b_IO_MODE		0x00000300
#define CF_CTR_8b_IO_MODE		0x00000200
#define CF_CTR_16b_MEM_MODE		0x00000100
#define CF_CTR_8b_MEM_MODE		0x00000000
#define CF_CTR_SZ_MASK			0x000000ff

#define CF_CTR_CONTPCA			0x20000000
#define CF_CTR_80X			0x10000000
#define CF_CTR_IE			0x00800000
#define CF_CTR_PE			0x00400000
#define CF_CTR_16b_UDMA_MODE		0x00000700

/* CF_CMD_REG */
#define CF_CMD_ADDR(x)			(((x) & 0x7ff) << 16)
#define CF_CMD_CMD(x)			((x) & 0x1ff)
#define CF_CMD_LEN(x)			((((x) & 0x07f) << 9) |	\
					 (((x) & 0xf80) << 20))
#define CF_CMD_NOP				0x000
#define CF_CMD_RESET				0x001
#define CF_CMD_READ_DATA			0x020
#define CF_CMD_READ_ERROR			0x021
#define CF_CMD_READ_SECTOR_COUNT		0x022
#define CF_CMD_READ_SECTOR_NUMBER		0x023
#define CF_CMD_READ_CYLINDER_LOW		0x024
#define CF_CMD_READ_CYLINDER_HIGH		0x025
#define CF_CMD_READ_DRIVE_HEAD			0x026
#define CF_CMD_READ_STATUS			0x027
#define CF_CMD_READ_ALTERNATE_STATUS		0x028
#define CF_CMD_READ_DRIVE_ADDRESS		0x029
#define CF_CMD_POLL_STATUS			0x02a
#define CF_CMD_WRITE_DATA			0x030
#define CF_CMD_WRITE_FEATURE			0x031
#define CF_CMD_WRITE_SECTOR_COUNT		0x032
#define CF_CMD_WRITE_SECTOR_NUMBER		0x033
#define CF_CMD_WRITE_CYLINDER_LOW		0x034
#define CF_CMD_WRITE_CYLINDER_HIGH		0x035
#define CF_CMD_WRITE_DRIVE_HEAD			0x036
#define CF_CMD_WRITE_COMMAND			0x037
#define CF_CMD_WRITE_DEVICE_CONTROL		0x038
#define CF_CMD_READ_ATTRIBUTE_MEMORY		0x040
#define CF_CMD_READ_COMMON_MEMORY_BYTE		0x041
#define CF_CMD_READ_IO_BYTE			0x042
#define CF_CMD_READ_COMMON_MEMORY_HWORD		0x045
#define CF_CMD_READ_IO_HWORD			0x046
#define CF_CMD_WRITE_ATTRIBUTE_MEMORY		0x050
#define CF_CMD_WRITE_COMMON_MEMORY_BYTE		0x051
#define CF_CMD_WRITE_IO_BYTE			0x052
#define CF_CMD_WRITE_COMMON_MEMORY_HWORD	0x055
#define CF_CMD_WRITE_IO_HWORD			0x056
#define CF_CMD_CFC_NOP				0x100
#define CF_CMD_CFC_REQUEST_SENSE		0x103

#define CF_CMD_CFC_READ_SECTOR			0x120
#define CF_CMD_CFC_READ_SECTOR_2		0x121
#define CF_CMD_CFC_READ_LONG_SECTOR		0x122
#define CF_CMD_CFC_READ_LONG_SECTOR_2		0x123
#define CF_CMD_CFC_WRITE_SECTOR			0x130
#define CF_CMD_CFC_WRITE_SECTOR_2		0x131
#define CF_CMD_CFC_WRITE_LONG_SECTOR		0x132
#define CF_CMD_CFC_WRITE_LONG_SECTOR_2		0x133
#define CF_CMD_CFC_WRITE_SECTOR_WO_ERASE	0x138
#define CF_CMD_CFC_WRITE_VERIFY			0x13c
#define CF_CMD_CFC_READ_VERIFY			0x140
#define CF_CMD_CFC_READ_VERIFY_2		0x141
#define CF_CMD_CFC_FORMAT_TRACK			0x150
#define CF_CMD_CFC_SEEK				0x170
#define CF_CMD_CFC_SEEK_2			0x171
#define CF_CMD_CFC_SEEK_3			0x172
#define CF_CMD_CFC_SEEK_4			0x173
#define CF_CMD_CFC_SEEK_5			0x174
#define CF_CMD_CFC_SEEK_6			0x175
#define CF_CMD_CFC_SEEK_7			0x176
#define CF_CMD_CFC_SEEK_8			0x177
#define CF_CMD_CFC_SEEK_9			0x178
#define CF_CMD_CFC_SEEK_10			0x179
#define CF_CMD_CFC_SEEK_11			0x17a
#define CF_CMD_CFC_SEEK_12			0x17b
#define CF_CMD_CFC_SEEK_13			0x17c
#define CF_CMD_CFC_SEEK_14			0x17d
#define CF_CMD_CFC_SEEK_15			0x17e
#define CF_CMD_CFC_SEEK_16			0x17f
#define CF_CMD_CFC_TRANSLATE_SECTOR		0x187
#define CF_CMD_CFC_EXECUTE_DIAGNOSTIC		0x190
#define CF_CMD_CFC_INITIALIZE_PARAMS		0x191
#define CF_CMD_CFC_STANDBY_IMMEDIATE		0x194
#define CF_CMD_CFC_IDLE_IMMEDIATE		0x195
#define CF_CMD_CFC_STANDBY			0x196
#define CF_CMD_CFC_IDLE				0x197
#define CF_CMD_CFC_CHECK_POWER			0x198
#define CF_CMD_CFC_SLEEP			0x199
#define CF_CMD_CFC_KEY_MANAGEMENT		0x1b9
#define CF_CMD_CFC_ERASE_SECTOR			0x1c0
#define CF_CMD_CFC_READ_MULTIPLE		0x1c4
#define CF_CMD_CFC_WRITE_MULTIPLE		0x1c5
#define CF_CMD_CFC_SET_MULTIPLE			0x1c6
#define CF_CMD_CFC_READ_DMA			0x1c8
#define CF_CMD_CFC_READ_DMA_2			0x1c9
#define CF_CMD_CFC_WRITE_DMA			0x1ca
#define CF_CMD_CFC_WRITE_DMA_2			0x1cb
#define CF_CMD_CFC_WRITE_MULTIPLE_WO_ERASE	0x1cd
#define CF_CMD_CFC_STANDBY_IMMEDIATE_2		0x1e0
#define CF_CMD_CFC_IDLE_IMMEDIATE_2		0x1e1
#define CF_CMD_CFC_STANDBY_2			0x1e2
#define CF_CMD_CFC_IDLE_2			0x1e3
#define CF_CMD_CFC_READ_BUFFER			0x1e4
#define CF_CMD_CFC_CHECK_POWER_2		0x1e5
#define CF_CMD_CFC_SLEEP_2			0x1e6
#define CF_CMD_CFC_FLUSH_CACHE			0x1e7
#define CF_CMD_CFC_WRITE_BUFFER			0x1e8
#define CF_CMD_CFC_IDENTIFY_DEVICE		0x1ec
#define CF_CMD_CFC_SET_FEATURES			0x1ef
#define CF_CMD_CFC_SECURITY_SET_PASSWD		0x1f1
#define CF_CMD_CFC_SECURITY_UNLOCK		0x1f2
#define CF_CMD_CFC_SECURITY_ERASE_PREPARE	0x1f3
#define CF_CMD_CFC_SECURITY_ERASE_UNIT		0x1f4
#define CF_CMD_CFC_SECURITY_FREEZE_LOCK		0x1f5
#define CF_CMD_CFC_WEAR_LEVEL			0x1f5
#define CF_CMD_CFC_SECURITY_DISABLE_PASSWD	0x1f6

/* Below are entries for LBA48 mode */
#define CF_CMD_CFC_FLUSH_CACHE_EXT		0x1ea
#define CF_CMD_CFC_READ_DMA_EXT			0x125
#define CF_CMD_CFC_READ_MULTIPLE_EXT		0x129
#define CF_CMD_CFC_READ_SECTOR_EXT		0x124
#define CF_CMD_CFC_READ_VERIFY_SECTOR_EXT	0x140
#define CF_CMD_CFC_WRITE_DMA_EXT		0x135
#define CF_CMD_CFC_WRITE_MULTIPLE_EXT		0x139
#define CF_CMD_CFC_WRITE_SECTOR_EXT		0x134

/* CF_CMD_REG */
#define CF_CMD_READ_MULTIPLE_DATA		0x0c4
#define CF_CMD_WRITE_MULTIPLE_DATA		0x0c5
#define CF_CMD_DMA_READ_DATA			0x0c8
#define CF_CMD_DMA_WRITE_DATA			0x0ca
#define CF_CMD_DEV_RST				0x108
#define CF_CMD_PACKET				0x1a0
#define CF_CMD_ID_PKT_DEV			0x1a1

/* CF_RD_TIM0_REG */
#define CF_RD_TIM0_TOCEHA(x)		((x) << 24)
#define CF_RD_TIM0_TCEOSA(x)		((x) << 16)
#define CF_RD_TIM0_TOAHA(x)		((x) << 8)
#define CF_RD_TIM0_TAOSA(x)		(x)

/* CF_RD_TIM1_REG */
#define CF_RD_TIM1_TRCA(x)		((x) << 24)
#define CF_RD_TIM1_TOEDELA(x)		((x) << 16)
#define CF_RD_TIM1_TCEDELA(x)		((x) << 8)
#define CF_RD_TIM1_TADELA(x)		(x)

/* CF_RD_TIM2_REG */
#define CF_RD_TIM2_TRDMARQ(x)		((x) << 24)
#define CF_RD_TIM2_TRPA(x)		((x) << 16)
#define CF_RD_TIM2_TOEHZA(x)		((x) << 8)
#define CF_RD_TIM2_TOWHA(x)		(x)

/* CF_RD_TIM3_REG */
#define CF_RD_TIM3_TOCEHC(x)		((x) << 24)
#define CF_RD_TIM3_TCEOSC(x)		((x) << 16)
#define CF_RD_TIM3_TOAHC(x)		((x) << 8)
#define CF_RD_TIM3_TAOSC(x)		(x)

/* CF_RD_TIM4_REG */
#define CF_RD_TIM4_TRCC(x)		((x) << 24)
#define CF_RD_TIM4_TOEDELC(x)		((x) << 16)
#define CF_RD_TIM4_TCEDELC(x)		((x) << 8)
#define CF_RD_TIM4_TADELC(x)		(x)

/* CF_RD_TIM5_REG */
#define CF_RD_TIM5_TRPC(x)		((x) << 16)
#define CF_RD_TIM5_TOEHZC(x)		((x) << 8)
#define CF_RD_TIM5_TOWHC(x)		(x)

/* CF_WR_TIM0_REG */
#define CF_WR_TIM0_TWCEHA(x)		((x) << 24)
#define CF_WR_TIM0_TCEWSA(x)		((x) << 16)
#define CF_WR_TIM0_TWAHA(x)		((x) << 8)
#define CF_WR_TIM0_TAWSA(x)		(x)

/* CF_WR_TIM1_REG */
#define CF_WR_TIM1_TWDHA(x)		((x) << 24)
#define CF_WR_TIM1_TDWSA(x)		((x) << 16)
#define CF_WR_TIM1_TCEWS2A(x)		((x) << 8)
#define CF_WR_TIM1_TAWS2A(x)		(x)

/* CF_WR_TIM2_REG */
#define CF_WR_TIM2_TWDMARQ(x)		((x) << 24)
#define CF_WR_TIM2_TWOHA(x)		((x) << 16)
#define CF_WR_TIM2_TWPA(x)		((x) << 8)
#define CF_WR_TIM2_TWCA(x)		(x)

/* CF_WR_TIM3_REG */
#define CF_WR_TIM3_TWCEHC(x)		((x) << 24)
#define CF_WR_TIM3_TCEWSC(x)		((x) << 16)
#define CF_WR_TIM3_TWAHC(x)		((x) << 8)
#define CF_WR_TIM3_TAWSC(x)		(x)

/* CF_WR_TIM4_REG */
#define CF_WR_TIM4_TWDHC(x)		((x) << 24)
#define CF_WR_TIM4_TDWSC(x)		((x) << 16)
#define CF_WR_TIM4_TCEWS2C(x)		((x) << 8)
#define CF_WR_TIM4_TAWS2C(x)		(x)

/* CF_WR_TIM5_REG */
#define CF_WR_TIM5_TDHZ(x)		((x) << 24)
#define CF_WR_TIM5_TWOHC(x)		((x) << 16)
#define CF_WR_TIM5_TWPC(x)		((x) << 8)
#define CF_WR_TIM5_TWCC(x)		(x)

/* CF_CM_TIM0_REG */
#define CF_CM_TIM0_TCMD(x)		((x) << 24)
#define CF_CM_TIM0_TDIAG(x)		(x)

/* CF_CM_TIM1_REG */
#define CF_CM_TIM1_TDMA(x)		((x) << 24)
#define CF_CM_TIM1_TDEVSEL(x)		((x) << 16)
#define CF_CM_TIM1_TRSTP(x)		(x)

/* CF_CM_TIM2_REG */
#define CF_CM_TIM2_TSRST(x)		(x)

/* CF_CM_TIM3_REG */
#define CF_CM_TIM3_THRST(x)		(x)

/* CF_CM_TIM4_REG */
#define CF_CM_TIM4_TPWR(x)		(x)

/* CF_STA_REG */
#define CF_STA_PR			0x80000000
#define CF_STA_SA			0x01000000
#define CF_STA_SC			0x00800000
#define CF_STA_IA			0x00008000
#define CF_STA_CW			0x00000020
#define CF_STA_DW			0x00000010
#define CF_STA_CI			0x00000002
#define CF_STA_DI			0x00000001

/* CF_STA_REG */
#define CF_STA_IW			0x00000080
#define CF_STA_PW			0x00000040
#define CF_STA_II			0x00000008
#define CF_STA_PI			0x00000004

/* ---------------------------------------------------------------------- */

/* CF_TRERR_REG	*/
/**
 * Error bits enable. Definition of enable bits are the same as error bits.
 */
#define CF_TRERR_ENABLE_MASK		0xffff0000
#define CF_TRERR_ENABLE(en)		(en << 16)
/**
 * [WR error bit 15]:
 * Device DMARQ never goes high
 */
#define CF_TRERR_ERR_18			0x00008000
/**
 * [General error bit 14]:
 * writing to read data buffer when buffer is full
 */
#define CF_TRERR_ERR_31			0x00004000
#define CF_TRERR_ERR_RSV_3		0x00002000
/**
 * [WR error bit 12]:
 * Device terminated before completion of command.
 * This might not be a real error!
 */
#define CF_TRERR_ERR_15			0x00001000
/**
 * [WR error bit 11]:
 * Transaction completed but DMARQ not deasserted
 */
#define CF_TRERR_ERR_14			0x00000800
/**
 * [WR error bit 10]:
 * Host stopped druing transaction will hang forever!
 */
#define CF_TRERR_ERR_13			0x00000400
/**
 * [WR error bit 9]:
 * Device stopped druing transaction (from CFA_RD_UDMA7->6)
 * (Note: this could also be a device DMARQ timeout for a complete transaction!)
 */
#define CF_TRERR_ERR_12			0x00000200
/**
 * [WR error bit 8]:
 * Device not responding in the beginning
 * (-DDMARDY nevers goes low, tLI time out)
 */
#define CF_TRERR_ERR_11			0x00000100
/**
 * [RD error bit 7]:
 * Device DMARQ never goes high
 */
#define CF_TRERR_ERR_8			0x00000080
/**
 * [ATAPI error bit 6]:
 * Device DMARQ never goes high
 */
#define CF_TRERR_ERR_21			0x00000040
#define CF_TRERR_ERR_RSV_2		0x00000020
#define CF_TRERR_ERR_RSV_1		0x00000010
/**
 * [RD error bit 3]:
 * Transaction completed but DMARQ not deasserted
 */
#define CF_TRERR_ERR_4			0x00000008
/**
 * [RD error bit 2]:
 * Host stopped druing transaction
 */
#define CF_TRERR_ERR_3			0x00000004
/**
 * [RD error bit 1]:
 * Device stopped druing transaction (from CFA_RD_UDMA3->5)
 */
#define CF_TRERR_ERR_2			0x00000002
/**
 * [RD error bit 0]:
 * Device not responding in the beginning
 * (-DSTROBE nevers goes low, tFS time out)
 */
#define CF_TRERR_ERR_1			0x00000001

/* CF_PKTCTL_REG */
#define CF_PKTCTL_REQSRC		0x00000020
#define CF_PKTCTL_CONT			0x00000010
#define CF_PKTCTL_DMA			0x00000008
#define CF_PKTCTL_L16			0x00000004
#define CF_PKTCTL_L12			0x00000000
#define CF_PKTCTL_VR			0x00000003
#define CF_PKTCTL_R			0x00000002
#define CF_PKTCTL_W			0x00000001
#define CF_PKTCTL_NOD			0x00000000

/* CF_CFSTAT_REG */
#define CF_CFSTAT_ST(x)			((x >> 24) & 0x7f)
#define CF_CFSTAT_NEXT_ST(x)		((x >> 16) & 0x7f)
#define CF_CFSTAT_ACC_ST(x)		((x >> 8)  & 0x7f)
#define CF_CFSTAT_NEXT_ACC_ST(x)	(x & 0x7f)

/* CF_CFLNTH_REG */
#define CF_CFLNGH_LEN(x)		((x >> 16) & 0xffff)
#define CF_CFLNGH_MULT_CNT(x)		(x & 0xffff)

/* CF_RSVOPT_REG */
#define CF_RSVOPT_II_EN			0x2000

/* NAND_NBR_SPA_REG */
#define NAND_NUM_PAGES(x)		(x)
#define NAND_STAT_CMD(x)		((x) << 8)

/* ---------------------------------------------------------------------- */
/***************************/
/* SATA Controller Registers */
/***************************/

#define SATA_SCRATCHPAD_OFFSET		0x00019000
#define SATA_PHY_DEVICE_OFFSET  	0x00019000

/* Generic Host Register */
#define SATA_AHCI_HBA_CAP_OFFSET	0x00000000
#define SATA_AHCI_GHBA_OFFSET		0x00000004
#define SATA_AHCI_ISR_OFFSET		0x00000008
#define SATA_AHCI_PORTS_IMPL_OFFSET	0x0000000C
#define SATA_AHCI_VER_REG_OFFSET	0x00000010
#define SATA_AHCI_CCC_CTL_OFFSET	0x00000014
#define SATA_AHCI_CCC_PORTS_OFFSET	0x00000018
#define SATA_AHCI_HBA_CAP2_OFFSET	0x00000024
#define SATA_AHCI_BIST_AFR_OFFSET	0x000000a0
#define SATA_AHCI_BISTCR_OFFSET		0x000000a4
#define SATA_AHCI_BISTFCTR_OFFSET	0x000000a8
#define SATA_AHCI_BISTSR_OFFSET		0x000000ac
#define SATA_AHCI_BISTDECR_OFFSET	0x000000b0
#define SATA_AHCI_OOBR_OFFSET		0x000000bc
#define SATA_AHCI_TIMER1MS_OFFSET	0x000000e0
#define SATA_AHCI_GPARAM1R_OFFSET	0x000000e8
#define SATA_AHCI_GPARAM2R_OFFSET	0x000000ec
#define SATA_AHCI_PPARAMR_OFFSET	0x000000f0
#define SATA_AHCI_TESTR_OFFSET		0x000000f4
#define SATA_AHCI_VERSIONR_OFFSET	0x000000f8
#define SATA_AHCI_IDR_OFFSET		0x000000fc

/* SATA Port Register */
#define	SATA_PORT0_CLB_OFFSET		0x00000100
#define	SATA_PORT0_CLBU_OFFSET		0x00000104
#define	SATA_PORT0_FIS_B_OFFSET		0x00000108
#define	SATA_PORT0_FIS_BU_OFFSET	0x0000010c
#define	SATA_PORT0_ISR_OFFSET		0x00000110
#define	SATA_PORT0_IER_OFFSET		0x00000114
#define	SATA_PORT0_CMD_OFFSET		0x00000118
#define	SATA_PORT0_TFD_OFFSET		0x00000120
#define	SATA_PORT0_SIG_OFFSET		0x00000124
#define	SATA_PORT0_SSTS_OFFSET		0x00000128
#define	SATA_PORT0_SCTL_OFFSET		0x0000012c
#define	SATA_PORT0_SERR_OFFSET		0x00000130
#define	SATA_PORT0_SACT_OFFSET		0x00000134
#define	SATA_PORT0_CIR_OFFSET		0x00000138
#define	SATA_PORT0_SNTF_OFFSET		0x0000013c
#define	SATA_PORT0_DMACR_OFFSET		0x00000170
#define	SATA_PORT0_PHYCR_OFFSET		0x00000178
#define	SATA_PORT0_PHYSR_OFFSET		0x0000017c

#define SATA_AHCI_HBA_CAP_REG		SATA_REG(0x000)
#define SATA_AHCI_GHBA_REG		SATA_REG(0x004)
#define SATA_AHCI_ISR_REG		SATA_REG(0x008)
#define SATA_AHCI_PORTS_IMPL_REG	SATA_REG(0x00c)
#define SATA_AHCI_VER_REG_REG		SATA_REG(0x010)
#define SATA_AHCI_CCC_CTL_REG		SATA_REG(0x014)
#define SATA_AHCI_CCC_PORTS_REG		SATA_REG(0x018)
#define SATA_AHCI_HBA_CAP2_REG		SATA_REG(0x024)
#define SATA_AHCI_BIST_AFR_REG		SATA_REG(0x0a0)
#define SATA_AHCI_BISTCR_REG		SATA_REG(0x0a4)
#define SATA_AHCI_BISTFCTR_REG		SATA_REG(0x0a8)
#define SATA_AHCI_BISTSR_REG		SATA_REG(0x0ac)
#define SATA_AHCI_BISTDECR_REG		SATA_REG(0x0b0)
#define SATA_AHCI_OOBR_REG		SATA_REG(0x0bc)
#define SATA_AHCI_TIMER1MS_REG		SATA_REG(0x0e0)
#define SATA_AHCI_GPARAM1R_REG		SATA_REG(0x0e8)
#define SATA_AHCI_GPARAM2R_REG		SATA_REG(0x0ec)
#define SATA_AHCI_PPARAMR_REG		SATA_REG(0x0f0)
#define SATA_AHCI_TESTR_REG		SATA_REG(0x0f4)
#define SATA_AHCI_VERSIONR_REG		SATA_REG(0x0f8)
#define SATA_AHCI_IDR_REG		SATA_REG(0x0fc)

#define	SATA_PORT0_CLB_REG		SATA_REG(0x100)
#define	SATA_PORT0_CLBU_REG		SATA_REG(0x104)
#define	SATA_PORT0_FIS_B_REG		SATA_REG(0x108)
#define	SATA_PORT0_FIS_BU_REG		SATA_REG(0x10c)
#define	SATA_PORT0_ISR_REG		SATA_REG(0x110)
#define	SATA_PORT0_IER_REG		SATA_REG(0x114)
#define	SATA_PORT0_CMD_REG		SATA_REG(0x118)
#define	SATA_PORT0_TFD_REG		SATA_REG(0x120)
#define	SATA_PORT0_SIG_REG		SATA_REG(0x124)
#define	SATA_PORT0_SSTS_REG		SATA_REG(0x128)
#define	SATA_PORT0_SCTL_REG		SATA_REG(0x12c)
#define	SATA_PORT0_SERR_REG		SATA_REG(0x130)
#define	SATA_PORT0_SACT_REG		SATA_REG(0x134)
#define	SATA_PORT0_CIR_REG		SATA_REG(0x138)
#define	SATA_PORT0_SNTF_REG		SATA_REG(0x13c)
#define	SATA_PORT0_DMACR_REG		SATA_REG(0x170)
#define	SATA_PORT0_PHYCR_REG		SATA_REG(0x178)
#define	SATA_PORT0_PHYSR_REG		SATA_REG(0x17c)

/* Generic Host Register(RO) */
#define SATA_HBA_CPA_S64A		0x80000000
#define SATA_HBA_CPA_SNCQ		0x00100000
#define SATA_HBA_CPA_SSNTF		0x00020000
#define SATA_HBA_CPA_SMPS		0x00010000
#define SATA_HBA_CPA_SSS		0x00008000
#define SATA_HBA_CPA_SALP		0x00004000
#define SATA_HBA_CPA_SAL		0x00003000
#define SATA_HBA_CPA_SCLO		0x00002000
#define SATA_HBA_CPA_ISS		0x00001000
#define SATA_HBA_CPA_SNZO		0x00000000
#define SATA_HBA_CPA_SAM		0x00000000
#define SATA_HBA_CPA_PMD		0x00000000
#define SATA_HBA_CPA_SSC		0x00000000
#define SATA_HBA_CPA_PSC		0x00000000
#define SATA_HBA_CPA_NCS		0x00000000
#define SATA_HBA_CPA_CCCS		0x00000000
#define SATA_HBA_CPA_EMS		0x00000000
#define SATA_HBA_CPA_NP			0x00000000

/* Global HBA Control Register */
#define SATA_AHCI_GHBA_AE		0x80000000
#define SATA_AHCI_GHBA_IE		0x00000002
#define SATA_AHCI_GHBA_HR		0x00000001

/* AHCI Version Register(RO) */
#define SATA_AHCI_VER_REG_MJR		0x00010000
#define SATA_AHCI_VER_REG_MNR		0x00000003

/* Command Completion Coalescing Control */
#define SATA_AHCI_CCC_CTL_TV(x)		((x) << 16)
#define SATA_AHCI_CCC_CTL_CC(x)		((x) << 8)
#define SATA_AHCI_CCC_CTL_INT(x)	((x) << 3)
#define SATA_AHCI_CCC_CTL_EN		0x000000001

/* HBA Capabilities Extended Register */
#define SATA_AHCI_HBA_CAP2_APST		0x000000002

/* BIST Activate FIS Register(RO) */

/* BIST Control Register */
#define SATA_AHCI_BISTCR_FERLB		0x000100000
#define SATA_AHCI_BISTCR_TXO		0x000040000
#define SATA_AHCI_BISTCR_CNTCLR		0x000020000
#define SATA_AHCI_BISTCR_NEALB		0x000010000
#define SATA_AHCI_BISTCR_SDFE		0x000001000
#define SATA_AHCI_BISTCR_LLC_SCRAM	0x000000400
#define SATA_AHCI_BISTCR_LLC_DESCRAM	0x000000200
#define SATA_AHCI_BISTCR_LLC_PRD	0x000000100
#define SATA_AHCI_BISTCR_LLC_EEREN	0x000000100
#define SATA_AHCI_BISTCR_LLC_FLIP	0x000000100
#define SATA_AHCI_BISTCR_LLC_PV		0x000000100



/* ---------------------------------------------------------------------- */

#endif
