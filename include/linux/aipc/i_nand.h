/*
 * include/linux/aipc/i_nand.h
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

#ifndef __AIPC_I_NAND_H__
#define __AIPC_I_NAND_H__

#ifdef __KERNEL__

extern int ipc_nand_reset(unsigned int bank);
extern int ipc_nand_read_id(unsigned int addr_hi, unsigned int addr,
			    unsigned int *id);
extern int ipc_nand_read_status(unsigned int addr_hi, unsigned int addr,
				unsigned int *status);
extern int ipc_nand_copyback(unsigned int addr_hi, unsigned int addr,
			     unsigned int dst, int interleave);
extern int ipc_nand_erase(unsigned int addr_hi, unsigned int addr,
			  unsigned int interleave);
extern int ipc_nand_read(unsigned int addr_hi, unsigned int addr,
			 char *buf, unsigned int len,
			 int area, int ecc, int interleave);
extern int ipc_nand_burst_read(unsigned int addr_hi, unsigned int addr,
			       char *buf, unsigned int len, int nbr,
			       int area, int ecc, int interleave);
extern int ipc_nand_write(unsigned int addr_hi, unsigned int addr,
			  char *buf, unsigned int len,
			  int area, int ecc, int interleave);

#endif  /* __KERNEL__ */

#endif
