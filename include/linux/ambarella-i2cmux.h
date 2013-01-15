/*
 * ambarella-i2cmux interface to platform code
 *
 * Zhenwu Xue <zwxue@ambarella.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_AMBARELLA_I2CMUX_H
#define _LINUX_AMBARELLA_I2CMUX_H

struct ambarella_i2cmux_platform_data {
	int		parent;
	unsigned	number;
	unsigned	gpio;
	unsigned	select_function;
	unsigned	deselect_function;
};

#endif /* _LINUX_AMBARELLA_I2CMUX_H */
