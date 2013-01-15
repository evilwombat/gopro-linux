#
# arch/arm/mach-ambarella/Makefile
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

obj-$(CONFIG_MACH_COCONUT)	+= init-coconut.o
obj-$(CONFIG_MACH_DURIAN)	+= init-durian.o
obj-$(CONFIG_MACH_ELEPHANT)	+= init-elephant.o

ifeq ($(CONFIG_PLAT_AMBARELLA_BOSS),y)
ifeq ($(CONFIG_SUPPORT_ANDROID_TT),y)
obj-$(CONFIG_SUPPORT_ANDROID_TT)       += init-boss_tt.o
else
obj-$(CONFIG_PLAT_AMBARELLA_I1_BOSS)	+= init-boss.o
obj-$(CONFIG_PLAT_AMBARELLA_A5S_BOSS)	+= init-boss_a5s.o
obj-$(CONFIG_PLAT_AMBARELLA_A7_BOSS)	+= init-boss_a7.o
obj-$(CONFIG_PLAT_AMBARELLA_A7L_BOSS)	+= init-boss_a7l.o
endif
else
ifeq ($(CONFIG_PLAT_AMBARELLA_A7L),y)
obj-$(CONFIG_PLAT_AMBARELLA_A7L)	+= init-a7l.o
else
obj-$(CONFIG_MACH_AMBARELLA)	+= init-generic.o
endif
endif

obj-$(CONFIG_MACH_FILBERT)	+= init-filbert.o

obj-$(CONFIG_AMBARELLA_VIN)			+= vin.o
obj-$(CONFIG_AMBARELLA_VOUT)			+= vout.o


obj-$(CONFIG_TOUCH_AMBARELLA_AK4183)		+= touch-ak4183.o
obj-$(CONFIG_TOUCH_AMBARELLA_CHACHA_MT4D)	+= touch-chacha_mt4d.o
obj-$(CONFIG_TOUCH_AMBARELLA_TM1510)		+= touch-tm1510.o
obj-$(CONFIG_TOUCH_AMBARELLA_TM1726)		+= touch-tm1726.o
obj-$(CONFIG_TOUCH_AMBARELLA_TM1927)		+= touch-tm1927.o
obj-$(CONFIG_TOUCH_AMBARELLA_NT11001)		+= touch-nt11001.o
obj-$(CONFIG_TOUCH_AMBARELLA_FT540)		+= touch-ft540.o

obj-$(CONFIG_LCD_AMBARELLA_1P3831)		+= lcd-1p3831.o
obj-$(CONFIG_TIWLAN_SDIO)               += board-1283sdp-wifi.o
obj-$(CONFIG_RTC_AMBARELLA_IS112022M)		+= rtc-is112022m.o

obj-$(CONFIG_CODEC_AMBARELLA_WM8994)		+= codec-wm8994.o
obj-$(CONFIG_CODEC_AMBARELLA_WM8737)		+= codec-wm8737.o

obj-$(CONFIG_PMIC_AMBARELLA_WM831X)		+= wm831x-config.o
