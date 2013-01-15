/*
 * ambhw/dma.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2008, Ambarella, Inc.
 */

#ifndef __AMBHW__DMA_H__
#define __AMBHW__DMA_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

#if (CHIP_REV == A1) || (CHIP_REV == A2) ||				\
    (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) ||	\
    (CHIP_REV == A5L) || defined(__FPGA__)
#define DMA_SUPPORT_DMA_FIOS	0
#else
#define DMA_SUPPORT_DMA_FIOS	1	/* DMA_FIOS */
#endif

/****************************/
/* DMA Channel Assignments  */
/****************************/

#if (DMA_SUPPORT_DMA_FIOS == 0)

/* DMA instance channel */
#if (CHIP_REV == A5L)

#define FIO_DMA_CHAN		3
#define I2S_RX_DMA_CHAN		1
#define I2S_TX_DMA_CHAN		2

#else

#define FIO_DMA_CHAN		0
#define I2S_RX_DMA_CHAN		1
#define I2S_TX_DMA_CHAN		2
#define HOST_RX_DMA_CHAN	3
#define HOST_TX_DMA_CHAN	4

#endif

#else

#if (CHIP_REV == I1)

/* FIO DMA instance channel */
#define FIO_DMA_CHAN		0	/* DMA2 */

/* General DMA instance channel */
#define NULL_DMA_CHAN		0
#define I2S_RX_DMA_CHAN		1
#define I2S_TX_DMA_CHAN		2
#define MS_AHB_SSI_TX_DMA_CHAN	3
#define SPDIF_AHB_SSI_DMA_CHAN	4

#else

/* FIO DMA instance channel */
#define FIO_DMA_CHAN		0

/* General DMA instance channel */
#define NULL_DMA_CHAN		0
#define I2S_RX_DMA_CHAN		1
#define I2S_TX_DMA_CHAN		2
#define HOST_RX_DMA_CHAN	3
#define HOST_TX_DMA_CHAN	4

#endif

#endif

#if (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A5S) || \
    (CHIP_REV == A7)  || (CHIP_REV == A7L) 
#define MS_DMA_CHAN		3
#elif (CHIP_REV == A6)
#define MS_DMA_CHAN		5
#define MS_PAGE_BUF_DMA_CHAN	6
#elif (CHIP_REV == I1)
#define MS_DMA_CHAN		MS_AHB_SSI_TX_DMA_CHAN		
#else
/* No AHB MS controller */
#define MS_DMA_CHAN		0
#endif

/* Max number of channel */
#define NUM_DMA_FIOS_CHANNELS 	DMA_SUPPORT_DMA_FIOS

#if (CHIP_REV == A6)
#define NUM_DMA_CHANNELS 	7
#elif (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == A7L)  
#define NUM_DMA_CHANNELS 	4
#else
#define NUM_DMA_CHANNELS 	5
#endif

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#define DMA_CHAN_CTR_REG(x)		DMA_REG((0x300 + ((x) << 4)))
#define DMA_CHAN_SRC_REG(x)		DMA_REG((0x304 + ((x) << 4)))
#define DMA_CHAN_DST_REG(x)		DMA_REG((0x308 + ((x) << 4)))
#define DMA_CHAN_STA_REG(x)		DMA_REG((0x30c + ((x) << 4)))
#define DMA_CHAN_DA_REG(x)		DMA_REG((0x380 + ((x) << 2)))

