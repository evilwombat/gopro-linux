/*
 * arch/arm/plat-ambarella/generic/fio.c
 *
 * History:
 *	2008/03/05 - [Chien-Yang Chen] created file
 *	2008/01/09 - [Anthony Ginger] Port to 2.6.28.
 *
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/delay.h>

#include <asm/io.h>
#include <asm/setup.h>

#include <mach/hardware.h>
#include <plat/fio.h>
#include <plat/nand.h>
#include <linux/aipc/ipc_mutex.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

#if    defined(CONFIG_AMBARELLA_IPC)
#define HANDLE_SDIO_FAKE_IRQ   1
#else
#define HANDLE_SDIO_FAKE_IRQ   0
#endif

/* ==========================================================================*/
static DECLARE_WAIT_QUEUE_HEAD(fio_lock);

static atomic_t fio_owner = ATOMIC_INIT(SELECT_FIO_FREE);
module_param_cb(fio_owner, &param_ops_int, &fio_owner, 0644);

int fio_default_owner = SELECT_FIO_FREE;
int fio_select_sdio_as_default = 1;

module_param_cb(fio_default_owner, &param_ops_int, &fio_default_owner, 0644);

/* ==========================================================================*/
void __fio_select_lock(int module)
{
	u32 fio_ctr;
	u32 fio_dmactr;

	fio_ctr = amba_readl(FIO_CTR_REG);
	fio_dmactr = amba_readl(FIO_DMACTR_REG);

	switch (module) {
	case SELECT_FIO_FL:
		fio_dmactr = (fio_dmactr & 0xcfffffff) | FIO_DMACTR_FL;
		break;

	case SELECT_FIO_XD:
		fio_ctr |= FIO_CTR_XD;
		fio_dmactr = (fio_dmactr & 0xcfffffff) | FIO_DMACTR_XD;
		break;

	case SELECT_FIO_CF:
		fio_ctr &= ~FIO_CTR_XD;
		fio_dmactr = (fio_dmactr & 0xcfffffff) | FIO_DMACTR_CF;
#if (FIO_SUPPORT_AHB_CLK_ENA == 1)
		fio_amb_sd2_disable();
		fio_amb_cf_enable();
#endif
		break;

	case SELECT_FIO_SD:
		fio_ctr &= ~FIO_CTR_XD;
		fio_dmactr = (fio_dmactr & 0xcfffffff) | FIO_DMACTR_SD;
		break;

	case SELECT_FIO_SDIO:
		fio_ctr |= FIO_CTR_XD;
		fio_dmactr = (fio_dmactr & 0xcfffffff) | FIO_DMACTR_SD;
		break;

	case SELECT_FIO_SD2:
#if (FIO_SUPPORT_AHB_CLK_ENA == 1)
		fio_amb_cf_disable();
		fio_amb_sd2_enable();
#endif
#if (CHIP_REV == A7L)
		fio_ctr &= ~FIO_CTR_XD;
		fio_dmactr = (fio_dmactr & 0xcfffffff) | FIO_DMACTR_SD;
#endif
		break;

	default:
		break;
	}

#if (SD_HAS_INTERNAL_MUXER == 1)
	if (module != SELECT_FIO_SDIO) {
#if (HANDLE_SDIO_FAKE_IRQ == 1)
		amba_clrbitsw(SD_NISEN_REG, SD_NISEN_CARD);
#endif
		//SMIO_38 ~ SMIO_43
		amba_clrbitsl(GPIO2_AFSEL_REG, 0x000007e0);
	}
#endif

	amba_writel(FIO_DMACTR_REG, fio_dmactr);
	amba_writel(FIO_CTR_REG, fio_ctr);

#if (SD_HAS_INTERNAL_MUXER == 1)
	if (module == SELECT_FIO_SDIO) {
		//SMIO_38 ~ SMIO_43
		amba_setbitsl(GPIO2_AFSEL_REG, 0x000007e0);
	}
#endif
}

