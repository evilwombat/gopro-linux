#ifndef __LINUX_I2C_FT540_H
#define __LINUX_I2C_FT540_H

/* linux/i2c/ft540.h */

typedef enum
{
	FT540_FAMILY_0,
	FT540_FAMILY_1,
	FT540_FAMILY_2,

	FT540_FAMILY_END,
} ft540_product_family_t;

struct ft540_fix_data {
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

struct ft540_platform_data {
	struct ft540_fix_data	fix[FT540_FAMILY_END];

	int (*get_pendown_state)(void);
	void (*clear_penirq)(void);
	int (*init_platform_hw)(void);
	void (*exit_platform_hw)(void);
};

#endif
