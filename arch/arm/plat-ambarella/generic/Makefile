#
# arch/arm/plat-ambarella/generic/Makefile
#
# Author: Anthony Ginger <hfjiang@ambarella.com>
#
# Copyright (C) 2004-2011, Ambarella, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

obj-y						+= adc.o
obj-$(CONFIG_PLAT_AMBARELLA_SUPPORT_SATA)	+= ahci.o
obj-y						+= audio.o
obj-y						+= crypto.o
obj-y						+= dma.o
obj-y						+= eth.o
obj-y						+= fio.o
obj-$(CONFIG_PLAT_AMBARELLA_SUPPORT_GDMA)	+= gdma.o
obj-y						+= gpio.o
obj-y						+= idc.o
obj-y						+= init.o
obj-y						+= ir.o
obj-y						+= irq.o
ifeq ($(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL), y)
obj-y						+= pll_hal.o
else
obj-y						+= pll_rct.o
endif
obj-y						+= pm.o
obj-y						+= pwm.o
obj-y						+= rct.o
obj-$(CONFIG_PLAT_AMBARELLA_ADD_REGISTER_LOCK)	+= reglock.o
obj-y						+= rtc.o
obj-y						+= sd.o
obj-y						+= spi.o
obj-y						+= timer.o
obj-y						+= uart.o
obj-y						+= udc.o
obj-y						+= wdt.o
obj-$(CONFIG_PLAT_AMBARELLA_SUPPORT_USB)	+= uhc.o

obj-y						+= adb.o

