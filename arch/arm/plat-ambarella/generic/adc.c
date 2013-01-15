/*
 * arch/arm/plat-ambarella/generic/adc.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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
#include <linux/bootmem.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/dma-mapping.h>

#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>
#include <plat/adc.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

/* ==========================================================================*/
#if ((CHIP_REV == A5) || (CHIP_REV == A6) || (CHIP_REV == A5S) ||\
     (CHIP_REV == A7) || (CHIP_REV == A5L))
#undef ADC_ONE_SHOT
#define ADC_ONE_SHOT
#else
#define ADC_ONE_SHOT
#endif	/* CHIP_REV */

#ifndef CONFIG_AMBARELLA_ADC_WAIT_COUNTER_LIMIT
#define CONFIG_AMBARELLA_ADC_WAIT_COUNTER_LIMIT	(100000)
#endif

/* ==========================================================================*/
u32 ambarella_adc_get_instances(void)
{
	return ADC_NUM_CHANNELS;
}

static inline u32 ambarella_adc_get_channel_inline(u32 channel_id)
{
	u32					adc_data = 0;

#if defined(ADC_ONE_SHOT)
	amba_setbitsl(ADC_CONTROL_REG, ADC_CONTROL_START);
#if ((CHIP_REV == A5) || (CHIP_REV == A6) || (CHIP_REV == A5S) || \
     (CHIP_REV == A7) || (CHIP_REV == A5L))
	while (amba_tstbitsl(ADC_CONTROL_REG, ADC_CONTROL_STATUS) == 0x0) {
		msleep(1);
	}
#else
	while (amba_tstbitsl(ADC_CONTROL_REG, ADC_CONTROL_STATUS) == 0x0);
#endif
#endif

	switch(channel_id) {
#if (ADC_NUM_CHANNELS == 8)
	case 0:
		adc_data = (amba_readl(ADC_DATA0_REG) + 0x8000) & 0xffff;
		break;
	case 1:
		adc_data = (amba_readl(ADC_DATA1_REG) + 0x8000) & 0xffff;
		break;
	case 2:
		adc_data = (amba_readl(ADC_DATA2_REG) + 0x8000) & 0xffff;
		break;
	case 3:
		adc_data = (amba_readl(ADC_DATA3_REG) + 0x8000) & 0xffff;
		break;
	case 4:
		adc_data = (amba_readl(ADC_DATA4_REG) + 0x8000) & 0xffff;
		break;
	case 5:
		adc_data = (amba_readl(ADC_DATA5_REG) + 0x8000) & 0xffff;
		break;
	case 6:
		adc_data = (amba_readl(ADC_DATA6_REG) + 0x8000) & 0xffff;
		break;
	case 7:
		adc_data = (amba_readl(ADC_DATA7_REG) + 0x8000) & 0xffff;
		break;
#else
	case 0:
		adc_data = amba_readl(ADC_DATA0_REG);
		break;
	case 1:
		adc_data = amba_readl(ADC_DATA1_REG);
		break;
	case 2:
		adc_data = amba_readl(ADC_DATA2_REG);
		break;
	case 3:
		adc_data = amba_readl(ADC_DATA3_REG);
		break;
#if (ADC_NUM_CHANNELS >= 6)
	case 4:
		adc_data = amba_readl(ADC_DATA4_REG);
		break;
	case 5:
		adc_data = amba_readl(ADC_DATA5_REG);
		break;
#endif
#if (ADC_NUM_CHANNELS >= 10)
	case 6:
		adc_data = amba_readl(ADC_DATA6_REG);
		break;
	case 7:
		adc_data = amba_readl(ADC_DATA7_REG);
		break;
	case 8:
		adc_data = amba_readl(ADC_DATA8_REG);
		break;
	case 9:
		adc_data = amba_readl(ADC_DATA9_REG);
		break;
#endif
#endif
	default:
		pr_warning("%s: invalid adc channel id %d!\n",
			__func__, channel_id);
		break;
	}
	pr_debug("%s: channel[%d] = %d.\n", __func__, channel_id, adc_data);

	return adc_data;
}

