#!/usr/bin/env bash
set -eu

if [ "$#" -ne 2 ]; then
  echo "usage: build_app_wsl.sh <source.c> <output>" >&2
  exit 2
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SYSROOT="$ROOT/third_party/sysroots/mips_tv"
SOURCE="$1"
OUT="$2"
BUILD_DIR="$ROOT/build/app/$(basename "${OUT}")"
LIBAOC="$ROOT/build/libaoc/libaoc.a"
START="$ROOT/runtime/start.S"
APPINIT="$ROOT/runtime/appinit.c"
COMPAT="$ROOT/runtime/uclibc_compat.c"

CC="${CC:-mips-linux-gnu-gcc}"
AS="${AS:-mips-linux-gnu-as}"
CFLAGS="${CFLAGS:--EB -mips32r2 -mabi=32 -O2 -Wall -Wextra -fno-stack-protector -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -D_DEFAULT_SOURCE}"
CPPFLAGS="${CPPFLAGS:-}"
LDFLAGS="${LDFLAGS:-}"

mkdir -p "$BUILD_DIR" "$(dirname "$OUT")"
"$ROOT/tools/build_libaoc_wsl.sh" >/dev/null

"$AS" -EB -32 "$START" -o "$BUILD_DIR/start.o"

"$CC" $CFLAGS $CPPFLAGS --sysroot="$SYSROOT" -I"$ROOT/include" -c "$APPINIT" -o "$BUILD_DIR/appinit.o"
"$CC" $CFLAGS $CPPFLAGS --sysroot="$SYSROOT" -I"$ROOT/include" -c "$COMPAT" -o "$BUILD_DIR/uclibc_compat.o"
"$CC" $CFLAGS $CPPFLAGS --sysroot="$SYSROOT" -I"$ROOT/include" -c "$SOURCE" -o "$BUILD_DIR/app.o"

"$CC" \
  $CFLAGS \
  $CPPFLAGS \
  -nostartfiles \
  --sysroot="$SYSROOT" \
  "$BUILD_DIR/start.o" \
  "$BUILD_DIR/appinit.o" \
  "$BUILD_DIR/app.o" \
  "$BUILD_DIR/uclibc_compat.o" \
  "$LIBAOC" \
  -L"$SYSROOT/usr/lib" \
  -L"$SYSROOT/lib" \
  -Wl,--dynamic-linker=/lib/ld-uClibc.so.0 \
  -Wl,-e,__start \
  -Wl,-rpath,/lib \
  $LDFLAGS \
  -o "$OUT"

echo "$OUT"
