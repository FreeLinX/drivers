# FreeLinX/drivers - vendored Linux UAPI headers

This tree is a small, self-contained subset of the Linux 6.6 userspace (UAPI)
headers used by `flx3dtest/` and `audio/tinyalsa/` when they talk to the DRM
and ALSA kernel interfaces.  The FreeLinX musl sysroot ships no kernel
headers (by design), so these files let the tools build with nothing more than
the FreeLinX clang toolchain.

## Provenance

Every header is byte-identical to the pristine `include/uapi/` (or
`arch/x86/include/uapi/`) file from the Linux **v6.6** release, unpacked from
<https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.6.tar.xz>, except for:

- `linux/compiler.h`, `linux/compiler_types.h` — hand-written *userspace
  compatibility* stand-ins for the kernel-internal `<linux/compiler.h>` /
  `<linux/compiler_types.h>`: the kernel defines `__user`, `__force`,
  `__always_inline`, `__packed`, ... in files that are not part of the UAPI.
  Distro kernel-header packages ship the same minimal definitions (an empty
  `compiler_types.h` stub plus the attribute macros in `compiler.h`).
- `asm-generic/ioctl.h` — the `_IO`/`_IOR`/`_IOW`/`_IOWR`/`_IOC` macros are
  wrapped in `#ifndef` guards because musl already provides them (with
  identical semantics) in `<sys/ioctl.h>`; without the guards the two headers
  collide when a program includes both.

The x86 `asm/` directory holds the real arch headers (`bitsperlong.h`,
`byteorder.h`, `posix_types_64.h`, `posix_types.h`, `swab.h`) plus copies of
the generic `ioctl.h`/`types.h` (x86 uses the generic versions; this mirrors
the `asm` → `asm-generic` fallback the kernel build generates).

## Structure

```
include/
  linux/          Linux UAPI: ioctl.h, types.h, posix_types.h, stddef.h,
                  swab.h, byteorder/little_endian.h, compiler{,_types}.h
  asm-generic/    generic UAPI fallbacks (ioctl, types, int-ll64, posix_types,
                  bitsperlong), with the musl-compat guard patch
  asm/            x86_64 arch headers (+ generic copies where x86 falls back)
  sound/          ALSA UAPI: asound.h
  drm/            DRM UAPI: drm.h, drm_mode.h
```

These are UAPI definitions that change almost never; when FreeLinX moves to a
new kernel major the tree should be refreshed from the matching tarball.