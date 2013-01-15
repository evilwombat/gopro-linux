/*
 * drivers/sensor/sensor.h
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
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
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <asm/uaccess.h>
#include <plat/ambasyncproc.h>
#include <plat/ambevent.h>

#ifdef	CONFIG_DEBUG_SENSOR
#define SENSOR_DEBUG(format, arg...)	printk(format , ## arg)
#else
#define SENSOR_DEBUG(format, arg...)
#endif

extern struct amb_async_proc_info	sensor_event_proc;
extern struct amb_event_pool		sensor_event_pool;
