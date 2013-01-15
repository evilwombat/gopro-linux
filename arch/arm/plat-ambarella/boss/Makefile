#
# arch/arm/plat-ambarella/boss/Makefile
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

obj-y						+= ../generic/adc.o
obj-$(CONFIG_PLAT_AMBARELLA_SUPPORT_SATA)	+= ../generic/ahci.o
obj-y						+= ../generic/audio.o
obj-y						+= ../generic/crypto.o
obj-y						+= ../generic/dma.o
obj-y						+= ../generic/eth.o
obj-y						+= ../generic/fio.o
obj-$(CONFIG_PLAT_AMBARELLA_SUPPORT_GDMA)	+= ../generic/gdma.o
obj-y						+= ../generic/gpio.o
obj-y						+= ../generic/idc.o
obj-y						+= init.o
obj-y						+= ../generic/ir.o
obj-y						+= irq.o
ifeq ($(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL), y)
obj-y						+= ../generic/pll_hal.o
else
obj-y						+= ../generic/pll_rct.o
endif
obj-y						+= pm.o
obj-y						+= ../generic/pwm.o
obj-y						+= ../generic/rct.o
obj-$(CONFIG_PLAT_AMBARELLA_ADD_REGISTER_LOCK)	+= ../generic/reglock.o
ifeq ($(CONFIG_PLAT_AMBARELLA_CORTEX), y)
obj-$(CONFIG_RTC_DRV_AMBARELLA)			+= ../generic/vrtc.o
else
obj-$(CONFIG_RTC_DRV_AMBARELLA)			+= ../generic/rtc.o
endif
obj-y						+= ../generic/sd.o
obj-y						+= ../generic/spi.o
obj-y						+= timer.o
obj-y						+= ../generic/uart.o
obj-y						+= ../generic/udc.o
obj-y						+= ../generic/wdt.o
obj-$(CONFIG_PLAT_AMBARELLA_SUPPORT_USB)	+= ../generic/uhc.o

obj-y						+= ../generic/adb.o

obj-y						+= boss.o
