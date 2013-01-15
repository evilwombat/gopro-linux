/*
 * arch/arm/mach-ambarella/init-elephant.c
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <mach/hardware.h>
#include <mach/init.h>
#include <mach/board.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/i2c.h>
#include <linux/i2c/pcf857x.h>
#include <linux/i2c/pca954x.h>

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <linux/input.h>
#include <plat/ambinput.h>

#include <linux/mmc/host.h>

#include "board-device.h"
#ifdef CONFIG_TIWLAN_SDIO
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <plat/sd.h>
#include <linux/wl12xx.h>
#endif
#ifdef CONFIG_TI_ST
#include <linux/ti_wilink_st.h>
#endif

#include <linux/rfkill-gpio.h>

#include <linux/mfd/wm831x/pdata.h>
/* ==========================================================================*/
#include <linux/pda_power.h>
#ifdef CONFIG_TIWLAN_SDIO

static struct sdio_embedded_func wifi_func_array[] = {
	{
		.f_class        = SDIO_CLASS_NONE,
		.f_maxblksize   = 512,
	},
	{
		.f_class        = SDIO_CLASS_WLAN,
		.f_maxblksize   = 512,
	},
};

static struct embedded_sdio_data omap_wifi_emb_data = {
	.cis    = {
		.vendor         = SDIO_VENDOR_ID_TI,
		.device         = SDIO_DEVICE_ID_TI_WL12xx,
		.blksize        = 512,
		.max_dtr        = 48000000,
	},
	.cccr   = {
		.multi_block	= 1,
		.low_speed	= 0,
		.wide_bus	= 1,
		.high_power	= 0,
		.high_speed	= 1,
		.disable_cd	= 1,
	},
	.funcs  = wifi_func_array,
	.quirks = MMC_QUIRK_VDD_165_195 | MMC_QUIRK_LENIENT_FUNC0,
};
#endif
#ifdef CONFIG_TI_ST
int plat_kim_suspend(struct platform_device *pdev, pm_message_t state)
{
    return 0;
}
int plat_kim_resume(struct platform_device *pdev)
{
    return 0;
}

/* wl128x BT, FM, GPS connectivity chip */
struct ti_st_plat_data wilink_pdata = {
	// OMAP4430 use GPIO55 to control BT_EN pin, please check which GPIO mapping for WL128X BT_EN pin.
	.nshutdown_gpio = GPIO(4),
	// OMAP4430 UART2's system node is /dev/ttyO1, please check which UART port mapping for WL128X in the platform.
	.dev_name = "/dev/ttyS1", //ambarella ttyS1
	.flow_cntrl = 1,
	.baud_rate = 115200,
	.suspend = plat_kim_suspend,
	.resume = plat_kim_resume,
};
static struct platform_device wl128x_device = {
	.name		= "kim",
	.id		= -1,
	.dev.platform_data = &wilink_pdata,
};
static struct platform_device btwilink_device = {
	.name = "btwilink",
	.id = -1,
};

#endif
static struct platform_device *ambarella_devices[] __initdata = {
	&ambarella_adc0,
#ifdef CONFIG_PLAT_AMBARELLA_SUPPORT_HW_CRYPTO
	&ambarella_crypto,
#endif
	&ambarella_ehci0,
	&ambarella_eth0,
	&ambarella_fb0,
	&ambarella_fb1,
	&ambarella_i2s0,
	&ambarella_pcm0,
	&ambarella_dummy_codec0,
	&ambarella_idc0,
	&ambarella_idc1,
	&ambarella_i2cmux,
	&ambarella_ir0,
	&ambarella_ohci0,
	&ambarella_ahci0,
	&ambarella_rtc0,
	&ambarella_sd0,
	&ambarella_sd1,
	&ambarella_spi0,
	&ambarella_spi1,
	&ambarella_spi2,
	&ambarella_spi3,
	&ambarella_spi4,
	&ambarella_uart,
	&ambarella_uart1,
	&ambarella_uart2,
	&ambarella_uart3,
	&ambarella_udc0,
	&ambarella_wdt0,
	&ambarella_fsg_device0,
#ifdef CONFIG_TI_ST
/* TI sample code origin register of WL128X BT, FM, GPS */
	&wl128x_device,
	&btwilink_device,
#endif
	&ambarella_usb_device0,
};

static struct platform_device *ambarella_pwm_devices[] __initdata = {
	&ambarella_pwm_platform_device0,
	&ambarella_pwm_platform_device1,
	&ambarella_pwm_platform_device2,
	&ambarella_pwm_platform_device3,
	&ambarella_pwm_platform_device4,
};

