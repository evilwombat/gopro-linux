/*
 * ambhw/pwm.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2011, Ambarella, Inc.
 */

#ifndef __AMBHW__PWM_H__
#define __AMBHW__PWM_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/
#if (CHIP_REV == A1 || CHIP_REV == A2 || CHIP_REV == A3 || \
     CHIP_REV == A2S || CHIP_REV == A2M || CHIP_REV == A6)
#define	PWM_SUPPRT_2_FIELDS		0
#else
#define	PWM_SUPPRT_2_FIELDS		1
#endif

#if (CHIP_REV == A1 || CHIP_REV == A2 || CHIP_REV == A3)
#define	PWM_1_4_SUPPORT_DIV1		0
#else
#define	PWM_1_4_SUPPORT_DIV1		1
#endif

#if (CHIP_REV == A2S || CHIP_REV == A2M || CHIP_REV == A2Q || \
     CHIP_REV == A2K || CHIP_REV == A6)
#define	PWM_1_4_SUPPORT_DIV2		0
#define PWM_SUPPORT_DUAL_BANK 		0
#else
#define	PWM_1_4_SUPPORT_DIV2		1
#define PWM_SUPPORT_DUAL_BANK 		1
#endif

#if (CHIP_REV == A5S) || (CHIP_REV == A7)  || (CHIP_REV == I1) || \
    (CHIP_REV == A7L)
#define PWM_SUPPORT_BANK_CLK_SRC_SEL	1
#else
#define PWM_SUPPORT_BANK_CLK_SRC_SEL	0
#endif

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#define PWM_CONTROL_OFFSET		0x00
#define	PWM_ENABLE_OFFSET		0x04
#define PWM_MODE_OFFSET			0x08
#define PWM_CONTROL1_OFFSET		0x0c

#define PWM_CONTROL_REG			PWM_REG(PWM_CONTROL_OFFSET)
#define	PWM_ENABLE_REG			PWM_REG(PWM_ENABLE_OFFSET)
#define PWM_MODE_REG			PWM_REG(PWM_MODE_OFFSET)
#define PWM_CONTROL1_REG		PWM_REG(PWM_CONTROL1_OFFSET)

/* Pins shared with Stepper interface */
#define	PWM_ST_REG(x)			(STEPPER_BASE + (x))

#define PWM_B0_DATA_OFFSET		0x300
#define PWM_B0_ENABLE_OFFSET		0x304
#define PWM_C0_DATA_OFFSET		0x310
#define PWM_C0_ENABLE_OFFSET		0x314
#define PWM_B0_DATA1_OFFSET		0x320
#define PWM_C0_DATA1_OFFSET		0x328
#if (PWM_SUPPRT_2_FIELDS == 0)
#define PWM_B1_DATA_OFFSET		PWM_B0_DATA_OFFSET
#define PWM_B1_ENABLE_OFFSET		PWM_B0_ENABLE_OFFSET
#define PWM_C1_DATA_OFFSET		PWM_C0_DATA_OFFSET
#define PWM_C1_ENABLE_OFFSET		PWM_C0_ENABLE_OFFSET
#define PWM_B1_DATA1_OFFSET		PWM_B0_DATA1_OFFSET
#define PWM_C1_DATA1_OFFSET		PWM_C0_DATA1_OFFSET
#else
#define PWM_B1_DATA_OFFSET		0x308
#define PWM_B1_ENABLE_OFFSET		0x30c
#define PWM_C1_DATA_OFFSET		0x318
#define PWM_C1_ENABLE_OFFSET		0x31c
#define PWM_B1_DATA1_OFFSET		0x324
#define PWM_C1_DATA1_OFFSET		0x32c
#endif

#define PWM_B0_DATA_REG			PWM_ST_REG(PWM_B0_DATA_OFFSET)
#define PWM_B0_ENABLE_REG		PWM_ST_REG(PWM_B0_ENABLE_OFFSET)
#define PWM_C0_DATA_REG			PWM_ST_REG(PWM_C0_DATA_OFFSET)
#define PWM_C0_ENABLE_REG		PWM_ST_REG(PWM_C0_ENABLE_OFFSET)
#define PWM_B0_DATA1_REG		PWM_ST_REG(PWM_B0_DATA1_OFFSET)
#define PWM_C0_DATA1_REG		PWM_ST_REG(PWM_C0_DATA1_OFFSET)

#if (PWM_SUPPRT_2_FIELDS == 0)
#define PWM_B1_DATA_REG			PWM_B0_DATA_REG
#define PWM_B1_ENABLE_REG		PWM_B0_ENABLE_REG
#define PWM_C1_DATA_REG			PWM_C0_DATA_REG
#define PWM_C1_ENABLE_REG		PWM_C0_ENABLE_REG
#define PWM_B1_DATA1_REG		PWM_B0_DATA1_REG
#define PWM_C1_DATA1_REG		PWM_C0_DATA1_REG
#else /* (PWM_SUPPRT_2_FIELDS == 1) */
#define PWM_B1_DATA_REG			PWM_ST_REG(PWM_B1_DATA_OFFSET)
#define PWM_B1_ENABLE_REG		PWM_ST_REG(PWM_B1_ENABLE_OFFSET)
#define PWM_C1_DATA_REG			PWM_ST_REG(PWM_C1_DATA_OFFSET)
#define PWM_C1_ENABLE_REG		PWM_ST_REG(PWM_C1_ENABLE_OFFSET)
#define PWM_B1_DATA1_REG		PWM_ST_REG(PWM_B1_DATA1_OFFSET)
#define PWM_C1_DATA1_REG		PWM_ST_REG(PWM_C1_DATA1_OFFSET) 
#endif

#endif
