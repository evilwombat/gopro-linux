/*
 * include/linux/aipc/aipc.h
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

#ifndef __AIPC__AIPC_H__
#define __AIPC__AIPC_H__

#ifdef __KERNEL__

#include <linux/aipc/aipc_struct.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/module.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/io.h>
#include <mach/hardware.h>

#include <ambhw/chip.h>

#if (CHIP_REV == I1)
#include <plat/ambcache.h>
#endif

#if (CHIP_REV == A5S)
#define CONFIG_NEED_PTR_CONV		0
#elif (CHIP_REV == A7)
#define CONFIG_NEED_PTR_CONV		0
#elif (CHIP_REV == A7L)
#define CONFIG_NEED_PTR_CONV		0
#elif (CHIP_REV == I1)
#define CONFIG_NEED_PTR_CONV		1
#else
#error "Not yet supported!"
#endif

#define AMBARELLA_IPC_INIT(x)		int aipc_init_##x(void) { return 0; }

#define STATIC_SVC
#define K_ASSERT(x)			BUG_ON(!(x))

#define CACHE_LINE_SIZE     32
#define CACHE_LINE_MASK     ~(CACHE_LINE_SIZE - 1)

#ifndef __ATTRIB_WEAK__
#define __ATTRIB_WEAK__	__attribute__((weak))
#endif
#ifndef __ARMCC_WEAK__
#define __ARMCC_WEAK__
#endif

struct SVCXPRT;

#define IPC_I2L_INT_REQ_VEC	BOSS_VIRT_H2G_INT_REQ_VEC
#define IPC_I2L_INT_RLY_VEC	BOSS_VIRT_H2G_INT_RLY_VEC
#define IPC_L2I_INT_REQ_VEC	BOSS_VIRT_G2H_INT_REQ_VEC
#define IPC_L2I_INT_RLY_VEC	BOSS_VIRT_G2H_INT_RLY_VEC

extern u32 ambarella_phys_to_virt(u32 paddr);
extern u32 ambarella_virt_to_phys(u32 vaddr);
extern unsigned int ipc_ctz(unsigned int val);
extern u32 ipc_tick_diff(u32 start, u32 end);
extern u32 ipc_tick_get(void);

#if CONFIG_NEED_PTR_CONV

extern u32 ipc_phys_to_virt (u32 phys);
extern u32 ipc_virt_to_phys (u32 virt);

#else	/* !CONFIG_NEED_PTR_CONV */

#define ipc_phys_to_virt(p)			(p)
#define ipc_virt_to_phys(v)			(v)
#define ipc_flush_cache(p,s)
#define ipc_inv_cache(p,s)
#define ipc_clean_cache(p,s)

#define ambcache_clean_range(addr, size)
#define ambcache_inv_range(addr, size)
#define ambcache_flush_range(addr, size)

#endif

extern struct proc_dir_entry *aipc_proc_dir;

enum {
	IPC_TIME_SEND_REQEST = 0,
	IPC_TIME_GOT_REQEST,
	IPC_TIME_SEND_REPLY,
	IPC_TIME_GOT_REPLY,
	IPC_TIME_NUM
};

#define IPC_DUMMY_ALIGN		9

#define IPC_TAG_BEGIN		0x42435049	/* 'IPCB' */
#define IPC_TAG_END		0x45435049	/* 'IPCE' */

enum clnt_stat
{
	IPC_SUCCESS = 0,
	/*
	 * Local errors
	 */
	IPC_BADCLIENT = 1,	/* Invalid client handle */
	IPC_NOPROG = 2,		/* Server program not registered */
	IPC_NOPROC = 3,		/* Server procedure not registered */
	IPC_CANTSEND = 4,	/* Failed in sending */
	IPC_CANTRECV = 5,	/* Failed in receiving */
	IPC_TIMEDOUT  = 6,	/* Call timeout */
	/*
	 * Remote errors
	 */
	IPC_VERSMISMATCH = 7,	/* Version mismatch */
	IPC_PROGUNAVAIL = 8,	/* Server program unavailable */
	IPC_PROCUNAVAIL = 9,	/* Server procedure unavailable */
	IPC_EINVAL = 10,	/* Server handler unable to process argument */
	IPC_NOMEM = 11,		/* Server ran out of memory */
	/*
	 * Everything else
	 */
	IPC_UNKNOWN = 12,
	IPC_PROCESSING = 13,	/* IPC Call is processing */
	IPC_CMD_QUEUE_FULL = 14,/* IPC command queue is full */
	IPC_UNITIALIZED = 15,	/* IPC service is not initialized */
	IPC_CANCEL = 16         /* IPC command canceled by server */
};

/*
 * Table of functions associated with a registered IPC server program.
 */
#define IPC_CALL_ASYNC			0x00000001
#define IPC_CALL_ASYNC_AUTO_DEL		0x00000002
#define IPC_CALL_ASYNC_MASK		0x00000003

