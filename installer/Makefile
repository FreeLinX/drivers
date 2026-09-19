# FreeLinX Installer Subsystem Makefile
# BSD 2-Clause License
# Copyright (c) 2026 FreeLinX Project

# Shared toolchain configuration (no hard-coded developer paths); all
# FREELINX_* variables are overridable from the environment.
include ../mk/common.mk

CC      = $(FREELINX_CC)
CFLAGS  ?= --target=$(FREELINX_TARGET) --sysroot=$(FREELINX_SYSROOT) -O2 -Wall -std=c99
LDFLAGS ?= --target=$(FREELINX_TARGET) --sysroot=$(FREELINX_SYSROOT) --rtlib=compiler-rt -fuse-ld=lld -static
DESTDIR ?= $(FREELINX_ROOTFS_DIR)

all: flxpart

flxpart: flxpart.c
	$(CC) $(CFLAGS) flxpart.c $(LDFLAGS) -o flxpart

clean:
	rm -f flxpart

.PHONY: all clean