void fio_select_lock(int module)
{
#if	defined(CONFIG_AMBARELLA_IPC)
#if (SD_HOST1_HOST2_HAS_MUX == 1)
	switch (module) {
	case SELECT_FIO_SD:
	case SELECT_FIO_SDIO:
		ipc_mutex_lock(IPC_MUTEX_ID_SD);
		break;
	case SELECT_FIO_SD2:
		ipc_mutex_lock(IPC_MUTEX_ID_SD2);
		break;
	default:
		ipc_mutex_lock(IPC_MUTEX_ID_SD);
		ipc_mutex_lock(IPC_MUTEX_ID_SD2);
	}
#else
	ipc_mutex_lock(IPC_MUTEX_ID_FIO);
#endif
	atomic_set(&fio_owner, module);
#else
	if (atomic_read(&fio_owner) != module) {
		wait_event(fio_lock, (atomic_cmpxchg(&fio_owner,
			SELECT_FIO_FREE, module) == SELECT_FIO_FREE));
	} else {
		pr_warning("%s: module[%d] reentry!\n", __func__, module);
	}
#endif

	__fio_select_lock(module);
}

#if (HANDLE_SDIO_FAKE_IRQ == 1)
static void fio_reactive_sdio_irq(void)
{
	int i;
	for (i = 5; i > 0; i--) {
		/* Fake SDIO irq can be clear by 		*/
		/* re-enable sdio irq bit while SDIO is enable. */
		/* If it's fake, it can be clear in 5 times.	*/
		amba_clrbitsw(SD_NISEN_REG, SD_NISEN_CARD);
		amba_setbitsw(SD_NISEN_REG, SD_NISEN_CARD);
		if ((amba_readw(SD_NIS_REG) & SD_NIS_CARD) == 0) {
			break;
		}
	}
}
#endif

void fio_unlock(int module)
{
	if (atomic_read(&fio_owner) == module) {
#if (SD_HAS_INTERNAL_MUXER == 1)
		if (fio_select_sdio_as_default && (module != SELECT_FIO_SDIO)) {
			u32 fio_dmactr;

#if (HANDLE_SDIO_FAKE_IRQ == 1)
			/* When swtich back to SDIO, SMIO38 ~ 43 connected to */
			/* SD controller internally and a detection of low of */
			/* SMIO41 cause fake SDIO irq. */
			unsigned long flags;

			flags = arm_irq_save();
			/* SMIO_38 ~ 43 HW */
			amba_setbitsl(GPIO2_AFSEL_REG, 0x000007e0);

			fio_dmactr = amba_readl(FIO_DMACTR_REG);
			fio_dmactr = (fio_dmactr & 0xcfffffff) | FIO_DMACTR_SD;

			amba_writel(FIO_DMACTR_REG, fio_dmactr);
			amba_setbitsl(FIO_CTR_REG, FIO_CTR_XD);

			fio_reactive_sdio_irq();
			arm_irq_restore(flags);
#else
			fio_dmactr = amba_readl(FIO_DMACTR_REG);
			fio_dmactr = (fio_dmactr & 0xcfffffff) | FIO_DMACTR_SD;
			amba_writel(FIO_DMACTR_REG, fio_dmactr);
			amba_setbitsl(FIO_CTR_REG, FIO_CTR_XD);
#endif
		}
#endif
#if (FIO_SUPPORT_AHB_CLK_ENA == 1)
		if (fio_select_sdio_as_default && (module == SELECT_FIO_CF)) {
			fio_amb_cf_disable();
			fio_amb_sd2_enable();
		}
#endif
	}

	if ((atomic_read(&fio_owner) == module) &&
		(fio_default_owner != SELECT_FIO_FREE) &&
		(fio_default_owner != module)) {
		__fio_select_lock(fio_default_owner);
	}

#if	defined(CONFIG_AMBARELLA_IPC)
	atomic_set(&fio_owner, SELECT_FIO_FREE);
#if (SD_HOST1_HOST2_HAS_MUX == 1)
	switch (module) {
	case SELECT_FIO_SD:
	case SELECT_FIO_SDIO:
		ipc_mutex_unlock(IPC_MUTEX_ID_SD);
		return;
	case SELECT_FIO_SD2:
		ipc_mutex_unlock(IPC_MUTEX_ID_SD2);
		return;
	default:
		ipc_mutex_unlock(IPC_MUTEX_ID_SD);
		ipc_mutex_unlock(IPC_MUTEX_ID_SD2);
		return;
	}
#else
	ipc_mutex_unlock(IPC_MUTEX_ID_FIO);
#endif	/* SD_HOST1_HOST2_HAS_MUX */
#else
	if (atomic_cmpxchg(&fio_owner, module, SELECT_FIO_FREE) == module) {
		wake_up(&fio_lock);
	} else {
		pr_err("%s: fio_owner[%d] != module[%d]!.\n",
			__func__, atomic_read(&fio_owner), module);
	}
#endif
}

