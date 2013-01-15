/*
 * ambhw/gdma.h
 *
 * History:
 *	2009/03/12 - [Jack Huang] created file
 *
 * Copyright (C) 2009-2009, Ambarella, Inc.
 */

#ifndef __AMBHW__GDMA_H__
#define __AMBHW__GDMA_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

#if (CHIP_REV == A3) || (CHIP_REV == A5) || (CHIP_REV == A6) || \
    (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == A7L)
#define NUMBERS_GDMA_INSTANCES	8
#else
#define NUMBERS_GDMA_INSTANCES	0
#endif

#if (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == A7L)
#define GDMA_ON_AHB		1
#else
#define GDMA_ON_AHB		0
#endif

#if (CHIP_REV == A7) || (CHIP_REV == A7L)
#define GDMA_SUPPORT_ALPHA_BLEND	1
#else
#define GDMA_SUPPORT_ALPHA_BLEND	0
#endif
/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#if (GDMA_ON_AHB == 1)
#define GDMA_SRC_1_BASE_OFFSET		0x00
#define GDMA_SRC_1_PITCH_OFFSET		0x04
#define GDMA_SRC_2_BASE_OFFSET		0x08
#define GDMA_SRC_2_PITCH_OFFSET		0x0c
#define GDMA_DST_BASE_OFFSET		0x10
#define GDMA_DST_PITCH_OFFSET		0x14
#define GDMA_WIDTH_OFFSET		0x18
#define GDMA_HIGHT_OFFSET		0x1c
#define GDMA_TRANSPARENT_OFFSET		0x20
#define GDMA_OPCODE_OFFSET		0x24
#define GDMA_PENDING_OPS_OFFSET		0x28

#if (GDMA_SUPPORT_ALPHA_BLEND == 1)
#define GDMA_PIXELFORMAT_OFFSET		0x2c
#define GDMA_ALPHA_OFFSET		0x30
#define GDMA_CLUT_BASE_OFFSET		0x400
#endif

#define GDMA_SRC_1_BASE_REG		GRAPHICS_DMA_REG(GDMA_SRC_1_BASE_OFFSET)
#define GDMA_SRC_1_PITCH_REG		GRAPHICS_DMA_REG(GDMA_SRC_1_PITCH_OFFSET)
#define GDMA_SRC_2_BASE_REG		GRAPHICS_DMA_REG(GDMA_SRC_2_BASE_OFFSET)
#define GDMA_SRC_2_PITCH_REG		GRAPHICS_DMA_REG(GDMA_SRC_2_PITCH_OFFSET)
#define GDMA_DST_BASE_REG		GRAPHICS_DMA_REG(GDMA_DST_BASE_OFFSET)
#define GDMA_DST_PITCH_REG		GRAPHICS_DMA_REG(GDMA_DST_PITCH_OFFSET)
#define GDMA_WIDTH_REG			GRAPHICS_DMA_REG(GDMA_WIDTH_OFFSET)
#define GDMA_HEIGHT_REG			GRAPHICS_DMA_REG(GDMA_HIGHT_OFFSET)
#define GDMA_TRANSPARENT_REG		GRAPHICS_DMA_REG(GDMA_TRANSPARENT_OFFSET)
#define GDMA_OPCODE_REG			GRAPHICS_DMA_REG(GDMA_OPCODE_OFFSET)
#define GDMA_PENDING_OPS_REG		GRAPHICS_DMA_REG(GDMA_PENDING_OPS_OFFSET)

#if (GDMA_SUPPORT_ALPHA_BLEND == 1)
#define GDMA_PIXELFORMAT_REG		GRAPHICS_DMA_REG(GDMA_PIXELFORMAT_OFFSET)
#define GDMA_ALPHA_REG			GRAPHICS_DMA_REG(GDMA_ALPHA_OFFSET)
#define GDMA_CLUT_BASE_REG		GRAPHICS_DMA_REG(GDMA_CLUT_BASE_OFFSET)

/* GDMA_PIXELFORMAT_REG */
#define GDMA_PIXELFORMAT_THROTTLE_DRAM	(1L << 11) 

#endif /* (GDMA_SUPPORT_ALPHA_BLEND == 1)*/
#endif /*	GDMA_ON_AHB == 1	*/

#endif

