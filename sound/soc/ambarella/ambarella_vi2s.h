/*
 * sound/soc/ambarella_vi2s.h
 *
 * History:
 *	2011/03/28 - [Eric Lee] Port from ambarella_i2s.h
 *	2011/06/23 - [Eric Lee] Port to 2.6.38
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

#ifndef AMBARELLA_VI2S_H_
#define AMBARELLA_VI2S_H_

struct amb_i2s_priv {
	u32 clock_reg;
	struct ambarella_i2s_controller *controller_info;
	struct ambarella_i2s_interface amb_i2s_intf;
};

#endif /*AMBARELLA_VI2S_H_*/

