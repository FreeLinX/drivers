# FreeLinX Drivers Repository Top-Level Makefile
# BSD 2-Clause License
# Copyright (c) 2026 FreeLinX Project

# Shared toolchain/rootfs configuration (no hard-coded developer paths).
include mk/common.mk

SUBDIRS := flxdriver audio/tinyalsa flx3dtest power installer modules
ROOTFS  ?= $(FREELINX_ROOTFS_DIR)

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
