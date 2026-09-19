#!/bin/sh
# FreeLinX Driver Subsystem Staging Script
#
# BSD 2-Clause License
# Copyright (c) 2026 FreeLinX Project
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
# No hard-coded developer paths: the default rootfs is the src sibling repo's
# rootfs tree, overridable with FREELINX_ROOTFS_DIR (or an argument).
FREELINX_WORKSPACE="${FREELINX_WORKSPACE:-$(cd "$SCRIPT_DIR/.." && pwd)}"
TARGET_ROOTFS="${1:-${FREELINX_ROOTFS_DIR:-$FREELINX_WORKSPACE/src/rootfs}}"

echo "=================================================="
echo "FreeLinX Full Hardware & Driver Subsystem Staging"
echo "Target RootFS: $TARGET_ROOTFS"
echo "=================================================="

# Directories
mkdir -p "$TARGET_ROOTFS/bin" \
         "$TARGET_ROOTFS/sbin" \
         "$TARGET_ROOTFS/usr/bin" \
         "$TARGET_ROOTFS/usr/sbin" \
         "$TARGET_ROOTFS/usr/share/limine" \
         "$TARGET_ROOTFS/etc" \
         "$TARGET_ROOTFS/media"

# 1. Install flxdriver
echo "[1/10] Installing flxdriver..."
make -C "$SCRIPT_DIR/flxdriver" install DESTDIR="$TARGET_ROOTFS"

# 2. Install TinyALSA audio subsystem
echo "[2/10] Installing TinyALSA audio subsystem..."
mkdir -p "$TARGET_ROOTFS/usr/include/tinyalsa"
make -C "$SCRIPT_DIR/audio/tinyalsa" install DESTDIR="$TARGET_ROOTFS"

# 3. Install flx3dtest 3D GPU test tool
echo "[3/10] Installing flx3dtest 3D GPU test tool..."
make -C "$SCRIPT_DIR/flx3dtest" install DESTDIR="$TARGET_ROOTFS"

# 4. Install essential firmware
echo "[4/10] Installing essential firmware blobs..."
"$SCRIPT_DIR/firmware/flxfirmware.sh" "$TARGET_ROOTFS"

# 5. Install out-of-tree kernel modules
echo "[5/10] Installing out-of-tree kernel modules..."
mkdir -p "$TARGET_ROOTFS/lib/modules/6.6.21/kernel/drivers/misc"
make -C "$SCRIPT_DIR/modules" install DESTDIR="$TARGET_ROOTFS"

# Refresh modules.dep
DEP_FILE="$TARGET_ROOTFS/lib/modules/6.6.21/modules.dep"
if [ -f "$DEP_FILE" ]; then
    if ! grep -q "flx_dummy.ko" "$DEP_FILE"; then
        echo "kernel/drivers/misc/flx_dummy.ko:" >> "$DEP_FILE"
    fi
fi

# 6. Power Management (flxpower, zzz, ZZZ)
echo "[6/10] Installing Power Management utilities..."
install -m 755 "$SCRIPT_DIR/power/flxpower" "$TARGET_ROOTFS/usr/bin/flxpower"
install -m 755 "$SCRIPT_DIR/power/zzz" "$TARGET_ROOTFS/usr/bin/zzz"
install -m 755 "$SCRIPT_DIR/power/ZZZ" "$TARGET_ROOTFS/usr/bin/ZZZ"

# 7. Bare-Metal Installer & Partitioning (flxpart, flxinstall)
echo "[7/10] Installing Bare-Metal Installer (flxpart, flxinstall)..."
install -m 755 "$SCRIPT_DIR/installer/flxpart" "$TARGET_ROOTFS/sbin/flxpart"
install -m 755 "$SCRIPT_DIR/installer/flxinstall" "$TARGET_ROOTFS/sbin/flxinstall"

# 8. Limine Bootloader
echo "[8/10] Installing Limine Bootloader files..."
if [ -f "$SCRIPT_DIR/bootloader/limine-binary/BOOTX64.EFI" ]; then
    install -m 644 "$SCRIPT_DIR/bootloader/limine-binary/BOOTX64.EFI" "$TARGET_ROOTFS/usr/share/limine/BOOTX64.EFI"
fi
if [ -f "$SCRIPT_DIR/bootloader/limine-binary/limine-bios.sys" ]; then
    install -m 644 "$SCRIPT_DIR/bootloader/limine-binary/limine-bios.sys" "$TARGET_ROOTFS/usr/share/limine/limine-bios.sys"
