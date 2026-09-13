# FreeLinX Driver Manager Makefile
CC      := /home/devuan/FreeLinX/toolchain/bin/clang
SYSROOT := /home/devuan/FreeLinX/toolchain/x86_64-linux-musl
TARGET  := x86_64-linux-musl

CFLAGS  ?= -O2 -Wall -Wextra --target=$(TARGET) --sysroot=$(SYSROOT)
LDFLAGS ?= -fuse-ld=lld --rtlib=compiler-rt -static

BIN     := flx-driver
SRC     := flx-driver.c

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(BIN)

install: $(BIN)
	install -d $(DESTDIR)/sbin
	install -m 755 $(BIN) $(DESTDIR)/sbin/

.PHONY: all clean install