struct ipcgen_table
{
	void (*proc)(void);	/* Function pointer */
	int fid;		/* Function ID */
	unsigned int type;	/* IPC call type */
	unsigned int len_arg;	/* Length of argument */
	unsigned int len_res;	/* Length of result */
	unsigned int *arg_ptbl;		/* Pointer offset table */
	unsigned int arg_ptbl_num;	/* Pointer offset table entry number */
	unsigned int *res_ptbl;		/* Pointer offset table */
	unsigned int res_ptbl_num;	/* Pointer offset table entry number */
	unsigned int timeout;		/* Timeout in mili-seconds */	
	const char *name;
};

/*
 * Meta object representing an IPC program.
 */
struct ipc_prog_s
{
	const char *name;
	unsigned int prog;
	unsigned int vers;
	struct ipcgen_table *table;
	unsigned int nproc;
	struct ipc_prog_s *next;
	struct ipc_prog_s *prev;
};

typedef unsigned int bool_t;

#define ipc_malloc ambarella_ipc_malloc
#define ipc_free ambarella_ipc_free

extern void *ambarella_ipc_malloc(unsigned int);
extern void ambarella_ipc_free(void *);

struct CLIENT;

/*
 * Service transport - used for holding an IPC transaction.
 *
 * This data structure is shared and pointed to by different operating systems
 * possibly compiled with different compilers. The sizes, offset, and format of
 * each fields must be intepreted correctly when porting to the target OS.
 */
typedef void (*ipc_call_compl_f)(void *, struct SVCXPRT *);

typedef struct SVCXPRT
{
	unsigned int tag;	/* Begin Mark */
	int xid;		/* Transaction ID */
	unsigned int pid;	/* Program ID */
	int fid;		/* Function ID */
	unsigned int vers;	/* Version */
	unsigned int rcode;	/* Result code set by server */

	void *arg;		/* Pointer to argument */
	unsigned int len_arg;	/* Length of argument */
	void *res;		/* Pointer to result */
	unsigned int len_res;	/* Length of result */

	struct ipcgen_table *ftab;	/* Function Table: only access by Linux */
	void *raw_ptr;		/* Raw pointere to be freed, only used for async IPC */
	unsigned int proc_type;	/* Function Type */
	ipc_call_compl_f compl_proc;	/* Completion function */
	void *compl_arg;		/* Completion function's argument */
	unsigned int lu_xid;

	unsigned int time[IPC_TIME_NUM];
	int svcxprt_lock;
	int svcxprt_cancel;
	unsigned int dummy[IPC_DUMMY_ALIGN];
	unsigned int tag_end;	/* End Mark */

	/* Event flag to wait on completion */
	/*
	 * Since this is a local-OS variable, it should be padded to ensure
	 * that size differences of implementations of "mutex" among different
	 * OS fit under the padding and is accounted for
	 */
	union {
		struct {
			struct completion cmpl;
			struct list_head list;
			unsigned int clnt_pid;	/* Used to save pid of client program */
			int timeout;
#ifdef STATIC_SVC
			unsigned int priv_id;
#endif
		} l;
		unsigned char reserved[256];	/* Padded up to 256 bytes */
	} __attribute__((packed)) u;
} __attribute__((packed)) SVCXPRT;

#define SVCXPRT_HEAD_SIZE	offsetof(SVCXPRT,u)
#define SVCXPRT_ALIGNED_SIZE    ((sizeof(SVCXPRT) + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK)

#define IPC_CMD_QUEUE_PTR_NEXT(ptr)	(((ptr) + 1) & IPC_BINDER_MSG_BUFSIZE_MASK)
#define IPC_CMD_QUEUE_IS_FULL(head, tail)	(IPC_CMD_QUEUE_PTR_NEXT(tail) == head)
#define IPC_CMD_QUEUE_NOT_FULL(head, tail)	(IPC_CMD_QUEUE_PTR_NEXT(tail) != head)
#define IPC_CMD_QUEUE_NOT_EMPTY(head, tail)	(head != tail)

/*
 * Request for client.
 */
struct clnt_req
{
	SVCXPRT *svcxprt;

	ipc_call_compl_f compl_proc;	/* Completion function */
	void *compl_arg;		/* Completion function's argument */
};

/*
 * Request for service.
 */
struct svc_req
{
	SVCXPRT *svcxprt;
};

/*
 * Client handle.
 */
typedef struct CLIENT
{
	struct ipc_prog_s *prog;	/* Connected program meta data */

	unsigned int *arg_ptbl;		/* Pointer offset table */
	unsigned int arg_ptbl_num;	/* Pointer offset table entry number */	

	unsigned int *res_ptbl;		/* Pointer offset table */
	unsigned int res_ptbl_num;	/* Pointer offset table entry number */	
	
	struct CLIENT *next;
	struct CLIENT *prev;
} CLIENT;

#define clnt_call ipc_clnt_call

extern enum clnt_stat ipc_clnt_call(CLIENT *clnt, unsigned long procnum,
				    char *in, char *out,
				    struct clnt_req *req);
