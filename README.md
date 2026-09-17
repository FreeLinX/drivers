# FreeLinX Hardware, Drivers & Bare-Metal Subsystem (no-GNU)

The **FreeLinX Driver Subsystem** provides hardware detection, kernel module management, userspace audio abstraction, power management, hotplug auto-mounting, and bare-metal disk installation designed specifically for FreeLinX's non-GNU, musl-based, BSD-licensed architecture.

---

## Architecture Overview

In FreeLinX, device support is strictly non-GNU and divided into two layers:

```
+-------------------------------------------------------------------------------+
|                      USERSPACE SUBSYSTEM (no-GNU / BSD)                       |
|                                                                               |
|  * flxdriver (Hardware scanning & auto-modprobe engine)                      |
|  * flxpower & zzz (Battery, backlight, CPU governors, suspend-to-RAM)        |
|  * mdevd & flxautomount (Netlink device hotplug & USB storage auto-mount)    |
|  * flxpart & flxinstall (Bare-metal GPT partitioner & Limine installer)     |
|  * doas & flxadduser (OpenBSD privilege elevation & desktop user setup)     |
|  * TinyALSA (Non-GNU audio driver: tinymix, tinyplay, tinycap, tinypcminfo)   |
|  * flx3dtest (Terminal DRM / KMS 3D renderer & GPU validator)                |
|  * /lib/firmware (Hardware blobs for Intel, Realtek, Atheros, Broadcom)       |
+-------------------------------------------------------------------------------+
                                        ▲
                                        │ (System Calls / Devfs / Netlink)
                                        ▼
+-------------------------------------------------------------------------------+
|                        KERNEL SPACE (Linux 6.6 / Clang)                       |
|                                                                               |
|  * DRM/KMS (Direct Rendering Manager for graphics, virtio-gpu, i915)          |
|  * ALSA PCM / Control (Audio endpoints /dev/snd/*)                            |
|  * mac80211 / cfg80211 (Wi-Fi networking stack)                              |
|  * .ko Kernel Modules (iwlwifi, rtw88, rtw89, ath9k, flx_dummy, etc.)        |
+-------------------------------------------------------------------------------+
```

---

## Subsystem Components

