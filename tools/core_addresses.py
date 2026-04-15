from __future__ import annotations

import argparse
import json
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import BinaryIO


DEFAULT_LIBC_BASE = 0x2BBB8000
DEFAULT_SYSTEM = 0x2BC07240
DEFAULT_IMPLICIT_A0_GADGET = 0x2BC02780
R_DEBUG_ADDR = 0x2AAAD000

LIBC_SYSTEM_OFF = DEFAULT_SYSTEM - DEFAULT_LIBC_BASE
LIBC_IMPLICIT_A0_GADGET_OFF = DEFAULT_IMPLICIT_A0_GADGET - DEFAULT_LIBC_BASE
LIBC_XDRREC_EOF_GADGET_OFF = 0x42A44
STACK_CMD_PTR_OFF = 0xA4


@dataclass(frozen=True)
class LinkMapEntry:
    base: int
    name: str


@dataclass(frozen=True)
class RuntimeInfo:
    libc_base: int
    sp: int | None = None
    link_map: tuple[LinkMapEntry, ...] = ()

    @property
    def system(self) -> int:
        return self.libc_base + LIBC_SYSTEM_OFF

    @property
    def implicit_a0_gadget(self) -> int:
        return self.libc_base + LIBC_IMPLICIT_A0_GADGET_OFF

    @property
    def xdrrec_eof_gadget(self) -> int:
        return self.libc_base + LIBC_XDRREC_EOF_GADGET_OFF

    @property
    def command_ptr(self) -> int | None:
        if self.sp is None:
            return None
        return self.sp + STACK_CMD_PTR_OFF


def derive_addresses(libc_base: int = DEFAULT_LIBC_BASE, sp: int | None = None) -> dict[str, int | None]:
    runtime = RuntimeInfo(libc_base=libc_base, sp=sp)
    return {
        "libc_base": runtime.libc_base,
        "sp": runtime.sp,
        "system": runtime.system,
        "implicit_a0_gadget": runtime.implicit_a0_gadget,
        "xdrrec_eof_gadget": runtime.xdrrec_eof_gadget,
        "command_ptr": runtime.command_ptr,
    }


def _pt_loads(elf) -> list[tuple[int, int, int]]:
    return [
        (seg["p_vaddr"], seg["p_vaddr"] + seg["p_filesz"], seg["p_offset"])
        for seg in elf.iter_segments()
        if seg["p_type"] == "PT_LOAD"
    ]


def _read_va(fh: BinaryIO, loads: list[tuple[int, int, int]], va: int, size: int) -> bytes:
    for start, end, file_off in loads:
        if start <= va and va + size <= end:
            fh.seek(file_off + (va - start))
            return fh.read(size)
    raise KeyError(f"endereco virtual nao esta no core: {va:#x}")


def _read_u32(fh: BinaryIO, loads: list[tuple[int, int, int]], va: int) -> int:
    return struct.unpack(">I", _read_va(fh, loads, va, 4))[0]


def _read_cstr(fh: BinaryIO, loads: list[tuple[int, int, int]], va: int, max_len: int = 256) -> str:
    out = bytearray()
    for i in range(max_len):
        b = _read_va(fh, loads, va + i, 1)
        if b == b"\x00":
            break
        out += b
    return out.decode("latin1", "replace")


def _extract_sp(elf) -> int | None:
    for seg in elf.iter_segments():
        if seg["p_type"] != "PT_NOTE":
            continue
        for note in seg.iter_notes():
            if note["n_type"] != "NT_PRSTATUS":
                continue
            desc = note["n_desc"]
            if not isinstance(desc, (bytes, bytearray)):
                continue
            regs = [
                struct.unpack(">I", desc[0x60 + i * 4 : 0x64 + i * 4])[0]
                for i in range((len(desc) - 0x60) // 4)
            ]
            if len(regs) > 29:
                return regs[29]
    return None


def extract_runtime_info(core_path: Path | str) -> RuntimeInfo:
    from elftools.elf.elffile import ELFFile

    path = Path(core_path)
    with path.open("rb") as fh:
        elf = ELFFile(fh)
        loads = _pt_loads(elf)
        r_map = _read_u32(fh, loads, R_DEBUG_ADDR + 0x4)

        entries: list[LinkMapEntry] = []
        libc_base = None
        cur = r_map
        seen: set[int] = set()
        while cur and cur not in seen:
            seen.add(cur)
            l_addr = _read_u32(fh, loads, cur + 0x0)
            l_name_ptr = _read_u32(fh, loads, cur + 0x4)
            l_next = _read_u32(fh, loads, cur + 0xC)
            name = _read_cstr(fh, loads, l_name_ptr) if l_name_ptr else ""
            entries.append(LinkMapEntry(base=l_addr, name=name))
            if name.endswith("/libc.so.0") or name == "libc.so.0":
                libc_base = l_addr
            cur = l_next

        if libc_base is None:
            raise RuntimeError("nao consegui recuperar a base da libc no link_map")

        return RuntimeInfo(libc_base=libc_base, sp=_extract_sp(elf), link_map=tuple(entries))


def _parse_int(value: str) -> int:
    return int(value, 0)


def _format_hex(value: int | None) -> str:
    return "none" if value is None else f"{value:#010x}"


def _print_text(runtime: RuntimeInfo) -> None:
    data = derive_addresses(runtime.libc_base, runtime.sp)
    for key, value in data.items():
        print(f"{key}={_format_hex(value)}")
    if runtime.link_map:
        print("link_map:")
        for entry in runtime.link_map:
            print(f"  {entry.base:#010x} {entry.name}")


def _json_ready(runtime: RuntimeInfo) -> dict[str, object]:
    data = derive_addresses(runtime.libc_base, runtime.sp)
    data["link_map"] = [{"base": entry.base, "name": entry.name} for entry in runtime.link_map]
    return data


def main() -> None:
    parser = argparse.ArgumentParser(description="Extrai enderecos uteis de um core ELF MIPS da TV.")
    parser.add_argument("core", nargs="?", type=Path, help="core.* gerado pela TV")
    parser.add_argument("--libc-base", type=_parse_int, default=None, help="usa uma base manual, ex: 0x2bbb8000")
    parser.add_argument("--sp", type=_parse_int, default=None, help="usa um SP manual, ex: 0x7c6e5d40")
    parser.add_argument("--json", action="store_true", help="imprime JSON em vez de key=value")
    args = parser.parse_args()

    if args.core:
        runtime = extract_runtime_info(args.core)
    else:
        runtime = RuntimeInfo(libc_base=args.libc_base or DEFAULT_LIBC_BASE, sp=args.sp)

    if args.json:
        print(json.dumps(_json_ready(runtime), indent=2, sort_keys=True))
    else:
        _print_text(runtime)


if __name__ == "__main__":
    main()
