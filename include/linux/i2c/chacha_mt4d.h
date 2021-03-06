#ifndef __LINUX_I2C_CHACHA_MT4D_H
#define __LINUX_I2C_CHACHA_MT4D_H

/* linux/i2c/chacha_mt4d.h */

struct chacha_mt4d_fix_data {
	u8	x_invert;
	u8	y_invert;
	u8	x_rescale;
	u8	y_rescale;
	u16	x_min;
	u16	x_max;
	u16	y_min;
	u16	y_max;
};

struct chacha_mt4d_platform_data {
	struct chacha_mt4d_fix_data	fix;

	int (*get_pendown_state)(void);
	void (*clear_penirq)(void);
	int (*init_platform_hw)(void);
	void (*exit_platform_hw)(void);
};

#endif
