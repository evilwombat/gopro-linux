/*
 * drivers/ambarella-ipc/binder.h
 *
 * Ambarella IPC in Linux kernel-space.
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

#ifndef __AMBARELLA_IPC__BINDER_H__
#define __AMBARELLA_IPC__BINDER_H__

#include <linux/aipc/aipc.h>
#include <linux/aipc/irq.h>
#include <mach/boss.h>

#ifdef __KERNEL__

struct aipc_nl_prog
{
	struct list_head list;		/* Linked to server head */
	enum aipc_nl_status status;
	struct aipc_nl_prog_info prog_info;
	struct aipc_nl_proc_info proc_info[0];
};

/*
 * The binder object contains four streaming circular buffers.
 * The client has two, and the server has two, that are used for
 * sending/receiving pointers to transactions to the remote OS.
 */
struct ipc_binder_s
{
	int init;

	unsigned int pending_req;
	unsigned int pending_rsp;

	SVCXPRT *svcxprt_in;
	SVCXPRT *svcxprt_out;

	SVCXPRT svcxprt_in_bak;
	SVCXPRT svcxprt_out_bak;

	/* Internal data */
	spinlock_t lock;
	spinlock_t lu_done_lock;
	spinlock_t lu_prog_lock;

	CLIENT *clients, *clients_tail;
	struct ipc_prog_s *prog_svc, *prog_svc_tail;
	struct ipc_prog_s *prog_clnt, *prog_clnt_tail;

	struct sock *nlsock;
	struct list_head lu_prog_list;
	struct list_head lu_req_list;
	struct list_head lu_done_list;
	unsigned int lu_prog_id;

	struct ipcstat_s *ipcstat;

	unsigned int tick_in_1_ms;
	unsigned int tick_irq_last;
	unsigned int tick_irq_interval;
	
#if defined(STATIC_SVC)
	spinlock_t svcxprt_lock;
	unsigned char *svcxprt_buf;
	unsigned char *svcxprt_buf_raw;
	SVCXPRT *svcxprts[IPC_BINDER_MSG_BUFSIZE - 1];
	int svcxprt_mask;   /* free slot is marked by 1 */
	int svcxprt_num;
	int svcxprt_in_queue;
	int svcxprt_in_queue_max;
#endif
};

extern struct ipc_binder_s *binder;

extern enum clnt_stat ipc_lu_clnt_call(unsigned int clnt_pid,
				       struct aipc_nl_data *aipc_hdr,
				       char *in, char *out);

#endif  /* __KERNEL__ */

#endif
