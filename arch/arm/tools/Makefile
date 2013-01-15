#
# linux/arch/arm/tools/Makefile
#
# Copyright (C) 2001 Russell King
#

ifeq ($(CONFIG_PLAT_AMBARELLA_BOSS),y)
BOSSDIR ?= $(shell echo $${PWD%boss*}boss)

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

# Make sure IPCGEN is an ELF executable
ifneq ($(shell file $(IPCGEN) | grep -o $(IPCGEN_TYPE)), $(IPCGEN_TYPE))
ERR := $(shell rm -rf $(IPCGEN)* $(IPCDIR)/.tmp)
endif

IPCGEN_X = $(wildcard $(IPCDIR)/defs/*.x)
IPCGEN_H = $(foreach f,$(IPCGEN_X),include/generated/ipcgen/$(patsubst %.x,%.h,$(notdir $(f))))

.PHONY: gen-aipc-headers

gen-aipc-headers: $(IPCGEN_H)

include/generated/ipcgen/%.h: $(IPCDIR)/defs/%.x $(IPCGEN)
	@echo "  IPCGEN  $@" ; \
	mkdir -p $(dir $@) ; \
	$(IPCGEN) -C -M -H -h $(realpath $<) > $@

$(IPCGEN): 	$(wildcard $(IPCDIR)/sunrpc/*.*) \
		$(wildcard $(IPCDIR)/sunrpc/rpc/*.*) \
		$(wildcard $(IPCDIR)/sunrpc/rpvsvc/*.*)
	@cd $(IPCDIR) && $(MAKE) $(notdir $@)
endif
include/generated/mach-types.h: $(src)/gen-mach-types $(src)/mach-types
	@echo '  Generating $@'
	@mkdir -p $(dir $@)
	$(Q)$(AWK) -f $^ > $@ || { rm -f $@; /bin/false; }
