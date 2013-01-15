/*
 * ambhw/rct/rct.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2008, Ambarella, Inc.
 */

#ifndef __AMBHW__RCT_AUDIO_H__
#define __AMBHW__RCT_AUDIO_H__

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

#define RCT_SUPPORT_CG_SETTING_CHANGED	1

#if (CHIP_REV == A1)
#define RCT_AUDIO_PLL_CONF_MODE		0
#define RCT_AUDIO_PLL_USE_HAL_API	0
#elif (CHIP_REV == A2) || (CHIP_REV == A3)
#define RCT_AUDIO_PLL_CONF_MODE		1
#define RCT_AUDIO_PLL_USE_HAL_API	0
#elif (CHIP_REV == A5) || (CHIP_REV == A2S) || \
	(CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
	(CHIP_REV == A6) || (CHIP_REV == A7M) || (CHIP_REV == A5L)
#define RCT_AUDIO_PLL_CONF_MODE		2
#define RCT_AUDIO_PLL_USE_HAL_API	0
#else
#define RCT_AUDIO_PLL_CONF_MODE		2
#define RCT_AUDIO_PLL_USE_HAL_API	1
#endif

#define PLL_LOCK_AUDIO  	(0x1 << 7)

#endif