/* ==========================================================================*/
static struct spi_board_info ambarella_spi_devices[] = {
	[0] = {
		.modalias	= "spidev",
		.bus_num	= 0,
		.chip_select	= 0,
	},
	[1] = {
		.modalias	= "spidev",
		.bus_num	= 0,
		.chip_select	= 1,
	},
	[2] = {
		.modalias	= "spidev",
		.bus_num	= 0,
		.chip_select	= 2,
	},
	[3] = {
		.modalias	= "spidev",
		.bus_num	= 0,
		.chip_select	= 3,
	},
	[4] = {
		.modalias	= "spidev",
		.bus_num	= 0,
		.chip_select	= 4,
	},
	[5] = {
		.modalias	= "spidev",
		.bus_num	= 0,
		.chip_select	= 5,
	},
	[6] = {
		.modalias	= "spidev",
		.bus_num	= 0,
		.chip_select	= 6,
	},
	[7] = {
		.modalias	= "spidev",
		.bus_num	= 0,
		.chip_select	= 7,
	},
	[8] = {
		.modalias	= "spidev",
		.bus_num	= 1,
		.chip_select	= 0,
	},
	[9] = {
		.modalias	= "spidev",
		.bus_num	= 2,
		.chip_select	= 0,
	},
	[10] = {
		.modalias	= "spidev",
		.bus_num	= 2,
		.chip_select	= 1,
	},
	[11] = {
		.modalias	= "spidev",
		.bus_num	= 2,
		.chip_select	= 2,
	},
	[12] = {
		.modalias	= "spidev",
		.bus_num	= 2,
		.chip_select	= 3,
	},
	[13] = {
		.modalias	= "spidev",
		.bus_num	= 2,
		.chip_select	= 4,
	},
	[14] = {
		.modalias	= "spidev",
		.bus_num	= 2,
		.chip_select	= 5,
	},
	[15] = {
		.modalias	= "spidev",
		.bus_num	= 2,
		.chip_select	= 6,
	},
	[16] = {
		.modalias	= "spidev",
		.bus_num	= 2,
		.chip_select	= 7,
	},
	[17] = {
		.modalias	= "spidev",
		.bus_num	= 3,
		.chip_select	= 0,
	},
	[18] = {
		.modalias	= "spidev",
		.bus_num	= 4,
		.chip_select	= 0,
	}
};

/* ==========================================================================*/
static struct ambarella_key_table elephant_keymap[AMBINPUT_TABLE_SIZE] = {
	{AMBINPUT_VI_KEY,	{.vi_key	= {0,	0,	0}}},
	{AMBINPUT_VI_REL,	{.vi_rel	= {0,	0,	0}}},
	{AMBINPUT_VI_ABS,	{.vi_abs	= {0,	0,	0}}},
	{AMBINPUT_VI_SW,	{.vi_sw		= {0,	0,	0}}},

	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_POWER,	3,	0x001a0015}}},	//POWER
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_HOME,	3,	0x001a0061}}},	//HOME
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_MENU,	3,	0x001a004d}}},	//MENU
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_ESC,	3,	0x001a0063}}},	//BACK
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_SEND,	3,	0x001a0010}}},	//CALL
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_END,	3,	0x001a0020}}},	//ENDCALL
	{AMBINPUT_IR_SW,	{.ir_key	= {0,		0,	0x001a003d}}},	//SW0 OFF
	{AMBINPUT_IR_SW,	{.ir_key	= {0,		1,	0x001a0064}}},	//SW0 ON
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_VOLUMEUP,3,	0x001a0077}}},	//VOLUME_UP
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_VOLUMEDOWN,3,	0x001a0078}}},	//VOLUME_DOWN
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_OK,	3,	0x001a0076}}},	//DPAD_CENTER
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_S,	3,	0x001a0073}}},	//S
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_W,	3,	0x001a0072}}},	//W
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_A,	3,	0x001a0075}}},	//A
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_D,	3,	0x001a0074}}},	//D
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_SEARCH,	3,	0x001a007e}}},	//SEARCH

	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_HOME,	1,	1,	GPIO(120),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},
	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_MENU,	1,	1,	GPIO(121),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},
	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_ESC,	1,	1,	GPIO(122),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},
	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_POWER,	1,	1,	GPIO(123),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},

	{AMBINPUT_END},
};

static struct ambarella_key_table elephant_keymap_vendor1[AMBINPUT_TABLE_SIZE] = {
	{AMBINPUT_VI_KEY,	{.vi_key	= {0,	0,	0}}},
	{AMBINPUT_VI_REL,	{.vi_rel	= {0,	0,	0}}},
	{AMBINPUT_VI_ABS,	{.vi_abs	= {0,	0,	0}}},
	{AMBINPUT_VI_SW,	{.vi_sw		= {0,	0,	0}}},

	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_ESC,	0,	1,	GPIO(186),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},

	{AMBINPUT_END},
};

static struct ambarella_key_table elephant_keymap_vendor51[AMBINPUT_TABLE_SIZE] = {
	{AMBINPUT_VI_KEY,	{.vi_key	= {0,	0,	0}}},
	{AMBINPUT_VI_REL,	{.vi_rel	= {0,	0,	0}}},
	{AMBINPUT_VI_ABS,	{.vi_abs	= {0,	0,	0}}},
	{AMBINPUT_VI_SW,	{.vi_sw		= {0,	0,	0}}},

	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_HOME,	1,	1,	GPIO(144),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},
	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_MENU,	1,	1,	GPIO(145),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},
	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_ESC,	1,	1,	GPIO(146),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},
	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_SEARCH,	1,	1,	GPIO(147),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},

	{AMBINPUT_END},
};


