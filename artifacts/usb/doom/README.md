# Doom USB Launcher

This payload stages the launcher tree under `doom/` for USB export.

The shell launcher runs the raw-framebuffer Doom port from `/mnt/doom`.
It writes early markers directly to the USB stick before starting Doom:

- `/mnt/doom/launcher_touched.txt`
- `/mnt/doom/launch_doom_usb.log`
- `/mnt/launch_doom.log`
- `/etc/core/launch_doom.log`

Doom stdout/stderr is redirected to `/mnt/doom/launch_doom_usb.log`, so a TV
hang after launch still leaves the latest marker on the stick.

Current Doom build uses `libaoc`, a small static C SDK for this TV. It owns:

- raw framebuffer setup through `/dev/hidtv2dge`
- high-level remote capture through `/tmp/hp_dfb_handler`
- `/dev/remote` fallback
- low-noise logging and MIPS/uClibc syscall helpers

Runtime knobs exported by `launch.sh`:

- `AOC_FB_PAGES=1` is the default fast path; it paints one framebuffer page per
  Doom frame to reduce lag
- `AOC_FB_PAGES=all` is the safe fallback; it paints every mapped page like the
  older build if the fast path does not show video
- `AOC_FB_FULL_REFRESH_EVERY=0` disables periodic full refreshes by default
- `AOC_INPUT_DEBUG=1` re-enables raw input logging when button mapping needs
  diagnosis; keep it `0` during gameplay to avoid log overhead

Current Doom input build:

- first tries to bind `/tmp/hp_dfb_handler` as an `AF_UNIX`/`SOCK_DGRAM` socket
- logs translated packets as `doomfb: trid packet kind=... code=...`
- falls back to opening `/dev/remote`
- maps recovered `DIKS_*` values plus the raw `system.cfg` remote codes
- maps observed TV socket codes for the current remote layout:
  `0x3c`/Vol+ -> forward, `0x3d`/Vol- -> backward, `0x00010319`/Menu -> fire,
  `0x00010316`/CH+ -> turn left, `0x00010317`/CH- -> turn right
- maps the observed Input packet forms `0x00010318`, `0x3e`, and `0x00100017`
  to Doom use/open
- logs raw and translated input to both `doom.log` and redirected stderr in
  `doom/launch_doom_usb.log`
- forces `fsync()` for log appends and keeps the first 128 input events
- keeps `-nomonsters -warp 1 1` in the launcher for safer movement testing

The USB tree also carries `netprobe.sh` for a shell-only network experiment:

- `PSB70_NETPROBE` copies `netprobe.sh` to `/tmp` and runs it in background
- `PSB71_PULL_NETPROBE` copies `/tmp/netprobe.*` back to `/etc/core`

This path is meant for checking whether USB phone tethering or a USB Ethernet
adapter creates a usable interface, without depending on another native ELF.
