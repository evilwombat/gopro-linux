/*
 * include/linux/aipc/irq.h
 *
 * Authors:
 *	Charles Chiou <cchiou@ambarella.com>
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
 * Copyright (C) 2009-2010, Ambarella Inc.
 */

#ifndef __IPC_IRQ_H__
#define __IPC_IRQ_H__

#define IPC_IRQ_REQ	0
#define IPC_IRQ_RLY	1

/*
 * Status of IPC-IRQ.
 */
struct ipc_irq_stat_s
{
	unsigned int enabled;
	unsigned int recv;
	unsigned int sent;
};

extern void ipc_send_irq(int);		/* Send IRQ to other OS */
extern void ipc_fake_irq(int);		/* Send IRQ to this OS (loop-back) */
extern void ipc_irq_enable(int, int);	/* Enable/disable IRQ */
extern void ipc_irq_get_stat(struct ipc_irq_stat_s *);

#endif