| Directory | Component | License | Description |
| :--- | :--- | :--- | :--- |
| [`flxdriver/`](file:///home/devuan/FreeLinX/drivers/flxdriver) | **Driver Manager** | BSD 2-Clause | C99 hardware detection & auto-probing engine. Scans PCI/USB, maps device IDs, resolves missing modules, and loads drivers via `/sbin/modprobe`. |
| [`flx3dtest/`](file:///home/devuan/FreeLinX/drivers/flx3dtest) | **3D GPU Tester** | BSD 2-Clause | Direct DRM/KMS hardware diagnostic validator, 3D vertex transform pipeline, and realtime ANSI/Z-buffer renderer and benchmark. |
| [`power/`](file:///home/devuan/FreeLinX/drivers/power) | **Power Management** | BSD 2-Clause | Battery capacity/wattage monitoring, screen backlight adjustment (`flxpower brightness`), CPU governors (`flxpower governor`), and BSD sleep (`zzz`). |
| [`installer/`](file:///home/devuan/FreeLinX/drivers/installer) | **Bare-Metal Installer** | BSD 2-Clause | C99 GPT partitioning engine (`flxpart`) and automated installer (`flxinstall`) with Limine UEFI/BIOS bootloader and ext4/FAT32 setup. |
| [`bootloader/`](file:///home/devuan/FreeLinX/drivers/bootloader) | **Limine Bootloader** | BSD 2-Clause / CC0 | Modern, lightweight bootloader supporting x86_64 UEFI (`BOOTX64.EFI`) and BIOS (`limine-bios.sys`). |
| [`hotplug/`](file:///home/devuan/FreeLinX/drivers/hotplug) | **Device Hotplug** | ISC / BSD | `mdevd` netlink daemon, `/etc/mdev.conf` device permissions, `/sbin/flxautomount` USB storage mounter, and driver auto-prober. |
| [`user/`](file:///home/devuan/FreeLinX/drivers/user) | **Security & Users** | ISC / BSD | OpenBSD `doas` integration, `/etc/doas.conf` (wheel group rules), and `flxadduser` desktop provisioning script. |
| [`audio/tinyalsa/`](file:///home/devuan/FreeLinX/drivers/audio/tinyalsa) | **Audio Driver** | BSD 3-Clause | Android/BSD minimal ALSA library & utilities. Replaces GNU/LGPL `alsa-lib` with static `tinymix`, `tinyplay`, `tinycap`, `tinypcminfo`. |
| [`firmware/`](file:///home/devuan/FreeLinX/drivers/firmware) | **Firmware Manager** | Permissive/Redist | Stages essential vendor firmware blobs for Wi-Fi (Intel `iwlwifi`, Realtek `rtw88`/`rtw89`, Atheros `ath9k`/`ath10k`, Broadcom `brcm`) into `/lib/firmware`. |
| [`modules/`](file:///home/devuan/FreeLinX/drivers/modules) | **Kernel Modules** | Dual BSD/GPL | Build framework for compiling out-of-tree kernel modules against FreeLinX kernel with Clang/LLVM (`LLVM=1 LLVM_IAS=1`). Includes reference driver `flx_dummy`. |
| [`scripts/`](file:///home/devuan/FreeLinX/drivers/scripts) | **Build & Test** | BSD 2-Clause | Rootfs staging script (`stage-rootfs.sh`) and verification test suite (`test-drivers.sh`). |

---

## 1. Bare-Metal OS Installation (`flxinstall`)

To install FreeLinX onto any physical hard drive or SSD:

```sh
# Run interactive installer (scans drives, confirms with YES, partitions, and installs):
flxinstall

# Or install directly to a target device (e.g. /dev/sda or /dev/nvme0n1):
flxinstall /dev/sda
```

### Installation Steps Executed by `flxinstall`:
1. **Partitioning**: Initializes a clean UEFI-compliant GPT table with `flxpart`:
   - Partition 1: EFI System Partition (512MB, Type `C12A7328-F81F-11D2-BA4B-00A0C93EC93B`).
   - Partition 2: FreeLinX Root Filesystem (remainder of drive, Type `0FC63DAF-8483-4772-8E79-3D69D8477DE4`).
2. **Formatting**: Formats ESP as FAT32 (`mkfs.fat -F 32`) and Root as Ext4 (`mke2fs -t ext4`).
3. **Deployment**: Deploys the complete operating system tree.
4. **Bootloader**: Deploys Limine UEFI (`BOOTX64.EFI`) and BIOS (`limine-bios.sys`), writing `/boot/efi/limine.conf`.
5. **Configuration**: Generates `/etc/fstab` with proper mount options and filesystem labels.

---

## 2. Power & Hardware Management (`flxpower`)

```sh
# Show overall system hardware and battery health
flxpower status

# Adjust backlight brightness (supports percentages, relative delta, or absolute)
flxpower brightness 80%
flxpower brightness +10
flxpower brightness -10

# Set CPU scaling governor across all cores
flxpower governor performance
flxpower governor powersave

# Suspend to RAM (Sleep)
zzz
# or: flxpower suspend

# Hibernate to disk
ZZZ
```

---

## 3. Hotplug Device Management & Auto-Mount

- **`mdevd`**: Fast, Netlink-based device event manager (ISC license, zero GNU dependencies).
- **USB Auto-Mount (`/sbin/flxautomount`)**: Automatically detects filesystem types upon drive insertion (`ext4`, `vfat`, `ntfs`, `iso9660`) and mounts cleanly under `/media/<devname>`.
- **Driver Auto-Probe (`/sbin/flxdriverhotplug`)**: Triggers driver detection whenever new PCI/USB devices are attached.

---

## 4. Privilege Elevation & Non-Root Setup (`doas`)

OpenBSD `doas` (ISC license) provides lightweight, auditable privilege elevation:

- Configuration: `/etc/doas.conf` (restricted to mode `0400`):
  ```
  permit persist :wheel
  permit nopass :wheel cmd reboot
  permit nopass :wheel cmd poweroff
  permit nopass :wheel cmd zzz
  permit nopass :wheel cmd ZZZ
  permit nopass :wheel cmd flxpower
  permit nopass :wheel cmd flxdriver
  ```
- **Create a new user with desktop privileges**:
  ```sh
  flxadduser alice
  ```
  This creates user `alice`, adds her to `wheel`, `video`, `audio`, `input`, and `disk`, and seeds her `.config/openbox` session.

---

## 5. FreeLinX Driver Manager (`flxdriver`)

```sh
# Scan all PCI and USB devices and display their driver bindings
flxdriver scan

# Auto-probe unbound devices and modprobe matching drivers
flxdriver probe

# Inspect DRM graphics and 3D acceleration status
flxdriver gpu

# Inspect ALSA soundcards and endpoints
flxdriver audio

# Inspect network and Wi-Fi adapters
flxdriver net
```

---

## 6. Realtime 3D GPU Benchmark (`flx3dtest`)

```sh
# Run realtime ANSI 3D wireframe & z-buffer rendering engine
flx3dtest
```

---

## 7. Audio Subsystem (`tinyalsa`)

```sh
# Display capabilities of default sound card:
tinypcminfo -D 0 -d 0

# Adjust volume levels and mixer switches:
tinymix

# Play raw / WAV audio to speaker:
tinyplay sample.wav -D 0 -d 0
```

---

## Top-Level Build & Verification

From `~/FreeLinX/drivers`:

```sh
# Compile all driver tools, installer, power management, and modules
make all

# Run complete verification test suite
make test

# Stage everything to rootfs
make install ROOTFS=~/FreeLinX/desktop-test/src/rootfs
```
