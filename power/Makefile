# FreeLinX Power Management Makefile
# BSD 2-Clause License
# Copyright (c) 2026 FreeLinX Project

CC      := /home/devuan/FreeLinX/toolchain/bin/clang
CFLAGS  := --target=x86_64-linux-musl --sysroot=/home/devuan/FreeLinX/toolchain/x86_64-linux-musl -O2 -Wall -std=c99
LDFLAGS := --target=x86_64-linux-musl --sysroot=/home/devuan/FreeLinX/toolchain/x86_64-linux-musl --rtlib=compiler-rt -fuse-ld=lld -static

all: flx-power

flx-power: flx-power.c
	$(CC) $(CFLAGS) flx-power.c $(LDFLAGS) -o flx-power

clean:
	rm -f flx-power

.PHONY: all clean
