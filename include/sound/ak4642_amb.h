/*
 * ak4642_amb.h  --  AK4642 Soc Audio driver
 *
 * Copyright 2009 Ambarella Ltd.
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _AK4642_AMB_H
#define _AK4642_AMB_H

struct ak4642_platform_data {
	unsigned int	rst_pin;
	unsigned int 	rst_delay;
};

#endif
