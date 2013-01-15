/*
 * include/linux/aipc/lk_pmic.h
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

#ifndef __AIPC_LK_PMIC_H__
#define __AIPC_LK_PMIC_H__

#ifdef __KERNEL__

#ifdef CONFIG_AMBAPMIC_POWER
extern int ambapmic_pwr_src_notify(unsigned int modules);
#else
int ambapmic_pwr_src_notify(unsigned int modules)
{
	printk("dummy %s function\n",__func__);
	return 0;
}
#endif

#endif  /* __KERNEL__ */

#endif

