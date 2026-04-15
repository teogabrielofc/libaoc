# Quick start

This flow is for AOC TVs with a similar MIPS/uClibc firmware and Media Center
external-subtitle path. The ready-made artifacts (`libaocdoom.*` and
`libaoccore.*`) were validated on the AOC LC32D1320.

## Requirements

- Windows with WSL/Ubuntu, or Linux.
- `python` with the packages from `requirements.txt`.
- MIPS toolchain in PATH:
  - `mips-linux-gnu-gcc`
  - `mips-linux-gnu-as`
  - `mips-linux-gnu-ar`
  - `mips-linux-gnu-readelf`
- `ffmpeg` only if you are replacing the base AVI video.

## Normal build

```sh
python -m pip install -r requirements.txt
make test
make sdk
make doom
make psb
make corepsb
make usb
```

## USB layout

The USB drive root should look like this:

```text
doom/
  doom
  doom1.wad
  launch.sh
  README.md
libaocdoom.avi
libaocdoom.psb
libaoccore.avi
libaoccore.psb
```

Open `libaocdoom.avi` in Media Center and enable subtitles.

## Note for other AOC models

If the model is not the LC32D1320, do not assume the PSB addresses are the
same. First generate/collect a core with `libaoccore.*`, run
`tools/core_addresses.py`, and regenerate the PSBs with `--core`.

## Custom PSB

To generate a PSB that runs a chosen command:

```sh
make psb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
make cmdpsb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
```

The generated pair is written to `artifacts/psb/libaoccmd.avi` and
`artifacts/psb/libaoccmd.psb`.

## If video does not appear

Edit `doom/launch.sh` and use:

```sh
export AOC_FB_PAGES="${AOC_FB_PAGES:-all}"
```

This uses the safe path that paints every framebuffer page.