void ambarella_adc_get_array(u32 *adc_data, u32 *array_size)
{
	int					i;

	if (unlikely(*array_size > ADC_NUM_CHANNELS)) {
		pr_err("%s: array_size should be %d, not %d!\n",
			__func__, ADC_NUM_CHANNELS, *array_size);
		return;
	}

#if defined(ADC_ONE_SHOT)
	amba_setbitsl(ADC_CONTROL_REG, ADC_CONTROL_START);
#if ((CHIP_REV == A5) || (CHIP_REV == A6) || (CHIP_REV == A5S) || \
     (CHIP_REV == A7) || (CHIP_REV == A5L))
	while (amba_tstbitsl(ADC_CONTROL_REG, ADC_CONTROL_STATUS) == 0x0) {
		msleep(1);
	}
#else
	while (amba_tstbitsl(ADC_CONTROL_REG, ADC_CONTROL_STATUS) == 0x0);
#endif
#endif

#if (ADC_NUM_CHANNELS == 8)
	adc_data[0] = (amba_readl(ADC_DATA0_REG) + 0x8000) & 0xffff;
	adc_data[1] = (amba_readl(ADC_DATA1_REG) + 0x8000) & 0xffff;
	adc_data[2] = (amba_readl(ADC_DATA2_REG) + 0x8000) & 0xffff;
	adc_data[3] = (amba_readl(ADC_DATA3_REG) + 0x8000) & 0xffff;
	adc_data[4] = (amba_readl(ADC_DATA4_REG) + 0x8000) & 0xffff;
	adc_data[5] = (amba_readl(ADC_DATA5_REG) + 0x8000) & 0xffff;
	adc_data[6] = (amba_readl(ADC_DATA6_REG) + 0x8000) & 0xffff;
	adc_data[7] = (amba_readl(ADC_DATA7_REG) + 0x8000) & 0xffff;
#else
	adc_data[0] = amba_readl(ADC_DATA0_REG);
	adc_data[1] = amba_readl(ADC_DATA1_REG);
	adc_data[2] = amba_readl(ADC_DATA2_REG);
	adc_data[3] = amba_readl(ADC_DATA3_REG);
#if (ADC_NUM_CHANNELS >= 6)
	adc_data[4] = amba_readl(ADC_DATA4_REG);
	adc_data[5] = amba_readl(ADC_DATA5_REG);
#endif
#if (ADC_NUM_CHANNELS >= 10)
	adc_data[6] = amba_readl(ADC_DATA6_REG);
	adc_data[7] = amba_readl(ADC_DATA7_REG);
	adc_data[8] = amba_readl(ADC_DATA8_REG);
	adc_data[9] = amba_readl(ADC_DATA9_REG);
#endif
#endif

	for (i = 0; i < ADC_NUM_CHANNELS; i++)
		pr_debug("%s: channel[%d] = %d.\n", __func__, i, adc_data[i]);
}

void ambarella_adc_start(void)
{
#if defined(ADC16_CTRL_REG)
	amba_clrbitsl(ADC16_CTRL_REG, 0x2);
#endif

#if (CHIP_REV == A5L)
	amba_writel(ADC_CONTROL_REG, 0x0);

	amba_writel(ADC_DATA0_REG, 0);
	amba_writel(ADC_DATA1_REG, 0);
	amba_writel(ADC_DATA2_REG, 0);
	amba_writel(ADC_DATA3_REG, 0);

	amba_writel(ADC_ENABLE_REG, 0x0);
#endif

#if (CHIP_REV == A5 || CHIP_REV == A6)
	/* SCALER_ADC_REG (default=4) */
	/* clk_au = 27MHz/2 */
	rct_set_adc_clk_freq_hz(PLL_CLK_13_5MHZ);

	/* ADC Analog (lowest power) */
	amba_writel(ADC16_CTRL_REG, 0x031cff);

	/* ADC reset */
	amba_writel(ADC_RESET_REG, 0x1);

	/* Fix nonlinearity */
	amba_writel(ADC16_CTRL_REG, 0x00031c00);
#endif

#if (CHIP_REV == A5S)
	/* stop conversion */
	amba_writel(ADC_CONTROL_REG, 0x0);

	amba_writel(ADC_DATA0_REG, 0);
	amba_writel(ADC_DATA1_REG, 0);
	amba_writel(ADC_DATA2_REG, 0);
	amba_writel(ADC_DATA3_REG, 0);

	amba_writel(ADC_ENABLE_REG, 0x0);
#endif

#if (CHIP_REV == A7)
	/* stop conversion */
	amba_writel(ADC_CONTROL_REG, 0x0);

	amba_writel(ADC_DATA0_REG, 0);
	amba_writel(ADC_DATA1_REG, 0);
	amba_writel(ADC_DATA2_REG, 0);
	amba_writel(ADC_DATA3_REG, 0);

	amba_writel(ADC_ENABLE_REG, 0x1);
	amba_writel(ADC_CONTROL_REG, 0x1);
#else
#if (CHIP_REV != I1)
	if (amba_readl(ADC_ENABLE_REG) != 0) {
		pr_err("%s: ADC_ENABLE_REG = %d.\n",
			__func__, amba_readl(ADC_ENABLE_REG));
		return;
	}
#endif
	amba_writel(ADC_ENABLE_REG, 0x1);
#endif

#ifdef ADC_ONE_SHOT
	/* ADC control mode, single */
	amba_clrbitsl(ADC_CONTROL_REG, ADC_CONTROL_MODE);
#else
	/* ADC control mode, continuous */
	amba_setbitsl(ADC_CONTROL_REG, ADC_CONTROL_MODE);

	/* start conversion */
	amba_setbitsl(ADC_CONTROL_REG, ADC_CONTROL_START);
	while (amba_tstbitsl(ADC_CONTROL_REG, ADC_CONTROL_STATUS) == 0x0);
#endif
}

