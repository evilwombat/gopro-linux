/*
 * arch/arm/plat-ambarella/include/plat/spi.h
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

#ifndef __PLAT_AMBARELLA_SPI_H
#define __PLAT_AMBARELLA_SPI_H

#define SPI_MASTER_INSTANCES	(SPI_INSTANCES + SPI_AHB_INSTANCES)

/* ==========================================================================*/
/* SPI rw mode */
#define SPI_WRITE_READ		0
#define SPI_WRITE_ONLY		1
#define SPI_READ_ONLY		2

/* Tx FIFO empty interrupt mask */
#define SPI_TXEIS_MASK		0x00000001
#define SPI_TXOIS_MASK 		0x00000002

/* SPI Parameters */
#define SPI_DUMMY_DATA		0xffff
#define MAX_QUERY_TIMES		10

/* Default SPI settings */
#define SPI_MODE		SPI_MODE_0
#define SPI_SCPOL		0
#define SPI_SCPH		0
#define SPI_FRF			0
#define SPI_CFS			0x0
#define SPI_DFS			0xf
#define SPI_BAUD_RATE		200000

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct ambarella_spi_hw_info {
	int	bus_id;
	int	cs_id;
};
typedef struct ambarella_spi_hw_info amba_spi_hw_t;

struct ambarella_spi_cfg_info {
	u8	spi_mode;
	u8	cfs_dfs;
	u8	lsb_first;
	u8	cs_change;
	u32	baud_rate;
};
typedef struct ambarella_spi_cfg_info amba_spi_cfg_t;

typedef struct {
	u8	bus_id;
	u8	cs_id;
	u8	*buffer;
	u16	n_size;
} amba_spi_write_t;

typedef struct {
	u8	bus_id;
	u8	cs_id;
	u8	*buffer;
	u16	n_size;
} amba_spi_read_t;

typedef struct {
	u8	bus_id;
	u8	cs_id;
	u8	*w_buffer;
	u8	*r_buffer;
	u16	w_size;
	u16	r_size;
} amba_spi_write_then_read_t;

typedef struct {
	u8	bus_id;
	u8	cs_id;
	u8	*w_buffer;
	u8	*r_buffer;
	u16	n_size;
} amba_spi_write_and_read_t;

struct ambarella_spi_cs_config {
	u8					bus_id;
	u8					cs_id;
	u8					cs_num;
	int					*cs_pins;
};

struct ambarella_spi_platform_info {
	int					support_dma;
	int					fifo_entries;
	int					cs_num;
	int					*cs_pins;
	void    				(*cs_activate)  (struct ambarella_spi_cs_config *);
	void    				(*cs_deactivate)(struct ambarella_spi_cs_config *);
	void					(*rct_set_ssi_pll)(void);
	u32					(*get_ssi_freq_hz)(void);
};
#define AMBA_SPI_PARAM_CALL(id, arg, perm) \
	module_param_cb(spi##id##_cs0, &param_ops_int, &(arg[0]), perm); \
	module_param_cb(spi##id##_cs1, &param_ops_int, &(arg[1]), perm); \
	module_param_cb(spi##id##_cs2, &param_ops_int, &(arg[2]), perm); \
	module_param_cb(spi##id##_cs3, &param_ops_int, &(arg[3]), perm); \
	module_param_cb(spi##id##_cs4, &param_ops_int, &(arg[4]), perm); \
	module_param_cb(spi##id##_cs5, &param_ops_int, &(arg[5]), perm); \
	module_param_cb(spi##id##_cs6, &param_ops_int, &(arg[6]), perm); \
	module_param_cb(spi##id##_cs7, &param_ops_int, &(arg[7]), perm)

/* ==========================================================================*/
extern struct platform_device			ambarella_spi0;
extern struct platform_device			ambarella_spi1;
extern struct platform_device			ambarella_spi2;
extern struct platform_device			ambarella_spi3;
extern struct platform_device			ambarella_spi4;

/* ==========================================================================*/
extern int ambarella_spi_write(amba_spi_cfg_t *spi_cfg,
	amba_spi_write_t *spi_write);
extern int ambarella_spi_read(amba_spi_cfg_t *spi_cfg,
	amba_spi_read_t *spi_read);
extern int ambarella_spi_write_then_read(amba_spi_cfg_t *spi_cfg,
	amba_spi_write_then_read_t *spi_write_then_read);
extern int ambarella_spi_write_and_read(amba_spi_cfg_t *spi_cfg,
	amba_spi_write_and_read_t *spi_write_and_read);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

