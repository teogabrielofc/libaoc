# Kernel Tether Design

## Goal

Add a dedicated `kernel/` subtree to `libaoc` for the USB tethering route on
the LC32D1320, focused on `rndis_host` and `cdc_ether`.

## Why This Is Kernel Work

Userland already proved:

- `telnetd` exists
- USB host exists
- the TV can see USB devices

But the tested firmware still does not expose a usable tethering interface.
The kernel config shows the most likely cause directly:

- `CONFIG_USB_NET_RNDIS_HOST` is not set
- `CONFIG_USB_NET_CDCETHER` is not set

So the next real step is not another app. It is kernel module preparation.

## Target Constraints

The TV modules report:

```text
2.6.18_pro500.default preempt mod_unload MIPS32_R2 32BIT gcc-4.2
```

That means the build path must account for:

- kernel release `2.6.18_pro500.default`
- `mod_unload`
- MIPS32 R2 / 32-bit
- the historical compiler string in `vermagic`

## Approach

1. use upstream Linux 2.6.18 USB network driver sources as the base
2. vendor the exact driver files in `libaoc/kernel/vendor/`
3. copy the TV kernel config into `libaoc/kernel/config/`
4. prepare an extracted kernel tree in `build/kernel/`
5. patch:
   - `EXTRAVERSION` to `_pro500`
   - the driver config toggles to enable `rndis_host` and `cdc_ether`
   - the staging `drivers/usb/net/Makefile` so `rndis_host` and `cdc_ether`
     can still build as forced modules when the vanilla tree drops vendor USB
     Kconfig state
   - the module architecture family marker so `vermagic` matches the observed
     `MIPS32_R2` TV modules instead of the vanilla `R4X00` default
   - the `vermagic` gcc string to `gcc-4.2`
6. build `.ko` candidates from WSL

## Success Criteria

- `libaoc/kernel/` exists and is documented
- the repo contains official source snapshots for `rndis_host` and
  `cdc_ether`
- the repo contains scripts to prepare and build against a matching tree
- the generated `.ko` files are close enough to the TV module format to move
  to on-device testing next
