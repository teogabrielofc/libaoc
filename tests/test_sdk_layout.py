from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_libaoc_public_api_and_platform_constants() -> None:
    text = (ROOT / "include/aoc/aoc.h").read_text(encoding="ascii")

    assert 'AOC_FB_DEFAULT_DEVICE "/dev/hidtv2dge"' in text
    assert 'AOC_INPUT_SOCKET_PATH "/tmp/hp_dfb_handler"' in text
    assert 'AOC_REMOTE_DEVICE_PATH "/dev/remote"' in text
    assert "enum aoc_input_action" in text
    assert "AOC_INPUT_VOLUP" in text
    assert "AOC_INPUT_VOLDOWN" in text
    assert "AOC_INPUT_MENU" in text
    assert "AOC_INPUT_CHUP" in text
    assert "AOC_INPUT_CHDOWN" in text
    assert "AOC_INPUT_INPUT" in text
    assert "AOC_INPUT_FORWARD = AOC_INPUT_VOLUP" in text
    assert "AOC_INPUT_BACKWARD = AOC_INPUT_VOLDOWN" in text
    assert "AOC_INPUT_FIRE = AOC_INPUT_MENU" in text
    assert "AOC_INPUT_TURN_LEFT = AOC_INPUT_CHUP" in text
    assert "AOC_INPUT_TURN_RIGHT = AOC_INPUT_CHDOWN" in text
    assert "AOC_INPUT_USE = AOC_INPUT_INPUT" in text
    assert "int aoc_fb_open(struct aoc_fb *fb, const char *device_path);" in text
    assert "void aoc_fb_present_xrgb8888_scaled(" in text
    assert "int aoc_input_open(struct aoc_input *input);" in text
    assert "enum aoc_input_action aoc_input_translate_raw(uint32_t raw);" in text


def test_libaoc_sources_include_confirmed_button_map_and_fast_fb_mode() -> None:
    fb = (ROOT / "src/aoc_fb.c").read_text(encoding="ascii")
    inp = (ROOT / "src/aoc_input.c").read_text(encoding="ascii")
    log = (ROOT / "src/aoc_log.c").read_text(encoding="ascii")

    assert "getenv(\"AOC_FB_PAGES\")" in fb
    assert "\"all\"" in fb
    assert "present_pages = 1U" in fb
    assert "AOC_FB_FULL_REFRESH_EVERY" in fb
    assert "FBIOGET_FSCREENINFO" in fb
    assert "FBIOGET_VSCREENINFO" in fb

    assert "REMOTE_KEY_SOCKET_VOL_UP 0x0000003CU" in inp
    assert "REMOTE_KEY_SOCKET_VOL_DOWN 0x0000003DU" in inp
    assert "REMOTE_KEY_SOCKET_MENU 0x00010319U" in inp
    assert "REMOTE_KEY_SOCKET_CH_UP 0x00010316U" in inp
    assert "REMOTE_KEY_SOCKET_CH_DOWN 0x00010317U" in inp
    assert "REMOTE_KEY_SOCKET_INPUT_1 0x00010318U" in inp
    assert "REMOTE_KEY_SOCKET_INPUT_2 0x0000003EU" in inp
    assert "REMOTE_KEY_SOCKET_INPUT_3 0x00100017U" in inp
    assert "getenv(\"AOC_INPUT_DEBUG\")" in inp

    assert "void aoc_log_set_fsync(int enabled)" in log
    assert "fsync(fd);" in log
