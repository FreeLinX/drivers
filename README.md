# FreeLinX Hardware, Drivers & Bare-Metal Subsystem (no-GNU)

The **FreeLinX Driver Subsystem** provides hardware detection, kernel module management, userspace audio abstraction, power management, hotplug auto-mounting, and bare-metal disk installation designed specifically for FreeLinX's non-GNU, musl-based, BSD-licensed architecture.

---

## Architecture Overview

In FreeLinX, device support is strictly non-GNU and divided into two layers:

```
+-------------------------------------------------------------------------------+
|                      USERSPACE SUBSYSTEM (no-GNU / BSD)                       |
|                                                                               |
|  * flx-driver (Hardware scanning & auto-modprobe engine)                      |
|  * flx-power & zzz (Battery, backlight, CPU governors, suspend-to-RAM)        |
|  * mdevd & flx-automount (Netlink device hotplug & USB storage auto-mount)    |
|  * flx-part & flx-install (Bare-metal GPT partitioner & Limine installer)     |
|  * doas & flx-adduser (OpenBSD privilege elevation & desktop user setup)     |
|  * TinyALSA (Non-GNU audio driver: tinymix, tinyplay, tinycap, tinypcminfo)   |
|  * flx-3dtest (Terminal DRM / KMS 3D renderer & GPU validator)                |
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
| [`flx-driver/`](file:///home/devuan/FreeLinX/drivers/flx-driver) | **Driver Manager** | BSD 2-Clause | C99 hardware detection & auto-probing engine. Scans PCI/USB, maps device IDs, resolves missing modules, and loads drivers via `/sbin/modprobe`. |
| [`flx-3dtest/`](file:///home/devuan/FreeLinX/drivers/flx-3dtest) | **3D GPU Tester** | BSD 2-Clause | Direct DRM/KMS hardware diagnostic validator, 3D vertex transform pipeline, and realtime ANSI/Z-buffer renderer and benchmark. |
| [`power/`](file:///home/devuan/FreeLinX/drivers/power) | **Power Management** | BSD 2-Clause | Battery capacity/wattage monitoring, screen backlight adjustment (`flx-power brightness`), CPU governors (`flx-power governor`), and BSD sleep (`zzz`). |
| [`installer/`](file:///home/devuan/FreeLinX/drivers/installer) | **Bare-Metal Installer** | BSD 2-Clause | C99 GPT partitioning engine (`flx-part`) and automated installer (`flx-install`) with Limine UEFI/BIOS bootloader and ext4/FAT32 setup. |
| [`bootloader/`](file:///home/devuan/FreeLinX/drivers/bootloader) | **Limine Bootloader** | BSD 2-Clause / CC0 | Modern, lightweight bootloader supporting x86_64 UEFI (`BOOTX64.EFI`) and BIOS (`limine-bios.sys`). |
| [`hotplug/`](file:///home/devuan/FreeLinX/drivers/hotplug) | **Device Hotplug** | ISC / BSD | `mdevd` netlink daemon, `/etc/mdev.conf` device permissions, `/sbin/flx-automount` USB storage mounter, and driver auto-prober. |
| [`user/`](file:///home/devuan/FreeLinX/drivers/user) | **Security & Users** | ISC / BSD | OpenBSD `doas` integration, `/etc/doas.conf` (wheel group rules), and `flx-adduser` desktop provisioning script. |
| [`audio/tinyalsa/`](file:///home/devuan/FreeLinX/drivers/audio/tinyalsa) | **Audio Driver** | BSD 3-Clause | Android/BSD minimal ALSA library & utilities. Replaces GNU/LGPL `alsa-lib` with static `tinymix`, `tinyplay`, `tinycap`, `tinypcminfo`. |
| [`firmware/`](file:///home/devuan/FreeLinX/drivers/firmware) | **Firmware Manager** | Permissive/Redist | Stages essential vendor firmware blobs for Wi-Fi (Intel `iwlwifi`, Realtek `rtw88`/`rtw89`, Atheros `ath9k`/`ath10k`, Broadcom `brcm`) into `/lib/firmware`. |
| [`modules/`](file:///home/devuan/FreeLinX/drivers/modules) | **Kernel Modules** | Dual BSD/GPL | Build framework for compiling out-of-tree kernel modules against FreeLinX kernel with Clang/LLVM (`LLVM=1 LLVM_IAS=1`). Includes reference driver `flx_dummy`. |
| [`scripts/`](file:///home/devuan/FreeLinX/drivers/scripts) | **Build & Test** | BSD 2-Clause | Rootfs staging script (`stage-rootfs.sh`) and verification test suite (`test-drivers.sh`). |

---

## 1. Bare-Metal OS Installation (`flx-install`)

To install FreeLinX onto any physical hard drive or SSD:

```sh
# Run interactive installer (scans drives, confirms with YES, partitions, and installs):
flx-install