extern enum clnt_stat ipc_clnt_wait_for_completion(CLIENT *clnt, struct clnt_req *req, int timeout);
extern void ipc_clnt_set_timeout(CLIENT *clnt, struct timeval tout);
extern void ipc_clnt_set_call_timeout(CLIENT *clnt, int fid, struct timeval tout);

extern unsigned int ipc_lu_clnt_next_prog_id (void);

#define svc_sendreply ipc_svc_sendreply
extern bool_t ipc_svc_sendreply(SVCXPRT *xprt, char *out);
extern bool_t ipc_svc_isr_sendreply(SVCXPRT *xprt, char *out);

extern const char *ipc_strerror(enum clnt_stat);

/* ------------------------------------------------------------------------- */

/*
 * Get registered serivce programs.
 */
extern struct ipc_prog_s *ipc_svc_progs(void);

/*
 * Get registered client programs.
 */
extern struct ipc_prog_s *ipc_clnt_progs(void);

/*
 * Get svc statistics.
 */
extern void  ipc_svc_get_stat(struct ipcstat_s *ipcstat);

/*
 * Register a service program.
 */
extern int ipc_svc_prog_register(struct ipc_prog_s *prog);

/*
 * Unregister a service program.
 */
extern int ipc_svc_prog_unregister(struct ipc_prog_s *prog);

/*
 * Register a client program.
 */
extern CLIENT *ipc_clnt_prog_register(struct ipc_prog_s *prog);

/*
 * Unregister a client program.
 */
extern int ipc_clnt_prog_unregister(struct ipc_prog_s *prog, CLIENT *client);

#ifdef STATIC_SVC
/*
 * Check ipc service cancel.
 */
extern int ipc_cancel_check(SVCXPRT *svcxprt);
#endif

/**************************/
/* IPC bottom half helper */
/**************************/

typedef bool_t (*ipc_bh_f)(void *, void *, SVCXPRT *);

/*
 * Queue up a function to be called by the IPC bottom half helper.
 */
extern void ipc_bh_queue(ipc_bh_f, void *, void *, SVCXPRT *);

/* ------------------------------------------------------------------------- */

/*
 * Initialize netlink.
 */
extern int aipc_nl_init(void);

/*
 * Notify user space that service request coming
 */
extern int aipc_nl_get_request_from_itron(SVCXPRT *svcxprt);

/*
 * Notify user space that result coming
 */
extern int aipc_nl_send_result_to_lu(SVCXPRT * svcxprt);

#endif  /* __KERNEL__ */

/* ------------------------------------------------------------------------- */

#if defined(__KERNEL__)
#define NL_MAX_BUFLEN	(16 * 1024)	/* Netlink Buffer */
#endif

#define NL_AIPC_HDRLEN	sizeof(struct aipc_nl_data)
#define NL_AIPC_DATA(aipc_hdr)	((aipc_hdr)->data)

#define NETLINK_AIPC		25
#define MIN_IIPC_PROG_NUM	0x10000000
#define MIN_LKIPC_PROG_NUM	0x20000000
#define MIN_LUIPC_PROG_NUM	0x30000000

/*
 * struct aipc_nl_prog_info and struct aipc_nl_proc_info are only used to
 * transfer user-space prog and proc information into kernel, then kernel
 * can provide meaningful information through /proc/aipc/proginfo.
 */
#define MAX_PROG_NAME_LENGTH	64
struct aipc_nl_prog_info
{
	unsigned int uid;
	unsigned int pid;
	unsigned int vers;
	unsigned int nproc;
	char name[MAX_PROG_NAME_LENGTH];
}__attribute__((packed));

struct aipc_nl_proc_info
{
	void (*proc)(void);
	unsigned int fid;
	unsigned int len_arg;
	unsigned int len_res;
	char name[MAX_PROG_NAME_LENGTH];
}__attribute__((packed));

struct aipc_nl_data
{
	int xid;		/* Transaction ID */
	unsigned int pid;	/* Program ID */
	int fid;		/* Function ID */
	unsigned int vers;	/* Version */
	unsigned int rcode;	/* Result code set by server */
	unsigned int len_arg;	/* Length of argument */
	unsigned int len_res;	/* Length of result */
	unsigned int proc_type;	/* Function type */
	unsigned int lu_xid;	/* LU xid */
	unsigned int timeout;	/* timeout */

	unsigned char data[0];
};

enum aipc_nl_status
{
	AIPC_NLSTATUS_ERR_EINVAL = -4,
	AIPC_NLSTATUS_ERR_EXIST = -3,
	AIPC_NLSTATUS_ERR_NOENT = -2,
	AIPC_NLSTATUS_ERR_OTHERS = -1,
	AIPC_NLSTATUS_UNREGISTERED = 0,
	AIPC_NLSTATUS_REGISTERED = 1,
};

enum aipc_nl_type
{
	AIPC_NLTYPE_UNKNOWN = 0,
	AIPC_NLTYPE_REGISTER = 1,
	AIPC_NLTYPE_UNREGISTER = 2,
	AIPC_NLTYPE_REQUEST = 3,
	AIPC_NLTYPE_RESULT = 4,
};

#endif
