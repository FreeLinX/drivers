# FreeLinX Driver Manager Makefile
# Shared toolchain configuration (no hard-coded developer paths); all
# FREELINX_* variables are overridable from the environment.
include ../mk/common.mk

CC      = $(FREELINX_CC)
SYSROOT ?= $(FREELINX_SYSROOT)
TARGET  := $(FREELINX_TARGET)
DESTDIR ?= $(FREELINX_ROOTFS_DIR)

CFLAGS  ?= -O2 -Wall -Wextra --target=$(TARGET) --sysroot=$(SYSROOT)
LDFLAGS ?= -fuse-ld=lld --rtlib=compiler-rt -static

BIN     := flxdriver
SRC     := flxdriver.c

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(BIN)

install: $(BIN)
	install -d $(DESTDIR)/sbin
	install -m 755 $(BIN) $(DESTDIR)/sbin/

.PHONY: all clean install
