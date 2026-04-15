# libaoc

`libaoc` is a small native C SDK for the AOC LC32D1320 TV firmware.
It packages the pieces needed to build and launch simple MIPS/uClibc apps
through the Media Center subtitle path.

The repository is intentionally self-contained:

- `include/` and `src/` contain the SDK.
- `runtime/` contains the MIPS entry/runtime shims.
- `examples/doom/` contains the DoomGeneric TV adapter and launcher.
- `third_party/sysroots/mips_tv/` contains the firmware sysroot used by the
  build scripts.
- `artifacts/usb/doom/` contains a ready-to-copy Doom payload.
- `artifacts/psb/` contains `PSB60_LAUNCH_DOOM.avi` and `.psb`.

## Quick Start

From WSL or a shell with the MIPS tools available:

```sh
make test
make sdk
make doom
make psb
make usb
```

If `make` is not installed, use the scripts directly:

```sh
python -m pytest -q
bash tools/build_libaoc_wsl.sh
bash examples/doom/build_doom_wsl.sh
python tools/make_psb.py doom-launcher
python tools/make_usb_tree.py --out dist/usb --include-psb
```

Copy `dist/usb` to a FAT32 USB stick, or copy these items manually to the USB
root:

- `doom/`
- `PSB60_LAUNCH_DOOM.avi`
- `PSB60_LAUNCH_DOOM.psb`

On the TV, open `PSB60_LAUNCH_DOOM.avi` in Media Center with subtitles enabled.
The subtitle payload runs:

```sh
chmod +x /mnt/doom/launch.sh; /mnt/doom/launch.sh
```

## Doom Runtime Knobs

The launcher exports conservative defaults:

- `AOC_FB_PAGES=1` paints one framebuffer page per frame and is the default
  playable path.
- `AOC_FB_PAGES=all` restores the old safe full-paint behavior if video does
  not appear.
- `AOC_FB_FULL_REFRESH_EVERY=0` disables periodic full refreshes.
- `AOC_INPUT_DEBUG=1` re-enables raw input logs when mapping buttons.

Confirmed Doom controls:

- Vol+ -> forward
- Vol- -> backward
- Menu -> fire
- CH+ -> turn left
- CH- -> turn right
- Input -> use/open

## Documentation

- `docs/quickstart.md`: shortest path from clone to TV.
- `docs/architecture.md`: SDK and launcher architecture.
- `docs/psb-launcher.md`: PSB60 payload details.
- `docs/troubleshooting.md`: common failure modes.
