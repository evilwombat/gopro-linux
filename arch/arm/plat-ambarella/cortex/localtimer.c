/*
 * arch/arm/plat-ambarella/cortex/localtimer.c
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

#include <linux/init.h>
#include <linux/smp.h>
#include <linux/clockchips.h>

#include <asm/irq.h>
#include <asm/smp_twd.h>
#include <asm/localtimer.h>

#include <hal/hal.h>

void __cpuinit local_timer_setup(struct clock_event_device *evt)
{
	evt->irq = LOCAL_TIMER_IRQ;
	twd_timer_setup_rate(evt, amb_get_axi_clock_frequency(HAL_BASE_VP));
}

void local_timer_update_rate(struct clock_event_device *evt, u32 timer_rate)
{
	twd_timer_update_rate(evt, timer_rate);
}

