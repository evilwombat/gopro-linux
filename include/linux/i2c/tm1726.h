#ifndef __LINUX_I2C_TM1726_H
#define __LINUX_I2C_TM1726_H

/* linux/i2c/tm1726.h */

typedef enum
{
	TM1726_FAMILY_0,
	TM1726_FAMILY_1,
	TM1726_FAMILY_2,

	TM1726_FAMILY_END,
} tm1726_product_family_t;

struct tm1726_fix_data {
	u8	x_invert;
	u8	y_invert;
	u8	x_rescale;
	u8	y_rescale;
	u16	x_min;
	u16	x_max;
	u16	y_min;
	u16	y_max;

	u8	family_code;
	u8	reserved1;
	u16	reserved2;
};

struct tm1726_platform_data {
	struct tm1726_fix_data	fix[TM1726_FAMILY_END];

	int (*get_pendown_state)(void);
	void (*clear_penirq)(void);
	int (*init_platform_hw)(void);
	void (*exit_platform_hw)(void);
};

#endif
