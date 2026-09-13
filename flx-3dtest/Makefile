# FreeLinX 3D Driver Test Tool Makefile
CC      := /home/devuan/FreeLinX/toolchain/bin/clang
SYSROOT := /home/devuan/FreeLinX/toolchain/x86_64-linux-musl
TARGET  := x86_64-linux-musl

CFLAGS  ?= -O3 -Wall -Wextra --target=$(TARGET) --sysroot=$(SYSROOT)
LDFLAGS ?= -fuse-ld=lld --rtlib=compiler-rt -static -lm

BIN     := flx-3dtest
SRC     := flx-3dtest.c

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(BIN)

install: $(BIN)
	install -d $(DESTDIR)/usr/bin
	install -m 755 $(BIN) $(DESTDIR)/usr/bin/

.PHONY: all clean install
