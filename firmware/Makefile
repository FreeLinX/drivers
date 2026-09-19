# FreeLinX Firmware Makefile
# Shared configuration (no hard-coded developer paths).
include ../mk/common.mk

DESTDIR ?= $(FREELINX_ROOTFS_DIR)

all:
	@echo "Run 'make install' to stage essential firmware to $(DESTDIR)"

install:
	./flxfirmware.sh $(DESTDIR)

clean:
	@echo "Firmware is a data-only package, nothing to clean."

.PHONY: all install clean
