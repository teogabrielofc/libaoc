#!/usr/bin/env bash
set -eu

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
KERNEL_ROOT="$ROOT/kernel"
CONFIG_PATH="$KERNEL_ROOT/config/lc32d1320-2.6.18_pro500.default.config"
VENDOR_NET_DIR="$KERNEL_ROOT/vendor/linux-2.6.18/drivers/usb/net"
BUILD_ROOT="$ROOT/build/kernel"
CACHE_DIR="$BUILD_ROOT/cache"
TREE_DIR="$BUILD_ROOT/linux-2.6.18"
TARBALL="$CACHE_DIR/linux-2.6.18.tar.bz2"
PYTHON="${PYTHON:-python3}"

mkdir -p "$CACHE_DIR" "$BUILD_ROOT"

if [ ! -f "$TARBALL" ]; then
  curl -L --fail -o "$TARBALL" \
    https://cdn.kernel.org/pub/linux/kernel/v2.6/linux-2.6.18.tar.bz2
fi

rm -rf "$TREE_DIR"

"$PYTHON" - <<'PY' "$TARBALL" "$BUILD_ROOT"
import io
import sys
import tarfile
from pathlib import Path

tarball = Path(sys.argv[1])
build_root = Path(sys.argv[2])
data = tarball.read_bytes()
with tarfile.open(fileobj=io.BytesIO(data), mode="r:bz2") as tf:
    tf.extractall(build_root)
PY

cp "$CONFIG_PATH" "$TREE_DIR/.config"

"$PYTHON" - <<'PY' "$TREE_DIR" "$VENDOR_NET_DIR"
import sys
from pathlib import Path

tree = Path(sys.argv[1])
vendor = Path(sys.argv[2])

makefile = tree / "Makefile"
text = makefile.read_text(encoding="utf-8")
text = text.replace("EXTRAVERSION =", "EXTRAVERSION = _pro500", 1)
text = text.replace(
    "config %config: scripts_basic outputmakefile FORCE\n"
    "\t$(Q)mkdir -p include/linux include/config\n"
    "\t$(Q)$(MAKE) $(build)=scripts/kconfig $@\n",
    "config: scripts_basic outputmakefile FORCE\n"
    "\t$(Q)mkdir -p include/linux include/config\n"
    "\t$(Q)$(MAKE) $(build)=scripts/kconfig $@\n\n"
    "%config: scripts_basic outputmakefile FORCE\n"
    "\t$(Q)mkdir -p include/linux include/config\n"
    "\t$(Q)$(MAKE) $(build)=scripts/kconfig $@\n"
)
text = text.replace(
    "/ %/: prepare scripts FORCE\n"
    "\t$(Q)$(MAKE) KBUILD_MODULES=$(if $(CONFIG_MODULES),1) \\\n"
    "\t$(build)=$(build-dir)\n",
    "/: prepare scripts FORCE\n"
    "\t$(Q)$(MAKE) KBUILD_MODULES=$(if $(CONFIG_MODULES),1) \\\n"
    "\t$(build)=$(build-dir)\n\n"
    "%/: prepare scripts FORCE\n"
    "\t$(Q)$(MAKE) KBUILD_MODULES=$(if $(CONFIG_MODULES),1) \\\n"
    "\t$(build)=$(build-dir)\n"
)
makefile.write_text(text, encoding="utf-8")

vermagic = tree / "include/linux/vermagic.h"
text = vermagic.read_text(encoding="utf-8")
old = '"gcc-" __stringify(__GNUC__) "." __stringify(__GNUC_MINOR__)'
new = '"gcc-4.2"'
if old not in text:
    raise SystemExit("vermagic gcc string not found")
vermagic.write_text(text.replace(old, new), encoding="utf-8")

compiler_h = tree / "include/linux/compiler.h"
text = compiler_h.read_text(encoding="utf-8")
old = """#if __GNUC__ > 4\n#error no compiler-gcc.h file for this gcc version\n#elif __GNUC__ == 4\n# include <linux/compiler-gcc4.h>\n"""
new = """#if __GNUC__ > 4\n# include <linux/compiler-gcc4.h>\n#elif __GNUC__ == 4\n# include <linux/compiler-gcc4.h>\n"""
if old not in text:
    raise SystemExit("compiler.h gcc guard not found")
compiler_h.write_text(text.replace(old, new), encoding="utf-8")

module_h = tree / "include/asm-mips/module.h"
text = module_h.read_text(encoding="utf-8")
old = '#define MODULE_PROC_FAMILY "R4X00 "'
new = '#define MODULE_PROC_FAMILY "MIPS32_R2 "'
if old not in text:
    raise SystemExit("module.h R4X00 proc family marker not found")
module_h.write_text(text.replace(old, new), encoding="utf-8")

sumversion = tree / "scripts/mod/sumversion.c"
text = sumversion.read_text(encoding="utf-8")
if "#include <limits.h>" not in text:
    text = text.replace("#include <ctype.h>\n", "#include <ctype.h>\n#include <limits.h>\n", 1)
sumversion.write_text(text, encoding="utf-8")

config_path = tree / ".config"
config_lines = config_path.read_text(encoding="utf-8").splitlines()
replacements = {
    "CONFIG_USB_NET_RNDIS_HOST": "CONFIG_USB_NET_RNDIS_HOST=m",
    "CONFIG_USB_NET_CDCETHER": "CONFIG_USB_NET_CDCETHER=m",
    "CONFIG_LOCALVERSION": 'CONFIG_LOCALVERSION=".default"',
    "CONFIG_USB_USBNET": "CONFIG_USB_USBNET=y",
    "CONFIG_MII": "CONFIG_MII=y",
}
seen = set()
out = []
for line in config_lines:
    replaced = False
    for key, value in replacements.items():
        if line.startswith(key + "=") or line.startswith("# " + key + " is not set"):
            out.append(value)
            seen.add(key)
            replaced = True
            break
    if not replaced:
        out.append(line)
for key, value in replacements.items():
    if key not in seen:
        out.append(value)
config_path.write_text("\n".join(out) + "\n", encoding="utf-8")

dst = tree / "drivers/usb/net"
for name in ["rndis_host.c", "cdc_ether.c", "usbnet.h", "Kconfig", "Makefile"]:
    (dst / name).write_bytes((vendor / name).read_bytes())

net_makefile = dst / "Makefile"
text = net_makefile.read_text(encoding="utf-8")
force_block = """

ifeq ($(AOC_TETHER_FORCE_MODULES),1)
obj-m += rndis_host.o
obj-m += cdc_ether.o
endif
"""
if "AOC_TETHER_FORCE_MODULES" not in text:
    text = text.rstrip() + force_block
net_makefile.write_text(text, encoding="utf-8")
PY

yes "" | make -C "$TREE_DIR" ARCH=mips CROSS_COMPILE=mips-linux-gnu- oldconfig
make -C "$TREE_DIR" ARCH=mips CROSS_COMPILE=mips-linux-gnu- prepare modules_prepare scripts

echo "$TREE_DIR"
