#
# drivers/ambarella-ipc/Makefile
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

BOSSDIR ?= $(shell echo $${PWD%boss*}boss)

# IPC direcotry
IPCDIR := $(realpath $(BOSSDIR)/ipc)
ifeq ($(IPCDIR),)
IPCDIR := $(realpath $(BOSSDIR)/android/device/ambarella/ipc)
endif
ifeq ($(IPCDIR),)
$(error "Cannot find IPC directory!!")
endif

IPCDEFDIR := $(realpath $(BOSSDIR)/android/device/ambarella/ipc/defs)

# IPCGEN
IPCGEN := $(IPCDIR)/ipcgen
IPCGEN_TYPE := ELF

ifneq ($(shell file $(IPCGEN) | grep -o $(IPCGEN_TYPE)), $(IPCGEN_TYPE))
ERR := $(shell rm -rf $(IPCGEN)* $(IPCDIR)/.tmp)
endif

include drivers/ambarella-ipc/ipcdef.mk

SRCS-S-y = $(foreach f,$(X-S-y),server/$(patsubst %.x,%_server.c,$(notdir $(f))))
OBJS-S-y = $(foreach f,$(SRCS-S-y),$(patsubst %.c,%.o,$(f)))

SRCS-C-y = $(foreach f,$(X-C-y),client/$(patsubst %.x,%_client.c,$(notdir $(f))))
OBJS-C-y = $(foreach f,$(SRCS-C-y),$(patsubst %.c,%.o,$(f)))

#TAB_I-S-y = $(foreach f,$(X-S-y),ipcgen/$(patsubst %.x,%_tab.i,$(notdir $(f))))

#TAB_I-C-y = $(foreach f,$(X-C-y),ipcgen/$(patsubst %.x,%_tab.i,$(notdir $(f))))
#CLNT_C-C-y = $(foreach f,$(X-C-y),ipcgen/$(patsubst %.x,%_clnt.c,$(notdir $(f))))
#CLNT_O-C-y = $(foreach f,$(X-C-y),ipcgen/$(patsubst %.x,%_clnt.o,$(notdir $(f))))

IPCGEN_TAB_I = $(TAB_I-S-y) $(TAB_I-C-y)
IPCGEN_CLNT_C = $(CLNT_C-C-y)

IPCGEN_FILES = $(IPCGEN_TAB_I) $(IPCGEN_CLNT_C)

IMPLEMENT_SRCS := $(foreach f,$(SRCS-S-y) $(SRCS-C-y),$(src)/$(f))

IPC_LIB_ENTRY := $(src)/ipc_lib.c

# Finally, obj-y collects for linux objects to be made

LIB_NAME := lib.a
LIB_FILE := $(src)/lib.a

ifeq ($(CONFIG_AMBARELLA_IPC_HAS_SRC),y)

obj-y := ipc.o irq.o mem.o binder.o ipc_bh.o errstr.o nl.o ipc_shm.o ipc_slock.o ipc_mutex.o ipc_log.o
obj-y += server/lkvfs_bh.o
obj-y += $(OBJS-S-y) $(OBJS-C-y)
obj-y += $(CLNT_O-C-y)

LIB_OBJS := $(foreach f,$(obj-y),$(src)/$(f))
LIB_INITS := $(shell grep -o 'AMBARELLA_IPC_INIT(.*);' $(IMPLEMENT_SRCS) | sed 's/^.*AMBARELLA_IPC_INIT(\(.*\));.*$$/\1/')

$(LIB_FILE): $(LIB_OBJS)
	rm -f $@ && $(AR) rcs $@ $(LIB_OBJS)
	rm -f $(IPC_LIB_ENTRY)
	printf "#include <linux/init.h>\n#include <linux/module.h>\n\n" >> $(IPC_LIB_ENTRY)
	for f in $(LIB_INITS); do printf "extern int aipc_init_$$f(void);\n"; done >> $(IPC_LIB_ENTRY)
	printf "\nstatic int __init ipc_lib_init(void)\n{" >> $(IPC_LIB_ENTRY)
	for f in $(LIB_INITS); do printf "\taipc_init_$$f();\n"; done >> $(IPC_LIB_ENTRY)
	printf "\treturn 0;\n" >> $(IPC_LIB_ENTRY)
	printf "}\n\nmodule_init(ipc_lib_init);" >> $(IPC_LIB_ENTRY)

$(LIB_FILE).bak: $(LIB_FILE)
	cp $(LIB_FILE) $(LIB_FILE).bak

always := $(IPCGEN_FILES) $(LIB_NAME) $(LIB_NAME).bak
else

$(LIB_FILE):
	cp $(LIB_FILE).bak $(LIB_FILE)

obj-y := ipc_lib.o

always := $(IPCGEN_FILES) $(LIB_NAME)
endif

ccflags-y += -Idrivers/ambarella-ipc -Iinclude/generated -Iinclude/generated/ipcgen

clean-dirs += ipcgen/

# Dependencies

$(IMPLEMENT_SRCS): $(foreach f,$(IPCGEN_FILES),$(src)/$(f))

# Explicit rules

drivers/ambarella-ipc/ipcgen/%_tab.i: $(IPCDEFDIR)/%.x $(IPCGEN)
	@echo "  IPCGEN  $@" ; \
	mkdir -p $(dir $@) ; \
	$(IPCGEN) -C -M -H -t $(realpath $<) > $@

drivers/ambarella-ipc/ipcgen/%_clnt.c: $(IPCDEFDIR)/%.x $(IPCGEN)
	@echo "  IPCGEN  $@" ; \
	mkdir -p $(dir $@) ; \
	$(IPCGEN) -C -M -H -l $(realpath $<) > $@

$(IPCGEN): 	$(wildcard $(IPCDIR)/sunrpc/*.*) \
		$(wildcard $(IPCDIR)/sunrpc/rpc/*.*) \
		$(wildcard $(IPCDIR)/sunrpc/rpvsvc/*.*)
	@cd $(IPCDIR) && $(MAKE) $(notdir $@)
