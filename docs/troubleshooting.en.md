# Troubleshooting

## Doom does not appear

Use the safe framebuffer mode:

```sh
export AOC_FB_PAGES="${AOC_FB_PAGES:-all}"
```

Then run the PSB launcher again.

## Doom appears but is slow

Use the playable default:

```sh
export AOC_FB_PAGES="${AOC_FB_PAGES:-1}"
```

Keep `AOC_INPUT_DEBUG=0`; raw input logging writes too much to the filesystem.

## Buttons do not work

Enable input debug:

```sh
export AOC_INPUT_DEBUG="${AOC_INPUT_DEBUG:-1}"
```

Then inspect `doom/launch_doom_usb.log` and `/etc/core/doom.log`.

## PSB does nothing

Confirm that the USB drive root contains:

- `libaocdoom.avi`
- `libaocdoom.psb`

Also confirm that subtitles are enabled in Media Center.

## Core dump does not appear

The firmware mounts `/etc/core` during boot. If the USB drive was inserted
after the TV was already on, the crash may happen and still no core will land
on USB.

Use this flow:

1. format the USB drive as FAT32 superfloppy
2. place `libaoccore.avi` and `libaoccore.psb` in the root
3. keep the USB drive connected
4. power-cycle the TV from the wall
5. open `libaoccore.avi`
6. enable subtitles
7. after the freeze, check for `core.*` on the PC

## Addresses changed

Run:

```sh
python tools/core_addresses.py core.plfApFusion71Di.875.11
```

Then generate the PSB again with:

```sh
python tools/make_psb.py doom-launcher --core core.plfApFusion71Di.875.11
```
