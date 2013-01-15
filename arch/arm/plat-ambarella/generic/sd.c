/*
 * arch/arm/plat-ambarella/generic/sd.c
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
#include <linux/irq.h>

#include <linux/mmc/host.h>

#include <mach/hardware.h>
#include <plat/fio.h>
#include <plat/sd.h>
#include <hal/hal.h>
#include <linux/aipc/ipc_mutex.h>

#if  defined(CONFIG_AMBARELLA_IPC) && defined(CONFIG_BOSS_MULTIPLE_CORE)
#define ENABLE_IPC_MUTEX_SD2   1
#else
#define ENABLE_IPC_MUTEX_SD2   0
#endif

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

/* ==========================================================================*/
typedef void (*mmc_dc_fn)(struct mmc_host *host, unsigned long delay);

int ambarella_sd_set_fixed_cd(const char *val, const struct kernel_param *kp)
{
	struct ambarella_sd_slot		*pslotinfo;
	mmc_dc_fn				mmc_dc = NULL;
	int					retval;

	retval = param_set_int(val, kp);
	pslotinfo = container_of(kp->arg, struct ambarella_sd_slot, fixed_cd);
#if defined(CONFIG_MMC)
	mmc_dc = mmc_detect_change;
#else
	mmc_dc = (mmc_dc_fn)module_kallsyms_lookup_name("mmc_detect_change");
#endif
	if (!retval && pslotinfo && mmc_dc && pslotinfo->pmmc_host) {
		mmc_dc(pslotinfo->pmmc_host, pslotinfo->cd_delay);
	}

	return retval;
}

static struct kernel_param_ops param_ops_ambarella_sd_cdpos = {
	.set = ambarella_sd_set_fixed_cd,
	.get = param_get_int,
};