fi
if [ -x "$SCRIPT_DIR/bootloader/limine-binary/limine" ]; then
    install -m 755 "$SCRIPT_DIR/bootloader/limine-binary/limine" "$TARGET_ROOTFS/usr/bin/limine"
fi

# 9. Storage & Filesystem Tools (e2fsprogs, dosfstools)
echo "[9/10] Staging filesystem tools (ext4, vfat)..."
E2FS_DIR="${E2FS_DIR:-$FREELINX_WORKSPACE/ports/build/work/e2fsprogs/e2fsprogs-1.47.3}"
if [ -x "$E2FS_DIR/misc/mke2fs" ]; then
    install -m 755 "$E2FS_DIR/misc/mke2fs" "$TARGET_ROOTFS/sbin/mke2fs"
    ln -sf mke2fs "$TARGET_ROOTFS/sbin/mkfs.ext4"
    ln -sf mke2fs "$TARGET_ROOTFS/sbin/mkfs.ext3"
    ln -sf mke2fs "$TARGET_ROOTFS/sbin/mkfs.ext2"
fi
if [ -x "$E2FS_DIR/e2fsck/e2fsck" ]; then
    install -m 755 "$E2FS_DIR/e2fsck/e2fsck" "$TARGET_ROOTFS/sbin/e2fsck"
    ln -sf e2fsck "$TARGET_ROOTFS/sbin/fsck.ext4"
fi
if [ -x "$E2FS_DIR/misc/tune2fs" ]; then
    install -m 755 "$E2FS_DIR/misc/tune2fs" "$TARGET_ROOTFS/sbin/tune2fs"
fi
if [ -x "$E2FS_DIR/resize/resize2fs" ]; then
    install -m 755 "$E2FS_DIR/resize/resize2fs" "$TARGET_ROOTFS/sbin/resize2fs"
fi

FAT_DIR="${FAT_DIR:-$FREELINX_WORKSPACE/ports/build/work/dosfstools/dosfstools-4.2/src}"
if [ -x "$FAT_DIR/mkfs.fat" ]; then
    install -m 755 "$FAT_DIR/mkfs.fat" "$TARGET_ROOTFS/sbin/mkfs.fat"
    ln -sf mkfs.fat "$TARGET_ROOTFS/sbin/mkfs.vfat"
    ln -sf mkfs.fat "$TARGET_ROOTFS/sbin/mkfs.msdos"
fi
if [ -x "$FAT_DIR/fsck.fat" ]; then
    install -m 755 "$FAT_DIR/fsck.fat" "$TARGET_ROOTFS/sbin/fsck.fat"
    ln -sf fsck.fat "$TARGET_ROOTFS/sbin/dosfsck"
fi
if [ -x "$FAT_DIR/fatlabel" ]; then
    install -m 755 "$FAT_DIR/fatlabel" "$TARGET_ROOTFS/sbin/fatlabel"
fi

# 10. Hotplug (mdevd), Automount, User Privileges (doas)
echo "[10/10] Configuring Hotplug (mdevd), Automount, and Security (doas)..."
MDEVD_DIR="${MDEVD_DIR:-$FREELINX_WORKSPACE/ports/build/work/mdevd/mdevd-0.1.8.1}"
if [ -x "$MDEVD_DIR/mdevd" ]; then
    install -m 755 "$MDEVD_DIR/mdevd" "$TARGET_ROOTFS/sbin/mdevd"
fi
if [ -x "$MDEVD_DIR/mdevd-coldplug" ]; then
    install -m 755 "$MDEVD_DIR/mdevd-coldplug" "$TARGET_ROOTFS/sbin/mdevd-coldplug"
fi

install -m 755 "$SCRIPT_DIR/hotplug/flxautomount" "$TARGET_ROOTFS/sbin/flxautomount"
install -m 755 "$SCRIPT_DIR/hotplug/flxdriverhotplug" "$TARGET_ROOTFS/sbin/flxdriverhotplug"
install -m 644 "$SCRIPT_DIR/hotplug/mdev.conf" "$TARGET_ROOTFS/etc/mdev.conf"

install -m 600 "$SCRIPT_DIR/user/doas.conf" "$TARGET_ROOTFS/etc/doas.conf"
install -m 755 "$SCRIPT_DIR/user/flxadduser" "$TARGET_ROOTFS/sbin/flxadduser"

if [ -f "$TARGET_ROOTFS/usr/bin/doas" ]; then
    chmod 4755 "$TARGET_ROOTFS/usr/bin/doas"
fi

echo "=================================================="
echo "All driver, power, installer & system tools staged!"
echo "=================================================="
