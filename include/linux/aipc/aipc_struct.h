/*
 * include/linux/aipc/aipc_struct.h
 *
 * Authors:
 *	Henry Lin <hllin@ambarella.com>
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

#ifndef __AIPC_STRUCT_H__
#define __AIPC_STRUCT_H__

#define K_ASSERT(x)			BUG_ON(!(x))

/*
 * IPC binder buffer.
 */
#define IPC_BINDER_MSG_BUFSIZE_BITS	4
#define IPC_BINDER_MSG_BUFSIZE_MASK	((1 << IPC_BINDER_MSG_BUFSIZE_BITS) - 1)
#define IPC_BINDER_MSG_BUFSIZE		(1 << IPC_BINDER_MSG_BUFSIZE_BITS)
#define IPC_BINDER_SVCXPRT_BUF_SIZE	(SVCXPRT_ALIGNED_SIZE * (IPC_BINDER_MSG_BUFSIZE - 1))

struct ipcbuf_stat_s
{
	unsigned int size;
	unsigned int head;
	unsigned int tail;
};

struct ipccall_stat_s
{
	unsigned int max;	/* maximium time */
	unsigned int min;	/* minimal time */
	unsigned int total;	/* total time */
	unsigned int slow;	/* slow count */

	unsigned int xid;	/* maximium time: xid */
	unsigned int pid;	/* maximium time: pid */
	unsigned int fid;	/* maximium time: fid */
};

struct ipcprog_stat_s
{
	unsigned int invocations;
	unsigned int success;
	unsigned int failure;

	struct ipccall_stat_s req;
	struct ipccall_stat_s rsp;
	struct ipccall_stat_s wakeup;
	struct ipccall_stat_s call;
};

struct ipcstat_s
{
	struct ipcbuf_stat_s clnt_outgoing;
	struct ipcbuf_stat_s clnt_incoming;
	struct ipcbuf_stat_s svc_incoming;
	struct ipcbuf_stat_s svc_outgoing;

	struct ipcprog_stat_s uitron_prog;
	struct ipcprog_stat_s linux_prog;

	unsigned int next_xid;
	unsigned int timescale;
};

struct ipc_buf_s
{
	/* Cicular buffers */
	void *svc_incoming[IPC_BINDER_MSG_BUFSIZE];
	void *svc_outgoing[IPC_BINDER_MSG_BUFSIZE];
	void *clnt_outgoing[IPC_BINDER_MSG_BUFSIZE];
	void *clnt_incoming[IPC_BINDER_MSG_BUFSIZE];

	/* Index to circular buffers */
	unsigned int svc_in_head;
	unsigned int svc_in_tail;
	unsigned int svc_out_head;
	unsigned int svc_out_tail;
	unsigned int clnt_out_head;
	unsigned int clnt_out_tail;
	unsigned int clnt_in_head;
	unsigned int clnt_in_tail;

	/* Spinlock for buffers */
	int svc_in_lock;
	int svc_out_lock;
	int clnt_out_lock;
	int clnt_in_lock;

	/* SVCXPRT spinlocks for Linux */
	unsigned int linux_svcxprt_lock_start;
	unsigned int linux_svcxprt_num;

	int lock;
	unsigned int ipc_xid;

	struct ipcstat_s ipcstat;

};

#endif
