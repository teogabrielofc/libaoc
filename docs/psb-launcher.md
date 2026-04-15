# PSB Launcher

`tools/make_psb.py doom-launcher` builds the PSB60 launcher payload.

The default constants are the confirmed values for the tested LC32D1320
firmware:

- libc base: `0x2bbb8000`
- system: `0x2bc07240`
- implicit-a0 call gadget: `0x2bc02780`
- saved s0 marker: `0x41414141`

The command is:

```sh
chmod +x /mnt/doom/launch.sh; /mnt/doom/launch.sh
```

The tool writes:

- `artifacts/psb/PSB60_LAUNCH_DOOM.avi`
- `artifacts/psb/PSB60_LAUNCH_DOOM.psb`
- `artifacts/psb/README.md`

For a different firmware/core, pass a core dump:

```sh
python tools/make_psb.py doom-launcher --core path/to/core
```

That requires `pyelftools`.
