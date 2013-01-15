/*
 * include/linux/aipc/i_sdresp.h
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
 * Copyright (C) 2011-2015, Ambarella Inc.
 */

#ifndef __AIPC_I_SDRESP_H__
#define __AIPC_I_SDRESP_H__

#ifdef __KERNEL__

struct ipc_sdinfo {

	u32 slot_id;	/**< from LK */
	u8  from_ipc;	/**< from LK */

	u8  is_init;
	u8  is_sdmem;
	u8  is_mmc;
	u8  is_sdio;
	u32 rca;
	u32 hcs;
	u32 ocr;
	u32 clk;
	u32 bus_width;
};

struct ipc_sdresp {

	u32 slot_id;
	u32 opcode;
	int ret;
	u32 resp[4];
	char buf[512];
};

extern int ipc_sdinfo_get(int index, struct ipc_sdinfo *sdinfo);

extern int ipc_sdresp_get(int index, struct ipc_sdresp *sdresp);

#endif	/* __KERNEL__ */

#endif


