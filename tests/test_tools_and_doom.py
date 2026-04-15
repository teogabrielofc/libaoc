from pathlib import Path
import importlib.util


ROOT = Path(__file__).resolve().parents[1]


def load_module(path: Path):
    spec = importlib.util.spec_from_file_location(path.stem, path)
    assert spec is not None
    assert spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_build_scripts_use_standalone_repo_paths() -> None:
    sdk = (ROOT / "tools/build_libaoc_wsl.sh").read_text(encoding="ascii")
    doom = (ROOT / "examples/doom/build_doom_wsl.sh").read_text(encoding="ascii")

    assert 'SYSROOT="$ROOT/third_party/sysroots/mips_tv"' in sdk
    assert "mips-linux-gnu-gcc" in sdk
    assert "mips-linux-gnu-ar" in sdk
    assert "src/aoc_fb.c" in sdk

    assert 'LIBAOC="$ROOT/build/libaoc/libaoc.a"' in doom
    assert 'DOOM_DIR="$ROOT/third_party/doomgeneric/doomgeneric"' in doom
    assert 'PORT_DIR="$ROOT/examples/doom"' in doom
    assert "$ROOT/tools/build_libaoc_wsl.sh" in doom
    assert "-nostartfiles" in doom
    assert "--dynamic-linker=/lib/ld-uClibc.so.0" in doom


def test_psb_tool_uses_confirmed_launcher_payload_constants(tmp_path: Path) -> None:
    module = load_module(ROOT / "tools/make_psb.py")

    assert module.build_command() == "chmod +x /mnt/doom/launch.sh; /mnt/doom/launch.sh #"
    result = module.build_doom_launcher(out_dir=tmp_path)

    assert result["variant"] == "PSB60_LAUNCH_DOOM"
    assert result["system"] == 0x2BC07240
    assert result["call_gadget"] == 0x2BC02780
    assert (tmp_path / "PSB60_LAUNCH_DOOM.psb").exists()
    assert (tmp_path / "PSB60_LAUNCH_DOOM.avi").exists()


def test_usb_tree_tool_copies_ready_to_run_doom_payload(tmp_path: Path) -> None:
    module = load_module(ROOT / "tools/make_usb_tree.py")
    tree = module.build_tree(tmp_path)

    assert tree == tmp_path / "doom"
    assert (tree / "doom").exists()
    assert (tree / "doom1.wad").exists()
    assert (tree / "launch.sh").exists()
    assert (tree / "README.md").exists()


def test_doom_example_adapter_uses_libaoc_not_raw_platform_paths() -> None:
    text = (ROOT / "examples/doom/doomgeneric_lc32d1320.c").read_text(encoding="ascii")

    assert '#include "aoc/aoc.h"' in text
    assert "aoc_fb_open(&g_fb, NULL)" in text
    assert "aoc_fb_present_xrgb8888_scaled(&g_fb" in text
    assert "aoc_input_poll(&g_input, g_draw_frame_count)" in text
    assert "AOC_INPUT_FORWARD" in text
    assert '"/dev/hidtv2dge"' not in text
    assert '"/tmp/hp_dfb_handler"' not in text
