# FreeLinX Installer Subsystem Makefile
# BSD 2-Clause License
# Copyright (c) 2026 FreeLinX Project

CC      := /home/devuan/FreeLinX/toolchain/bin/clang
CFLAGS  := --target=x86_64-linux-musl --sysroot=/home/devuan/FreeLinX/toolchain/x86_64-linux-musl -O2 -Wall -std=c99
LDFLAGS := --target=x86_64-linux-musl --sysroot=/home/devuan/FreeLinX/toolchain/x86_64-linux-musl --rtlib=compiler-rt -fuse-ld=lld -static

all: flx-part

flx-part: flx-part.c
	$(CC) $(CFLAGS) flx-part.c $(LDFLAGS) -o flx-part

clean:
	rm -f flx-part

.PHONY: all clean
