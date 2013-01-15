/*
 * system/include/ambhw/wdog.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2011, Ambarella, Inc.
 * 
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBHW__WDOG_H__
#define __AMBHW__WDOG_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

/* On A5S WDT_RST_L_REG can not be cleared. 	 */
/* When is more then 0xff. To work around we set */
/* WDT_RST_L_REG to 0x0 before WDT start because */
/* it is preserved during soft reset, if it's  	 */
/* still 0x0 we could know the reset is from WDT */
#if (CHIP_REV == A5S)
#define RCT_WDT_RESET_VAL	0
#else
#define RCT_WDT_RESET_VAL	1
#endif

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#define WDOG_STATUS_OFFSET		0x00
#define WDOG_RELOAD_OFFSET		0x04
#define WDOG_RESTART_OFFSET		0x08
#define WDOG_CONTROL_OFFSET		0x0c
#define WDOG_TIMEOUT_OFFSET		0x10
#define WDOG_CLR_TMO_OFFSET		0x14
#define WDOG_RST_WD_OFFSET		0x18

#define WDOG_STATUS_REG			WDOG_REG(WDOG_STATUS_OFFSET)
#define WDOG_RELOAD_REG			WDOG_REG(WDOG_RELOAD_OFFSET)
#define WDOG_RESTART_REG		WDOG_REG(WDOG_RESTART_OFFSET)
#define WDOG_CONTROL_REG		WDOG_REG(WDOG_CONTROL_OFFSET)
#define WDOG_TIMEOUT_REG		WDOG_REG(WDOG_TIMEOUT_OFFSET)
#define WDOG_CLR_TMO_REG		WDOG_REG(WDOG_CLR_TMO_OFFSET)
#define WDOG_RST_WD_REG			WDOG_REG(WDOG_RST_WD_OFFSET)

/* Bit field definition of watch dog timer control register */
#define WDOG_CTR_INT_EN			0x00000004
#define WDOG_CTR_RST_EN			0x00000002
#define WDOG_CTR_EN			0x00000001

#define WDT_RST_L_REG			RCT_REG(0x78)

/* WDOG_RESTART_REG only works with magic 0x4755. */
/* Set this value would transferred the value in  */
/* WDOG_RELOAD_REG into WDOG_STATUS_REG and would */
/* not trigger the underflow event.  	  	  */
#define WDT_RESTART_VAL		0X4755
#endif
