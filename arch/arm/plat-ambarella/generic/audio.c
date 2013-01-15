/*
 * arch/arm/plat-ambarella/generic/audio.c
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

#include <mach/hardware.h>
#include <plat/audio.h>

/* ==========================================================================*/
u32 alsa_tx_enable_flag = 0;
EXPORT_SYMBOL(alsa_tx_enable_flag);

/* ==========================================================================*/
static struct resource ambarella_i2s0_resources[] = {
	[0] = {
		.start	= I2S_BASE,
		.end	= I2S_BASE + 0x0FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= I2STX_IRQ,
		.end	= I2SRX_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static void aucodec_digitalio_on_0(void)
{
	/* aucodec_digitalio_on */
#if (CHIP_REV == A2S) || (CHIP_REV == A2M)
	amba_setbitsl(GPIO2_AFSEL_REG, (0xf << 18) | (0xf << 13));

#elif (CHIP_REV == A2)
	amba_setbitsl(GPIO2_AFSEL_REG, (0x3 << 15) | (0x3 << 20));

#elif (CHIP_REV == A3)||(CHIP_REV == A5)||(CHIP_REV == A6)|| \
	(CHIP_REV == A5S)||(CHIP_REV == I1)
	amba_clrbitsl(GPIO1_AFSEL_REG, 0x80000000);
	/* GPIO77~GPIO81 program as hardware mode */
	amba_setbitsl(GPIO2_AFSEL_REG, 0x0003e000);
#else
	pr_err("aucodec_digitalio_on: Unknown Chip Architecture\n");
#endif
}

static void aucodec_digitalio_on_1(void)
{
	/* aucodec_digitalio_on */
#if (CHIP_REV == A3)||(CHIP_REV == A5)||(CHIP_REV == A6)|| \
	(CHIP_REV == A5S)||(CHIP_REV == I1)
	amba_clrbitsl(GPIO1_AFSEL_REG, 0x80000000);
	/* GPIO77~GPIO78 and GPIO81~GPIO83 program as hardware mode */
	amba_setbitsl(GPIO2_AFSEL_REG, 0x000e6000);
#else
	pr_err("aucodec_digitalio_on: Unknown Chip Architecture\n");
#endif
}

static void aucodec_digitalio_on_2(void)
{
	/* aucodec_digitalio_on */
#if (CHIP_REV == A3)||(CHIP_REV == A5)||(CHIP_REV == A6)|| \
	(CHIP_REV == A5S)||(CHIP_REV == I1)
	amba_clrbitsl(GPIO1_AFSEL_REG, 0x80000000);
	/* GPIO77~GPIO78, GPIO81 and GPIO84~GPIO85 program as hardware mode */
	amba_setbitsl(GPIO2_AFSEL_REG, 0x00326000);
#else
	pr_err("aucodec_digitalio_on: Unknown Chip Architecture\n");
#endif
}

static void i2s_channel_select(u32 ch)
{
#if (CHIP_REV == A3)||(CHIP_REV == A5)||(CHIP_REV == A6)|| \
	(CHIP_REV == A5S)||(CHIP_REV == I1)
	u32 ch_reg_num;

	ch_reg_num = amba_readl(I2S_CHANNEL_SELECT_REG);

	switch (ch) {
	case 2:
		if (ch_reg_num != 0)
			amba_writel(I2S_CHANNEL_SELECT_REG, I2S_2CHANNELS_ENB);
		break;
	case 4:
		if (ch_reg_num != 1)
			amba_writel(I2S_CHANNEL_SELECT_REG, I2S_4CHANNELS_ENB);
		break;
	case 6:
		if (ch_reg_num != 2)
			amba_writel(I2S_CHANNEL_SELECT_REG, I2S_6CHANNELS_ENB);
		break;
	default:
		printk("Don't support %d channels\n", ch);
		break;
	}
#endif
}

static void set_audio_pll(u8 clksrc, u8 mclk)
{
#if (RCT_AUDIO_PLL_USE_HAL_API == 0)
	rct_set_aud_ctrl2_reg();
	rct_set_pll_frac_mode();
#endif
	rct_set_audio_pll_fs(clksrc, mclk);
}

static struct ambarella_i2s_controller ambarella_platform_i2s_controller0 = {
	.aucodec_digitalio_0	= aucodec_digitalio_on_0,
	.aucodec_digitalio_1	= aucodec_digitalio_on_1,
	.aucodec_digitalio_2	= aucodec_digitalio_on_2,
	.channel_select		= i2s_channel_select,
	.set_audio_pll		= set_audio_pll,
};

struct platform_device ambarella_i2s0 = {
	.name		= "ambarella-i2s",
	.id		= 0,
	.resource	= ambarella_i2s0_resources,
	.num_resources	= ARRAY_SIZE(ambarella_i2s0_resources),
	.dev		= {
		.platform_data		= &ambarella_platform_i2s_controller0,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

struct platform_device ambarella_pcm0 = {
	.name		= "ambarella-pcm-audio",
	.id		= -1,
};

struct platform_device ambarella_dummy_codec0 = {
	.name		= "ambdummy-codec",
	.id		= -1,
};

