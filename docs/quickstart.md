# Quickstart

## Requirements

- Windows plus WSL/Ubuntu, or Linux.
- `python` with packages from `requirements.txt`.
- MIPS cross tools in PATH:
  - `mips-linux-gnu-gcc`
  - `mips-linux-gnu-as`
  - `mips-linux-gnu-ar`
  - `mips-linux-gnu-readelf`

## Build

```sh
python -m pip install -r requirements.txt
python -m pytest -q
bash tools/build_libaoc_wsl.sh
bash examples/doom/build_doom_wsl.sh
python tools/make_psb.py doom-launcher
python tools/make_usb_tree.py --out dist/usb --include-psb
```

## USB Layout

The USB root should contain:

```text
doom/
  doom
  doom1.wad
  launch.sh
  README.md
PSB60_LAUNCH_DOOM.avi
PSB60_LAUNCH_DOOM.psb
```

Open the AVI in the TV Media Center and enable subtitles.

## If Video Does Not Appear

Edit `doom/launch.sh` and set:

```sh
export AOC_FB_PAGES="${AOC_FB_PAGES:-all}"
```

That uses the safe full-page paint path.