#define DMA_CHAN0_CTR_OFFSET		0x300
#define DMA_CHAN0_SRC_OFFSET		0x304
#define DMA_CHAN0_DST_OFFSET		0x308
#define DMA_CHAN0_STA_OFFSET		0x30c
#define DMA_CHAN1_CTR_OFFSET		0x310
#define DMA_CHAN1_SRC_OFFSET		0x314
#define DMA_CHAN1_DST_OFFSET		0x318
#define DMA_CHAN1_STA_OFFSET		0x31c
#define DMA_CHAN2_CTR_OFFSET		0x320
#define DMA_CHAN2_SRC_OFFSET		0x324
#define DMA_CHAN2_DST_OFFSET		0x328
#define DMA_CHAN2_STA_OFFSET		0x32c
#define DMA_CHAN3_CTR_OFFSET		0x330
#define DMA_CHAN3_SRC_OFFSET		0x334
#define DMA_CHAN3_DST_OFFSET		0x338
#define DMA_CHAN3_STA_OFFSET		0x33c
#define DMA_CHAN4_CTR_OFFSET		0x340
#define DMA_CHAN4_SRC_OFFSET		0x344
#define DMA_CHAN4_DST_OFFSET		0x348
#define DMA_CHAN4_STA_OFFSET		0x34c
#define DMA_CHAN5_CTR_OFFSET		0x350
#define DMA_CHAN5_SRC_OFFSET		0x354
#define DMA_CHAN5_DST_OFFSET		0x358
#define DMA_CHAN5_STA_OFFSET		0x35c
#define DMA_CHAN6_CTR_OFFSET		0x360
#define DMA_CHAN6_SRC_OFFSET		0x364
#define DMA_CHAN6_DST_OFFSET		0x368
#define DMA_CHAN6_STA_OFFSET		0x36c

#if	defined(__FPGA__)
#define DMA_CHAN7_CTR_OFFSET		0x370
#define DMA_CHAN7_SRC_OFFSET		0x374
#define DMA_CHAN7_DST_OFFSET		0x378
#define DMA_CHAN7_STA_OFFSET		0x37c
#endif

#define DMA_CHAN0_NDA_OFFSET		0x380
#define DMA_CHAN1_NDA_OFFSET		0x384
#define DMA_CHAN2_NDA_OFFSET		0x388
#define DMA_CHAN3_NDA_OFFSET		0x38c
#define DMA_CHAN4_NDA_OFFSET		0x390
#define DMA_CHAN5_NDA_OFFSET		0x394
#define DMA_CHAN6_NDA_OFFSET		0x398
#define DMA_CHAN7_NDA_OFFSET		0x39c

#define DMA_INT_OFFSET			0x3f0

#define DMA_CHAN0_CTR_REG		DMA_REG(0x300)
#define DMA_CHAN0_SRC_REG		DMA_REG(0x304)
#define DMA_CHAN0_DST_REG		DMA_REG(0x308)
#define DMA_CHAN0_STA_REG		DMA_REG(0x30c)
#define DMA_CHAN1_CTR_REG		DMA_REG(0x310)
#define DMA_CHAN1_SRC_REG		DMA_REG(0x314)
#define DMA_CHAN1_DST_REG		DMA_REG(0x318)
#define DMA_CHAN1_STA_REG		DMA_REG(0x31c)
#define DMA_CHAN2_CTR_REG		DMA_REG(0x320)
#define DMA_CHAN2_SRC_REG		DMA_REG(0x324)
#define DMA_CHAN2_DST_REG		DMA_REG(0x328)
#define DMA_CHAN2_STA_REG		DMA_REG(0x32c)
#define DMA_CHAN3_CTR_REG		DMA_REG(0x330)
#define DMA_CHAN3_SRC_REG		DMA_REG(0x334)
#define DMA_CHAN3_DST_REG		DMA_REG(0x338)
#define DMA_CHAN3_STA_REG		DMA_REG(0x33c)
#define DMA_CHAN4_CTR_REG		DMA_REG(0x340)
#define DMA_CHAN4_SRC_REG		DMA_REG(0x344)
#define DMA_CHAN4_DST_REG		DMA_REG(0x348)
#define DMA_CHAN4_STA_REG		DMA_REG(0x34c)
#define DMA_CHAN5_CTR_REG		DMA_REG(0x350)
#define DMA_CHAN5_SRC_REG		DMA_REG(0x354)
#define DMA_CHAN5_DST_REG		DMA_REG(0x358)
#define DMA_CHAN5_STA_REG		DMA_REG(0x35c)
#define DMA_CHAN6_CTR_REG		DMA_REG(0x360)
#define DMA_CHAN6_SRC_REG		DMA_REG(0x364)
#define DMA_CHAN6_DST_REG		DMA_REG(0x368)
#define DMA_CHAN6_STA_REG		DMA_REG(0x36c)

#if	defined(__FPGA__)
#define DMA_CHAN7_CTR_REG		DMA_REG(0x370)
#define DMA_CHAN7_SRC_REG		DMA_REG(0x374)
#define DMA_CHAN7_DST_REG		DMA_REG(0x378)
#define DMA_CHAN7_STA_REG		DMA_REG(0x37c)
#endif