int fio_amb_sd0_is_enable(void)
{
	u32 fio_ctr;
	u32 fio_dmactr;

	fio_ctr = amba_readl(FIO_CTR_REG);
	fio_dmactr = amba_readl(FIO_DMACTR_REG);

	return (((fio_ctr & FIO_CTR_XD) == 0) &&
		((fio_dmactr & FIO_DMACTR_SD) == FIO_DMACTR_SD));
}

int fio_amb_sdio0_is_enable(void)
{
	u32 fio_ctr;
	u32 fio_dmactr;

	fio_ctr = amba_readl(FIO_CTR_REG);
	fio_dmactr = amba_readl(FIO_DMACTR_REG);

	return (((fio_ctr & FIO_CTR_XD) == FIO_CTR_XD) &&
		((fio_dmactr & FIO_DMACTR_SD) == FIO_DMACTR_SD));
}

int fio_dma_parse_error(u32 reg)
{
	int rval = 0;

	if (reg & FIO_DMASTA_RE) {
		pr_err("%s: fio dma read error 0x%x.\n", __func__, reg);
		rval = FIO_READ_ER;
		goto done;
	}

	if (reg & FIO_DMASTA_AE) {
		pr_err("%s: fio dma address error 0x%x.\n", __func__, reg);
		rval = FIO_ADDR_ER;
		goto done;
	}

	if (!(reg & FIO_DMASTA_DN)) {
		pr_err("%s: fio dma operation not done error 0x%x.\n",
			__func__, reg);
		rval = FIO_OP_NOT_DONE_ER;
	}

done:
	return rval;
}

#if (FIO_SUPPORT_AHB_CLK_ENA == 1)
void fio_amb_fl_enable(void)
{
	amba_setbitsl(HOST_AHB_CLK_ENABLE_REG, HOST_AHB_FLASH_CLK_ENB);
}

void fio_amb_fl_disable(void)
{
	amba_clrbitsl(HOST_AHB_CLK_ENABLE_REG, HOST_AHB_FLASH_CLK_ENB);
}

int fio_amb_fl_is_enable(void)
{
	return amba_tstbitsl(HOST_AHB_CLK_ENABLE_REG, HOST_AHB_FLASH_CLK_ENB);
}

void fio_amb_cf_enable(void)
{
	amba_setbitsl(HOST_AHB_CLK_ENABLE_REG, HOST_AHB_CF_CLK_ENB);
}

void fio_amb_cf_disable(void)
{
	amba_clrbitsl(HOST_AHB_CLK_ENABLE_REG, HOST_AHB_CF_CLK_ENB);
}

int fio_amb_cf_is_enable(void)
{
	return amba_tstbitsl(HOST_AHB_CLK_ENABLE_REG, HOST_AHB_CF_CLK_ENB);
}

void fio_amb_sd2_enable(void)
{
	amba_setbitsl(HOST_AHB_CLK_ENABLE_REG, HOST_AHB_SDIO_SEL);
}

void fio_amb_sd2_disable(void)
{
	amba_clrbitsl(HOST_AHB_CLK_ENABLE_REG, HOST_AHB_SDIO_SEL);
}

int fio_amb_sd2_is_enable(void)
{
	return amba_tstbitsl(HOST_AHB_CLK_ENABLE_REG, HOST_AHB_SDIO_SEL);
}

