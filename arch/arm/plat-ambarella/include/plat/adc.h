/*
 * arch/arm/plat-ambarella/include/plat/adc.h
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

#ifndef __PLAT_AMBARELLA_ADC_H
#define __PLAT_AMBARELLA_ADC_H

/* ==========================================================================*/
#if (CHIP_REV == A5)
#define ADC_EN_HI(x)			((x) << 21)
#define ADC_EN_LO(x)			((x) << 20)
#else
#define ADC_EN_HI(x)			((x) << 31)
#define ADC_EN_LO(x)			((x) << 30)
#endif
#define ADC_VAL_LO(x)			((x) & 0x3ff)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

enum ambarella_adc_mode{
	AMBA_ADC_IRQ_MODE,
	AMBA_ADC_POL_MODE
};

struct ambarella_adc_controller {
	void				(*read_channels)(u32*, u32*);
	u32				(*is_irq_supported)(void);
	void				(*set_irq_threshold)(u32, u32, u32);
	void				(*reset)(void);
	void				(*stop)(void);
	u32				(*get_channel_num)(void);

	u32				scan_delay;
};
#define AMBA_ADC_PARAM_CALL(arg, perm) \
	module_param_cb(adc_scan_delay, &param_ops_int, &arg.scan_delay, perm)

/* ==========================================================================*/
extern struct platform_device		ambarella_adc0;

/* ==========================================================================*/
extern int ambarella_init_adc(void);
extern u32 ambarella_adc_suspend(u32 level);
extern u32 ambarella_adc_resume(u32 level);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