void ambarella_adc_stop(void)
{
#ifndef ADC_ONE_SHOT
	amba_writel(ADC_CONTROL_REG, 0x0);
#endif
	amba_writel(ADC_ENABLE_REG, 0x0);
#if (CHIP_REV == A7)
	amba_writel(ADC_CONTROL_REG, 0x0);
#endif
#if defined(ADC16_CTRL_REG)
	amba_setbitsl(ADC16_CTRL_REG, 0x2);
#endif
}

#ifdef CONFIG_AMBARELLA_ADC_PROC
#define AMBARELLA_ADC_PROC_READ_SIZE		(13)
static const char adc_proc_name[] = "adc";
static struct proc_dir_entry *adc_file;

static int ambarella_adc_proc_write(struct file *file,
	const char __user *buffer, unsigned long count, void *data)
{
	int					read_counter = 0;
	char					cmd;

	if (copy_from_user(&cmd, buffer, 1)) {
		pr_err("%s: copy_from_user fail!\n", __func__);
		read_counter = -EFAULT;
		goto ambarella_adc_proc_write_exit;
	}
	read_counter = count;

	if (cmd == '1') {
		ambarella_adc_start();
	} else {
		ambarella_adc_stop();
	}

ambarella_adc_proc_write_exit:
	return read_counter;
}

static int ambarella_adc_proc_read(char *page, char **start,
	off_t off, int count, int *eof, void *data)
{
	int					len = 0;
	int					i;
	u32					adc_data[ADC_NUM_CHANNELS];
	int					adc_size;

	adc_size = ambarella_adc_get_instances();

	if (off > (adc_size * AMBARELLA_ADC_PROC_READ_SIZE)) {
		*eof = 1;
		return 0;
	}

	*start = page + off;

	if (count > (adc_size * AMBARELLA_ADC_PROC_READ_SIZE)) {
		count = (adc_size * AMBARELLA_ADC_PROC_READ_SIZE);
		*eof = 1;
	}

	if ((off + count) > (adc_size * AMBARELLA_ADC_PROC_READ_SIZE)) {
		count = (adc_size * AMBARELLA_ADC_PROC_READ_SIZE) - off;
		*eof = 1;
	}

	adc_size = count / AMBARELLA_ADC_PROC_READ_SIZE;
	ambarella_adc_get_array(adc_data, &adc_size);
	for (i = off / AMBARELLA_ADC_PROC_READ_SIZE; i < adc_size; i++)
		len += sprintf(*start + len, "adc%d = 0x%03x\n",
			i, adc_data[i]);

	return len;
}
#endif

int __init ambarella_init_adc(void)
{
	int					retval = 0;

#ifdef CONFIG_AMBARELLA_ADC_PROC
	adc_file = create_proc_entry(adc_proc_name, S_IRUGO | S_IWUSR,
		get_ambarella_proc_dir());
	if (adc_file == NULL) {
		retval = -ENOMEM;
		pr_err("%s: %s fail!\n", __func__, adc_proc_name);
	} else {
		adc_file->read_proc = ambarella_adc_proc_read;
		adc_file->write_proc = ambarella_adc_proc_write;
	}
#endif

	return retval;
}

