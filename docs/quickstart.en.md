# Quick start

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
