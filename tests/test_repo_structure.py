from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_repository_has_sdk_docs_and_artifact_layout() -> None:
    required = [
        "README.md",
        "README.en.md",
        "Makefile",
        ".gitignore",
        "requirements.txt",
        "include/aoc/aoc.h",
        "src/aoc_fb.c",
        "src/aoc_input.c",
        "src/aoc_log.c",
        "src/aoc_runtime.c",
        "runtime/start.S",
        "runtime/appinit.c",
        "runtime/uclibc_compat.c",
        "docs/quickstart.md",
        "docs/quickstart.en.md",
        "docs/architecture.md",
        "docs/architecture.en.md",
        "docs/psb-launcher.md",
        "docs/psb-launcher.en.md",
        "docs/troubleshooting.md",
        "docs/troubleshooting.en.md",
        "tools/build_libaoc_wsl.sh",
        "tools/build_app_wsl.sh",
        "tools/core_addresses.py",
        "tools/make_psb.py",
        "tools/make_usb_tree.py",
        "examples/doom/build_doom_wsl.sh",
        "examples/doom/launch.sh",
        "examples/doom/doomgeneric_lc32d1320.c",
        "examples/hello/hello.c",
        "third_party/sysroots/mips_tv/lib/ld-uClibc.so.0",
        "third_party/doomgeneric/doomgeneric/doomgeneric.c",
        "third_party/doomgeneric/doom1.wad",
        "artifacts/usb/doom/doom",
        "artifacts/usb/doom/doom1.wad",
        "artifacts/usb/doom/launch.sh",
        "artifacts/psb/libaocdoom.psb",
        "artifacts/psb/libaocdoom.avi",
        "artifacts/psb/libaoccore.psb",
        "artifacts/psb/libaoccore.avi",
    ]

    missing = [path for path in required if not (ROOT / path).exists()]
    assert missing == []


def test_gitignore_keeps_build_outputs_out_but_allows_selected_artifacts() -> None:
    text = (ROOT / ".gitignore").read_text(encoding="ascii")

    assert "build/" in text
    assert "dist/" in text
    assert "__pycache__/" in text
    assert "!artifacts/usb/doom/doom" in text
    assert "!artifacts/psb/libaocdoom.psb" in text
    assert "!artifacts/psb/libaoccore.psb" in text


def test_readme_is_actionable_for_usb_launch() -> None:
    text = (ROOT / "README.md").read_text(encoding="utf-8")
    english = (ROOT / "README.en.md").read_text(encoding="utf-8")

    assert "TVs AOC" in text
    assert "LC32D1320" in text
    assert "Início rápido" in text
    assert "make doom" in text
    assert "make usb" in text
    assert "make psb" in text
    assert "make corepsb" in text
    assert "make cmdpsb CMD=" in text
    assert "AOC_FB_PAGES=1" in text
    assert "AOC_FB_PAGES=all" in text
    assert "libaocdoom" in text
    assert "libaoccore" in text
    assert "Quick start" in english
    assert "AOC TVs" in english
    assert "LC32D1320" in english
    assert "Custom command PSB" in english
    assert "libaocdoom" in english
