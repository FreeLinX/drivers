# FreeLinX 3D Driver Test Tool Makefile
# Shared toolchain configuration (no hard-coded developer paths); all
# FREELINX_* variables are overridable from the environment.
include ../mk/common.mk

CC      = $(FREELINX_CC)
SYSROOT ?= $(FREELINX_SYSROOT)
TARGET  := $(FREELINX_TARGET)
DESTDIR ?= $(FREELINX_ROOTFS_DIR)

CFLAGS  ?= -O3 -Wall -Wextra --target=$(TARGET) --sysroot=$(SYSROOT) -I$(FREELINX_DRIVERS_ROOT)/include -include $(FREELINX_DRIVERS_ROOT)/include/linux/compiler.h
LDFLAGS ?= -fuse-ld=lld --rtlib=compiler-rt -static -lm

BIN     := flx3dtest
SRC     := flx3dtest.c

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(BIN)

install: $(BIN)
	install -d $(DESTDIR)/usr/bin
	install -m 755 $(BIN) $(DESTDIR)/usr/bin/

.PHONY: all clean install
