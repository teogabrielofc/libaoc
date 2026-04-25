#!/usr/bin/env bash
set -eu

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_ROOT="$ROOT/build/kernel"
OUT_DIR="$BUILD_ROOT/out"
TREE_DIR="$("$ROOT/kernel/scripts/prepare_kernel_tree_wsl.sh" | tail -n 1)"

mkdir -p "$OUT_DIR"

AOC_MODULE_DIR="drivers/usb/net"

make -C "$TREE_DIR" ARCH=mips CROSS_COMPILE=mips-linux-gnu- \
  AOC_TETHER_FORCE_MODULES=1 \
  M="$AOC_MODULE_DIR" \
  modules

if [ ! -f "$TREE_DIR/$AOC_MODULE_DIR/rndis_host.ko" ] || [ ! -f "$TREE_DIR/$AOC_MODULE_DIR/cdc_ether.ko" ]; then
  echo "expected tether modules were not emitted; available module artifacts:" >&2
  find "$TREE_DIR/$AOC_MODULE_DIR" -maxdepth 1 \
    \( -name '*.ko' -o -name '*.o' -o -name '*.mod.c' -o -name '*.cmd' \) \
    | sort >&2
  exit 1
fi

cp "$TREE_DIR/$AOC_MODULE_DIR/rndis_host.ko" "$OUT_DIR/rndis_host.ko"
cp "$TREE_DIR/$AOC_MODULE_DIR/cdc_ether.ko" "$OUT_DIR/cdc_ether.ko"

strings "$OUT_DIR/rndis_host.ko" | grep -m1 '^vermagic=' > "$OUT_DIR/rndis_host.vermagic.txt" || true
strings "$OUT_DIR/cdc_ether.ko" | grep -m1 '^vermagic=' > "$OUT_DIR/cdc_ether.vermagic.txt" || true

echo "$OUT_DIR"
