# Troubleshooting

## Doom Does Not Appear

Use the safe framebuffer mode:

```sh
export AOC_FB_PAGES="${AOC_FB_PAGES:-all}"
```

Then rerun the PSB launcher.

## Doom Appears But Is Slow

Use the default:

```sh
export AOC_FB_PAGES="${AOC_FB_PAGES:-1}"
```

Leave `AOC_INPUT_DEBUG=0`; raw input logging adds unnecessary filesystem work.

## Buttons Do Not Work

Set:

```sh
export AOC_INPUT_DEBUG="${AOC_INPUT_DEBUG:-1}"
```

Check `doom/launch_doom_usb.log` and `/etc/core/doom.log` after the run.

## PSB Does Nothing

Confirm the USB root contains both:

- `PSB60_LAUNCH_DOOM.avi`
- `PSB60_LAUNCH_DOOM.psb`

Also confirm the TV Media Center has subtitles enabled for the AVI.
