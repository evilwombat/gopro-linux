/*
 * ambhw/ir.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2007, Ambarella, Inc.
 */

#ifndef __AMBHW__IR_H__
#define __AMBHW__IR_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

/* None so far */

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#define IR_CONTROL_OFFSET		0x00
#define IR_STATUS_OFFSET		0x04
#define IR_DATA_OFFSET			0x08

#define IR_CONTROL_REG			IR_REG(0x00)
#define IR_STATUS_REG			IR_REG(0x04)
#define IR_DATA_REG			IR_REG(0x08)

/* IR_CONTROL_REG */
#define IR_CONTROL_RESET		0x00004000
#define IR_CONTROL_ENB			0x00002000
#define IR_CONTROL_LEVINT		0x00001000
#define IR_CONTROL_INTLEV(x)		(((x) & 0x3f)  << 4)
#define IR_CONTROL_FIFO_OV		0x00000008
#define IR_CONTROL_INTENB		0x00000004

#define IR_STATUS_COUNT(x)		((x) & 0x3f)
#define IR_DATA_DATA(x)			((x) & 0xffff)

#endif
