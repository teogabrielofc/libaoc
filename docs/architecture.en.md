# Architecture

`libaoc` is a static C library for native apps on MIPS/uClibc AOC TVs. The
goal is to keep TV-specific details out of apps.

The architecture is general, but the current backend was researched and
validated on the LC32D1320. Porting to another AOC model probably requires
confirming devices, PSB addresses, and framebuffer/input layout.

## SDK modules

- `aoc_fb.c`: opens `/dev/hidtv2dge`, reads geometry, maps the framebuffer, and
  presents XRGB8888 frames.
- `aoc_input.c`: uses `/tmp/hp_dfb_handler`, decodes remote-control packets,
  and tries `/dev/remote` as fallback. It also exposes the raw
  `aoc_input_pop_raw()` queue for apps that need untranslated events.
- `aoc_usb_kbd.c`: enumerates `usbfs`, finds a boot-HID keyboard interface,
  claims it in userland, reads reports through URBs, and translates them into
  ASCII bytes or terminal escape sequences.
- `aoc_log.c`: writes small logs to the TV filesystem and can call `fsync`.
- `aoc_runtime.c`: small wrappers for MIPS syscalls, time, and sleep.

## Input

The SDK now keeps input in two separate paths:

- `aoc_input`: remote control / TV stack.
- `aoc_usb_kbd`: raw USB keyboard through `usbfs`.

This split is intentional. On the LC32D1320, the validated USB keyboard path
did not arrive in a useful way through the same event socket used by the remote
stack, so the keyboard backend was implemented directly on top of the HID
interface.

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

Doom still focuses on remote-control input, but the USB keyboard backend is now
available for interactive apps closer to a shell/terminal workflow.

## PSB

`tools/make_psb.py` generates three families:

- `doom-launcher`: `libaocdoom.*`, launches `/mnt/doom/launch.sh`.
- `core-crash`: `libaoccore.*`, tries to generate a parser core dump.
- `command`: name chosen with `--base-name`, runs a chosen shell command.

The custom PSB uses the same `system(a0)` path already validated on the TV.
The default constants for that path are from the LC32D1320; for another model,
collect a core and regenerate the PSB with `--core`.

## Current network limit

The SDK does not yet include a networking/tethering backend. Practical testing
confirmed the presence of `telnetd` and USB host support, but did not confirm a
usable userland network interface for phone tethering on this firmware. If that
route moves forward, it will likely need explicit driver/kernel support or some
other adaptation outside the current `libaoc` scope.
