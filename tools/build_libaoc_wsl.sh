#!/usr/bin/env bash
set -eu

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SYSROOT="$ROOT/third_party/sysroots/mips_tv"
BUILD_DIR="$ROOT/build/libaoc"
OUT="$ROOT/build/libaoc/libaoc.a"

CC="${CC:-mips-linux-gnu-gcc}"
AR="${AR:-mips-linux-gnu-ar}"
CFLAGS="${CFLAGS:--EB -mips32r2 -mabi=32 -O2 -Wall -Wextra -fno-stack-protector -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -D_DEFAULT_SOURCE}"
CPPFLAGS="${CPPFLAGS:-}"

SRC=(
  src/aoc_fb.c
  src/aoc_input.c
  src/aoc_log.c
  src/aoc_runtime.c
)

mkdir -p "$BUILD_DIR"

OBJS=()
for src in "${SRC[@]}"; do
  obj="$BUILD_DIR/$(basename "${src%.c}").o"
  "$CC" \
    $CFLAGS \
    $CPPFLAGS \
    --sysroot="$SYSROOT" \
    -I"$ROOT/include" \
    -c "$ROOT/$src" \
    -o "$obj"
  OBJS+=("$obj")
done

"$AR" rcs "$OUT" "${OBJS[@]}"
echo "$OUT"