#endif

#if (NAND_DUMMY_XFER == 1)
void nand_dummy_xfer(void)
{
	amba_writel(FIO_CTR_REG, 0x15);
	amba_writel(NAND_CTR_REG, 0x4080110);

	amba_writel(DMA_FIOS_CHAN0_STA_REG, 0x0);
	amba_writel(DMA_FIOS_CHAN0_SRC_REG, 0x60000000);
	amba_writel(DMA_FIOS_CHAN0_DST_REG, 0xc0000000);
	amba_writel(DMA_FIOS_CHAN0_CTR_REG, 0xae800020);

	amba_writel(FIO_DMAADR_REG, 0x0);
	amba_writel(FIO_DMACTR_REG, 0x86800020);

	while ((amba_readl(DMA_FIOS_INT_REG) & 0x1) != 0x1);
	amba_writel(DMA_FIOS_CHAN0_STA_REG, 0x0);

	while ((amba_readl(FIO_STA_REG) & 0x1) != 0x1);
	amba_writel(FIO_CTR_REG, 0x1);
	amba_writel(NAND_INT_REG, 0x0);

	while ((amba_readl(FIO_DMASTA_REG) & 0x1000000) != 0x1000000);
	amba_writel(FIO_DMASTA_REG, 0x0);
}
#endif

void enable_fio_dma(void)
{
#if ((HOST_MAX_AHB_CLK_EN_BITS == 10) || (I2S_24BITMUX_MODE_REG_BITS == 4))
	u32 val;
#endif

#if (HOST_MAX_AHB_CLK_EN_BITS == 10)
	/* Disable boot-select */
	val = amba_readl(HOST_AHB_CLK_ENABLE_REG);
	val &= ~(HOST_AHB_BOOT_SEL);
	val &= ~(HOST_AHB_FDMA_BURST_DIS);
	amba_writel(HOST_AHB_CLK_ENABLE_REG, val);
#endif

#if (I2S_24BITMUX_MODE_REG_BITS == 4)
	val = amba_readl(I2S_24BITMUX_MODE_REG);
	val &= ~(I2S_24BITMUX_MODE_DMA_BOOTSEL);
	val &= ~(I2S_24BITMUX_MODE_FDMA_BURST_DIS);
	amba_writel(I2S_24BITMUX_MODE_REG, val);
#endif

#if (NAND_DUMMY_XFER == 1)
	nand_dummy_xfer();
#endif
}

void fio_amb_exit_random_mode(void)
{
	amba_clrbitsl(FIO_CTR_REG, FIO_CTR_RR);
}

int __init ambarella_init_fio(void)
{
#ifndef CONFIG_AMBARELLA_QUICK_INIT
	/* Following should be handled by the bootloader... */
#if (HOST_MAX_AHB_CLK_EN_BITS == 10)
	amba_clrbitsl(HOST_AHB_CLK_ENABLE_REG,
		(HOST_AHB_BOOT_SEL | HOST_AHB_FDMA_BURST_DIS));
#endif
	rct_reset_fio();
	fio_amb_exit_random_mode();
	enable_fio_dma();
	amba_writel(FLASH_INT_REG, 0x0);
	amba_writel(XD_INT_REG, 0x0);
	amba_writel(CF_STA_REG, CF_STA_CW | CF_STA_DW);
#endif

	//SMIO_38 ~ SMIO_43
	amba_setbitsl(GPIO2_MASK_REG, 0x000007e0);
	amba_clrbitsl(GPIO2_DIR_REG, 0x00000780);
	amba_setbitsl(GPIO2_DIR_REG, 0x00000060);
	amba_setbitsl(GPIO2_DATA_REG, 0x00000040);
	amba_clrbitsl(GPIO2_DATA_REG, 0x00000020);

	return 0;
}

/* ==========================================================================*/
static struct ambarella_nand_set ambarella_nand_default_set = {
	.name		= "ambarella_nand_set",
	.nr_chips	= 1,
	.nr_partitions	= 0,
};

