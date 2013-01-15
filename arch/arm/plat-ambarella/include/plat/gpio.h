/*
 * arch/arm/plat-ambarella/include/plat/gpio.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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

#ifndef __PLAT_AMBARELLA_GPIO_H
#define __PLAT_AMBARELLA_GPIO_H

/* ==========================================================================*/
#define GPIO_BANK_SIZE			32
#define AMBGPIO_SIZE			(GPIO_INSTANCES * GPIO_BANK_SIZE)

#ifndef CONFIG_AMBARELLA_EXT_GPIO_NUM
#define CONFIG_AMBARELLA_EXT_GPIO_NUM	(64)
#endif
#define EXT_GPIO(x)			(AMBGPIO_SIZE + x)

#define ARCH_NR_GPIOS			EXT_GPIO(CONFIG_AMBARELLA_EXT_GPIO_NUM)

/* ==========================================================================*/
#ifndef __ASSEMBLER__
#include <asm-generic/gpio.h>

#define gpio_get_value	__gpio_get_value
#define gpio_set_value	__gpio_set_value
#define gpio_cansleep	__gpio_cansleep

/* ==========================================================================*/
struct ambarella_gpio_io_info {
	int	gpio_id;
	int	active_level;
	int	active_delay;		//ms
};
#define AMBA_GPIO_IO_MODULE_PARAM_CALL(name_prefix, arg, perm) \
	module_param_cb(name_prefix##gpio_id, &param_ops_int, &(arg.gpio_id), perm); \
	module_param_cb(name_prefix##active_level, &param_ops_int, &(arg.active_level), perm); \
	module_param_cb(name_prefix##active_delay, &param_ops_int, &(arg.active_delay), perm)
#define AMBA_GPIO_RESET_MODULE_PARAM_CALL(name_prefix, arg, perm) \
	module_param_cb(name_prefix##gpio_id, &param_ops_int, &(arg.gpio_id), perm); \
	module_param_cb(name_prefix##active_level, &param_ops_int, &(arg.active_level), perm); \
	module_param_cb(name_prefix##active_delay, &param_ops_int, &(arg.active_delay), perm)

extern int ambarella_set_gpio_output(struct ambarella_gpio_io_info *pinfo, u32 on);
extern u32 ambarella_get_gpio_input(struct ambarella_gpio_io_info *pinfo);
extern int ambarella_set_gpio_reset(struct ambarella_gpio_io_info *pinfo);

struct ambarella_gpio_irq_info {
	int	irq_gpio;
	int	irq_line;
	int	irq_type;
	int	irq_gpio_val;
	int	irq_gpio_mode;
};
#define AMBA_GPIO_IRQ_MODULE_PARAM_CALL(name_prefix, arg, perm) \
	module_param_cb(name_prefix##irq_gpio, &param_ops_int, &(arg.irq_gpio), perm); \
	module_param_cb(name_prefix##irq_line, &param_ops_int, &(arg.irq_line), perm); \
	module_param_cb(name_prefix##irq_type, &param_ops_int, &(arg.irq_type), perm); \
	module_param_cb(name_prefix##irq_gpio_val, &param_ops_int, &(arg.irq_gpio_val), perm); \
	module_param_cb(name_prefix##irq_gpio_mode, &param_ops_int, &(arg.irq_gpio_mode), perm)
extern int ambarella_is_valid_gpio_irq(struct ambarella_gpio_irq_info *pgpio_irq);

/* ==========================================================================*/
extern int ambarella_init_gpio(void);
extern void ambarella_gpio_set_valid(unsigned pin, int valid);

extern void ambarella_gpio_config(int id, int func);
extern void ambarella_gpio_set(int id, int value);
extern int ambarella_gpio_get(int id);

extern u32 ambarella_gpio_suspend(u32 level);
extern u32 ambarella_gpio_resume(u32 level);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

