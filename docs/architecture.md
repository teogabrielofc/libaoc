# Architecture

`libaoc` is a static C SDK. It keeps TV-specific details out of apps.

## SDK Modules

- `aoc_fb.c`: opens `/dev/hidtv2dge`, reads framebuffer geometry, maps video
  memory, and presents XRGB8888 frames.
- `aoc_input.c`: binds `/tmp/hp_dfb_handler`, decodes remote packets, and falls
  back to `/dev/remote`.
- `aoc_log.c`: writes small logs to the TV filesystem and optionally calls
  `fsync`.
- `aoc_runtime.c`: wraps raw MIPS syscalls and time/sleep helpers.

## Runtime

TV binaries use:

- `runtime/start.S` for the MIPS/uClibc process entry.
- `runtime/appinit.c` for empty `_init` and `_fini`.
- `runtime/uclibc_compat.c` for glibc-style builtin shims that the cross
  compiler may emit.

## Doom Example

The Doom adapter only translates DoomGeneric hooks to `libaoc` calls:

- `DG_Init` opens framebuffer and input.
- `DG_DrawFrame` calls `aoc_fb_present_xrgb8888_scaled`.
- `DG_GetKey` polls `libaoc` input events and maps them to Doom keys.

The default framebuffer mode paints one page per frame. Use `AOC_FB_PAGES=all`
if the visible page differs on another unit or firmware.