/* ==========================================================================*/
struct resource ambarella_sd0_resources[] = {
	[0] = {
		.start	= SD_BASE,
		.end	= SD_BASE + 0x0FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SD_IRQ,
		.end	= SD_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

void fio_amb_sd0_slot1_request(void)
{
	fio_select_lock(SELECT_FIO_SD);
}

void fio_amb_sd0_slot1_release(void)
{
	fio_unlock(SELECT_FIO_SD);
}

#if (SD_HAS_INTERNAL_MUXER == 1)
void fio_amb_sd0_slot2_request(void)
{
	fio_select_lock(SELECT_FIO_SDIO);
}

void fio_amb_sd0_slot2_release(void)
{
	fio_unlock(SELECT_FIO_SDIO);
}
#endif

struct ambarella_sd_controller ambarella_platform_sd_controller0 = {
	.slot[0] = {
		.pmmc_host	= NULL,
		.check_owner	= fio_amb_sd0_is_enable,
		.request	= fio_amb_sd0_slot1_request,
		.release	= fio_amb_sd0_slot1_release,
		.ext_power	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.ext_reset	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.use_bounce_buffer	= 0,
		.max_blk_sz		= SD_BLK_SZ_512KB,
		.fixed_cd		= -1,
#if (SD_HAS_INTERNAL_MUXER == 1)
		.gpio_cd	= {
			.irq_gpio	= SD1_CD,
			.irq_line	= SD1CD_IRQ,
			.irq_type	= IRQ_TYPE_EDGE_BOTH,
			.irq_gpio_val	= GPIO_LOW,
			.irq_gpio_mode	= GPIO_FUNC_HW,
		},
#else
		.gpio_cd	= {
			.irq_gpio	= -1,
			.irq_line	= -1,
			.irq_type	= -1,
			.irq_gpio_val	= GPIO_LOW,
			.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
		},
#endif
		.cd_delay	= 50,
		.fixed_wp	= -1,
		.gpio_wp	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.caps		= MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ |
				MMC_CAP_ERASE | MMC_CAP_WAIT_WHILE_BUSY,
	},
#if (SD_HAS_INTERNAL_MUXER == 1)
	.slot[1] = {
		.pmmc_host	= NULL,
		.check_owner	= fio_amb_sdio0_is_enable,
		.request	= fio_amb_sd0_slot2_request,
		.release	= fio_amb_sd0_slot2_release,
		.ext_power	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.ext_reset	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.use_bounce_buffer	= 0,
		.max_blk_sz		= SD_BLK_SZ_512KB,
		.fixed_cd		= -1,
		.gpio_cd	= {
			.irq_gpio	= -1,
			.irq_line	= -1,
			.irq_type	= -1,
			.irq_gpio_val	= GPIO_LOW,
			.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
		},
		.cd_delay	= 50,
		.fixed_wp	= -1,
		.gpio_wp	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.caps		= MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ |
				MMC_CAP_ERASE | MMC_CAP_WAIT_WHILE_BUSY,
	},
	.num_slots		= 2,
#else
	.num_slots		= 1,
#endif
	.clk_limit		= 12500000,
	.wait_tmo		= (1 * HZ),
	.set_pll		= rct_set_sd_pll,
	.get_pll		= get_sd_freq_hz,
#if (SD_SUPPORT_PLL_SCALER == 1)
	.support_pll_scaler	= 1,
#else
	.support_pll_scaler	= 0,
#endif
	.max_clk		= 48000000,
};
module_param_cb(sd0_clk_limit, &param_ops_int,
	&(ambarella_platform_sd_controller0.clk_limit), 0644);
module_param_cb(sd0_wait_timoout, &param_ops_int,
	&(ambarella_platform_sd_controller0.wait_tmo), 0644);
AMBA_SD_PARAM_CALL(0, 0, ambarella_platform_sd_controller0,
	&param_ops_ambarella_sd_cdpos, 0644);
#if (SD_HAS_INTERNAL_MUXER == 1)
AMBA_SD_PARAM_CALL(0, 1, ambarella_platform_sd_controller0,
	&param_ops_ambarella_sd_cdpos, 0644);
#endif

struct platform_device ambarella_sd0 = {
	.name		= "ambarella-sd",
	.id		= 0,
	.resource	= ambarella_sd0_resources,
	.num_resources	= ARRAY_SIZE(ambarella_sd0_resources),
	.dev		= {
		.platform_data		= &ambarella_platform_sd_controller0,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

#if (SD_INSTANCES >= 2)
struct resource ambarella_sd1_resources[] = {
	[0] = {
		.start	= SD2_BASE,
		.end	= SD2_BASE + 0x0FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SD2_IRQ,
		.end	= SD2_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

int fio_amb_sd2_check_owner(void)
{
#if (FIO_SUPPORT_AHB_CLK_ENA == 1)
	return fio_amb_sd2_is_enable();
#else
	return 1;
#endif
}

void fio_amb_sd2_request(void)
{
#if (FIO_SUPPORT_AHB_CLK_ENA == 1)
	fio_select_lock(SELECT_FIO_SD2);
#elif  defined(CONFIG_AMBARELLA_IPC) && (ENABLE_IPC_MUTEX_SD2 == 1) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON) && \
	(SD_HOST1_HOST2_HAS_MUX == 0)
       ipc_mutex_lock(IPC_MUTEX_ID_SD2);
#elif  defined(CONFIG_AMBARELLA_IPC)
	fio_select_lock(SELECT_FIO_SD2);
#else
	return;
#endif
}

void fio_amb_sd2_release(void)
{
#if (FIO_SUPPORT_AHB_CLK_ENA == 1)
	fio_unlock(SELECT_FIO_SD2);
#elif  defined(CONFIG_AMBARELLA_IPC) && (ENABLE_IPC_MUTEX_SD2 == 1) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON) && \
	(SD_HOST1_HOST2_HAS_MUX == 0)
       ipc_mutex_unlock(IPC_MUTEX_ID_SD2);
#elif  defined(CONFIG_AMBARELLA_IPC)
	fio_unlock(SELECT_FIO_SD2);
#else
	return;
#endif
}

#if (CHIP_REV == I1)
void ambarella_platform_sdxc_set_pll(u32 freq_hz)
{
	amb_set_sdxc_clock_frequency(HAL_BASE_VP, freq_hz);
}

u32 ambarella_platform_sdxc_get_pll(void)
{
	return (u32)amb_get_sdxc_clock_frequency(HAL_BASE_VP);
}
#endif

struct ambarella_sd_controller ambarella_platform_sd_controller1 = {
	.slot[0] = {
		.pmmc_host	= NULL,
		.check_owner	= fio_amb_sd2_check_owner,
		.request	= fio_amb_sd2_request,
		.release	= fio_amb_sd2_release,
		.ext_power	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.ext_reset	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.use_bounce_buffer	= 0,
		.max_blk_sz		= SD_BLK_SZ_512KB,
		.fixed_cd		= -1,
		.gpio_cd	= {
			.irq_gpio	= -1,
			.irq_line	= -1,
			.irq_type	= -1,
			.irq_gpio_val	= GPIO_LOW,
			.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
		},
		.cd_delay	= 50,
		.fixed_wp	= -1,
		.gpio_wp	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.caps		= MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ |
				MMC_CAP_ERASE | MMC_CAP_WAIT_WHILE_BUSY,
	},
	.num_slots		= 1,
	.clk_limit		= 25000000,
	.wait_tmo		= (1 * HZ),
#if (CHIP_REV == I1)
	.set_pll		= ambarella_platform_sdxc_set_pll,
	.get_pll		= ambarella_platform_sdxc_get_pll,
#else
	.set_pll		= rct_set_sd_pll,
	.get_pll		= get_sd_freq_hz,
#endif
#if (SD_SUPPORT_PLL_SCALER == 1)
#if (CHIP_REV == I1)
	.support_pll_scaler	= 0,
#else
	.support_pll_scaler	= 1,
#endif
#else
	.support_pll_scaler	= 0,
#endif
	.max_clk		= 48000000,
};
module_param_cb(sd1_clk_limit, &param_ops_int,
	&(ambarella_platform_sd_controller1.clk_limit), 0644);
module_param_cb(sd1_wait_timoout, &param_ops_int,
	&(ambarella_platform_sd_controller1.wait_tmo), 0644);
AMBA_SD_PARAM_CALL(1, 0, ambarella_platform_sd_controller1,
	&param_ops_ambarella_sd_cdpos, 0644);

struct platform_device ambarella_sd1 = {
#ifdef CONFIG_MMC_AMBARELLA_SDIO
	.name		= "ambarella-sd",
#else
	.name		= "ambarella-sdio",
#endif
	.id		= 1,
	.resource	= ambarella_sd1_resources,
	.num_resources	= ARRAY_SIZE(ambarella_sd1_resources),
	.dev		= {
		.platform_data		= &ambarella_platform_sd_controller1,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif

int __init ambarella_init_sd(void)
{
	int retval = 0;

	return retval;
}

void ambarella_detect_sd_slot(int bus, int slot, int fixed_cd)
{
	struct ambarella_sd_slot		*pslotinfo = NULL;
	mmc_dc_fn				mmc_dc = NULL;

	pr_debug("%s:%d[%d:%d:%d]\n", __func__, __LINE__, bus, slot, fixed_cd);
	if ((bus == 0) && (slot == 0)) {
		pslotinfo = &ambarella_platform_sd_controller0.slot[0];
	}
#if (SD_HAS_INTERNAL_MUXER == 1)
	if ((bus == 0) && (slot == 1)) {
		pslotinfo = &ambarella_platform_sd_controller0.slot[1];
	}
#endif
#if (SD_INSTANCES >= 2)
	if ((bus == 1) && (slot == 0)) {
		pslotinfo = &ambarella_platform_sd_controller1.slot[0];
	}
#endif

#if defined(CONFIG_MMC)
	mmc_dc = mmc_detect_change;
#else
	mmc_dc = (mmc_dc_fn)module_kallsyms_lookup_name("mmc_detect_change");
#endif
	if (pslotinfo && mmc_dc && pslotinfo->pmmc_host) {
		pslotinfo->fixed_cd = fixed_cd;
		mmc_dc(pslotinfo->pmmc_host, pslotinfo->cd_delay);
		pr_debug("%s:%d[%p:%p:%p:%d]\n", __func__, __LINE__, pslotinfo,
			mmc_dc, pslotinfo->pmmc_host, pslotinfo->cd_delay);
	}
}
EXPORT_SYMBOL(ambarella_detect_sd_slot);

