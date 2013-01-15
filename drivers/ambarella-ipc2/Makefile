#
# drivers/ambarella-ipc2/Makefile
#
# Ambarella IPC in Linux kernel-space.
#
# Authors:
#	Charles Chiou <cchiou@ambarella.com>
#
# Copyright (C) 2009-2010, Ambarella, Inc.
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

BOSSDIR := $(shell echo $${PWD%boss*}boss)

# IPC direcotry
IPCDIR := $(realpath $(BOSSDIR)/ipc)
ifeq ($(IPCDIR),)
IPCDIR := $(realpath $(BOSSDIR)/android/device/ambarella/ipc)
endif
ifeq ($(IPCDIR),)
$(error "Cannot find IPC directory!!")
endif

# IPCGEN
IPCGEN := $(IPCDIR)/ipcgen
IPCGEN_TYPE := ELF

ifneq ($(shell file $(IPCGEN) | grep -o $(IPCGEN_TYPE)), $(IPCGEN_TYPE))
ERR := $(shell rm -rf $(IPCGEN)* $(IPCDIR)/.tmp)
endif

# Servers
X-S-y +=	$(IPCDIR)/defs/lk_util.x \
		$(IPCDIR)/defs/lk_sdhc.x \
		$(IPCDIR)/defs/lk_gpio.x \
		$(IPCDIR)/defs/lk_pmic.x \
		$(IPCDIR)/defs/lk_vfs.x \
		$(IPCDIR)/defs/lk_fifo.x \
		$(IPCDIR)/defs/lk_host_dsp.x \
		$(IPCDIR)/defs/lk_sdresp.x \
		$(IPCDIR)/defs/lk_msg_reply.x

# Clients
X-C-y +=	$(IPCDIR)/defs/i_util.x \
		$(IPCDIR)/defs/i_status.x \
		$(IPCDIR)/defs/i_flpart.x \
		$(IPCDIR)/defs/i_nand.x \
		$(IPCDIR)/defs/i_sdhc.x \
		$(IPCDIR)/defs/i_gpio.x \
		$(IPCDIR)/defs/i_pmic.x \
		$(IPCDIR)/defs/i_ffs.x \
		$(IPCDIR)/defs/i_fifo.x \
		$(IPCDIR)/defs/i_sdresp.x \
		$(IPCDIR)/defs/i_host_dsp.x \
		$(IPCDIR)/defs/i_msg_request.x


ifeq ($(CONFIG_FB_DSP_CMD),y)
X-C-y +=	$(IPCDIR)/defs/i_dsp.x
else
X-C-y +=	$(IPCDIR)/defs/i_display.x
endif

#vserial for gps
X-S-y +=	$(IPCDIR)/defs/lk_vserial.x

# vtouch
X-S-y +=	$(IPCDIR)/defs/lk_touch.x
X-C-y +=	$(IPCDIR)/defs/i_touch.x

#vbutton
X-S-y +=	$(IPCDIR)/defs/lk_button.x
X-C-y +=	$(IPCDIR)/defs/i_button.x

#Streaming .x file
X-C-y +=	$(IPCDIR)/defs/i_streamer.x

# Virtual ALSA
X-S-y +=	$(IPCDIR)/defs/lk_alsa_fifo.x
X-C-y +=	$(IPCDIR)/defs/i_alsa_op.x

# Collect intermediate files

SRCS-S-y = $(foreach f,$(X-S-y),server/$(patsubst %.x,%_server.c,$(notdir $(f))))
OBJS-S-y = $(foreach f,$(SRCS-S-y),$(patsubst %.c,%.o,$(f)))

SRCS-C-y = $(foreach f,$(X-C-y),client/$(patsubst %.x,%_client.c,$(notdir $(f))))
OBJS-C-y = $(foreach f,$(SRCS-C-y),$(patsubst %.c,%.o,$(f)))

TAB_I-S-y = $(foreach f,$(X-S-y),ipcgen/$(patsubst %.x,%_tab.i,$(notdir $(f))))

TAB_I-C-y = $(foreach f,$(X-C-y),ipcgen/$(patsubst %.x,%_tab.i,$(notdir $(f))))
CLNT_C-C-y = $(foreach f,$(X-C-y),ipcgen/$(patsubst %.x,%_clnt.c,$(notdir $(f))))
CLNT_O-C-y = $(foreach f,$(X-C-y),ipcgen/$(patsubst %.x,%_clnt.o,$(notdir $(f))))

IPCGEN_TAB_I = $(TAB_I-S-y) $(TAB_I-C-y)
IPCGEN_CLNT_C = $(CLNT_C-C-y)

IPCGEN_FILES = $(IPCGEN_TAB_I) $(IPCGEN_CLNT_C)

# Finally, obj-y collects for linux objects to be made

obj-y := ipc.o irq.o mem.o binder.o ipc_bh.o errstr.o nl.o ipc_shm.o ipc_slock.o ipc_mutex.o ipc_log.o server/lkvfs_bh.o
obj-y += $(OBJS-S-y) $(OBJS-C-y)
obj-y += $(CLNT_O-C-y)

ccflags-y += -Idrivers/ambarella-ipc2 -Iinclude/generated -Iinclude/generated/ipcgen

clean-dirs += ipcgen/

# Dependencies

always := $(IPCGEN_FILES)
IMPLEMENT_SRCS := $(foreach f,$(SRCS-S-y) $(SRCS-C-y),$(src)/$(f))
$(IMPLEMENT_SRCS): $(foreach f,$(IPCGEN_FILES),$(src)/$(f))

# Explicit rules

drivers/ambarella-ipc2/ipcgen/%_tab.i: $(IPCDIR)/defs/%.x $(IPCGEN)
	@echo "  IPCGEN  $@" ; \
	mkdir -p $(dir $@) ; \
	(cd $(realpath $(dir $<)) ; $(IPCGEN) -C -M -t $(notdir $<)) > $@

drivers/ambarella-ipc2/ipcgen/%_clnt.c: $(IPCDIR)/defs/%.x $(IPCGEN)
	@echo "  IPCGEN  $@" ; \
	mkdir -p $(dir $@) ; \
	(cd $(realpath $(dir $<)) ; $(IPCGEN) -C -M -l $(notdir $<)) > $@

$(IPCGEN): 	$(wildcard $(IPCDIR)/sunrpc/*.*) \
		$(wildcard $(IPCDIR)/sunrpc/rpc/*.*) \
		$(wildcard $(IPCDIR)/sunrpc/rpvsvc/*.*)
	@cd $(IPCDIR) && $(MAKE) $(notdir $@)
