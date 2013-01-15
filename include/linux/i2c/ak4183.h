#ifndef __LINUX_I2C_AK4183_H
#define __LINUX_I2C_AK4183_H

/* linux/i2c/ak4183.h */

struct ak4183_fix_data {
	u8	x_invert;
	u8	y_invert;
	u8	x_rescale;
	u8	y_rescale;
	u16	x_min;
	u16	x_max;
	u16	y_min;
	u16	y_max;
};

struct ak4183_platform_data {
	u16	model;				/* 4183 */
	u16	x_plate_ohms;
	struct	ak4183_fix_data	fix;

	int	(*get_pendown_state)(void);
	void	(*clear_penirq)(void);		/* If needed, clear 2nd level
						   interrupt source */
	int	(*init_platform_hw)(void);
	void	(*exit_platform_hw)(void);
};

#endif
