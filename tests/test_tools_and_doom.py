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
    app = (ROOT / "tools/build_app_wsl.sh").read_text(encoding="ascii")
    doom = (ROOT / "examples/doom/build_doom_wsl.sh").read_text(encoding="ascii")

    assert 'SYSROOT="$ROOT/third_party/sysroots/mips_tv"' in sdk
    assert "mips-linux-gnu-gcc" in sdk
    assert "mips-linux-gnu-ar" in sdk
    assert "src/aoc_fb.c" in sdk

    assert 'LIBAOC="$ROOT/build/libaoc/libaoc.a"' in app
    assert "usage: build_app_wsl.sh <source.c> <output>" in app
    assert "-nostartfiles" in app
    assert "--dynamic-linker=/lib/ld-uClibc.so.0" in app

    assert 'LIBAOC="$ROOT/build/libaoc/libaoc.a"' in doom
    assert 'DOOM_DIR="$ROOT/third_party/doomgeneric/doomgeneric"' in doom
    assert 'PORT_DIR="$ROOT/examples/doom"' in doom
    assert "$ROOT/tools/build_libaoc_wsl.sh" in doom
    assert "-nostartfiles" in doom
    assert "--dynamic-linker=/lib/ld-uClibc.so.0" in doom


def test_psb_tool_uses_libaoc_names_and_confirmed_launcher_constants(tmp_path: Path) -> None:
    module = load_module(ROOT / "tools/make_psb.py")

    assert module.build_command() == "chmod +x /mnt/doom/launch.sh; /mnt/doom/launch.sh #"
    result = module.build_doom_launcher(out_dir=tmp_path)

    assert result["variant"] == "libaocdoom"
    assert result["system"] == 0x2BC07240
    assert result["call_gadget"] == 0x2BC02780
    assert (tmp_path / "libaocdoom.psb").exists()
    assert (tmp_path / "libaocdoom.avi").exists()
    assert not (tmp_path / "PSB60_LAUNCH_DOOM.psb").exists()


def test_psb_tool_builds_core_crash_and_custom_command_payloads(tmp_path: Path) -> None:
    module = load_module(ROOT / "tools/make_psb.py")

    core = module.build_core_crash(out_dir=tmp_path)
    custom = module.build_command_payload(
        command="echo OK>/etc/core/libaoc_cmd.ok",
        out_dir=tmp_path,
        base_name="libaoctest",
    )

    assert core["variant"] == "libaoccore"
    assert core["text_len"] == 344
    assert (tmp_path / "libaoccore.psb").read_bytes().startswith(b"{0:0:1}{0:0:10}")
    assert (tmp_path / "libaoccore.avi").exists()

    assert custom["variant"] == "libaoctest"
    assert custom["command"] == "echo OK>/etc/core/libaoc_cmd.ok #"
    assert b"echo OK>/etc/core/libaoc_cmd.ok #" in (tmp_path / "libaoctest.psb").read_bytes()
    assert (tmp_path / "libaoctest.avi").exists()


def test_core_addresses_tool_derives_runtime_addresses_from_values() -> None:
    module = load_module(ROOT / "tools/core_addresses.py")

    result = module.derive_addresses(libc_base=0x2BBB8000, sp=0x7C6E5D40)

    assert result["libc_base"] == 0x2BBB8000
    assert result["sp"] == 0x7C6E5D40
    assert result["system"] == 0x2BC07240
    assert result["implicit_a0_gadget"] == 0x2BC02780
    assert result["xdrrec_eof_gadget"] == 0x2BBFAA44
    assert result["command_ptr"] == 0x7C6E5DE4


def test_usb_tree_tool_copies_ready_to_run_doom_payload(tmp_path: Path) -> None:
    module = load_module(ROOT / "tools/make_usb_tree.py")
    (tmp_path / "PSB60_LAUNCH_DOOM.avi").write_text("stale", encoding="ascii")
    (tmp_path / "PSB60_LAUNCH_DOOM.psb").write_text("stale", encoding="ascii")

    tree = module.build_tree(tmp_path, include_psb=True)

    assert tree == tmp_path / "doom"
    assert (tree / "doom").exists()
    assert (tree / "doom1.wad").exists()
    assert (tree / "launch.sh").exists()
    assert (tree / "README.md").exists()
    assert (tmp_path / "libaocdoom.avi").exists()
    assert (tmp_path / "libaocdoom.psb").exists()
    assert (tmp_path / "libaoccore.avi").exists()
    assert (tmp_path / "libaoccore.psb").exists()
    assert not (tmp_path / "PSB60_LAUNCH_DOOM.avi").exists()
    assert not (tmp_path / "PSB60_LAUNCH_DOOM.psb").exists()


def test_makefile_exposes_custom_command_psb_and_generic_app_build() -> None:
    text = (ROOT / "Makefile").read_text(encoding="ascii")

    assert "cmdpsb:" in text
    assert "CMD ?=" in text
    assert "BASE ?= libaoccmd" in text
    assert "if [ -n \"$(CMD)\" ]; then" in text
    assert "tools/make_psb.py command --command" in text
    assert "corepsb:" in text
    assert "tools/make_psb.py core-crash" in text
    assert "app:" in text
    assert "tools/build_app_wsl.sh" in text


def test_doom_example_adapter_uses_libaoc_not_raw_platform_paths() -> None:
    text = (ROOT / "examples/doom/doomgeneric_lc32d1320.c").read_text(encoding="ascii")

    assert '#include "aoc/aoc.h"' in text
    assert "aoc_fb_open(&g_fb, NULL)" in text
    assert "aoc_fb_present_xrgb8888_scaled(&g_fb" in text
    assert "aoc_input_poll(&g_input, g_draw_frame_count)" in text
    assert "AOC_INPUT_FORWARD" in text
    assert '"/dev/hidtv2dge"' not in text
    assert '"/tmp/hp_dfb_handler"' not in text