u32 adc_is_irq_supported(void)
{
#if (ADC_SUPPORT_THRESHOLD_INT == 1)
#ifndef ADC_ONE_SHOT
	return 1;
#else
	return 0;
#endif
#else
	return 0;
#endif
}

void adc_set_irq_threshold(u32 ch, u32 h_level, u32 l_level)
{
#if (ADC_SUPPORT_THRESHOLD_INT == 1)
	u32 irq_control_address = 0;
	u32 value = ADC_EN_HI(!!h_level) | ADC_EN_LO(!!l_level) |
		ADC_VAL_HI(h_level) | ADC_VAL_LO(l_level);

	switch (ch) {
	case 0:
		irq_control_address = ADC_CHAN0_INTR_REG;
		break;
	case 1:
		irq_control_address = ADC_CHAN1_INTR_REG;
		break;
	case 2:
		irq_control_address = ADC_CHAN2_INTR_REG;
		break;
	case 3:
		irq_control_address = ADC_CHAN3_INTR_REG;
		break;
#if (ADC_NUM_CHANNELS >= 6)
	case 4:
		irq_control_address = ADC_CHAN4_INTR_REG;
		break;
	case 5:
		irq_control_address = ADC_CHAN5_INTR_REG;
		break;
#endif
#if (ADC_NUM_CHANNELS >= 8)
	case 6:
		irq_control_address = ADC_CHAN6_INTR_REG;
		break;
	case 7:
		irq_control_address = ADC_CHAN7_INTR_REG;
		break;
#endif
#if (ADC_NUM_CHANNELS >= 10)
	case 8:
		irq_control_address = ADC_CHAN8_INTR_REG;
		break;
	case 9:
		irq_control_address = ADC_CHAN9_INTR_REG;
		break;
#endif
	default:
		printk("Don't support %d channels\n", ch);
		return;
	}
	amba_writel(irq_control_address, value);
	pr_err("%s: set ch[%d] h[%d], l[%d], 0x%08X!\n",
		__func__, ch, h_level, l_level, value);
#endif
}

/* ==========================================================================*/
struct ambarella_adc_pm_info {
#if defined(ADC16_CTRL_REG)
	u32 adc_rct_reg;
#endif
	u32 adc_control_reg;
	u32 adc_enable_reg;
	u32 adc_chan0_intr_reg;
	u32 adc_chan1_intr_reg;
	u32 adc_chan2_intr_reg;
	u32 adc_chan3_intr_reg;
#if (ADC_NUM_CHANNELS >= 6)
	u32 adc_chan4_intr_reg;
	u32 adc_chan5_intr_reg;
#endif
#if (ADC_NUM_CHANNELS >= 8)
	u32 adc_chan6_intr_reg;
	u32 adc_chan7_intr_reg;
#endif
#if (ADC_NUM_CHANNELS >= 10)
	u32 adc_chan8_intr_reg;
	u32 adc_chan9_intr_reg;
#endif
};

struct ambarella_adc_pm_info ambarella_adc_pm;

u32 ambarella_adc_suspend(u32 level)
{
#if defined(ADC16_CTRL_REG)
	ambarella_adc_pm.adc_rct_reg = amba_readl(ADC16_CTRL_REG);
#endif
	ambarella_adc_pm.adc_control_reg = amba_readl(ADC_CONTROL_REG);
	ambarella_adc_pm.adc_enable_reg = amba_readl(ADC_ENABLE_REG);
	ambarella_adc_pm.adc_chan0_intr_reg = amba_readl(ADC_CHAN0_INTR_REG);
	ambarella_adc_pm.adc_chan1_intr_reg = amba_readl(ADC_CHAN1_INTR_REG);
	ambarella_adc_pm.adc_chan2_intr_reg = amba_readl(ADC_CHAN2_INTR_REG);
	ambarella_adc_pm.adc_chan3_intr_reg = amba_readl(ADC_CHAN3_INTR_REG);
#if (ADC_NUM_CHANNELS >= 6)
	ambarella_adc_pm.adc_chan4_intr_reg = amba_readl(ADC_CHAN4_INTR_REG);
	ambarella_adc_pm.adc_chan5_intr_reg = amba_readl(ADC_CHAN5_INTR_REG);
#endif
#if (ADC_NUM_CHANNELS >= 8)
	ambarella_adc_pm.adc_chan6_intr_reg = amba_readl(ADC_CHAN6_INTR_REG);
	ambarella_adc_pm.adc_chan7_intr_reg = amba_readl(ADC_CHAN7_INTR_REG);
#endif
#if (ADC_NUM_CHANNELS >= 10)
	ambarella_adc_pm.adc_chan8_intr_reg = amba_readl(ADC_CHAN8_INTR_REG);
	ambarella_adc_pm.adc_chan9_intr_reg = amba_readl(ADC_CHAN9_INTR_REG);
#endif

	return 0;
}

