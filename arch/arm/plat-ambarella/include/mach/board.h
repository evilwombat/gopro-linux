/*
 * arch/arm/plat-ambarella/include/mach/board.h
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

#ifndef __PLAT_AMBARELLA_BOARD_H
#define __PLAT_AMBARELLA_BOARD_H

/* ==========================================================================*/
#include <plat/gpio.h>
#include <plat/spi.h>

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct ambarella_board_info {
	u32					board_chip;
	u32					board_type;
	u32					board_rev;

	u8					eth0_mac[6];
	u8					eth1_mac[6];
	u8					wifi_mac[6];
	u8					ueth_mac[6];

	struct ambarella_gpio_irq_info		power_detect;
	struct ambarella_gpio_io_info		power_control;
	struct ambarella_gpio_io_info		debug_led0;
	struct ambarella_gpio_io_info		debug_switch;
	struct ambarella_gpio_io_info		rs485;

	struct ambarella_gpio_irq_info		audio_irq;
	struct ambarella_gpio_io_info		audio_power;
	struct ambarella_gpio_io_info		audio_reset;
	struct ambarella_gpio_io_info		audio_speaker;
	struct ambarella_gpio_io_info		audio_headphone;
	struct ambarella_gpio_io_info		audio_microphone;

	struct ambarella_gpio_irq_info		touch_panel_irq;
	struct ambarella_gpio_io_info		touch_panel_power;
	struct ambarella_gpio_io_info		touch_panel_reset;

	struct ambarella_gpio_irq_info		lcd_irq;
	struct ambarella_gpio_io_info		lcd_power;
	struct ambarella_gpio_io_info		lcd_reset;
	struct ambarella_gpio_io_info		lcd_backlight;
	struct ambarella_spi_hw_info		lcd_spi_hw;
	struct ambarella_spi_cfg_info		lcd_spi_cfg;

	struct ambarella_gpio_irq_info		vin_vsync;
	struct ambarella_gpio_io_info		vin_power;
	struct ambarella_gpio_io_info		vin_reset;
	struct ambarella_gpio_io_info		vin_trigger;
	struct ambarella_gpio_irq_info		vin_strobe;
	struct ambarella_gpio_io_info		vin_hdmi_hpd;

	struct ambarella_gpio_irq_info		flash_charge_ready;
	struct ambarella_gpio_io_info		flash_enable;
	struct ambarella_gpio_io_info		flash_trigger;

	struct ambarella_gpio_irq_info		avplug_detect;

	struct ambarella_gpio_io_info		hdmi_extpower;

	struct ambarella_gpio_irq_info		gps_irq;
	struct ambarella_gpio_io_info		gps_power;
	struct ambarella_gpio_io_info		gps_reset;
	struct ambarella_gpio_io_info		gps_wakeup;

	struct ambarella_gpio_irq_info		lens_irq;
	struct ambarella_gpio_io_info		lens_power;
	struct ambarella_gpio_io_info		lens_reset;

	struct ambarella_gpio_irq_info		gyro_irq;
	struct ambarella_gpio_io_info		gyro_power;
	struct ambarella_gpio_io_info		gyro_reset;
	struct ambarella_gpio_io_info		gyro_hps;

	struct ambarella_gpio_irq_info		fm_irq;
	struct ambarella_gpio_io_info		fm_power;
	struct ambarella_gpio_io_info		fm_reset;

	struct ambarella_gpio_irq_info		gsensor_irq;
	struct ambarella_gpio_io_info		gsensor_power;
	struct ambarella_gpio_io_info		gsensor_reset;

	struct ambarella_gpio_irq_info		bb_irq;
	struct ambarella_gpio_io_info		bb_power;
	struct ambarella_gpio_io_info		bb_reset;
	struct ambarella_gpio_io_info		bb_en;
	struct ambarella_gpio_io_info		bb_switch;

	struct ambarella_pwm_info		pwm0_config;
	struct ambarella_pwm_info		pwm1_config;
	struct ambarella_pwm_info		pwm2_config;
	struct ambarella_pwm_info		pwm3_config;
	struct ambarella_pwm_info		pwm4_config;

	int					wifi_sd_bus;
	int					wifi_sd_slot;
	struct ambarella_gpio_irq_info		wifi_irq;
	struct ambarella_gpio_io_info		wifi_power;
	struct ambarella_gpio_io_info		wifi_reset;

	struct ambarella_gpio_irq_info		pmic_irq;
};
#define AMBA_BOARD_CALL(arg, perm) \
	module_param_cb(board_chip, &param_ops_uint, &(arg.board_chip), 0444); \
	module_param_cb(board_type, &param_ops_uint, &(arg.board_type), 0444); \
	module_param_cb(board_rev, &param_ops_uint, &(arg.board_rev), 0444); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##power_detect##_, arg.power_detect, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##power_control##_, arg.power_control, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##debug_led0##_, arg.debug_led0, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##debug_switch##_, arg.debug_switch, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##rs485##_, arg.rs485, perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##audio_irq##_, arg.audio_irq, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##audio_power##_, arg.audio_power, perm); \
	AMBA_GPIO_RESET_MODULE_PARAM_CALL(board_##audio_reset##_, arg.audio_reset, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##audio_speaker##_, arg.audio_speaker, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##audio_headphone##_, arg.audio_headphone, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##audio_microphone##_, arg.audio_microphone, perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##touch_panel_irq##_, arg.touch_panel_irq, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##touch_panel_power##_, arg.touch_panel_power, perm); \
	AMBA_GPIO_RESET_MODULE_PARAM_CALL(board_##touch_panel_reset##_, arg.touch_panel_reset, perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##lcd_irq##_, arg.lcd_irq, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##lcd_power##_, arg.lcd_power, perm); \
	AMBA_GPIO_RESET_MODULE_PARAM_CALL(board_##lcd_reset##_, arg.lcd_reset, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##lcd_backlight##_, arg.lcd_backlight, perm); \
	module_param_cb(board_lcd_spi_bus_id, &param_ops_int, &(arg.lcd_spi_hw.bus_id), perm); \
	module_param_cb(board_lcd_spi_cs_id, &param_ops_int, &(arg.lcd_spi_hw.cs_id), perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##vin_vsync##_, arg.vin_vsync, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##vin_power##_, arg.vin_power, perm); \
	AMBA_GPIO_RESET_MODULE_PARAM_CALL(board_##vin_reset##_, arg.vin_reset, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##vin_trigger##_, arg.vin_trigger, perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##vin_strobe##_, arg.vin_strobe, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##vin_hdmi_hpd##_, arg.vin_hdmi_hpd, perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##flash_charge_ready##_, arg.flash_charge_ready, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##flash_enable##_, arg.flash_enable, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##flash_trigger##_, arg.flash_trigger, perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##avplug_detect##_, arg.avplug_detect, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##hdmi_extpower##_, arg.hdmi_extpower, perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##gps_irq##_, arg.gps_irq, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##gps_power##_, arg.gps_power, perm); \
	AMBA_GPIO_RESET_MODULE_PARAM_CALL(board_##gps_reset##_, arg.gps_reset, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##gps_wakeup##_, arg.gps_wakeup, perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##lens_irq##_, arg.lens_irq, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##lens_power##_, arg.lens_power, perm); \
	AMBA_GPIO_RESET_MODULE_PARAM_CALL(board_##lens_reset##_, arg.lens_reset, perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##gyro_irq##_, arg.gyro_irq, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##gyro_power##_, arg.gyro_power, perm); \
	AMBA_GPIO_RESET_MODULE_PARAM_CALL(board_##gyro_reset##_, arg.gyro_reset, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##gyro_hps##_, arg.gyro_hps, perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##fm_irq##_, arg.fm_irq, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##fm_power##_, arg.fm_power, perm); \
	AMBA_GPIO_RESET_MODULE_PARAM_CALL(board_##fm_reset##_, arg.fm_reset, perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##gsensor_irq##_, arg.gsensor_irq, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##gsensor_power##_, arg.gsensor_power, perm); \
	AMBA_GPIO_RESET_MODULE_PARAM_CALL(board_##gsensor_reset##_, arg.gsensor_reset, perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##bb_irq##_, arg.gsensor_irq, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##bb_power##_, arg.gsensor_power, perm); \
	AMBA_GPIO_RESET_MODULE_PARAM_CALL(board_##bb_reset##_, arg.gsensor_reset, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##bb_en##_, arg.gsensor_power, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##bb_switch##_, arg.gsensor_power, perm); \
	AMBA_PWM_MODULE_PARAM_CALL(board_##pwm0##_, arg.pwm0_config, perm); \
	AMBA_PWM_MODULE_PARAM_CALL(board_##pwm1##_, arg.pwm1_config, perm); \
	AMBA_PWM_MODULE_PARAM_CALL(board_##pwm2##_, arg.pwm2_config, perm); \
	AMBA_PWM_MODULE_PARAM_CALL(board_##pwm3##_, arg.pwm3_config, perm); \
	AMBA_PWM_MODULE_PARAM_CALL(board_##pwm4##_, arg.pwm4_config, perm); \
	module_param_cb(board_wifi_sd_bus, &param_ops_int, &(arg.wifi_sd_bus), perm); \
	module_param_cb(board_wifi_sd_slot, &param_ops_int, &(arg.wifi_sd_slot), perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##wifi_irq##_, arg.wifi_irq, perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(board_##wifi_power##_, arg.wifi_power, perm); \
	AMBA_GPIO_RESET_MODULE_PARAM_CALL(board_##wifi_reset##_, arg.wifi_reset, perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(board_##pmic_irq##_, arg.pmic_irq, perm);

/* ==========================================================================*/
extern struct ambarella_board_info ambarella_board_generic;

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

