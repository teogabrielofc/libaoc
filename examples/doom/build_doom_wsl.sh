#!/usr/bin/env bash
set -eu

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SYSROOT="$ROOT/third_party/sysroots/mips_tv"
OUT="$ROOT/artifacts/usb/doom/doom"
BUILD_DIR="$ROOT/build/doom"
DOOM_DIR="$ROOT/third_party/doomgeneric/doomgeneric"
PORT_DIR="$ROOT/examples/doom"
LIBAOC="$ROOT/build/libaoc/libaoc.a"
START="$ROOT/runtime/start.S"
APPINIT="$ROOT/runtime/appinit.c"
COMPAT="$ROOT/runtime/uclibc_compat.c"

CC="${CC:-mips-linux-gnu-gcc}"
AS="${AS:-mips-linux-gnu-as}"
CFLAGS="${CFLAGS:--EB -mips32r2 -mabi=32 -O2 -Wall -Wextra -fno-stack-protector -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -DNORMALUNIX -DLINUX -D_DEFAULT_SOURCE}"
CPPFLAGS="${CPPFLAGS:-}"
LDFLAGS="${LDFLAGS:-}"

SRC_DOOM=(
  dummy.c am_map.c doomdef.c doomstat.c dstrings.c d_event.c d_items.c d_iwad.c
  d_loop.c d_main.c d_mode.c d_net.c f_finale.c f_wipe.c g_game.c hu_lib.c
  hu_stuff.c info.c i_cdmus.c i_endoom.c i_joystick.c i_scale.c i_sound.c
  i_system.c i_timer.c memio.c m_argv.c m_bbox.c m_cheat.c m_config.c
  m_controls.c m_fixed.c m_menu.c m_misc.c m_random.c p_ceilng.c p_doors.c
  p_enemy.c p_floor.c p_inter.c p_lights.c p_map.c p_maputl.c p_mobj.c
  p_plats.c p_pspr.c p_saveg.c p_setup.c p_sight.c p_spec.c p_switch.c
  p_telept.c p_tick.c p_user.c r_bsp.c r_data.c r_draw.c r_main.c r_plane.c
  r_segs.c r_sky.c r_things.c sha1.c sounds.c statdump.c st_lib.c st_stuff.c
  s_sound.c tables.c v_video.c wi_stuff.c w_checksum.c w_file.c w_main.c
  w_wad.c z_zone.c w_file_stdc.c i_input.c i_video.c doomgeneric.c mus2mid.c
)

mkdir -p "$(dirname "$OUT")"
mkdir -p "$BUILD_DIR"

"$ROOT/tools/build_libaoc_wsl.sh" >/dev/null

OBJS=()

"$AS" -EB -32 "$START" -o "$BUILD_DIR/start.o"
OBJS+=("$BUILD_DIR/start.o")

"$CC" \
  $CFLAGS \
  $CPPFLAGS \
  --sysroot="$SYSROOT" \
  -I"$ROOT/include" \
  -I"$DOOM_DIR" \
  -I"$PORT_DIR" \
  -c "$APPINIT" \
  -o "$BUILD_DIR/appinit.o"
OBJS+=("$BUILD_DIR/appinit.o")

for src in "${SRC_DOOM[@]}"; do
  obj="$BUILD_DIR/${src%.c}.o"
  "$CC" \
    $CFLAGS \
    $CPPFLAGS \
    --sysroot="$SYSROOT" \
    -I"$ROOT/include" \
    -I"$DOOM_DIR" \
    -I"$PORT_DIR" \
    -c "$DOOM_DIR/$src" \
    -o "$obj"
  OBJS+=("$obj")
done

"$CC" \
  $CFLAGS \
  $CPPFLAGS \
  --sysroot="$SYSROOT" \
  -I"$ROOT/include" \
  -I"$DOOM_DIR" \
  -I"$PORT_DIR" \
  -c "$PORT_DIR/doomgeneric_lc32d1320.c" \
  -o "$BUILD_DIR/doomgeneric_lc32d1320.o"
OBJS+=("$BUILD_DIR/doomgeneric_lc32d1320.o")

"$CC" \
  $CFLAGS \
  $CPPFLAGS \
  --sysroot="$SYSROOT" \
  -I"$ROOT/include" \
  -I"$DOOM_DIR" \
  -I"$PORT_DIR" \
  -c "$COMPAT" \
  -o "$BUILD_DIR/uclibc_compat.o"
OBJS+=("$BUILD_DIR/uclibc_compat.o")

"$CC" \
  $CFLAGS \
  $CPPFLAGS \
  -nostartfiles \
  --sysroot="$SYSROOT" \
  "${OBJS[@]}" \
  "$LIBAOC" \
  -L"$SYSROOT/usr/lib" \
  -L"$SYSROOT/lib" \
  -Wl,--dynamic-linker=/lib/ld-uClibc.so.0 \
  -Wl,-e,__start \
  -Wl,-rpath,/lib \
  -lm \
  $LDFLAGS \
  -o "$OUT"

cp -f "$ROOT/third_party/doomgeneric/doom1.wad" "$ROOT/artifacts/usb/doom/doom1.wad"
cp -f "$PORT_DIR/launch.sh" "$ROOT/artifacts/usb/doom/launch.sh"
cp -f "$PORT_DIR/README.md" "$ROOT/artifacts/usb/doom/README.md"

echo "$OUT"