#define DMA_CHAN0_NDA_REG		DMA_REG(0x380)
#define DMA_CHAN1_NDA_REG		DMA_REG(0x384)
#define DMA_CHAN2_NDA_REG		DMA_REG(0x388)
#define DMA_CHAN3_NDA_REG		DMA_REG(0x38c)
#define DMA_CHAN4_NDA_REG		DMA_REG(0x390)
#define DMA_CHAN5_NDA_REG		DMA_REG(0x394)
#define DMA_CHAN6_NDA_REG		DMA_REG(0x398)
#define DMA_CHAN7_NDA_REG		DMA_REG(0x39c)

#define DMA_INT_REG			DMA_REG(0x3f0)

#if (DMA_SUPPORT_DMA_FIOS == 1)
#define DMA_FIOS_CHAN_CTR_REG(x)	DMA_FIOS_REG((0x300 + ((x) << 4)))
#define DMA_FIOS_CHAN_SRC_REG(x)	DMA_FIOS_REG((0x304 + ((x) << 4)))
#define DMA_FIOS_CHAN_DST_REG(x)	DMA_FIOS_REG((0x308 + ((x) << 4)))
#define DMA_FIOS_CHAN_STA_REG(x)	DMA_FIOS_REG((0x30c + ((x) << 4)))
#define DMA_FIOS_CHAN_DA_REG(x)		DMA_FIOS_REG((0x380 + ((x) << 2)))

#define DMA_FIOS_CHAN0_CTR_REG		DMA_FIOS_REG(0x300)
#define DMA_FIOS_CHAN0_SRC_REG		DMA_FIOS_REG(0x304)
#define DMA_FIOS_CHAN0_DST_REG		DMA_FIOS_REG(0x308)
#define DMA_FIOS_CHAN0_STA_REG		DMA_FIOS_REG(0x30c)

#define DMA_FIOS_CHAN0_NDA_REG		DMA_FIOS_REG(0x380)
#define DMA_FIOS_INT_REG		DMA_FIOS_REG(0x3f0)
#endif

/* DMA_CHANX_CTR_REG */
#define DMA_CHANX_CTR_EN		0x80000000
#define DMA_CHANX_CTR_D			0x40000000
#define DMA_CHANX_CTR_WM		0x20000000
#define DMA_CHANX_CTR_RM		0x10000000
#define DMA_CHANX_CTR_NI		0x08000000
#define DMA_CHANX_CTR_BLK_1024B		0x07000000
#define DMA_CHANX_CTR_BLK_512B		0x06000000
#define DMA_CHANX_CTR_BLK_256B		0x05000000
#define DMA_CHANX_CTR_BLK_128B		0x04000000
#define DMA_CHANX_CTR_BLK_64B		0x03000000
#define DMA_CHANX_CTR_BLK_32B		0x02000000
#define DMA_CHANX_CTR_BLK_16B		0x01000000
#define DMA_CHANX_CTR_BLK_8B		0x00000000
#define DMA_CHANX_CTR_TS_8B		0x00C00000
#define DMA_CHANX_CTR_TS_4B		0x00800000
#define DMA_CHANX_CTR_TS_2B		0x00400000
#define DMA_CHANX_CTR_TS_1B		0x00000000

/* DMA descriptor bit fields */
#define DMA_DESC_EOC			0x01000000
#define DMA_DESC_WM			0x00800000
#define DMA_DESC_RM			0x00400000
#define DMA_DESC_NI			0x00200000
#define DMA_DESC_TS_8B			0x00180000
#define DMA_DESC_TS_4B			0x00100000
#define DMA_DESC_TS_2B			0x00080000
#define DMA_DESC_TS_1B			0x00000000
#define DMA_DESC_BLK_1024B		0x00070000
#define DMA_DESC_BLK_512B		0x00060000
#define DMA_DESC_BLK_256B		0x00050000
#define DMA_DESC_BLK_128B		0x00040000
#define DMA_DESC_BLK_64B		0x00030000
#define DMA_DESC_BLK_32B		0x00020000
#define DMA_DESC_BLK_16B		0x00010000
#define DMA_DESC_BLK_8B			0x00000000
#define DMA_DESC_ID			0x00000004
#define DMA_DESC_IE			0x00000002
#define DMA_DESC_ST			0x00000001

