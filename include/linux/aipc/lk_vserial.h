/*
 * include/linux/aipc/lk_vserial.h
 *
 * Authors:
 *	Keny Huang <skhuang@ambarella.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * Copyright (C) 2009-2011, Ambarella Inc.
 */

#ifndef __AIPC_LK_VSERIAL_H__
#define __AIPC_LK_VSERIAL_H__

struct amba_vserial_msg {
	char *base_addr;
	int size;
};
#ifdef __KERNEL__

#ifdef CONFIG_VIRTUAL_SERIAL_AMBARELLA
extern int amba_vserial_report_msg(struct amba_vserial_msg *msg);
#else
int amba_vserial_report_msg(struct amba_vserial_msg *msg)
{
//	printk("Dummy %s function.\n",__func__);
	return 0;
}
#endif

#endif  /* __KERNEL__ */

#endif
