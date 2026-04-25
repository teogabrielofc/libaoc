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
    assert "struct aoc_input_raw_event" in text
    assert "struct aoc_usb_kbd_event" in text
    assert "struct aoc_usb_kbd" in text
    assert "error_streak" in text
    assert "trid_bind_state" in text
    assert "trid_last_errno" in text
    assert "trid_packet_count" in text
    assert "trid_last_kind" in text
    assert "trid_last_code" in text
    assert "int aoc_fb_open(struct aoc_fb *fb, const char *device_path);" in text
    assert "void aoc_fb_present_xrgb8888_scaled(" in text
    assert "int aoc_input_open(struct aoc_input *input);" in text
    assert "int aoc_input_pop_raw(struct aoc_input *input, struct aoc_input_raw_event *event);" in text
    assert "int aoc_usb_kbd_open(struct aoc_usb_kbd *kbd);" in text
    assert "int aoc_usb_kbd_poll(struct aoc_usb_kbd *kbd);" in text
    assert "int aoc_usb_kbd_pop_event(struct aoc_usb_kbd *kbd, struct aoc_usb_kbd_event *event);" in text
    assert "int aoc_usb_kbd_translate_event(const struct aoc_usb_kbd_event *event, char *out, unsigned int out_cap);" in text
    assert "void aoc_usb_kbd_close(struct aoc_usb_kbd *kbd);" in text
    assert "enum aoc_input_action aoc_input_translate_raw(uint32_t raw);" in text


def test_libaoc_sources_include_confirmed_button_map_and_fast_fb_mode() -> None:
    fb = (ROOT / "src/aoc_fb.c").read_text(encoding="ascii")
    inp = (ROOT / "src/aoc_input.c").read_text(encoding="ascii")
    log = (ROOT / "src/aoc_log.c").read_text(encoding="ascii")
    usb = (ROOT / "src/aoc_usb_kbd.c").read_text(encoding="ascii")

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
    assert "queue_raw_input_event(" in inp
    assert "note_trid_raw_event(" in inp
    assert "aoc_input_pop_raw(" in inp
    assert "input->trid_bind_state = 1;" in inp
    assert "input->trid_bind_state = -1;" in inp
    assert "input->trid_last_errno" in inp
    assert "input->trid_packet_count" in inp
    assert "input->trid_last_kind" in inp
    assert "input->trid_last_code" in inp

    assert '"/proc/bus/usb/devices"' in usb
    assert '"/proc/bus/usb/%03u/%03u"' in usb or '"/proc/bus/usb/%03d/%03d"' in usb
    assert "USBDEVFS_GETDRIVER" in usb
    assert "USBDEVFS_DISCONNECT_CLAIM" in usb
    assert "USBDEVFS_SUBMITURB" in usb
    assert "USBDEVFS_REAPURBNDELAY" in usb
    assert "USBDEVFS_CLAIMINTERFACE" in usb
    assert "Cls=03" in usb
    assert "Sub=01" in usb
    assert "Prot=01" in usb
    assert "aoc_usb_kbd_open(" in usb
    assert "aoc_usb_kbd_poll(" in usb
    assert "aoc_usb_kbd_pop_event(" in usb
    assert "aoc_usb_kbd_translate_event(" in usb
    assert "aoc_usb_kbd_close(" in usb
    assert "kbd->error_streak" in usb
    assert "case 0x35U:" in usb
    assert "case 0x64U:" in usb
    assert "case 0x87U:" in usb
    assert "altgr_down" in usb
    assert "access(kbd->device_path, F_OK)" in usb or "path_exists(kbd->device_path)" in usb
    assert "if (!kbd->urb_submitted)" in usb
    assert "priv->urb.status != 0" in usb
    assert "if (path_exists(kbd->device_path))" in usb

    assert "void aoc_log_set_fsync(int enabled)" in log
    assert "fsync(fd);" in log


def test_kernel_tether_staging_layout_exists() -> None:
    readme = (ROOT / "kernel" / "README.md").read_text(encoding="utf-8")
    build = (ROOT / "kernel" / "scripts" / "build_tether_modules_wsl.sh").read_text(encoding="utf-8")
    prepare = (ROOT / "kernel" / "scripts" / "prepare_kernel_tree_wsl.sh").read_text(encoding="utf-8")
    config = (ROOT / "kernel" / "config" / "lc32d1320-2.6.18_pro500.default.config").read_text(encoding="utf-8")
    rndis = (ROOT / "kernel" / "vendor" / "linux-2.6.18" / "drivers" / "usb" / "net" / "rndis_host.c").read_text(encoding="utf-8")
    cdc = (ROOT / "kernel" / "vendor" / "linux-2.6.18" / "drivers" / "usb" / "net" / "cdc_ether.c").read_text(encoding="utf-8")

    assert "rndis_host" in readme
    assert "cdc_ether" in readme
    assert "2.6.18_pro500.default" in readme
    assert "gcc-4.2" in readme
    assert "CONFIG_USB_NET_RNDIS_HOST" in prepare
    assert "CONFIG_USB_NET_CDCETHER" in prepare
    assert "AOC_TETHER_FORCE_MODULES" in prepare
    assert "EXTRAVERSION = _pro500" in prepare
    assert 'MODULE_PROC_FAMILY "MIPS32_R2 "' in prepare
    assert "gcc-4.2" in prepare
    assert "rndis_host.ko" in build
    assert "cdc_ether.ko" in build
    assert "M=\"$AOC_MODULE_DIR\"" in build
    assert "AOC_TETHER_FORCE_MODULES=1" in build
    assert "CONFIG_USB_NET_RNDIS_HOST is not set" in config
    assert "CONFIG_USB_NET_CDCETHER is not set" in config
    assert "Host Side support for RNDIS Networking Links" in rndis
    assert "CDC Ethernet based networking peripherals" in cdc
