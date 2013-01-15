#ifndef __LINUX_I2C_CY8CTMG_H
#define __LINUX_I2C_CY8CTMG_H

/* linux/i2c/cy8ctmg.h */

struct cy8ctmg_fix_data {
	u8	x_invert;
	u8	y_invert;
	u8	x_rescale;
	u8	y_rescale;
	u16	x_min;
	u16	x_max;
	u16	y_min;
	u16	y_max;
};

struct cy8ctmg_platform_data {
	struct cy8ctmg_fix_data	fix;

	void (*clear_penirq)(void);
	int (*init_platform_hw)(void);
	void (*exit_platform_hw)(void);
};

#endif
