#!/bin/sh
# FreeLinX Firmware Manager (flxfirmware)
#
# BSD 2-Clause License
# Copyright (c) 2026 FreeLinX Project
#
# Manages, stages, and verifies hardware firmware blobs for FreeLinX.
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MANIFEST="$SCRIPT_DIR/manifests/essential-wifi.txt"
# No hard-coded developer paths: the default rootfs is the src sibling repo's
# rootfs tree, overridable with FREELINX_ROOTFS_DIR (or an argument).
FREELINX_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
FREELINX_WORKSPACE="${FREELINX_WORKSPACE:-$(cd "$FREELINX_ROOT/.." && pwd)}"
DESTDIR="${1:-${FREELINX_ROOTFS_DIR:-$FREELINX_WORKSPACE/src/rootfs}}"
FW_DIR="$DESTDIR/lib/firmware"

echo "=== FreeLinX Firmware Manager ==="
echo "Target Rootfs: $DESTDIR"
echo "Firmware Directory: $FW_DIR"

mkdir -p "$FW_DIR"

install_from_host() {
    echo "Installing essential Wi-Fi firmware..."

    # 1. Realtek rtw88
    if [ -d "/lib/firmware/rtw88" ]; then
        echo "  - Staging Realtek rtw88 firmware..."
        mkdir -p "$FW_DIR/rtw88"
        cp -a /lib/firmware/rtw88/*.bin "$FW_DIR/rtw88/" 2>/dev/null || true
    fi

    # 2. Realtek rtw89
    if [ -d "/lib/firmware/rtw89" ]; then
        echo "  - Staging Realtek rtw89 firmware..."
        mkdir -p "$FW_DIR/rtw89"
        cp -a /lib/firmware/rtw89/*.bin "$FW_DIR/rtw89/" 2>/dev/null || true
    fi

    # 3. Intel iwlwifi
    echo "  - Staging Intel iwlwifi firmware..."
    cp -a /lib/firmware/iwlwifi-*.ucode "$FW_DIR/" 2>/dev/null || true
    cp -a /lib/firmware/iwlwifi-*.pnvm "$FW_DIR/" 2>/dev/null || true

    # 4. Atheros ath10k & ath9k
    if [ -d "/lib/firmware/ath10k" ]; then
        echo "  - Staging Atheros ath10k firmware..."
        mkdir -p "$FW_DIR/ath10k"
        cp -a /lib/firmware/ath10k/* "$FW_DIR/ath10k/" 2>/dev/null || true
    fi
    if [ -d "/lib/firmware/ath9k_htc" ]; then
        echo "  - Staging Atheros ath9k_htc firmware..."
        mkdir -p "$FW_DIR/ath9k_htc"
        cp -a /lib/firmware/ath9k_htc/* "$FW_DIR/ath9k_htc/" 2>/dev/null || true
    fi

    # 5. Broadcom brcm
    if [ -d "/lib/firmware/brcm" ]; then
        echo "  - Staging Broadcom brcm firmware..."
        mkdir -p "$FW_DIR/brcm"
        cp -a /lib/firmware/brcm/* "$FW_DIR/brcm/" 2>/dev/null || true
    fi

    echo "Firmware installation complete."
}

verify_firmware() {
    echo "Verifying firmware files in $FW_DIR:"
    COUNT=$(find "$FW_DIR" -type f 2>/dev/null | wc -l)
    SIZE=$(du -sh "$FW_DIR" 2>/dev/null | cut -f1)
    echo "  Total firmware blobs installed: $COUNT"
    echo "  Total storage footprint: $SIZE"
}

install_from_host
verify_firmware