# Or install directly to a target device (e.g. /dev/sda or /dev/nvme0n1):
flx-install /dev/sda
```

### Installation Steps Executed by `flx-install`:
1. **Partitioning**: Initializes a clean UEFI-compliant GPT table with `flx-part`:
   - Partition 1: EFI System Partition (512MB, Type `C12A7328-F81F-11D2-BA4B-00A0C93EC93B`).
   - Partition 2: FreeLinX Root Filesystem (remainder of drive, Type `0FC63DAF-8483-4772-8E79-3D69D8477DE4`).
2. **Formatting**: Formats ESP as FAT32 (`mkfs.fat -F 32`) and Root as Ext4 (`mke2fs -t ext4`).
3. **Deployment**: Deploys the complete operating system tree.
4. **Bootloader**: Deploys Limine UEFI (`BOOTX64.EFI`) and BIOS (`limine-bios.sys`), writing `/boot/efi/limine.conf`.
5. **Configuration**: Generates `/etc/fstab` with proper mount options and filesystem labels.

---

## 2. Power & Hardware Management (`flx-power`)

```sh
# Show overall system hardware and battery health
flx-power status

# Adjust backlight brightness (supports percentages, relative delta, or absolute)
flx-power brightness 80%
flx-power brightness +10
flx-power brightness -10

# Set CPU scaling governor across all cores
flx-power governor performance
flx-power governor powersave

# Suspend to RAM (Sleep)
zzz
# or: flx-power suspend

# Hibernate to disk
ZZZ
```

---

## 3. Hotplug Device Management & Auto-Mount

- **`mdevd`**: Fast, Netlink-based device event manager (ISC license, zero GNU dependencies).
- **USB Auto-Mount (`/sbin/flx-automount`)**: Automatically detects filesystem types upon drive insertion (`ext4`, `vfat`, `ntfs`, `iso9660`) and mounts cleanly under `/media/<devname>`.
- **Driver Auto-Probe (`/sbin/flx-driver-hotplug`)**: Triggers driver detection whenever new PCI/USB devices are attached.

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
  permit nopass :wheel cmd flx-power
  permit nopass :wheel cmd flx-driver
  ```
- **Create a new user with desktop privileges**:
  ```sh
  flx-adduser alice
  ```
  This creates user `alice`, adds her to `wheel`, `video`, `audio`, `input`, and `disk`, and seeds her `.config/openbox` session.

---

## 5. FreeLinX Driver Manager (`flx-driver`)

```sh
# Scan all PCI and USB devices and display their driver bindings
flx-driver scan

# Auto-probe unbound devices and modprobe matching drivers
flx-driver probe

# Inspect DRM graphics and 3D acceleration status
flx-driver gpu

# Inspect ALSA soundcards and endpoints
flx-driver audio

# Inspect network and Wi-Fi adapters
flx-driver net
```

---

## 6. Realtime 3D GPU Benchmark (`flx-3dtest`)

```sh
# Run realtime ANSI 3D wireframe & z-buffer rendering engine
flx-3dtest
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