static struct ambarella_nand_timing ambarella_nand_default_timing = {
	.control	= 0,
	.size		= 0,
	.timing0	= 0x20202020,
	.timing1	= 0x20202020,
	.timing2	= 0x20202020,
	.timing3	= 0x20202020,
	.timing4	= 0x20202020,
	.timing5	= 0x00202020,
};

static DEFINE_MUTEX(fio_nand_mtx);

static void fio_amb_nand_request(void)
{
	mutex_lock(&fio_nand_mtx);
	fio_select_lock(SELECT_FIO_FL);
}

static void fio_amb_nand_release(void)
{
	fio_unlock(SELECT_FIO_FL);
	mutex_unlock(&fio_nand_mtx);
}

static int fio_amb_nand_parse_error(u32 reg)
{
	return fio_dma_parse_error(reg);
}

static struct ambarella_platform_nand ambarella_platform_default_nand = {
	.sets		= &ambarella_nand_default_set,
	.timing		= &ambarella_nand_default_timing,
	.flash_bbt	= 1,

	.parse_error	= fio_amb_nand_parse_error,
	.request	= fio_amb_nand_request,
	.release	= fio_amb_nand_release,
};

static int __init parse_nand_tag_cs(const struct tag *tag)
{
	ambarella_nand_default_timing.control = tag->u.serialnr.low;
	ambarella_nand_default_timing.size = tag->u.serialnr.high;

	return 0;
}
__tagtable(ATAG_AMBARELLA_NAND_CS, parse_nand_tag_cs);

static int __init parse_nand_tag_t0(const struct tag *tag)
{
	ambarella_nand_default_timing.timing0 = tag->u.serialnr.low;
	ambarella_nand_default_timing.timing1 = tag->u.serialnr.high;

	return 0;
}
__tagtable(ATAG_AMBARELLA_NAND_T0, parse_nand_tag_t0);

static int __init parse_nand_tag_t1(const struct tag *tag)
{
	ambarella_nand_default_timing.timing2 = tag->u.serialnr.low;
	ambarella_nand_default_timing.timing3 = tag->u.serialnr.high;

	return 0;
}
__tagtable(ATAG_AMBARELLA_NAND_T1, parse_nand_tag_t1);

static int __init parse_nand_tag_t2(const struct tag *tag)
{
	ambarella_nand_default_timing.timing4 = tag->u.serialnr.low;
	ambarella_nand_default_timing.timing5 = tag->u.serialnr.high;

	return 0;
}
__tagtable(ATAG_AMBARELLA_NAND_T2, parse_nand_tag_t2);

void __init ambarella_init_nand_hotboot(
	struct ambarella_nand_timing *hot_nand_timing)
{
	ambarella_nand_default_timing = *hot_nand_timing;
}

static struct resource ambarella_fio_resources[] = {
	[0] = {
		.start	= FIO_BASE,
		.end	= FIO_BASE + 0x0FFF,
		.name	= "registers",
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= FIOCMD_IRQ,
		.end	= FIOCMD_IRQ,
		.name	= "ambarella-fio-cmd",
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		.start	= FIODMA_IRQ,
		.end	= FIODMA_IRQ,
		.name	= "ambarella-fio-dma",
		.flags	= IORESOURCE_IRQ,
	},
	[3] = {
		.start	= FL_WP,
		.end	= FL_WP,
		.name	= "wp_gpio",
		.flags	= IORESOURCE_IO,
	},
	[4] = {
		.start	= FIO_FIFO_BASE,
		.end	= FIO_FIFO_BASE + 0x0FFF,
		.name	= "dma",
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device ambarella_nand = {
	.name		= "ambarella-nand",
	.id		= -1,
	.resource	= ambarella_fio_resources,
	.num_resources	= ARRAY_SIZE(ambarella_fio_resources),
	.dev		= {
		.platform_data		= &ambarella_platform_default_nand,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

struct platform_device ambarella_nor = {
	.name		= "ambarella-nor",
	.id		= -1,
	.resource	= ambarella_fio_resources,
	.num_resources	= ARRAY_SIZE(ambarella_fio_resources),
	.dev		= {
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