u32 ambarella_adc_resume(u32 level)
{
#if defined(ADC16_CTRL_REG)
	amba_writel(ADC16_CTRL_REG, ambarella_adc_pm.adc_rct_reg);
#endif
	if (ambarella_adc_pm.adc_enable_reg & 0x01) {
		amba_writel(ADC_ENABLE_REG, ambarella_adc_pm.adc_enable_reg);

		if ((amba_readl(ADC_CONTROL_REG) & 0xfffffffc) != 0) {
			amba_writel(ADC_CONTROL_REG, 0x0);
			while ((amba_readl(ADC_CONTROL_REG) & ADC_CONTROL_STATUS) == 0x0);
		}
		if ((ambarella_adc_pm.adc_control_reg & 0xfffffffe) != 0) {
			amba_writel(ADC_CONTROL_REG, ambarella_adc_pm.adc_control_reg & 0xfffffffe);
			while ((amba_readl(ADC_CONTROL_REG) & ADC_CONTROL_STATUS) == 0x0);
		}

		amba_writel(ADC_CHAN0_INTR_REG, ambarella_adc_pm.adc_chan0_intr_reg);
		amba_writel(ADC_CHAN1_INTR_REG, ambarella_adc_pm.adc_chan1_intr_reg);
		amba_writel(ADC_CHAN2_INTR_REG, ambarella_adc_pm.adc_chan2_intr_reg);
		amba_writel(ADC_CHAN3_INTR_REG, ambarella_adc_pm.adc_chan3_intr_reg);
#if (ADC_NUM_CHANNELS >= 6)
		amba_writel(ADC_CHAN4_INTR_REG, ambarella_adc_pm.adc_chan4_intr_reg);
		amba_writel(ADC_CHAN5_INTR_REG, ambarella_adc_pm.adc_chan5_intr_reg);
#endif
#if (ADC_NUM_CHANNELS >= 8)
		amba_writel(ADC_CHAN6_INTR_REG, ambarella_adc_pm.adc_chan6_intr_reg);
		amba_writel(ADC_CHAN7_INTR_REG, ambarella_adc_pm.adc_chan7_intr_reg);
#endif
#if (ADC_NUM_CHANNELS >= 10)
		amba_writel(ADC_CHAN8_INTR_REG, ambarella_adc_pm.adc_chan8_intr_reg);
		amba_writel(ADC_CHAN9_INTR_REG, ambarella_adc_pm.adc_chan9_intr_reg);
#endif
	}

	return 0;
}

/* ==========================================================================*/
struct ambarella_adc_controller ambarella_platform_adc_controller0 = {
	.read_channels		= ambarella_adc_get_array,
	.is_irq_supported	= adc_is_irq_supported,
	.set_irq_threshold	= adc_set_irq_threshold,
	.reset			= ambarella_adc_start,
	.stop			= ambarella_adc_stop,
	.get_channel_num	= ambarella_adc_get_instances,

	.scan_delay		= 20,
};
AMBA_ADC_PARAM_CALL(ambarella_platform_adc_controller0, 0644);

struct resource ambarella_adc_resources[] = {
	[0] = {
		.start	= ADC_BASE,
		.end	= ADC_BASE + 0x0FFF,
		.name	= "registers",
		.flags	= IORESOURCE_MEM,
	},
#if (CHIP_REV == A5S || CHIP_REV == A7)
	[1] = {
		.start	= ADC_LEVEL_IRQ,
		.end	= ADC_LEVEL_IRQ,
		.name	= "adc-level-irq",
		.flags	= IORESOURCE_IRQ,
	},
#endif
};

struct platform_device ambarella_adc0 = {
	.name		= "ambarella-adc",
	.id		= -1,
	.resource	= ambarella_adc_resources,
	.num_resources	= ARRAY_SIZE(ambarella_adc_resources),
	.dev		= {
		.platform_data		= &ambarella_platform_adc_controller0,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