static struct ambarella_key_table elephant_keymap_evk[AMBINPUT_TABLE_SIZE] = {
	{AMBINPUT_VI_KEY,	{.vi_key	= {0,	0,	0}}},
	{AMBINPUT_VI_REL,	{.vi_rel	= {0,	0,	0}}},
	{AMBINPUT_VI_ABS,	{.vi_abs	= {0,	0,	0}}},
	{AMBINPUT_VI_SW,	{.vi_sw		= {0,	0,	0}}},

	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_POWER,	3,	0x001a0015}}},	//POWER
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_HOME,	3,	0x001a0061}}},	//HOME
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_MENU,	3,	0x001a004d}}},	//MENU
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_ESC,	3,	0x001a0063}}},	//BACK
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_SEND,	3,	0x001a0010}}},	//CALL
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_END,	3,	0x001a0020}}},	//ENDCALL
	{AMBINPUT_IR_SW,	{.ir_key	= {0,		0,	0x001a003d}}},	//SW0 OFF
	{AMBINPUT_IR_SW,	{.ir_key	= {0,		1,	0x001a0064}}},	//SW0 ON
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_VOLUMEUP,3,	0x001a0077}}},	//VOLUME_UP
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_VOLUMEDOWN,3,	0x001a0078}}},	//VOLUME_DOWN
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_OK,	3,	0x001a0076}}},	//DPAD_CENTER
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_S,	3,	0x001a0073}}},	//S
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_W,	3,	0x001a0072}}},	//W
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_A,	3,	0x001a0075}}},	//A
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_D,	3,	0x001a0074}}},	//D
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_SEARCH,	3,	0x001a007e}}},	//SEARCH

	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_RESERVED,	0,	2,	3000,	4095}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_HOME,		0,	2,	1820,	2250}}},	//HOME
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_MENU,		0,	2,	1450,	1750}}},	//MENU
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_ESC,		0,	2,	1050,	1350}}},	//ESC
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_SEARCH,		0,	2,	700,	900}}},		//SEARCH

	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_RESERVED,	0,	4,	3000,	4095}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_VOLUMEDOWN,	0,	4,	1750,	2150}}},	//VOLUME DOWN
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_VOLUMEUP,	0,	4,	1300,	1650}}},	//VOLUME UP
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_STOP,		0,	4,	850,	1100}}},	//STOP
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_RECORD,		0,	4,	400,	600}}},		//RECORD

	{AMBINPUT_END},
};

static struct ambarella_input_board_info elephant_board_input_info = {
	.pkeymap		= elephant_keymap,
	.pinput_dev		= NULL,
	.pdev			= NULL,

	.abx_max_x		= 4095,
	.abx_max_y		= 4095,
	.abx_max_pressure	= 4095,
	.abx_max_width		= 16,
};

