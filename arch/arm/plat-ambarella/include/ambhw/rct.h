/*
 * ambhw/rct.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2008, Ambarella, Inc.
 */

#ifndef __AMBHW__RCT_H__
#define __AMBHW__RCT_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

#define PLL_FORCE_LOCK         0x1

#if (CHIP_REV == A1)
#define REF_CLK_FREQ	27000000
#include <ambhw/rct/a1.h>
#if defined(CONFIG_REF_CLK_48MHZ)
#undef REF_CLK_FREQ
#define REF_CLK_FREQ	48000000
#endif
#elif (CHIP_REV == A2)
#define REF_CLK_FREQ	27000000
#include <ambhw/rct/a2.h>
#if defined(CONFIG_REF_CLK_48MHZ)
#undef REF_CLK_FREQ
#define REF_CLK_FREQ	48000000
#endif
#elif (CHIP_REV == A2M) || (CHIP_REV == A2S) || (CHIP_REV == A2Q)
#define REF_CLK_FREQ	27000000
#include <ambhw/rct/a2s.h>
#elif (CHIP_REV == A3)
#define REF_CLK_FREQ	27000000
#include <ambhw/rct/a3.h>
#elif (CHIP_REV == A5)
#define REF_CLK_FREQ	27000000
#include <ambhw/rct/a5.h>
#elif (CHIP_REV == A5S)
#define REF_CLK_FREQ	24000000
#include <ambhw/rct/a5s.h>
#elif (CHIP_REV == A5L)
#define REF_CLK_FREQ	24000000
#include <ambhw/rct/a5l.h>
#elif (CHIP_REV == A6)
#define REF_CLK_FREQ	27000000
#include <ambhw/rct/a6.h>
#elif (CHIP_REV == A7)
#define REF_CLK_FREQ	24000000
#include <ambhw/rct/a7.h>
#elif (CHIP_REV == A7L)
#define REF_CLK_FREQ	24000000
#include <ambhw/rct/a7l.h>
#elif (CHIP_REV == A7M)
#define REF_CLK_FREQ	24000000
#include <ambhw/rct/a7m.h>
#elif (CHIP_REV == I1)
#define REF_CLK_FREQ	24000000
#include <ambhw/rct/i1.h>
#endif

#ifndef RCT_MAX_DLL_CTRL
#define RCT_MAX_DLL_CTRL	1
#endif

#include <ambhw/rct/audio.h>

#endif
