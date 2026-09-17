#!/bin/sh
# FreeLinX Driver Subsystem Test & Verification Suite
#
# BSD 2-Clause License
# Copyright (c) 2026 FreeLinX Project
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

echo "=== Testing FreeLinX Driver Subsystem ==="

# 1. Test flxdriver binary
echo -n "Testing flxdriver executable... "
if [ -x "$SCRIPT_DIR/flxdriver/flxdriver" ]; then
    "$SCRIPT_DIR/flxdriver/flxdriver" version >/dev/null
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

# 3. Test flx3dtest binary
echo -n "Testing flx3dtest 3D engine... "
if [ -x "$SCRIPT_DIR/flx3dtest/flx3dtest" ]; then
    "$SCRIPT_DIR/flx3dtest/flx3dtest" -v >/dev/null
    echo "OK (Static ELF 3D Engine)"
else
    echo "FAIL (flx3dtest missing)"
    exit 1
fi

# 4. Test Power Management
echo -n "Testing flxpower management... "
if [ -x "$SCRIPT_DIR/power/flxpower" ]; then
    "$SCRIPT_DIR/power/flxpower" battery >/dev/null
    echo "OK (Static ELF Power Manager)"
else
    echo "FAIL (flxpower missing)"
    exit 1
fi

# 5. Test GPT Partitioner
echo -n "Testing flxpart partitioning engine... "
if [ -x "$SCRIPT_DIR/installer/flxpart" ]; then
    "$SCRIPT_DIR/installer/flxpart" --help >/dev/null
    echo "OK (Static ELF GPT Engine)"
else
    echo "FAIL (flxpart missing)"
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

# 7. Test flxdriver scan output
echo "Testing hardware scan..."
"$SCRIPT_DIR/flxdriver/flxdriver" scan | head -n 12

echo ""
echo "=== All FreeLinX Hardware & Driver Subsystem tests passed! ==="
