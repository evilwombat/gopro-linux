/*
 * arch/arm/plat-ambarella/include/plat/ambinput.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __PLAT_AMBARELLA_AMBINPUT_H__
#define __PLAT_AMBARELLA_AMBINPUT_H__

/* ==========================================================================*/
#define AMBINPUT_TABLE_SIZE	(256)

#define AMBINPUT_SOURCE_MASK	(0x0F)
#define AMBINPUT_SOURCE_IR	(0x01)
#define AMBINPUT_SOURCE_ADC	(0x02)
#define AMBINPUT_SOURCE_GPIO	(0x04)
#define AMBINPUT_SOURCE_VI	(0x08)

#define AMBINPUT_TYPE_MASK	(0xF0)
#define AMBINPUT_TYPE_KEY	(0x10)
#define AMBINPUT_TYPE_REL	(0x20)
#define AMBINPUT_TYPE_ABS	(0x40)
#define AMBINPUT_TYPE_SW	(0x80)

#define	AMBINPUT_IR_KEY		(AMBINPUT_SOURCE_IR | AMBINPUT_TYPE_KEY)
#define	AMBINPUT_IR_REL		(AMBINPUT_SOURCE_IR | AMBINPUT_TYPE_REL)
#define	AMBINPUT_IR_ABS		(AMBINPUT_SOURCE_IR | AMBINPUT_TYPE_ABS)
#define	AMBINPUT_IR_SW		(AMBINPUT_SOURCE_IR | AMBINPUT_TYPE_SW)
#define	AMBINPUT_ADC_KEY	(AMBINPUT_SOURCE_ADC | AMBINPUT_TYPE_KEY)
#define	AMBINPUT_ADC_REL	(AMBINPUT_SOURCE_ADC | AMBINPUT_TYPE_REL)
#define	AMBINPUT_ADC_ABS	(AMBINPUT_SOURCE_ADC | AMBINPUT_TYPE_ABS)
#define	AMBINPUT_GPIO_KEY	(AMBINPUT_SOURCE_GPIO | AMBINPUT_TYPE_KEY)
#define	AMBINPUT_GPIO_REL	(AMBINPUT_SOURCE_GPIO | AMBINPUT_TYPE_REL)
#define	AMBINPUT_GPIO_ABS	(AMBINPUT_SOURCE_GPIO | AMBINPUT_TYPE_ABS)
#define	AMBINPUT_GPIO_SW	(AMBINPUT_SOURCE_GPIO | AMBINPUT_TYPE_SW)
#define	AMBINPUT_VI_KEY		(AMBINPUT_SOURCE_VI | AMBINPUT_TYPE_KEY)
#define	AMBINPUT_VI_REL		(AMBINPUT_SOURCE_VI | AMBINPUT_TYPE_REL)
#define	AMBINPUT_VI_ABS		(AMBINPUT_SOURCE_VI | AMBINPUT_TYPE_ABS)
#define	AMBINPUT_VI_SW		(AMBINPUT_SOURCE_VI | AMBINPUT_TYPE_SW)

#define	AMBINPUT_END		(0xFFFFFFFF)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct ambarella_key_table {
	u32					type;
	union {
	struct {
		u32				key_code;
		u32				key_flag;
		u32				raw_id;
	} ir_key;
	struct {
		u32				key_code;
		s32				rel_step;
		u32				raw_id;
	} ir_rel;
	struct {
		s32				abs_x;
		s32				abs_y;
		u32				raw_id;
	} ir_abs;
	struct {
		u32				key_code;
		u32				key_value;
		u32				raw_id;
	} ir_sw;
	struct {
		u32				key_code;
		u16				irq_trig;// 0 low trigger, 1 high trigger
		u16				chan;
		u16				low_level;
		u16				high_level;
	} adc_key;
	struct {
		u16				key_code;
		s16				rel_step;
		u16				irq_trig;// 0 low trigger, 1 high trigger
		u16				chan;
		u16				low_level;
		u16				high_level;
	} adc_rel;
	struct {
		s16				abs_x;
		s16				abs_y;
		u16				irq_trig;// 0 low trigger, 1 high trigger
		u16				chan;
		u16				low_level;
		u16				high_level;
	} adc_abs;
	struct {
		u32				key_code;
		u32				active_val;
		u16				can_wakeup;
		u8				id;
		u8				irq_mode;
	} gpio_key;
	struct {
		u32				key_code;
		s32				rel_step;
		u16				can_wakeup;
		u8				id;
		u8				irq_mode;
	} gpio_rel;
	struct {
		s32				abs_x;
		s32				abs_y;
		u16				can_wakeup;
		u8				id;
		u8				irq_mode;
	} gpio_abs;
	struct {
		u32				key_code;
		u32				active_val;
		u16				can_wakeup;
		u8				id;
		u8				irq_mode;
	} gpio_sw;
	struct {
		u32				reserve;
		u32				reserve0;
		u32				reserve1;
	} vi_key;
	struct {
		u32				reserve;
		u32				reserve0;
		u32				reserve1;
	} vi_rel;
	struct {
		u32				reserve;
		u32				reserve0;
		u32				reserve1;
	} vi_abs;
	struct {
		u32				reserve;
		u32				reserve0;
		u32				reserve1;
	} vi_sw;
	};
};

struct ambarella_adc_channel_info{
	u32				adc_channel_used;
	u32 				adc_high_trig;
	u32 				adc_low_trig;
};

struct ambarella_adc_info {
	struct input_dev		*dev;
	unsigned char __iomem 		*regbase;

	u32				id;
	struct resource			*mem;
	unsigned int			support_irq;
	unsigned long			irqflags;
	unsigned int			work_mode;
	unsigned int			irq;

	struct workqueue_struct		*workqueue;

	struct delayed_work		detect_adc;
	u32				*adc_key_pressed;
	u32				*adc_data;

	struct ambarella_key_table	*pkeymap;
	struct ambarella_adc_controller	*pcontroller_info;

	u32				adc_channel_num;
	struct ambarella_adc_channel_info	*adc_channel_info;
};

/* ==========================================================================*/
struct ambarella_input_board_info {
	struct ambarella_key_table		*pkeymap;
	struct input_dev			*pinput_dev;
	struct platform_device			*pdev;

	int					abx_max_x;
	int					abx_max_y;
	int					abx_max_pressure;
	int					abx_max_width;
};

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

