/*
 * arch/arm/plat-ambarella/include/plat/crypto.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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

#ifndef __PLAT_AMBARELLA_CRYPTO_H
#define __PLAT_AMBARELLA_CRYPTO_H

/* ==========================================================================*/
#define AMBARELLA_CRYPTO_ALIGNMENT	(16)
#define AMBARELLA_CRA_PRIORITY		(300)
#define AMBARELLA_COMPOSITE_PRIORITY	(400)

#define AMBA_HW_ENCRYPT_CMD		(0)
#define AMBA_HW_DECRYPT_CMD		(1)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct ambarella_platform_crypto_info{
	u32					reserved;
};

/* ==========================================================================*/
extern struct platform_device			ambarella_crypto;

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

