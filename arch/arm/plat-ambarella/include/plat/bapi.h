/*
 * arch/arm/plat-ambarella/include/plat/bapi.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __PLAT_AMBARELLA_BAPI_H
#define __PLAT_AMBARELLA_BAPI_H

/* ==========================================================================*/
#define DEFAULT_BAPI_TAG_MAGIC			(0x19450107)
#define DEFAULT_BAPI_MAGIC			(0x19790110)
#define DEFAULT_BAPI_VERSION			(0x00000001)
#define DEFAULT_BAPI_SIZE			(4096)
#define DEFAULT_BAPI_AOSS_SIZE			(1024)

#define DEFAULT_BAPI_AOSS_MAGIC			(0x19531110)

#define DEFAULT_BAPI_REBOOT_MAGIC		(0x4a32e9b0)
#define AMBARELLA_BAPI_CMD_REBOOT_NORMAL	(0xdeadbeaf)
#define AMBARELLA_BAPI_CMD_REBOOT_RECOVERY	(0x5555aaaa)
#define AMBARELLA_BAPI_CMD_REBOOT_FASTBOOT	(0x555aaaa5)
#define AMBARELLA_BAPI_CMD_REBOOT_SELFREFERESH	(0x55aaaa55)
#define AMBARELLA_BAPI_CMD_REBOOT_HIBERNATE	(0x5aaaa555)

#define AMBARELLA_BAPI_REBOOT_HIBERNATE		(0x1 << 0)
#define AMBARELLA_BAPI_REBOOT_SELFREFERESH	(0x1 << 1)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

enum ambarella_bapi_cmd_e {
	AMBARELLA_BAPI_CMD_INIT			= 0x0000,

	AMBARELLA_BAPI_CMD_AOSS_INIT		= 0x1000,
	AMBARELLA_BAPI_CMD_AOSS_COPY_PAGE	= 0x1001,
	AMBARELLA_BAPI_CMD_AOSS_SAVE		= 0x1002,

	AMBARELLA_BAPI_CMD_SET_REBOOT_INFO	= 0x2000,
	AMBARELLA_BAPI_CMD_CHECK_REBOOT		= 0x2001,

	AMBARELLA_BAPI_CMD_UPDATE_FB_INFO	= 0x3000,
};

struct ambarella_bapi_aoss_page_info_s {
	u32					src;
	u32					dst;
	u32					size;
};

struct ambarella_bapi_aoss_s {
	u32					fn_pri[256 - 4];
	u32					magic;
	u32					total_pages;
	u32					copy_pages;
	u32					page_info;
};

struct ambarella_bapi_reboot_info_s {
	u32					magic;
	u32					mode;
	u32					flag;
	u32					rev;
};

struct ambarella_bapi_fb_info_s {
	int					xres;
	int					yres;
	int					xvirtual;
	int					yvirtual;
	int					format;
	u32					fb_start;
	u32					fb_length;
	u32					bits_per_pixel;
};

struct ambarella_bapi_s {
	u32					magic;
	u32					version;
	int					size;
	u32					crc;
	u32					mode;
	u32					block_dev;
	u32					block_start;
	u32					block_num;
	u32					rev0[64 - 8];
	struct ambarella_bapi_reboot_info_s	reboot_info;
	u32					fb_start;
	u32					fb_length;
	struct ambarella_bapi_fb_info_s		fb0_info;
	struct ambarella_bapi_fb_info_s		fb1_info;
	u32					rev1[64 - 4 - 8 - 8 - 2];
	u32					debug[128];
	u32					rev2[1024 - 128 - 128 - 256];
	struct ambarella_bapi_aoss_s		aoss_info;
};

struct ambarella_bapi_tag_s {
	u32					magic;
	u32					pbapi_info;
};

/* ==========================================================================*/
typedef unsigned int (*ambarella_bapi_aoss_call_t)(u32, u32, u32, u32);
typedef void (*ambarella_bapi_aoss_return_t)(void);

extern int ambarella_bapi_cmd(enum ambarella_bapi_cmd_e cmd, void *args);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

