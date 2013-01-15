/*
 * include/linux/aipc/i_gpio.h
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

#ifndef __AIPC_I_GPIO_H__
#define __AIPC_I_GPIO_H__

#ifdef __KERNEL__

extern int ipc_gpio_config(int line, int func);
extern int ipc_gpio_set(int line);
extern int ipc_gpio_clr(int line);
extern int ipc_gpio_get(int line);
extern int ipc_gpio_query_line(int line, int *type, int *state, int *dir);

typedef void (*ipc_gfi_handler)(int line, void *arg);

extern int ipc_gfi_req(int line, int time, ipc_gfi_handler h, void *arg);
extern int ipc_gfi_free(int line, ipc_gfi_handler h);

#endif  /* __KERNEL__ */

#endif
