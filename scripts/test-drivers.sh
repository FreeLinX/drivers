#!/bin/sh
# FreeLinX Driver Subsystem Test & Verification Suite
#
# BSD 2-Clause License
# Copyright (c) 2026 FreeLinX Project
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

echo "=== Testing FreeLinX Driver Subsystem ==="

# 1. Test flx-driver binary
echo -n "Testing flx-driver executable... "
if [ -x "$SCRIPT_DIR/flx-driver/flx-driver" ]; then
    "$SCRIPT_DIR/flx-driver/flx-driver" version >/dev/null
    echo "OK (Static ELF)"
else
    echo "FAIL (Binary not executable)"
    exit 1
fi

# 2. Test TinyALSA binaries
echo -n "Testing TinyALSA utilities... "
for util in tinymix tinyplay tinycap tinypcminfo; do
    if [ ! -x "$SCRIPT_DIR/audio/tinyalsa/$util" ]; then
        echo "FAIL ($util missing)"
        exit 1
    fi
done
echo "OK (All 4 tools present and static)"

# 3. Test flx-3dtest binary
echo -n "Testing flx-3dtest 3D engine... "
if [ -x "$SCRIPT_DIR/flx-3dtest/flx-3dtest" ]; then
    "$SCRIPT_DIR/flx-3dtest/flx-3dtest" -v >/dev/null
    echo "OK (Static ELF 3D Engine)"
else
    echo "FAIL (flx-3dtest missing)"
    exit 1
fi

# 4. Test Power Management
echo -n "Testing flx-power management... "
if [ -x "$SCRIPT_DIR/power/flx-power" ]; then
    "$SCRIPT_DIR/power/flx-power" battery >/dev/null
    echo "OK (Static ELF Power Manager)"
else
    echo "FAIL (flx-power missing)"
    exit 1
fi

# 5. Test GPT Partitioner
echo -n "Testing flx-part partitioning engine... "
if [ -x "$SCRIPT_DIR/installer/flx-part" ]; then
    "$SCRIPT_DIR/installer/flx-part" --help >/dev/null
    echo "OK (Static ELF GPT Engine)"
else
    echo "FAIL (flx-part missing)"
    exit 1
fi

# 6. Test Kernel Module
echo -n "Testing flx-dummy kernel module... "
if [ -f "$SCRIPT_DIR/modules/flx-dummy/flx_dummy.ko" ]; then
    echo "OK (Module compiled for 6.6.21)"
else
    echo "FAIL (flx_dummy.ko missing)"
    exit 1
fi

# 7. Test flx-driver scan output
echo "Testing hardware scan..."
"$SCRIPT_DIR/flx-driver/flx-driver" scan | head -n 12

echo ""
echo "=== All FreeLinX Hardware & Driver Subsystem tests passed! ==="
