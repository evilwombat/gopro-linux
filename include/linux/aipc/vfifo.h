/*
 * include/linux/aipc/vfifo.h
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

#ifndef __AIPC_VFIFO_H__
#define __AIPC_VFIFO_H__

#ifdef __KERNEL__

#define VFIFO_EVENT_REBASED	0
#define VFIFO_EVENT_DELETED	1
#define VFIFO_EVENT_UPDATED	2

#define VFIFO_MASK_REBASED	0x1
#define VFIFO_MASK_DELETED	0x2
#define VFIFO_MASK_UPDATED	0x4
#define VFIFO_MASK_ALL		0xffffffff

#define VFIFO_UPDATE_TYPE_ALWAYS	0
#define VFIFO_UPDATE_TYPE_COUNT		1
#define VFIFO_UPDATE_TYPE_THRESHOLD	2

/*
 * Client notification parameter.
 */
struct vfifo_notify
{
	int index;
	int evmask;
	int uptype;
	int count;
	unsigned int threshold;
};

struct vfifo_event
{
	int index;
	int event;
	unsigned int saddr;
	unsigned int eaddr;
};

/*
 * Remote vfifo server info.
 */
struct vfifo_info
{
	char name[8];
	char description[32];
	unsigned int base;
	unsigned int size;
};

struct vfifo_stat
{
	unsigned int waddr;
	unsigned int raddr;
	struct vfifo_notify *notify;
	struct vfifo_event *event;
	unsigned int nupdate;
};

/* ------------ */
/* vfifo server */
/*------------- */

extern int local_vfifo_max(void);
extern int local_vfifo_num(void);
extern int local_vfifo_index_of(const char *name);
extern const struct vfifo_info *local_vfifo_get_info(int index);
extern const struct vfifo_stat *local_vfifo_get_stat(int index);

extern int local_vfifo_register(const char *name,
				const char *description,
				unsigned int base,
				unsigned int size,
				unsigned int waddr);
extern int local_vfifo_unregister(int index);

extern int local_vfifo_rebase(int index,
			      unsigned int base,
			      unsigned int size,
			      unsigned int waddr);
extern int local_vfifo_update(int index, unsigned int waddr);

#if defined(__VFIFO_IMPL__)

extern int local_vfifo_set_notify(struct vfifo_notify *notify);

#endif

/* ------------ */
/* vfifo client */
/* ------------ */

typedef void (*vfifo_event_cb)(struct vfifo_event event, void *args);

extern int remote_vfifo_max(void);
extern int remote_vfifo_num(void);
extern int remote_vfifo_index_of(const char *name);
extern const struct vfifo_info *remote_vfifo_get_info(int index);
extern const struct vfifo_stat *remote_vfifo_get_stat(int index);

extern int remote_vfifo_subscribe(int index,
				  struct vfifo_notify *clnt_param,
				  struct vfifo_event *event,
				  vfifo_event_cb cb,
				  void *args);
extern int remote_vfifo_unsubscribe(int index, vfifo_event_cb cb);
extern int remote_vfifo_update(int index, unsigned int raddr);

#if defined(__VFIFO_IMPL__)

extern int remote_vfifo_changed(void);
extern int remote_vfifo_pushed_event(struct vfifo_event *event);

#endif

/* ------------------------------------------------------------------------- */

#define VPIPE_REQ_CREATE	0
#define VPIPE_REQ_DESTROY	1
typedef int (*vpipe_handler)(int type, int inpipe, void *args);

extern int vpipe_add_handler(const char *name, vpipe_handler hdl, void *args);
extern int vpipe_del_handler(const char *name, vpipe_handler hdl);

extern int vpipe_create(const char *name, int inpipe);
extern int vpipe_destroy(const char *name, int inpipe);

/* ------------------------------------------------------------------------- */

#if defined(__VFIFO_IMPL__)

#define MAX_VFIFO_OBJECTS	32

/*
 * Call-back to registered subscriber(s).
 */
struct vfifo_cb
{
	vfifo_event_cb func;
	void *args;
};

/*
 * Master vfifo object.
 */
struct vfifo
{
	struct vfifo_info info;
	struct vfifo_stat stat;
	struct vfifo_notify notify;
	struct vfifo_event event;
	int index;
	int valid;
	struct vfifo_cb *cb;
};

#endif  /* __VFIFO_IMPLE__ */

#endif  /* __KERNEL__ */

#endif
