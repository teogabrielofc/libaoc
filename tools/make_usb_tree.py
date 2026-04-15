from __future__ import annotations

import argparse
import shutil
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PAYLOAD_ROOT = ROOT / "artifacts" / "usb" / "doom"
PSB_ROOT = ROOT / "artifacts" / "psb"


def copy_file(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def build_tree(outdir: Path | str, include_psb: bool = False) -> Path:
    out = Path(outdir)
    destination = out / "doom"
    destination.mkdir(parents=True, exist_ok=True)

    for name in ["doom", "doom1.wad", "launch.sh", "README.md"]:
        copy_file(PAYLOAD_ROOT / name, destination / name)

    if include_psb:
        copy_file(PSB_ROOT / "PSB60_LAUNCH_DOOM.avi", out / "PSB60_LAUNCH_DOOM.avi")
        copy_file(PSB_ROOT / "PSB60_LAUNCH_DOOM.psb", out / "PSB60_LAUNCH_DOOM.psb")

    return destination


def main() -> None:
    parser = argparse.ArgumentParser(description="Build a USB tree for the AOC Doom launcher.")
    parser.add_argument("--out", type=Path, default=ROOT / "dist" / "usb")
    parser.add_argument("--include-psb", action="store_true")
    args = parser.parse_args()

    print(build_tree(args.out, args.include_psb))


if __name__ == "__main__":
    main()
