# libaoc

`libaoc` is a small native C SDK for MIPS/uClibc-based AOC TVs. It packages
the pieces needed to build apps and launch them through the Media Center PSB
subtitle path.

The tested reference target is the AOC LC32D1320. Some parts of the repo,
especially PSB addresses, framebuffer/input devices, and ready-made artifacts,
are focused on that model and firmware family.

The repo is self-contained:

- `include/` and `src/`: C SDK.
- `runtime/`: MIPS entry point and uClibc shims.
- `examples/doom/`: DoomGeneric adapter for the TV.
- `third_party/sysroots/mips_tv/`: sysroot extracted from the firmware.
- `artifacts/usb/doom/`: ready-to-copy USB payload.
- `artifacts/psb/`: `libaocdoom.*` and `libaoccore.*`.
- `tools/`: build, PSB, USB superfloppy, and core dump address tools.

## License

This repository is distributed under `GPL-2.0-only`. That became necessary
because the `kernel/` subtree vendors official Linux 2.6.18 files, including
`rndis_host.c`, `cdc_ether.c`, `usbnet.h`, `Kconfig`, and `Makefile`.

See [LICENSE](C:/Users/teoga/Desktop/pastas/libaoc/LICENSE).

## Current validated state

- Native framebuffer validated through `/dev/hidtv2dge`.
- Remote control validated through `aoc_input` with `/tmp/hp_dfb_handler` and
  `/dev/remote` fallback.
- USB keyboard validated in userland through `usbfs`/boot-HID keyboard, exposed
  by the `aoc_usb_kbd` backend.
- USB phone tethering is still not validated on this firmware. The TV clearly
  has USB host support and sees some USB devices, but no usable network
  interface has appeared in userland so far without additional driver/kernel
  support.

## Compatibility scope

- General: SDK structure, MIPS/uClibc build flow, minimal runtime, and payload
  organization for similar AOC TVs.
- LC32D1320: ready-made PSB, `system()` constants, gadgets, input paths,
  framebuffer paths, and Doom validated on real hardware.
- Other models: treat them as new ports; use `libaoccore.*` and
  `tools/core_addresses.py` to recover addresses before trusting the PSBs.

## Quick start

On WSL/Linux with the MIPS toolchain in PATH:

```sh
make test
make sdk
make doom
make psb
make corepsb
make usb
```

Without `make`, run the scripts directly:

```sh
python -m pytest -q
bash tools/build_libaoc_wsl.sh
bash examples/doom/build_doom_wsl.sh
python tools/make_psb.py doom-launcher
python tools/make_psb.py core-crash
python tools/make_usb_tree.py --out dist/usb --include-psb
```

Copy the contents of `dist/usb` to the USB drive root:

```text
doom/
libaocdoom.avi
libaocdoom.psb
libaoccore.avi
libaoccore.psb
```

On the TV, open `libaocdoom.avi` in Media Center and enable the
`libaocdoom.psb` subtitle. The payload runs:

```sh
chmod +x /mnt/doom/launch.sh; /mnt/doom/launch.sh
```

## Custom command PSB

To generate a PSB that runs a chosen command:

```sh
make psb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
make cmdpsb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
```

This creates `artifacts/psb/libaoccmd.avi` and
`artifacts/psb/libaoccmd.psb`.

## Core dump

`libaoccore.*` is the pair used to try to generate a PSB parser core dump. Use
a FAT32 superfloppy USB drive, connected before powering the TV from the wall.
After the freeze, look for `core.*` in the USB drive root.

To read useful addresses:

```sh
python tools/core_addresses.py core.plfApFusion71Di.875.11
```

## Build your own app

A C app with `main()` can be built like this:

```sh
make app APP=examples/hello/hello.c OUT=artifacts/usb/hello
```

## Doom knobs

- `AOC_FB_PAGES=1` paints one framebuffer page per frame and is the default
  playable path.
- `AOC_FB_PAGES=all` paints every page and is the safe fallback if video does
  not appear.
- `AOC_INPUT_DEBUG=1` enables raw input logs for button mapping.

The SDK also exposes the `aoc_usb_kbd` backend for apps that need a USB
keyboard path separate from the remote-control stack.

Confirmed controls:

- Vol+ -> forward
- Vol- -> backward
- Menu -> fire
- CH+ -> turn left
- CH- -> turn right
- Input -> use/open

## Docs

- `docs/quickstart.md`: shortest path to running on the TV.
- `docs/architecture.md`: SDK and runtime architecture.
- `docs/psb-launcher.md`: PSB details.
- `docs/troubleshooting.md`: common failure modes.

English versions:

- `README.en.md`
- `docs/quickstart.en.md`
- `docs/architecture.en.md`
- `docs/psb-launcher.en.md`
- `docs/troubleshooting.en.md`
