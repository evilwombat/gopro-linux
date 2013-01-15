/*
 * include/linux/aipc/i_pmic.h
 *
 * Authors:
 *	
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

#ifndef __AIPC_I_PMIC_H__
#define __AIPC_I_PMIC_H__

#ifdef __KERNEL__

#define IPC_PMIC_MOD_WALL	(0x00000001)
#define IPC_PMIC_MOD_BAT	(0x00000002)
#define IPC_PMIC_MOD_USB	(0x00000004)

unsigned int ipc_ipmic_get_modules(unsigned int pmic_id);
int ipc_ipmic_init_module(unsigned int module_id);
int ipc_ipmic_get_prop_int(unsigned int module_id, unsigned int prop_id);
int ipc_ipmic_set_prop_int(unsigned int module_id, unsigned int prop_id, int prop_value);
int ipc_ipmic_get_prop_str(unsigned int module_id, unsigned int prop_id, char *prop_value);
int ipc_ipmic_set_prop_str(unsigned int module_id, unsigned int prop_id, char *prop_value);

#endif  /* __KERNEL__ */

#endif

