# PSB launcher

`tools/make_psb.py doom-launcher` generates `libaocdoom.avi` and
`libaocdoom.psb` for AOC TVs compatible with this PSB subtitle path.

The default constants below are from the tested LC32D1320:

- libc base: `0x2bbb8000`
- `system`: `0x2bc07240`
- implicit `system(a0)` gadget: `0x2bc02780`
- `saved s0` marker: `0x41414141`

Doom command:

```sh
chmod +x /mnt/doom/launch.sh; /mnt/doom/launch.sh
```

## Generate a core dump

```sh
python tools/make_psb.py core-crash
```

This generates:

- `artifacts/psb/libaoccore.avi`
- `artifacts/psb/libaoccore.psb`

The operational detail matters if you want the core to land on the USB drive:

- use a FAT32 superfloppy USB drive
- keep the USB drive connected before powering the TV
- boot the TV with the USB drive already present
- open `libaoccore.avi` and enable `libaoccore.psb`
- after the freeze, look for `core.*` in the USB drive root

## Read a core dump

```sh
python tools/core_addresses.py core.plfApFusion71Di.875.11
```

Expected output:

```text
libc_base=0x...
sp=0x...
system=0x...
implicit_a0_gadget=0x...
xdrrec_eof_gadget=0x...
command_ptr=0x...
```

## PSB with a chosen command

```sh
make psb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
make cmdpsb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
```

Or directly:

```sh
python tools/make_psb.py command --command 'echo OK>/etc/core/libaoc.ok' --base-name libaoccmd
```

If you have a core from another boot or firmware, use:

```sh
python tools/make_psb.py command --core core.plfApFusion71Di.875.11 --command 'echo OK>/etc/core/libaoc.ok'
```

For another AOC model, treat `--core` as required until you confirm that bases
and gadgets match.
