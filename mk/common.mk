# -*- makefile -*-
# FreeLinX/drivers - mk/common.mk : shared toolchain / rootfs configuration.
#
# BSD 2-Clause License
# Copyright (c) 2026 FreeLinX Project
#
# Every FreeLinX repository (src, toolchain, kernel, drivers, ports, xpkg)
# lives as a sibling under one workspace root. No developer machine paths are
# hard-coded anywhere in this tree: every default below resolves relative to
# this repository's location, and every FREELINX_* variable can be overridden
# by the environment (same convention as FreeLinX/ports mk/common.mk).
#
# A component Makefile pulls this in with `include ../../mk/common.mk` (path
# relative to the component); the repo root is derived from this file's own
# location so it works from any depth.

FREELINX_DRIVERS_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))..)
FREELINX_ROOT         ?= $(abspath $(FREELINX_DRIVERS_ROOT)/..)

# Toolchain (FreeLinX/toolchain): clang + LLD + llvm-ar + musl sysroot.
FREELINX_TOOLCHAIN_DIR ?= $(abspath $(FREELINX_ROOT)/toolchain)
FREELINX_TOOLCHAIN_BIN ?= $(FREELINX_TOOLCHAIN_DIR)/bin
FREELINX_CC            ?= $(FREELINX_TOOLCHAIN_BIN)/clang
FREELINX_AR            ?= $(FREELINX_TOOLCHAIN_BIN)/llvm-ar
FREELINX_SYSROOT       ?= $(FREELINX_TOOLCHAIN_DIR)/x86_64-linux-musl
FREELINX_TARGET        ?= x86_64-linux-musl

# Sibling repo locations.
FREELINX_SRC_DIR       ?= $(abspath $(FREELINX_ROOT)/src)
FREELINX_ROOTFS_DIR    ?= $(FREELINX_SRC_DIR)/rootfs
FREELINX_KERNEL_DIR    ?= $(abspath $(FREELINX_ROOT)/kernel)
FREELINX_PORTS_DIR     ?= $(abspath $(FREELINX_ROOT)/ports)
FREELINX_PORTS_BUILD   ?= $(FREELINX_PORTS_DIR)/build/work

# Kernel source tree used for out-of-tree module builds (KDIR).  The full
# linux-6.6.21 source tree is a build-time input, not tracked by the repo.
FREELINX_KDIR          ?= $(FREELINX_KERNEL_DIR)/linux-6.6.21