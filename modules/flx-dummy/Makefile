# FreeLinX Kernel Module Makefile (flx-dummy)
# Shared configuration (no hard-coded developer paths); all FREELINX_*
# variables are overridable from the environment.
#
# kbuild re-parses this Makefile with CWD == the kernel source dir, so the
# common.mk include must resolve from this file's own location, not from
# the current working directory.
include $(abspath $(dir $(lastword $(MAKEFILE_LIST)))../../mk/common.mk)

obj-m := flx_dummy.o

KDIR    ?= $(FREELINX_KDIR)
CLANG   ?= $(FREELINX_CC)
DESTDIR ?= $(FREELINX_ROOTFS_DIR)

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) LLVM=1 LLVM_IAS=1 ARCH=x86_64 CC=$(CLANG) KBUILD_MODPOST_WARN=1 modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

install:
	@if [ ! -f flx_dummy.ko ]; then \
		echo "SKIP: flx_dummy.ko not built (needs the linux-6.6.21 kernel source; run 'make' with KDIR)"; \
	else \
		install -d $(DESTDIR)/lib/modules/6.6.21/kernel/drivers/misc && \
		install -m 644 flx_dummy.ko $(DESTDIR)/lib/modules/6.6.21/kernel/drivers/misc/; \
	fi

.PHONY: all clean install
