# Kernel Tether Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a `kernel/` subtree to `libaoc` that stages `rndis_host` and `cdc_ether` module work for the LC32D1320 tethering route.

**Architecture:** Keep this work separate from the userland SDK. Store the TV kernel config, vendor the upstream Linux 2.6.18 USB network driver sources, and provide WSL scripts that prepare a matching kernel tree and attempt to build the two `.ko` files with the TV’s expected release/vermagic.

**Tech Stack:** Linux 2.6.18 source layout, WSL shell scripts, Python-based tar extraction/patching, pytest source-layout checks.

---

### Task 1: Add failing kernel subtree tests

**Files:**
- Modify: `C:\Users\teoga\Desktop\pastas\libaoc\tests\test_sdk_layout.py`

**Step 1: Write the failing test**

Assert that the repo contains:

- `kernel/README.md`
- `kernel/config/lc32d1320-2.6.18_pro500.default.config`
- `kernel/scripts/prepare_kernel_tree_wsl.sh`
- `kernel/scripts/build_tether_modules_wsl.sh`
- vendored `rndis_host.c` and `cdc_ether.c`

**Step 2: Run test to verify it fails**

Run:

```bash
python -m pytest tests/test_sdk_layout.py -q
```

Expected: FAIL because the kernel subtree is incomplete.

### Task 2: Add the kernel subtree

**Files:**
- Create: `C:\Users\teoga\Desktop\pastas\libaoc\kernel\README.md`
- Create: `C:\Users\teoga\Desktop\pastas\libaoc\kernel\config\lc32d1320-2.6.18_pro500.default.config`
- Create: `C:\Users\teoga\Desktop\pastas\libaoc\kernel\scripts\prepare_kernel_tree_wsl.sh`
- Create: `C:\Users\teoga\Desktop\pastas\libaoc\kernel\scripts\build_tether_modules_wsl.sh`
- Create: `C:\Users\teoga\Desktop\pastas\libaoc\kernel\vendor\linux-2.6.18\drivers\usb\net\rndis_host.c`
- Create: `C:\Users\teoga\Desktop\pastas\libaoc\kernel\vendor\linux-2.6.18\drivers\usb\net\cdc_ether.c`

**Step 1: Implement minimal working scripts**

The scripts should:

- download Linux 2.6.18
- extract the tree
- patch `EXTRAVERSION` to `_pro500`
- force `CONFIG_USB_NET_RNDIS_HOST=m`
- force `CONFIG_USB_NET_CDCETHER=m`
- patch `vermagic` to `gcc-4.2`
- attempt to build `rndis_host.ko` and `cdc_ether.ko`

**Step 2: Re-run source tests**

Run:

```bash
python -m pytest tests/test_sdk_layout.py -q
```

Expected: PASS.

### Task 3: Verify normal libaoc health

**Files:**
- Existing repo files only

**Step 1: Run full tests**

```bash
python -m pytest -q
```

Expected: full suite passes.

**Step 2: Run libaoc build**

```bash
wsl.exe bash -lc "cd '/mnt/c/Users/teoga/Desktop/pastas/libaoc' && bash tools/build_libaoc_wsl.sh"
```

Expected: `build/libaoc/libaoc.a` is produced.

### Task 4: Attempt kernel module build

**Files:**
- Output: `C:\Users\teoga\Desktop\pastas\libaoc\build\kernel\out`

**Step 1: Run tether build**

```bash
wsl.exe bash -lc "cd '/mnt/c/Users/teoga/Desktop/pastas/libaoc' && bash kernel/scripts/build_tether_modules_wsl.sh"
```

Expected: either:

- `.ko` files are produced, or
- the failure point is captured precisely for the next iteration

**Step 2: Commit**

Create a focused commit for the new `kernel/` subtree and docs.
