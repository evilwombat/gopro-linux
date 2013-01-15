/**
 * ambhw/chip.h
 *
 * History:
 *	2007/11/29 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2007, Ambarella, Inc.
 */

#ifndef __AMBHW__CHIP_H__
#define __AMBHW__CHIP_H__

#if defined(__PRKERNEL_AMB__)
#include <config.h>
#endif

#define A1	1000
#define A2	2000

#define A2S	2100
#define A2M	2200
#define A2Q	2300
#define A2K	2400
#define A3	3000

#define A5	5000
#define A530	5030
#define A550	5050
#define A570	5070
#define A580	5080

#define A5S	5100
#define A5L	5500
#define A6	6000
#define A7	7000
#define A7M	7100
#define I1	7200
#define A7L	7500

#define CHIP_ID(x)	((x / 1000))
#define CHIP_MAJOR(x)	((x / 100) % 10)
#define CHIP_MINOR(x)	((x / 10) % 10)

#if defined(__KERNEL__)
#if	defined(CONFIG_PLAT_AMBARELLA_A1)
#define CHIP_REV	A1
#elif	defined(CONFIG_PLAT_AMBARELLA_A2)
#define CHIP_REV	A2
#elif	defined(CONFIG_PLAT_AMBARELLA_A2S)
#define CHIP_REV	A2S
#elif	defined(CONFIG_PLAT_AMBARELLA_A2M)
#define CHIP_REV	A2M
#elif	defined(CONFIG_PLAT_AMBARELLA_A2Q)
#define CHIP_REV	A2Q
#elif	defined(CONFIG_PLAT_AMBARELLA_A3)
#define CHIP_REV	A3
#elif	defined(CONFIG_PLAT_AMBARELLA_A5)
#define CHIP_REV	A5
#elif	defined(CONFIG_PLAT_AMBARELLA_A5S)
#define CHIP_REV	A5S
#elif	defined(CONFIG_PLAT_AMBARELLA_A5S_BOSS)
#define CHIP_REV	A5S
#elif	defined(CONFIG_PLAT_AMBARELLA_A5L)
#define CHIP_REV	A5L
#elif	defined(CONFIG_PLAT_AMBARELLA_A6)
#define CHIP_REV	A6
#elif	defined(CONFIG_PLAT_AMBARELLA_A7)
#define CHIP_REV	A7
#elif	defined(CONFIG_PLAT_AMBARELLA_A7_BOSS)
#define CHIP_REV	A7
#elif	defined(CONFIG_PLAT_AMBARELLA_A7L)
#define CHIP_REV	A7L
#elif	defined(CONFIG_PLAT_AMBARELLA_A7L_BOSS)
#define CHIP_REV	A7L
#elif	defined(CONFIG_PLAT_AMBARELLA_A7M)
#define CHIP_REV	A7M
#elif	defined(CONFIG_PLAT_AMBARELLA_I1)
#define CHIP_REV	I1
#elif	defined(CONFIG_PLAT_AMBARELLA_I1_BOSS)
#define CHIP_REV	I1
#else
#error "Undefined CHIP_REV"
#endif
#else
#if	defined(CUSTOM_CHIP_REV)
#define CHIP_REV	CUSTOM_CHIP_REV

#else  /* CUSTOM_CHIP_REV */

#if	defined(CONFIG_ARCH_AMBARELLA_A1) || \
	defined(CONFIG_ARCH_A1) ||	     \
	defined(CONFIG_ARCH_MMP2_A1)
#define CHIP_REV	A1
#elif	defined(CONFIG_ARCH_AMBARELLA_A2) ||	\
	defined(CONFIG_ARCH_A2) ||		\
	defined(CONFIG_ARCH_MMP2_A2)
#define CHIP_REV	A2
#elif	defined(CONFIG_ARCH_AMBARELLA_A2S) ||	\
	defined(CONFIG_ARCH_A2S)
#define CHIP_REV	A2S
#elif	defined(CONFIG_ARCH_AMBARELLA_A2M) ||	\
	defined(CONFIG_ARCH_A2M)
#define CHIP_REV	A2M
#elif	defined(CONFIG_ARCH_AMBARELLA_A2Q) ||	\
	defined(CONFIG_ARCH_A2Q)
#define CHIP_REV	A2Q
#elif	defined(CONFIG_ARCH_AMBARELLA_A3) || \
	defined(CONFIG_ARCH_A3) ||	     \
	defined(CONFIG_ARCH_MMP2_A3)
#define CHIP_REV	A3
#elif	defined(CONFIG_ARCH_AMBARELLA_A5) || \
	defined(CONFIG_ARCH_A5) ||	     \
	defined(CONFIG_ARCH_MMP2_A5)
#define CHIP_REV	A5
#elif	defined(CONFIG_ARCH_AMBARELLA_A5S) || \
	defined(CONFIG_ARCH_A5S) ||	      \
	defined(CONFIG_ARCH_MMP2_A5S)
#define CHIP_REV	A5S
#elif	defined(CONFIG_ARCH_AMBARELLA_A5L) ||	\
	defined(CONFIG_ARCH_A5L) ||	      \
	defined(CONFIG_ARCH_MMP2_A5L)
#define CHIP_REV	A5L
#elif	defined(CONFIG_ARCH_AMBARELLA_A6) || \
	defined(CONFIG_ARCH_A6) ||	     \
	defined(CONFIG_ARCH_MMP2_A6)
#define CHIP_REV	A6
#elif	defined(CONFIG_ARCH_AMBARELLA_A7) || \
	defined(CONFIG_ARCH_A7) ||	     \
	defined(CONFIG_ARCH_MMP2_A7)
#define CHIP_REV	A7
#elif	defined(CONFIG_ARCH_AMBARELLA_A7L) || \
	defined(CONFIG_ARCH_A7L) ||	     \
	defined(CONFIG_ARCH_MMP2_A7L)
#define CHIP_REV	A7L
#elif	defined(CONFIG_ARCH_AMBARELLA_A7M) || \
	defined(CONFIG_ARCH_A7M) ||	     \
	defined(CONFIG_ARCH_MMP2_A7M)
#define CHIP_REV	A7M
#elif	defined(CONFIG_ARCH_AMBARELLA_I1) || \
	defined(CONFIG_ARCH_I1) ||	     \
	defined(CONFIG_ARCH_MMP2_I1)
#define CHIP_REV	I1
#else
#error "Undefined CHIP_REV"
#endif
#endif /* CUSTOM_CHIP_REV */
#endif /* __KERNEL__ */

#if (CHIP_REV == A5S) || (CHIP_REV == A5L) || (CHIP_REV == A7) || \
    (CHIP_REV == A7L)
#define	BROKEN_UNALIGNED_ACCESS_SUPPORT		1
#endif

/* Audio ucode. */
#if (CHIP_REV == A3) || (CHIP_REV == A5) || (CHIP_REV == A6)
#define HAS_AORC_UCODE	1
#else
#define HAS_AORC_UCODE	0
#endif

#endif
