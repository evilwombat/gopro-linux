/*
 * drivers/sensor/sensor.c
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

#include "sensor.h"

struct amb_async_proc_info	sensor_event_proc;
struct amb_event_pool		sensor_event_pool;
EXPORT_SYMBOL(sensor_event_proc);
EXPORT_SYMBOL(sensor_event_pool);

static ssize_t amba_sensor_event_proc_read(struct file *filp,
	char __user *buf, size_t count, loff_t *offset)
{
	struct amb_event_pool			*pool;
	struct amb_event			event;
	size_t					i;
	int					ret;

	pool = (struct amb_event_pool *)filp->private_data;
	if (!pool) {
		return -EINVAL;
	}

	for (i = 0; i < count / sizeof(struct amb_event); i++) {
		if (amb_event_pool_query_event(pool, &event, *offset))
			break;
		ret = copy_to_user(&buf[i * sizeof(struct amb_event)],
			&event, sizeof(event));
		if (ret)
			return -EFAULT;
		(*offset)++;
	}

	if (i == 0) {
		*offset = amb_event_pool_query_index(pool);
		if (!amb_event_pool_query_event(pool, &event, *offset)) {
			ret = copy_to_user(buf, &event, sizeof(event));
			if (ret)
				return -EFAULT;
			(*offset)++;
			i++;
		}
	}

	return i * sizeof(struct amb_event);
}

static int __init amba_sensor_init(void)
{
	int		errorCode	= 0;

	amb_event_pool_init(&sensor_event_pool);
	snprintf(sensor_event_proc.proc_name,	sizeof(sensor_event_proc.proc_name), "sensor_event");
	sensor_event_proc.fops.read	= amba_sensor_event_proc_read;
	sensor_event_proc.private_data	= &sensor_event_pool;
	errorCode = amb_async_proc_create(&sensor_event_proc);
	if (errorCode) {
		printk("Create event proc %s failed!\n", sensor_event_proc.proc_name);
		goto amba_sensor_init_exit;
	}

amba_sensor_init_exit:
	return errorCode;
}

static void __exit amba_sensor_exit(void)
{
	amb_async_proc_remove(&sensor_event_proc);
}

module_init(amba_sensor_init);
module_exit(amba_sensor_exit);

MODULE_DESCRIPTION("Ambarella Sensor Driver Core");
MODULE_AUTHOR("Zhenwu Xue, <zwxue@ambarella.com>");
MODULE_LICENSE("GPL");
