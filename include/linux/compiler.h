/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Minimal UAPI-compatible <linux/compiler.h>.
 *
 * FreeLinX/drivers vendored copy.  The kernel's asm/swab.h and other UAPI
 * headers reference a handful of compiler-attribute macros from the kernel's
 * (non-UAPI) <linux/compiler.h>.  Distro kernel-header packages ship the same
 * minimal subset; this mirrors those definitions so the vendored UAPI headers
 * compile cleanly against the musl sysroot.  Values match Linux 6.6.
 */
#ifndef _LINUX_COMPILER_H
#define _LINUX_COMPILER_H

#define __user
#define __kernel
#define __safe
#define __force
#define __iomem
#define __chk_user_ptr(x)		(void)0
#define __chk_io_ptr(x)			(void)0
#define __ACCESS_CRED

#define __attribute_const__		__attribute__((__const__))
#define __always_inline			inline __attribute__((__always_inline__))
#define __packed			__attribute__((__packed__))
#define __aligned(x)			__attribute__((__aligned__(x)))
#define __bitwise
#define __bitwise__
#define __must_check			__attribute__((__warn_unused_result__))
#define __noreturn			__attribute__((__noreturn__))
#define __deprecated			__attribute__((__deprecated__))
#define __printf(a, b)			__attribute__((__format__(printf, a, b)))
#define __scanf(a, b)			__attribute__((__format__(scanf, a, b)))

#define __stringify_1(x)		#x
#define __stringify(x)			__stringify_1(x)

#endif /* _LINUX_COMPILER_H */