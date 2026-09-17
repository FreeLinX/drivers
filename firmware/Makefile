# FreeLinX Firmware Makefile
DESTDIR ?= /home/devuan/FreeLinX/src/rootfs

all:
	@echo "Run 'make install' to stage essential firmware to $(DESTDIR)"

install:
	./flxfirmware.sh $(DESTDIR)

clean:
	@echo "Firmware is a data-only package, nothing to clean."

.PHONY: all install clean