struct platform_device elephant_board_input = {
	.name		= "ambarella-input",
	.id		= -1,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &elephant_board_input_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

static struct rfkill_gpio_platform_data elephant_board_bt_info = {
	.name		= "bt-gpio",
	.reset_gpio	= GPIO(190),
	.shutdown_gpio	= -1,
	.type		= RFKILL_TYPE_BLUETOOTH,
};

static struct platform_device elephant_bt_rfkill = {
	.name		= "rfkill_gpio",
	.id		= -1,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &elephant_board_bt_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};


/* ==========================================================================*/
static struct pcf857x_platform_data elephant_board_ext_gpio0_pdata = {
	.gpio_base	= EXT_GPIO(0),
};

static struct i2c_board_info elephant_board_ext_gpio_info = {
	.type			= "pcf8574a",
	.addr			= 0x3e,
	.flags			= 0,
	.platform_data		= &elephant_board_ext_gpio0_pdata,
};

static struct pca954x_platform_mode elephant_board_ext_i2c_mode = {
	.adap_id		= 0,
	.deselect_on_exit	= 1,
};

static struct pca954x_platform_data elephant_board_ext_i2c_pdata = {
	.modes			= &elephant_board_ext_i2c_mode,
	.num_modes		= 0,
};

static struct i2c_board_info elephant_board_ext_i2c_info = {
	.type			= "pca9542",
	.addr			= 0x70,
	.flags			= 0,
	.platform_data		= &elephant_board_ext_i2c_pdata,
};

/* ==========================================================================*/
static void __init ambarella_init_elephant(void)
{
	int					i, ret;
	int					use_bub_default = 1;

	ambarella_init_machine("Elephant");

	if (AMBARELLA_BOARD_TYPE(system_rev) == AMBARELLA_BOARD_TYPE_EVK) {
		switch (AMBARELLA_BOARD_REV(system_rev)) {
		case 'B':
			ambarella_platform_sd_controller1.slot[0].caps |= MMC_CAP_NONREMOVABLE;
			ambarella_platform_sd_controller1.slot[0].ext_power.gpio_id = GPIO(111);
			ambarella_platform_sd_controller1.slot[0].ext_power.active_level = GPIO_HIGH;
			ambarella_platform_sd_controller1.slot[0].ext_power.active_delay = 300;
		 	/* the cs_pin of spi0.1 is used to determine wm8994's
		 	 * I2C address, and the cs_pin of spi0.4, spi0,5, spi0.6
		 	 * spi0.7 are used as I2S signals, so we need to prevent
		 	 * them from be modified by SPI driver */
			ambarella_spi0_cs_pins[1] = -1;
			ambarella_spi0_cs_pins[4] = -1;
			ambarella_spi0_cs_pins[5] = -1;
			ambarella_spi0_cs_pins[6] = -1;
			ambarella_spi0_cs_pins[7] = -1;
			ambarella_init_wm8994();
		case 'A':
			ambarella_board_generic.lcd_reset.gpio_id = GPIO(105);
			ambarella_board_generic.lcd_reset.active_level = GPIO_LOW;
			ambarella_board_generic.lcd_reset.active_delay = 10;

			ambarella_board_generic.lcd_backlight.gpio_id = GPIO(16);
			ambarella_board_generic.lcd_backlight.active_level = GPIO_HIGH;
			ambarella_board_generic.lcd_backlight.active_delay = 1;

			ambarella_board_generic.lcd_spi_hw.bus_id = 2;
			ambarella_board_generic.lcd_spi_hw.cs_id = 0;

			ambarella_board_generic.lcd_spi_cfg.spi_mode = 0;
			ambarella_board_generic.lcd_spi_cfg.cfs_dfs = 16;
			ambarella_board_generic.lcd_spi_cfg.baud_rate = 5000000;
			ambarella_board_generic.lcd_spi_cfg.lsb_first = 0;
			ambarella_board_generic.lcd_spi_cfg.cs_change = 0;

			ambarella_board_generic.touch_panel_irq.irq_gpio = GPIO(44);
			ambarella_board_generic.touch_panel_irq.irq_line = gpio_to_irq(44);
			ambarella_board_generic.touch_panel_irq.irq_type = IRQF_TRIGGER_FALLING;
			ambarella_board_generic.touch_panel_irq.irq_gpio_val = GPIO_LOW;
			ambarella_board_generic.touch_panel_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

			ambarella_board_generic.hdmi_extpower.gpio_id = GPIO(104);
			ambarella_board_generic.hdmi_extpower.active_level = GPIO_HIGH;
			ambarella_board_generic.hdmi_extpower.active_delay = 1;

			ambarella_board_generic.wifi_power.gpio_id = GPIO(109);
			ambarella_board_generic.wifi_power.active_level = GPIO_HIGH;
			ambarella_board_generic.wifi_power.active_delay = 300;
			ambarella_board_generic.wifi_sd_bus = 0;
			ambarella_board_generic.wifi_sd_slot = 1;

			ambarella_board_generic.pmic_irq.irq_gpio = GPIO(54);
			ambarella_board_generic.pmic_irq.irq_line = gpio_to_irq(54);
			ambarella_board_generic.pmic_irq.irq_type = IRQF_TRIGGER_FALLING;
			ambarella_board_generic.pmic_irq.irq_gpio_val = GPIO_LOW;
			ambarella_board_generic.pmic_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

			ambarella_board_generic.power_control.gpio_id = GPIO(120);
			ambarella_board_generic.power_control.active_level = GPIO_LOW;

			memcpy(ambarella_spi_devices[12].modalias, "wm8310", 6);
			ambarella_spi_devices[12].max_speed_hz = 500000;
			ambarella_spi_devices[12].platform_data = &elephant_wm8310_pdata;

			ambarella_board_generic.gsensor_power.gpio_id = GPIO(151);
			ambarella_board_generic.gsensor_power.active_level = GPIO_HIGH;
			ambarella_board_generic.gsensor_power.active_delay = 10;

			ambarella_board_generic.gsensor_irq.irq_gpio = GPIO(49);
			ambarella_board_generic.gsensor_irq.irq_line = gpio_to_irq(GPIO(49));
			ambarella_board_generic.gsensor_irq.irq_type = IRQF_TRIGGER_RISING;
			ambarella_board_generic.gsensor_irq.irq_gpio_val = GPIO_LOW;
			ambarella_board_generic.gsensor_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

			ambarella_eth0_platform_info.mii_id = 0;
			ambarella_eth0_platform_info.phy_id = 0x001cc912;
			ambarella_eth0_platform_info.phy_irq.irq_gpio = GPIO(21);
			ambarella_eth0_platform_info.phy_irq.irq_line = gpio_to_irq(GPIO(21));
			ambarella_eth0_platform_info.phy_irq.irq_type = IRQF_TRIGGER_LOW;
			ambarella_eth0_platform_info.phy_irq.irq_gpio_val = GPIO_LOW;
			ambarella_eth0_platform_info.phy_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;
			ambarella_eth0_platform_info.mii_power.gpio_id = GPIO(97);
			ambarella_eth0_platform_info.mii_power.active_level = GPIO_HIGH;
			ambarella_eth0_platform_info.mii_power.active_delay = 20;
			ambarella_eth0_platform_info.mii_reset.gpio_id = GPIO(98);
			ambarella_eth0_platform_info.mii_reset.active_level = GPIO_LOW;
			ambarella_eth0_platform_info.mii_reset.active_delay = 20;

			fio_default_owner = SELECT_FIO_SDIO;
			ambarella_platform_sd_controller0.clk_limit = 24000000;
			ambarella_platform_sd_controller0.slot[0].use_bounce_buffer = 1;
			ambarella_platform_sd_controller0.slot[0].caps |= (MMC_CAP_8_BIT_DATA | MMC_CAP_NONREMOVABLE);
			ambarella_platform_sd_controller0.slot[0].max_blk_sz = SD_BLK_SZ_128KB;
			ambarella_platform_sd_controller0.slot[0].cd_delay = 100;
			ambarella_platform_sd_controller0.slot[0].fixed_cd = 1;
			ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_gpio = -1;
			ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_line = -1;
			ambarella_platform_sd_controller0.slot[0].fixed_wp = 0;
			ambarella_platform_sd_controller0.slot[0].gpio_wp.gpio_id = -1;
			ambarella_platform_sd_controller0.slot[0].ext_power.gpio_id = GPIO(157);
			ambarella_platform_sd_controller0.slot[0].ext_power.active_level = GPIO_HIGH;
			ambarella_platform_sd_controller0.slot[0].ext_power.active_delay = 300;
			ambarella_platform_sd_controller0.slot[1].use_bounce_buffer = 1;
			ambarella_platform_sd_controller0.slot[1].max_blk_sz = SD_BLK_SZ_128KB;
			ambarella_platform_sd_controller0.slot[1].cd_delay = 100;
			ambarella_platform_sd_controller0.slot[1].fixed_cd = 0;
			ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_gpio = -1;
			ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_line = -1;
			ambarella_platform_sd_controller0.slot[1].fixed_wp = 0;
			ambarella_platform_sd_controller0.slot[1].gpio_wp.gpio_id = -1;
			ambarella_platform_sd_controller1.clk_limit = 24000000;
			ambarella_platform_sd_controller1.slot[0].use_bounce_buffer = 1;
			ambarella_platform_sd_controller1.slot[0].caps |= (MMC_CAP_8_BIT_DATA | MMC_CAP_BUS_WIDTH_TEST);
			ambarella_platform_sd_controller1.slot[0].max_blk_sz = SD_BLK_SZ_128KB;
			ambarella_platform_sd_controller1.slot[0].cd_delay = 100;

			ambarella_tm1726_board_info.irq = ambarella_board_generic.touch_panel_irq.irq_line;
			i2c_register_board_info(2, &ambarella_tm1726_board_info, 1);

			ambarella_board_generic.vin_power.gpio_id = GPIO(122);
			ambarella_board_generic.vin_power.active_level = GPIO_HIGH;
			ambarella_board_generic.vin_power.active_delay = 1;
			i2c_register_board_info(0, ambarella_board_vin_infos, ARRAY_SIZE(ambarella_board_vin_infos));

			platform_device_register(&elephant_bt_rfkill);
			platform_device_register(&lcd_1p3831);

			elephant_board_input_info.pkeymap = elephant_keymap_evk;

			use_bub_default = 0;
			break;

		default:
			pr_warn("%s: Unknown EVK Rev[%d]\n", __func__, AMBARELLA_BOARD_REV(system_rev));
			break;
		}
	}

	if (AMBARELLA_BOARD_TYPE(system_rev) == AMBARELLA_BOARD_TYPE_VENDOR) {
		switch (AMBARELLA_BOARD_REV(system_rev)) {
		case 51:
			ambarella_board_generic.lcd_reset.gpio_id = GPIO(103);
			ambarella_board_generic.lcd_reset.active_level = GPIO_LOW;
			ambarella_board_generic.lcd_reset.active_delay = 10;

			ambarella_board_generic.lcd_backlight.gpio_id = GPIO(16);
			ambarella_board_generic.lcd_backlight.active_level = GPIO_HIGH;
			ambarella_board_generic.lcd_backlight.active_delay = 1;

			ambarella_board_generic.lcd_spi_hw.bus_id = 1;
			ambarella_board_generic.lcd_spi_hw.cs_id = 0;

			ambarella_board_generic.lcd_spi_cfg.spi_mode = 0;
			ambarella_board_generic.lcd_spi_cfg.cfs_dfs = 16;
			ambarella_board_generic.lcd_spi_cfg.baud_rate = 5000000;
			ambarella_board_generic.lcd_spi_cfg.lsb_first = 0;
			ambarella_board_generic.lcd_spi_cfg.cs_change = 0;

			ambarella_board_generic.touch_panel_irq.irq_gpio = GPIO(118);
			ambarella_board_generic.touch_panel_irq.irq_line = gpio_to_irq(118);
			ambarella_board_generic.touch_panel_irq.irq_type = IRQF_TRIGGER_FALLING;
			ambarella_board_generic.touch_panel_irq.irq_gpio_val = GPIO_LOW;
			ambarella_board_generic.touch_panel_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

			ambarella_board_generic.gsensor_irq.irq_gpio = GPIO(49);
			ambarella_board_generic.gsensor_irq.irq_line = gpio_to_irq(GPIO(49));
			ambarella_board_generic.gsensor_irq.irq_type = IRQF_TRIGGER_RISING;
			ambarella_board_generic.gsensor_irq.irq_gpio_val = GPIO_LOW;
			ambarella_board_generic.gsensor_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

			ambarella_eth0_platform_info.mii_id = 1;
			ambarella_eth0_platform_info.phy_id = 0x00008201;
			ambarella_eth0_platform_info.mii_reset.gpio_id = GPIO(187);
			ambarella_eth0_platform_info.mii_reset.active_level = GPIO_LOW;
			ambarella_eth0_platform_info.mii_reset.active_delay = 20;

			fio_default_owner = SELECT_FIO_SDIO;
			ambarella_platform_sd_controller0.clk_limit = 24000000;
			ambarella_platform_sd_controller0.slot[0].use_bounce_buffer = 1;
			ambarella_platform_sd_controller0.slot[0].caps |= (MMC_CAP_8_BIT_DATA | MMC_CAP_NONREMOVABLE);
			ambarella_platform_sd_controller0.slot[0].max_blk_sz = SD_BLK_SZ_128KB;
			ambarella_platform_sd_controller0.slot[0].cd_delay = 100;
			ambarella_platform_sd_controller0.slot[0].fixed_cd = 1;
			ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_gpio = -1;
			ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_line = -1;
			ambarella_platform_sd_controller0.slot[0].fixed_wp = 0;
			ambarella_platform_sd_controller0.slot[0].gpio_wp.gpio_id = -1;
			ambarella_platform_sd_controller0.slot[1].use_bounce_buffer = 1;
			ambarella_platform_sd_controller0.slot[1].max_blk_sz = SD_BLK_SZ_128KB;
			ambarella_platform_sd_controller0.slot[1].cd_delay = 100;
			ambarella_platform_sd_controller0.slot[1].fixed_cd = 0;
			ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_gpio = -1;
			ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_line = -1;
			ambarella_platform_sd_controller0.slot[1].fixed_wp = 0;
			ambarella_platform_sd_controller0.slot[1].gpio_wp.gpio_id = -1;
			ambarella_platform_sd_controller1.clk_limit = 24000000;
			ambarella_platform_sd_controller1.slot[0].ext_power.gpio_id = -1;
			ambarella_platform_sd_controller1.slot[0].use_bounce_buffer = 1;
			ambarella_platform_sd_controller1.slot[0].caps |=
				(MMC_CAP_8_BIT_DATA | MMC_CAP_BUS_WIDTH_TEST | MMC_CAP_NONREMOVABLE);
			ambarella_platform_sd_controller1.slot[0].max_blk_sz = SD_BLK_SZ_128KB;
			ambarella_platform_sd_controller1.slot[0].cd_delay = 100;

			ambarella_tm1726_board_info.irq = ambarella_board_generic.touch_panel_irq.irq_line;
			i2c_register_board_info(2, &ambarella_tm1726_board_info, 1);

			ambarella_board_generic.vin_power.gpio_id = GPIO(113);
			ambarella_board_generic.vin_power.active_level = GPIO_HIGH;
			ambarella_board_generic.vin_power.active_delay = 10;
			i2c_register_board_info(2, ambarella_board_vin_infos, ARRAY_SIZE(ambarella_board_vin_infos));

			platform_device_register(&lcd_1p3831);

			elephant_board_input_info.pkeymap = elephant_keymap_vendor51;
			break;
		case 2:
			ambarella_board_generic.wifi_power.gpio_id = GPIO(128);
			ambarella_board_generic.wifi_power.active_level = GPIO_HIGH;
			ambarella_board_generic.wifi_power.active_delay = 300;
			/* sdio slot */
			ambarella_board_generic.wifi_sd_bus = 0;
			ambarella_board_generic.wifi_sd_slot = 1;
		case 1:
			ambarella_board_generic.lcd_power.gpio_id = GPIO(41);
			ambarella_board_generic.lcd_power.active_level = GPIO_HIGH;
			ambarella_board_generic.lcd_power.active_delay = 1;

			ambarella_board_generic.lcd_reset.gpio_id = GPIO(32);
			ambarella_board_generic.lcd_reset.active_level = GPIO_LOW;
			ambarella_board_generic.lcd_reset.active_delay = 1;

			ambarella_board_generic.lcd_backlight.gpio_id = GPIO(16);
			ambarella_board_generic.lcd_backlight.active_level = GPIO_HIGH;
			ambarella_board_generic.lcd_backlight.active_delay = 1;

			ambarella_board_generic.lcd_spi_hw.bus_id = 0;
			ambarella_board_generic.lcd_spi_hw.cs_id = 1;

			ambarella_board_generic.touch_panel_reset.gpio_id = GPIO(29);
			ambarella_board_generic.touch_panel_reset.active_level = GPIO_LOW;
			ambarella_board_generic.touch_panel_reset.active_delay = 1;

			ambarella_board_generic.touch_panel_irq.irq_gpio = GPIO(51);
			ambarella_board_generic.touch_panel_irq.irq_line = gpio_to_irq(51);
			ambarella_board_generic.touch_panel_irq.irq_type = IRQF_TRIGGER_FALLING;
			ambarella_board_generic.touch_panel_irq.irq_gpio_val = GPIO_LOW;
			ambarella_board_generic.touch_panel_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

			fio_default_owner = SELECT_FIO_SDIO;
			ambarella_platform_sd_controller0.clk_limit = 24000000;
			ambarella_platform_sd_controller0.slot[0].use_bounce_buffer = 1;
			ambarella_platform_sd_controller0.slot[0].max_blk_sz = SD_BLK_SZ_128KB;
			ambarella_platform_sd_controller0.slot[0].cd_delay = 100;
			ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_gpio = GPIO(67);
			ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_line = gpio_to_irq(67);
			ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_type = IRQ_TYPE_EDGE_BOTH;
			ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_gpio_val = GPIO_LOW;
			ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_gpio_mode = GPIO_FUNC_SW_INPUT;
			ambarella_platform_sd_controller0.slot[0].gpio_wp.gpio_id = GPIO(68);
			ambarella_platform_sd_controller0.slot[1].use_bounce_buffer = 1;
			ambarella_platform_sd_controller0.slot[1].max_blk_sz = SD_BLK_SZ_128KB;
			ambarella_platform_sd_controller0.slot[1].cd_delay = 100;
			ambarella_platform_sd_controller0.slot[1].fixed_cd = 1;
			ambarella_platform_sd_controller0.slot[1].fixed_wp = 0;
			ambarella_platform_sd_controller1.clk_limit = 24000000;
			ambarella_platform_sd_controller1.slot[0].cd_delay = 100;
			ambarella_platform_sd_controller1.slot[0].use_bounce_buffer = 1;
			ambarella_platform_sd_controller1.slot[0].max_blk_sz = SD_BLK_SZ_128KB;

			ambarella_nt11001_board_info.irq = ambarella_board_generic.touch_panel_irq.irq_line;
			ambarella_nt11001_board_info.flags = 0;
			i2c_register_board_info(0, &ambarella_nt11001_board_info, 1);

			elephant_board_input_info.pkeymap = elephant_keymap_vendor1;
			break;

		default:
			pr_warn("%s: Unknown VENDOR Rev[%d]\n", __func__, AMBARELLA_BOARD_REV(system_rev));
			break;
		}

		use_bub_default = 0;
	}

	if (use_bub_default) {
		pr_info("%s: Default BUB config:\n", __func__);
		ambarella_board_generic.lcd_reset.gpio_id = GPIO(46);
		ambarella_board_generic.lcd_reset.active_level = GPIO_LOW;
		ambarella_board_generic.lcd_reset.active_delay = 1;

		ambarella_board_generic.lcd_spi_hw.bus_id = 3;
		ambarella_board_generic.lcd_spi_hw.cs_id = 0;

		ambarella_board_generic.touch_panel_irq.irq_gpio = GPIO(45);
		ambarella_board_generic.touch_panel_irq.irq_line = gpio_to_irq(45);
		ambarella_board_generic.touch_panel_irq.irq_type = IRQF_TRIGGER_FALLING;
		ambarella_board_generic.touch_panel_irq.irq_gpio_val = GPIO_LOW;
		ambarella_board_generic.touch_panel_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

		ambarella_eth0_platform_info.mii_id = 0;
		ambarella_eth0_platform_info.phy_id = 0x001cc912;

		fio_default_owner = SELECT_FIO_SDIO;
#ifdef CONFIG_TIWLAN_SDIO
		//because Ione chip is too far away from TI chip on bub board ,TI sdio is not stable in this case .;
		ambarella_platform_sd_controller0.clk_limit = 2000000;//24000000
#else
		ambarella_platform_sd_controller0.clk_limit = 24000000;
#endif
		ambarella_platform_sd_controller0.slot[0].use_bounce_buffer = 1;
#ifdef CONFIG_TIWLAN_SDIO
		ambarella_platform_sd_controller0.slot[0].max_blk_sz = SD_BLK_SZ_256KB;//SD_BLK_SZ_128KB;
#else
		ambarella_platform_sd_controller0.slot[0].max_blk_sz = SD_BLK_SZ_128KB;
#endif
		ambarella_platform_sd_controller0.slot[0].cd_delay = 100;
		ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_gpio = GPIO(67);
		ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_line = gpio_to_irq(67);
		ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_type = IRQ_TYPE_EDGE_BOTH;
		ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_gpio_val = GPIO_LOW;
		ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_gpio_mode = GPIO_FUNC_SW_INPUT;
		ambarella_platform_sd_controller0.slot[0].gpio_wp.gpio_id = GPIO(68);

#ifdef CONFIG_TIWLAN_SDIO
		ambarella_platform_sd_controller0.slot[0].embedded_sdio = &omap_wifi_emb_data;
		ambarella_platform_sd_controller0.slot[0].register_status_notify =&omap_wifi_status_register;
		ambarella_platform_sd_controller0.slot[0].card_detect = &omap_wifi_status;
		ambarella_platform_sd_controller1.slot[0].embedded_sdio = NULL;
#endif
		ambarella_platform_sd_controller0.slot[1].use_bounce_buffer = 1;
		ambarella_platform_sd_controller0.slot[1].max_blk_sz = SD_BLK_SZ_128KB;
		ambarella_platform_sd_controller0.slot[1].cd_delay = 100;
		ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_gpio = GPIO(75);
		ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_line = gpio_to_irq(75);
		ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_type = IRQ_TYPE_EDGE_BOTH;
		ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_gpio_val = GPIO_LOW;
		ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_gpio_mode = GPIO_FUNC_SW_INPUT;
		ambarella_platform_sd_controller0.slot[1].gpio_wp.gpio_id = GPIO(76);
		ambarella_platform_sd_controller1.clk_limit = 24000000;
		ambarella_platform_sd_controller1.slot[0].cd_delay = 100;
		ambarella_platform_sd_controller1.slot[0].use_bounce_buffer = 1;
		ambarella_platform_sd_controller1.slot[0].max_blk_sz = SD_BLK_SZ_128KB;
		ambarella_platform_sd_controller1.slot[0].ext_power.gpio_id = GPIO(106);
		ambarella_platform_sd_controller1.slot[0].ext_power.active_level = GPIO_HIGH;
		ambarella_platform_sd_controller1.slot[0].ext_power.active_delay = 300;

		ambarella_tm1510_board_info.irq = ambarella_board_generic.touch_panel_irq.irq_line;
/*<-------------------------------------wl12xx start --------------------------------------------->*/
/*-->WiFi_EN  = GPIO2 WiFi_IRQ=GPIO3 BT_EN=GPIO4<--*/
#ifdef CONFIG_TIWLAN_SDIO
		ambarella_board_generic.wifi_sd_bus = 0;
		ambarella_board_generic.wifi_sd_slot = 0;
		ambarella_board_generic.wifi_irq.irq_gpio = GPIO(3);
		ambarella_board_generic.wifi_irq.irq_line = gpio_to_irq(3);
		ambarella_board_generic.wifi_irq.irq_type = IRQF_TRIGGER_FALLING;
		ambarella_board_generic.wifi_irq.irq_gpio_val = GPIO_LOW;
		ambarella_board_generic.wifi_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;
		ambarella_board_generic.wifi_power.gpio_id = GPIO(2);
		ambarella_board_generic.wifi_power.active_level = GPIO_HIGH;
		ambarella_board_generic.wifi_power.active_delay = 300;
#endif
#ifdef CONFIG_TI_ST
		struct ambarella_gpio_io_info		BT_power;
        BT_power.gpio_id = GPIO(4);
		BT_power.active_level = GPIO_HIGH;
		BT_power.active_delay = 1;

		ambarella_set_gpio_output(&BT_power, 0);
		mdelay(10);
		ambarella_set_gpio_output(&BT_power, 1);
		mdelay(100);
		ambarella_set_gpio_output(&BT_power, 0);

#endif
/*	<----------------------------------- wl12xxx end-------------------------------------------->*/
		i2c_register_board_info(2, &ambarella_tm1510_board_info, 1);

		i2c_register_board_info(0, &elephant_board_ext_gpio_info, 1);
		i2c_register_board_info(0, &elephant_board_ext_i2c_info, 1);

		i2c_register_board_info(2, ambarella_board_vin_infos, ARRAY_SIZE(ambarella_board_vin_infos));
	}

	ambarella_platform_ir_controller0.protocol = AMBA_IR_PROTOCOL_SONY;

	platform_add_devices(ambarella_devices, ARRAY_SIZE(ambarella_devices));
	for (i = 0; i < ARRAY_SIZE(ambarella_devices); i++) {
		device_set_wakeup_capable(&ambarella_devices[i]->dev, 1);
		device_set_wakeup_enable(&ambarella_devices[i]->dev, 0);
	}

	for (i = 0; i < ARRAY_SIZE(ambarella_pwm_devices); i++) {
		if (i == 1
			&& AMBARELLA_BOARD_TYPE(system_rev) == AMBARELLA_BOARD_TYPE_BUB)
			continue;
		if (i == 2
			&& AMBARELLA_BOARD_TYPE(system_rev) == AMBARELLA_BOARD_TYPE_VENDOR
			&& AMBARELLA_BOARD_REV(system_rev) == 2)
			continue;
		if (i == 2
			&& AMBARELLA_BOARD_TYPE(system_rev) == AMBARELLA_BOARD_TYPE_BUB)
			continue;
		if (i == 4
			&& AMBARELLA_BOARD_TYPE(system_rev) == AMBARELLA_BOARD_TYPE_EVK)
			continue;
		ret = platform_device_register(ambarella_pwm_devices[i]);
		if (ret)
			continue;
		device_set_wakeup_capable(&ambarella_pwm_devices[i]->dev, 1);
		device_set_wakeup_enable(&ambarella_pwm_devices[i]->dev, 0);
	}

	spi_register_board_info(ambarella_spi_devices, ARRAY_SIZE(ambarella_spi_devices));

	platform_device_register(&elephant_board_input);

	i2c_register_board_info(1, &ambarella_board_hdmi_info, 1);
}

/* ==========================================================================*/
MACHINE_START(ELEPHANT, "Elephant")
	.boot_params	= CONFIG_AMBARELLA_PARAMS_PHYS,
	.map_io		= ambarella_map_io,
	.reserve	= ambarella_memblock_reserve,
	.init_irq	= ambarella_init_irq,
	.timer		= &ambarella_timer,
	.init_machine	= ambarella_init_elephant,
MACHINE_END

