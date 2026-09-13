# FreeLinX Kernel Module Makefile (flx-dummy)
obj-m := flx_dummy.o

KDIR    ?= /home/devuan/FreeLinX/kernel/linux-6.6.21
CLANG   ?= /home/devuan/FreeLinX/toolchain/bin/clang
DESTDIR ?= /home/devuan/FreeLinX/src/rootfs

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) LLVM=1 LLVM_IAS=1 ARCH=x86_64 CC=$(CLANG) KBUILD_MODPOST_WARN=1 modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

install:
	install -d $(DESTDIR)/lib/modules/6.6.21/kernel/drivers/misc
	install -m 644 flx_dummy.ko $(DESTDIR)/lib/modules/6.6.21/kernel/drivers/misc/

.PHONY: all clean install