/* DMA_CHANX_STA_REG */
#define DMA_CHANX_STA_DM		0x80000000
#define DMA_CHANX_STA_OE		0x40000000
#define DMA_CHANX_STA_DA		0x20000000
#define DMA_CHANX_STA_DD		0x10000000
#define DMA_CHANX_STA_OD		0x08000000
#define DMA_CHANX_STA_ME		0x04000000
#define DMA_CHANX_STA_BE		0x02000000
#define DMA_CHANX_STA_RWE		0x01000000
#define DMA_CHANX_STA_AE		0x00800000
#define DMA_CHANX_STA_DN		0x00400000

/* DMA_INT_REG */
#define DMA_INT_CHAN(x)			(0x1 << (x))

#if	defined(__FPGA__)
#define DMA_INT_CHAN7			0x00000080
#define DMA_INT_CHAN6			0x00000040
#define DMA_INT_CHAN5			0x00000020
#endif

#define DMA_INT_CHAN4			0x00000010
#define DMA_INT_CHAN3			0x00000008
#define DMA_INT_CHAN2			0x00000004
#define DMA_INT_CHAN1			0x00000002
#define DMA_INT_CHAN0			0x00000001


/*********************************/
/* FIO/DMA Burst Setup           */
/*  - descriptor, non-descriptor */
/*  - main, spare                */
/*********************************/

#if defined(__FPGA__)

#define DMA_NODC_MN_BURST_SIZE	(DMA_CHANX_CTR_BLK_32B | DMA_CHANX_CTR_TS_4B)
#define DMA_NODC_SP_BURST_SIZE	(DMA_CHANX_CTR_BLK_32B | DMA_CHANX_CTR_TS_4B)
#define DMA_DESC_MN_BURST_SIZE	(DMA_DESC_BLK_32B | DMA_DESC_TS_4B)
#define DMA_DESC_SP_BURST_SIZE	(DMA_DESC_BLK_32B | DMA_DESC_TS_4B)
#define FIO_MN_BURST_SIZE	(FIO_DMACTR_BLK_32B | FIO_DMACTR_TS4B)
#define FIO_SP_BURST_SIZE	(FIO_DMACTR_BLK_32B | FIO_DMACTR_TS4B)

#else

#if (DMA_SUPPORT_DMA_FIOS == 0)
#define DMA_NODC_MN_BURST_SIZE	(DMA_CHANX_CTR_BLK_512B | DMA_CHANX_CTR_TS_4B)
#define DMA_NODC_SP_BURST_SIZE	(DMA_CHANX_CTR_BLK_512B | DMA_CHANX_CTR_TS_4B)
#define DMA_DESC_MN_BURST_SIZE	(DMA_DESC_BLK_512B | DMA_DESC_TS_4B)
#define DMA_DESC_SP_BURST_SIZE	(DMA_DESC_BLK_512B | DMA_DESC_TS_4B)
#define FIO_MN_BURST_SIZE	(FIO_DMACTR_BLK_512B | FIO_DMACTR_TS4B)
#define FIO_SP_BURST_SIZE	(FIO_DMACTR_BLK_512B | FIO_DMACTR_TS4B)
#else
#define DMA_NODC_MN_BURST_SIZE	(DMA_CHANX_CTR_BLK_512B | DMA_CHANX_CTR_TS_4B)
#define DMA_NODC_SP_BURST_SIZE	(DMA_CHANX_CTR_BLK_16B | DMA_CHANX_CTR_TS_4B)
#define DMA_DESC_MN_BURST_SIZE	(DMA_DESC_BLK_512B | DMA_DESC_TS_4B)
#define DMA_DESC_SP_BURST_SIZE	(DMA_DESC_BLK_16B | DMA_DESC_TS_4B)
#define FIO_MN_BURST_SIZE	(FIO_DMACTR_BLK_512B | FIO_DMACTR_TS4B)
#define FIO_SP_BURST_SIZE	(FIO_DMACTR_BLK_16B | FIO_DMACTR_TS4B)
#endif

#endif

#endif /* __AMBHW__DMA_H__ */
