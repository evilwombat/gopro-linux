/*
 * arch/arm/plat-ambarella/include/plat/fio.h
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

#ifndef __PLAT_AMBARELLA_COMMON_FIO_H
#define __PLAT_AMBARELLA_COMMON_FIO_H

/* ==========================================================================*/
#define SELECT_FIO_FREE		(0)
#define SELECT_FIO_FL		(1)
#define SELECT_FIO_XD		(2)
#define SELECT_FIO_CF		(3)
#define SELECT_FIO_SD		(4)
#define SELECT_FIO_SDIO		(5)
#define SELECT_FIO_SD2		(6)

#define FIO_OP_NOT_DONE_ER	(-1)	/* operation(xfer) not done error */
#define FIO_READ_ER		(-2)	/* uncorrected ECC error */
#define	FIO_ADDR_ER		(-3)	/* address unaligned error */

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/

/* ==========================================================================*/
extern void fio_amb_fl_enable(void);
extern void fio_amb_fl_disable(void);
extern int fio_amb_fl_is_enable(void);

extern int fio_amb_sd0_is_enable(void);
extern int fio_amb_sdio0_is_enable(void);

extern void fio_amb_cf_enable(void);
extern void fio_amb_cf_disable(void);
extern int fio_amb_cf_is_enable(void);

extern void fio_amb_sd2_enable(void);
extern void fio_amb_sd2_disable(void);
extern int fio_amb_sd2_is_enable(void);

extern int fio_dma_parse_error(u32 reg);

extern void fio_select_lock(int module);
extern void fio_unlock(int module);

extern int ambarella_init_fio(void);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

