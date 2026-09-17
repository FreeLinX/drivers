# FreeLinX Drivers Repository Top-Level Makefile
# BSD 2-Clause License
# Copyright (c) 2026 FreeLinX Project

SUBDIRS := flxdriver audio/tinyalsa flx3dtest power installer modules
ROOTFS  ?= /home/devuan/FreeLinX/src/rootfs

all:
	@for dir in $(SUBDIRS); do \
		echo "=================================================="; \
		echo "Building $$dir..."; \
		echo "=================================================="; \
		$(MAKE) -C $$dir all || exit 1; \
	done

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

install: all
	./scripts/stage-rootfs.sh $(ROOTFS)

test: all
	./scripts/test-drivers.sh

.PHONY: all clean install test
