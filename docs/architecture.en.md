# Architecture

`libaoc` is a static C library. The goal is to keep TV-specific details out of
apps.

## SDK modules

- `aoc_fb.c`: opens `/dev/hidtv2dge`, reads geometry, maps the framebuffer, and
  presents XRGB8888 frames.
- `aoc_input.c`: uses `/tmp/hp_dfb_handler`, decodes remote-control packets,
  and tries `/dev/remote` as fallback.
- `aoc_log.c`: writes small logs to the TV filesystem and can call `fsync`.
- `aoc_runtime.c`: small wrappers for MIPS syscalls, time, and sleep.

## Runtime

TV binaries use:

- `runtime/start.S`: MIPS/uClibc process entry point.
- `runtime/appinit.c`: empty `_init` and `_fini`.
- `runtime/uclibc_compat.c`: shims for glibc-style symbols that the compiler
  may emit.

## Doom example

The DoomGeneric adapter only translates hooks into `libaoc` calls:

- `DG_Init` opens framebuffer and input.
- `DG_DrawFrame` calls `aoc_fb_present_xrgb8888_scaled`.
- `DG_GetKey` polls `libaoc` events and converts them to Doom keys.

The default mode paints one framebuffer page per frame. Use `AOC_FB_PAGES=all`
if another unit or firmware shows the wrong page.

## PSB

`tools/make_psb.py` generates three families:

- `doom-launcher`: `libaocdoom.*`, launches `/mnt/doom/launch.sh`.
- `core-crash`: `libaoccore.*`, tries to generate a parser core dump.
- `command`: name chosen with `--base-name`, runs a chosen shell command.

The custom PSB uses the same `system(a0)` path already validated on the TV.
