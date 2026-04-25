#include "aoc/aoc.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/usb/ch9.h>
#include <linux/usbdevice_fs.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define AOC_USB_KBD_DEVICES_PATH "/proc/bus/usb/devices"
#define AOC_USB_KBD_RAW_FMT "/proc/bus/usb/%03u/%03u"
#define AOC_USB_KBD_TEXT_CAP 8192U
#define AOC_USB_KBD_CFG_CAP 512U
#define AOC_USB_KBD_PRIV_REPORT_CAP 16U
#define USB_HID_REQ_SET_IDLE 0x0A
#define USB_HID_REQ_SET_PROTOCOL 0x0B

#ifndef USB_DT_HID
#define USB_DT_HID 0x21
#endif

struct aoc_usb_kbd_private {
    struct usbdevfs_urb urb;
    unsigned char data[AOC_USB_KBD_PRIV_REPORT_CAP];
    unsigned char previous_report[AOC_USB_KBD_REPORT_CAP];
};

struct aoc_usb_kbd_candidate {
    int found;
    unsigned int bus;
    unsigned int dev;
    unsigned int iface_number;
    unsigned int endpoint;
    unsigned int max_packet;
    char path[AOC_USB_KBD_PATH_CAP];
    char desc[AOC_USB_KBD_NAME_CAP];
};

static int starts_with(const char *text, const char *prefix) {
    return text != 0 && prefix != 0 && strncmp(text, prefix, strlen(prefix)) == 0;
}

static int path_exists(const char *path) {
    return path != 0 && access(path, F_OK) == 0;
}

static int contains_text(const char *text, const char *needle) {
    return text != 0 && needle != 0 && strstr(text, needle) != 0;
}

static const char *next_line(const char *line) {
    while (*line != '\0' && *line != '\n') {
        ++line;
    }
    if (*line == '\n') {
        ++line;
    }
    return line;
}

static void trim_copy(char *out, unsigned int out_cap, const char *text) {
    unsigned int len = 0;

    if (out == 0 || out_cap == 0U) {
        return;
    }
    if (text == 0) {
        out[0] = '\0';
        return;
    }
    while (text[len] != '\0' && text[len] != '\r' && text[len] != '\n') {
        ++len;
    }
    while (len != 0U && (text[len - 1U] == ' ' || text[len - 1U] == '\t')) {
        --len;
    }
    if (len >= out_cap) {
        len = out_cap - 1U;
    }
    memcpy(out, text, len);
    out[len] = '\0';
}

static int parse_uint_after(const char *text, const char *label, unsigned int *out) {
    const char *p;
    unsigned int value = 0;
    int saw_digit = 0;

    if (text == 0 || label == 0 || out == 0) {
        return 0;
    }
    p = strstr(text, label);
    if (p == 0) {
        return 0;
    }
    p += strlen(label);
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    while (*p >= '0' && *p <= '9') {
        value = (value * 10U) + (unsigned int)(*p - '0');
        saw_digit = 1;
        ++p;
    }
    if (!saw_digit) {
        return 0;
    }
    *out = value;
    return 1;
}

static unsigned int le16_at(const unsigned char *buf) {
    return (unsigned int)buf[0] | ((unsigned int)buf[1] << 8);
}

static int read_file_limited(const char *path, char *buf, unsigned int cap) {
    int fd;
    unsigned int used = 0;

    if (buf == 0 || cap == 0U) {
        return -1;
    }
    buf[0] = '\0';
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    while (used + 1U < cap) {
        ssize_t n = read(fd, buf + used, cap - used - 1U);

        if (n < 0) {
            close(fd);
            buf[0] = '\0';
            return -1;
        }
        if (n == 0) {
            break;
        }
        used += (unsigned int)n;
    }
    close(fd);
    buf[used] = '\0';
    return 0;
}

static int report_has_usage(const unsigned char *report, uint8_t usage) {
    unsigned int i;

    for (i = 2U; i < AOC_USB_KBD_REPORT_CAP; ++i) {
        if (report[i] == usage) {
            return 1;
        }
    }
    return 0;
}

static int queue_event(struct aoc_usb_kbd *kbd, int pressed, uint8_t usage) {
    unsigned int next;

    if (kbd == 0 || usage == 0U) {
        return 0;
    }
    next = (kbd->queue_head + 1U) % AOC_USB_KBD_QUEUE_CAP;
    if (next == kbd->queue_tail) {
        return 0;
    }
    kbd->queue[kbd->queue_head].pressed = pressed;
    kbd->queue[kbd->queue_head].usage = usage;
    kbd->queue[kbd->queue_head].modifiers = kbd->modifiers;
    kbd->queue_head = next;
    return 1;
}

static int usb_control_transfer(
    int fd,
    unsigned int request_type,
    unsigned int request,
    unsigned int descriptor_type,
    unsigned int descriptor_index,
    unsigned int w_index,
    void *data,
    unsigned int length
) {
    struct usbdevfs_ctrltransfer ctrl;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.bRequestType = (uint8_t)request_type;
    ctrl.bRequest = (uint8_t)request;
    ctrl.wValue = (uint16_t)(((descriptor_type & 0xffU) << 8) | (descriptor_index & 0xffU));
    ctrl.wIndex = (uint16_t)w_index;
    ctrl.wLength = (uint16_t)length;
    ctrl.timeout = 1000U;
    ctrl.data = data;
    return aoc_raw_ioctl(fd, USBDEVFS_CONTROL, &ctrl);
}

static int usb_control_get_descriptor(
    int fd,
    unsigned int request_type,
    unsigned int descriptor_type,
    unsigned int descriptor_index,
    unsigned int w_index,
    void *data,
    unsigned int length
) {
    return usb_control_transfer(
        fd,
        request_type,
        USB_REQ_GET_DESCRIPTOR,
        descriptor_type,
        descriptor_index,
        w_index,
        data,
        length
    );
}

static void finalize_candidate(
    struct aoc_usb_kbd_candidate *candidate,
    unsigned int block_bus,
    unsigned int block_dev,
    int have_busdev,
    int block_boot_keyboard,
    int block_keyboard,
    const char *block_desc
) {
    int matches;

    if (candidate == 0 || !have_busdev) {
        return;
    }
    matches = block_boot_keyboard || block_keyboard;
    if (!matches) {
        return;
    }
    if (!candidate->found) {
        candidate->found = 1;
        candidate->bus = block_bus;
        candidate->dev = block_dev;
        snprintf(candidate->path, sizeof(candidate->path), AOC_USB_KBD_RAW_FMT, block_bus, block_dev);
        snprintf(candidate->desc, sizeof(candidate->desc), "%s", block_desc != 0 ? block_desc : "");
        return;
    }
    {
        int current_score = 0;
        int candidate_score = 0;

        if (candidate->desc[0] != '\0') {
            if (contains_text(candidate->desc, "Keyboard") ||
                contains_text(candidate->desc, "keyboard") ||
                contains_text(candidate->desc, "kbd")) {
                current_score += 60;
            }
            if (contains_text(candidate->desc, "EHCI") ||
                contains_text(candidate->desc, "OHCI") ||
                contains_text(candidate->desc, "UHCI") ||
                contains_text(candidate->desc, "Hub") ||
                contains_text(candidate->desc, "hub")) {
                current_score -= 80;
            }
        }
        if (candidate->dev > 1U) {
            current_score += 10;
        }
        if (block_desc != 0 && block_desc[0] != '\0') {
            if (contains_text(block_desc, "Keyboard") ||
                contains_text(block_desc, "keyboard") ||
                contains_text(block_desc, "kbd")) {
                candidate_score += 60;
            }
            if (contains_text(block_desc, "EHCI") ||
                contains_text(block_desc, "OHCI") ||
                contains_text(block_desc, "UHCI") ||
                contains_text(block_desc, "Hub") ||
                contains_text(block_desc, "hub")) {
                candidate_score -= 80;
            }
        }
        if (block_dev > 1U) {
            candidate_score += 10;
        }
        if (candidate_score > current_score ||
            (candidate_score == current_score && block_dev >= candidate->dev)) {
            candidate->bus = block_bus;
            candidate->dev = block_dev;
            snprintf(candidate->path, sizeof(candidate->path), AOC_USB_KBD_RAW_FMT, block_bus, block_dev);
            snprintf(candidate->desc, sizeof(candidate->desc), "%s", block_desc != 0 ? block_desc : "");
        }
    }
}

static int scan_candidate(struct aoc_usb_kbd_candidate *candidate) {
    char text[AOC_USB_KBD_TEXT_CAP];
    const char *line;
    unsigned int block_bus = 0;
    unsigned int block_dev = 0;
    int have_busdev = 0;
    int block_keyboard = 0;
    int block_boot_keyboard = 0;
    char block_desc[AOC_USB_KBD_NAME_CAP];

    if (candidate == 0) {
        errno = EINVAL;
        return -1;
    }
    memset(candidate, 0, sizeof(*candidate));
    if (read_file_limited(AOC_USB_KBD_DEVICES_PATH, text, sizeof(text)) != 0) {
        return -1;
    }
    block_desc[0] = '\0';
    line = text;
    while (*line != '\0') {
        if (*line == '\n' || *line == '\r') {
            finalize_candidate(
                candidate,
                block_bus,
                block_dev,
                have_busdev,
                block_boot_keyboard,
                block_keyboard,
                block_desc
            );
            block_bus = 0;
            block_dev = 0;
            have_busdev = 0;
            block_keyboard = 0;
            block_boot_keyboard = 0;
            block_desc[0] = '\0';
            line = next_line(line);
            continue;
        }
        if (starts_with(line, "T:")) {
            unsigned int parsed_bus = 0;
            unsigned int parsed_dev = 0;

            if (parse_uint_after(line, "Bus=", &parsed_bus) &&
                parse_uint_after(line, "Dev#=", &parsed_dev)) {
                block_bus = parsed_bus;
                block_dev = parsed_dev;
                have_busdev = 1;
            }
        }
        if (starts_with(line, "I:")) {
            if (contains_text(line, "Cls=03") &&
                contains_text(line, "Sub=01") &&
                contains_text(line, "Prot=01")) {
                block_boot_keyboard = 1;
            }
        }
        if (starts_with(line, "S:  Product=") &&
            (contains_text(line, "Keyboard") || contains_text(line, "keyboard") || contains_text(line, "kbd"))) {
            block_keyboard = 1;
            if (block_desc[0] == '\0') {
                trim_copy(block_desc, sizeof(block_desc), line + 12);
            }
        }
        if (starts_with(line, "S:  Product=") && block_desc[0] == '\0') {
            trim_copy(block_desc, sizeof(block_desc), line + 12);
        }
        line = next_line(line);
    }
    finalize_candidate(
        candidate,
        block_bus,
        block_dev,
        have_busdev,
        block_boot_keyboard,
        block_keyboard,
        block_desc
    );
    if (!candidate->found) {
        errno = ENODEV;
        return -1;
    }
    return 0;
}

static void parse_config_descriptors(
    struct aoc_usb_kbd_candidate *candidate,
    const unsigned char *buf,
    unsigned int len
) {
    unsigned int offset = 0;
    int current_is_keyboard = 0;
    unsigned int current_iface = 0;

    if (candidate == 0 || buf == 0) {
        return;
    }
    while (offset + 2U <= len) {
        unsigned int desc_len = buf[offset];
        unsigned int desc_type = buf[offset + 1U];

        if (desc_len < 2U || offset + desc_len > len) {
            break;
        }
        if (desc_type == USB_DT_INTERFACE && desc_len >= 9U) {
            current_iface = buf[offset + 2U];
            current_is_keyboard =
                (buf[offset + 5U] == 3U) &&
                (buf[offset + 6U] == 1U) &&
                (buf[offset + 7U] == 1U);
            if (current_is_keyboard) {
                candidate->iface_number = current_iface;
            }
        } else if (current_is_keyboard && desc_type == USB_DT_ENDPOINT && desc_len >= 7U) {
            if ((buf[offset + 2U] & USB_DIR_IN) != 0U &&
                (buf[offset + 3U] & 0x03U) == 0x03U &&
                candidate->endpoint == 0U) {
                candidate->endpoint = buf[offset + 2U];
                candidate->max_packet = le16_at(buf + offset + 4U);
            }
        }
        offset += desc_len;
    }
}

static int inspect_candidate(struct aoc_usb_kbd_candidate *candidate) {
    unsigned char cfg_head[9];
    unsigned char cfg_desc[AOC_USB_KBD_CFG_CAP];
    int fd;
    int rc;
    unsigned int total_len = 0;

    if (candidate == 0 || !candidate->found || candidate->path[0] == '\0') {
        errno = ENODEV;
        return -1;
    }
    fd = open(candidate->path, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        fd = open(candidate->path, O_RDONLY | O_NONBLOCK);
    }
    if (fd < 0) {
        return -1;
    }
    rc = usb_control_get_descriptor(
        fd,
        USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
        USB_DT_CONFIG,
        0U,
        0U,
        cfg_head,
        sizeof(cfg_head)
    );
    if (rc < 0) {
        close(fd);
        return -1;
    }
    if ((unsigned int)rc >= 4U) {
        total_len = le16_at(cfg_head + 2U);
    }
    if (total_len < sizeof(cfg_head)) {
        total_len = sizeof(cfg_head);
    }
    if (total_len > sizeof(cfg_desc)) {
        total_len = sizeof(cfg_desc);
    }
    rc = usb_control_get_descriptor(
        fd,
        USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
        USB_DT_CONFIG,
        0U,
        0U,
        cfg_desc,
        total_len
    );
    if (rc < 0) {
        close(fd);
        return -1;
    }
    parse_config_descriptors(candidate, cfg_desc, (unsigned int)rc);
    close(fd);
    if (candidate->endpoint == 0U) {
        errno = ENODEV;
        return -1;
    }
    return 0;
}

static int submit_urb(struct aoc_usb_kbd *kbd) {
    struct aoc_usb_kbd_private *priv;

    if (kbd == 0 || kbd->fd < 0 || kbd->priv == 0) {
        errno = EINVAL;
        return -1;
    }
    priv = (struct aoc_usb_kbd_private *)kbd->priv;
    memset(priv->data, 0, sizeof(priv->data));
    memset(&priv->urb, 0, sizeof(priv->urb));
    priv->urb.type = USBDEVFS_URB_TYPE_INTERRUPT;
    priv->urb.endpoint = (unsigned char)kbd->endpoint;
    priv->urb.buffer = priv->data;
    priv->urb.buffer_length =
        (int)((kbd->max_packet != 0U && kbd->max_packet < sizeof(priv->data)) ? kbd->max_packet : sizeof(priv->data));
    priv->urb.usercontext = &priv->urb;
    if (aoc_raw_ioctl(kbd->fd, USBDEVFS_SUBMITURB, &priv->urb) != 0) {
        kbd->last_errno = errno;
        kbd->urb_submitted = 0;
        return -1;
    }
    kbd->urb_submitted = 1;
    return 0;
}

int aoc_usb_kbd_pop_event(struct aoc_usb_kbd *kbd, struct aoc_usb_kbd_event *event) {
    if (kbd == 0 || event == 0 || kbd->queue_tail == kbd->queue_head) {
        return 0;
    }
    *event = kbd->queue[kbd->queue_tail];
    kbd->queue_tail = (kbd->queue_tail + 1U) % AOC_USB_KBD_QUEUE_CAP;
    return 1;
}

int aoc_usb_kbd_translate_event(const struct aoc_usb_kbd_event *event, char *out, unsigned int out_cap) {
    static const char normal_digits[] = "1234567890-=";
    static const char shift_digits[] = "!@#$%^&*()_+";
    int shift_down;
    int ctrl_down;
    int altgr_down;
    char ch = '\0';

    if (event == 0 || out == 0 || out_cap == 0U || !event->pressed) {
        return 0;
    }
    shift_down = (event->modifiers & 0x22U) != 0U;
    ctrl_down = (event->modifiers & 0x11U) != 0U;
    altgr_down = (event->modifiers & 0x40U) != 0U;

    if (event->usage >= 0x04U && event->usage <= 0x1dU) {
        static const char letter_map[] = "abcdefghijklmnopqrstuvwxyz";

        if (altgr_down) {
            switch (event->usage) {
                case 0x14U:
                    out[0] = '/';
                    return 1;
                case 0x1aU:
                    out[0] = '?';
                    return 1;
                default:
                    break;
            }
        }
        ch = letter_map[event->usage - 0x04U];
        if (ctrl_down) {
            out[0] = (char)(ch - 'a' + 1);
            return 1;
        }
        if (shift_down) {
            ch = (char)(ch - ('a' - 'A'));
        }
        out[0] = ch;
        return 1;
    }
    if (event->usage >= 0x1eU && event->usage <= 0x27U) {
        out[0] = shift_down ? shift_digits[event->usage - 0x1eU] : normal_digits[event->usage - 0x1eU];
        return 1;
    }
    switch (event->usage) {
        case 0x28U:
            out[0] = '\r';
            return 1;
        case 0x29U:
            out[0] = 0x1b;
            return 1;
        case 0x2aU:
            out[0] = 0x7f;
            return 1;
        case 0x2bU:
            out[0] = '\t';
            return 1;
        case 0x2cU:
            out[0] = ' ';
            return 1;
        case 0x2dU:
            out[0] = shift_down ? '_' : '-';
            return 1;
        case 0x2eU:
            out[0] = shift_down ? '+' : '=';
            return 1;
        case 0x2fU:
            out[0] = shift_down ? '`' : '\'';
            return 1;
        case 0x30U:
            out[0] = shift_down ? '{' : '[';
            return 1;
        case 0x31U:
            out[0] = shift_down ? '}' : ']';
            return 1;
        case 0x33U:
            if (altgr_down) {
                out[0] = '\'';
                return 1;
            }
            return 0;
        case 0x34U:
            out[0] = shift_down ? '^' : '~';
            return 1;
        case 0x35U:
            out[0] = shift_down ? '"' : '\'';
            return 1;
        case 0x64U:
            out[0] = shift_down ? '|' : '\\';
            return 1;
        case 0x36U:
            out[0] = shift_down ? '<' : ',';
            return 1;
        case 0x37U:
            out[0] = shift_down ? '>' : '.';
            return 1;
        case 0x38U:
            out[0] = shift_down ? ':' : ';';
            return 1;
        case 0x87U:
            out[0] = shift_down ? '?' : '/';
            return 1;
        case 0x4aU:
            if (out_cap < 3U) {
                return 0;
            }
            memcpy(out, "\x1b[H", 3);
            return 3;
        case 0x4cU:
            if (out_cap < 4U) {
                return 0;
            }
            memcpy(out, "\x1b[3~", 4);
            return 4;
        case 0x4dU:
            if (out_cap < 3U) {
                return 0;
            }
            memcpy(out, "\x1b[F", 3);
            return 3;
        case 0x4fU:
            if (out_cap < 3U) {
                return 0;
            }
            memcpy(out, "\x1b[C", 3);
            return 3;
        case 0x50U:
            if (out_cap < 3U) {
                return 0;
            }
            memcpy(out, "\x1b[D", 3);
            return 3;
        case 0x51U:
            if (out_cap < 3U) {
                return 0;
            }
            memcpy(out, "\x1b[B", 3);
            return 3;
        case 0x52U:
            if (out_cap < 3U) {
                return 0;
            }
            memcpy(out, "\x1b[A", 3);
            return 3;
        default:
            return 0;
    }
}

void aoc_usb_kbd_close(struct aoc_usb_kbd *kbd) {
    unsigned int iface;

    if (kbd == 0) {
        return;
    }
    if (kbd->fd >= 0) {
        if (kbd->urb_submitted && kbd->priv != 0) {
            struct aoc_usb_kbd_private *priv = (struct aoc_usb_kbd_private *)kbd->priv;

            aoc_raw_ioctl(kbd->fd, USBDEVFS_DISCARDURB, &priv->urb);
            kbd->urb_submitted = 0;
        }
        if (kbd->iface_claimed) {
            iface = kbd->iface_number;
            aoc_raw_ioctl(kbd->fd, USBDEVFS_RELEASEINTERFACE, &iface);
            kbd->iface_claimed = 0;
        }
        if (kbd->driver_detached) {
            aoc_raw_ioctl(kbd->fd, USBDEVFS_CONNECT, 0);
            kbd->driver_detached = 0;
        }
        close(kbd->fd);
    }
    if (kbd->priv != 0) {
        free(kbd->priv);
    }
    memset(kbd, 0, sizeof(*kbd));
    kbd->fd = -1;
}

int aoc_usb_kbd_open(struct aoc_usb_kbd *kbd) {
    struct aoc_usb_kbd_candidate candidate;
    struct usbdevfs_getdriver get_driver;
    struct usbdevfs_disconnect_claim disconnect_claim;
    unsigned int iface;
    int disconnect_errno = 0;
    int get_driver_ok = 0;

    if (kbd == 0) {
        errno = EINVAL;
        return -1;
    }
    aoc_usb_kbd_close(kbd);
    memset(kbd, 0, sizeof(*kbd));
    kbd->fd = -1;
    kbd->priv = calloc(1U, sizeof(struct aoc_usb_kbd_private));
    if (kbd->priv == 0) {
        kbd->last_errno = ENOMEM;
        errno = ENOMEM;
        return -1;
    }
    if (scan_candidate(&candidate) != 0 || inspect_candidate(&candidate) != 0) {
        kbd->last_errno = errno;
        aoc_usb_kbd_close(kbd);
        errno = kbd->last_errno;
        return -1;
    }

    kbd->fd = open(candidate.path, O_RDWR | O_NONBLOCK);
    if (kbd->fd < 0) {
        kbd->fd = open(candidate.path, O_RDONLY | O_NONBLOCK);
    }
    if (kbd->fd < 0) {
        kbd->last_errno = errno;
        aoc_usb_kbd_close(kbd);
        errno = kbd->last_errno;
        return -1;
    }

    snprintf(kbd->device_path, sizeof(kbd->device_path), "%s", candidate.path);
    snprintf(kbd->driver_name, sizeof(kbd->driver_name), "%s", "NONE");
    kbd->iface_number = candidate.iface_number;
    kbd->endpoint = candidate.endpoint;
    kbd->max_packet = candidate.max_packet;
    kbd->error_streak = 0U;

    memset(&get_driver, 0, sizeof(get_driver));
    get_driver.interface = kbd->iface_number;
    if (aoc_raw_ioctl(kbd->fd, USBDEVFS_GETDRIVER, &get_driver) == 0) {
        get_driver_ok = 1;
        snprintf(
            kbd->driver_name,
            sizeof(kbd->driver_name),
            "%.88s",
            get_driver.driver[0] != '\0' ? get_driver.driver : "UNKNOWN"
        );
    } else if (!(errno == ENODATA || errno == ENOENT || errno == ENODEV)) {
        snprintf(kbd->driver_name, sizeof(kbd->driver_name), "GET %d", errno);
    }

    if (get_driver_ok && get_driver.driver[0] != '\0') {
        memset(&disconnect_claim, 0, sizeof(disconnect_claim));
        disconnect_claim.interface = kbd->iface_number;
        disconnect_claim.flags = USBDEVFS_DISCONNECT_CLAIM_IF_DRIVER;
        snprintf(disconnect_claim.driver, sizeof(disconnect_claim.driver), "%s", get_driver.driver);
        if (aoc_raw_ioctl(kbd->fd, USBDEVFS_DISCONNECT_CLAIM, &disconnect_claim) == 0) {
            kbd->iface_claimed = 1;
            kbd->driver_detached = 1;
            snprintf(kbd->driver_name, sizeof(kbd->driver_name), "%.80s->usbfs", get_driver.driver);
        } else {
            disconnect_errno = errno;
        }
    }

    if (!kbd->iface_claimed) {
        iface = kbd->iface_number;
        if (aoc_raw_ioctl(kbd->fd, USBDEVFS_CLAIMINTERFACE, &iface) != 0) {
            kbd->last_errno = errno;
            aoc_usb_kbd_close(kbd);
            errno = kbd->last_errno;
            return -1;
        }
        kbd->iface_claimed = 1;
        if (disconnect_errno != 0) {
            char driver_prefix[AOC_USB_KBD_NAME_CAP];

            snprintf(driver_prefix, sizeof(driver_prefix), "%s", kbd->driver_name);
            snprintf(kbd->driver_name, sizeof(kbd->driver_name), "%.80s DC %d", driver_prefix, disconnect_errno);
        } else if (kbd->driver_name[0] == '\0' || strcmp(kbd->driver_name, "NONE") == 0) {
            snprintf(kbd->driver_name, sizeof(kbd->driver_name), "%s", "USBFS");
        }
    }

    usb_control_transfer(
        kbd->fd,
        USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
        USB_HID_REQ_SET_PROTOCOL,
        0U,
        0U,
        kbd->iface_number,
        0,
        0U
    );
    usb_control_transfer(
        kbd->fd,
        USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
        USB_HID_REQ_SET_IDLE,
        0U,
        0U,
        kbd->iface_number,
        0,
        0U
    );
    if (submit_urb(kbd) != 0) {
        int saved_errno = kbd->last_errno;

        aoc_usb_kbd_close(kbd);
        errno = saved_errno;
        return -1;
    }
    return 0;
}

int aoc_usb_kbd_poll(struct aoc_usb_kbd *kbd) {
    struct aoc_usb_kbd_private *priv;
    void *completed = 0;
    int rc;
    unsigned int i;

    if (kbd == 0 || kbd->fd < 0 || kbd->priv == 0) {
        errno = ENODEV;
        return -1;
    }
    priv = (struct aoc_usb_kbd_private *)kbd->priv;
    if (!kbd->urb_submitted) {
        if (!path_exists(kbd->device_path)) {
            kbd->last_errno = ENODEV;
            aoc_usb_kbd_close(kbd);
            errno = ENODEV;
            return -1;
        }
        if (submit_urb(kbd) != 0) {
            if (path_exists(kbd->device_path)) {
                ++kbd->error_streak;
                errno = 0;
                return 0;
            }
            {
                int saved_errno = kbd->last_errno;

                aoc_usb_kbd_close(kbd);
                errno = saved_errno;
                return -1;
            }
        }
        return 0;
    }
    rc = aoc_raw_ioctl(kbd->fd, USBDEVFS_REAPURBNDELAY, &completed);
    if (rc != 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        if (path_exists(kbd->device_path)) {
            kbd->last_errno = errno;
            kbd->urb_submitted = 0;
            ++kbd->error_streak;
            errno = 0;
            return 0;
        }
        kbd->last_errno = errno;
        aoc_usb_kbd_close(kbd);
        errno = kbd->last_errno;
        return -1;
    }

    kbd->urb_submitted = 0;
    if (completed != &priv->urb) {
        if (path_exists(kbd->device_path)) {
            kbd->urb_submitted = 0;
            ++kbd->error_streak;
            errno = 0;
            return 0;
        }
        kbd->last_errno = EIO;
        aoc_usb_kbd_close(kbd);
        errno = EIO;
        return -1;
    }
    if (priv->urb.status != 0) {
        if (path_exists(kbd->device_path)) {
            kbd->last_errno = -priv->urb.status;
            kbd->urb_submitted = 0;
            ++kbd->error_streak;
            return 0;
        }
        kbd->last_errno = priv->urb.status;
        aoc_usb_kbd_close(kbd);
        errno = EIO;
        return -1;
    }

    memset(kbd->last_report, 0, sizeof(kbd->last_report));
    if (priv->urb.actual_length > 0) {
        unsigned int len = (unsigned int)priv->urb.actual_length;

        if (len > sizeof(kbd->last_report)) {
            len = sizeof(kbd->last_report);
        }
        memcpy(kbd->last_report, priv->data, len);
        kbd->modifiers = kbd->last_report[0];
        ++kbd->report_count;
        for (i = 2U; i < AOC_USB_KBD_REPORT_CAP; ++i) {
            uint8_t usage = priv->previous_report[i];

            if (usage != 0U && !report_has_usage(kbd->last_report, usage)) {
                queue_event(kbd, 0, usage);
            }
        }
        for (i = 2U; i < AOC_USB_KBD_REPORT_CAP; ++i) {
            uint8_t usage = kbd->last_report[i];

            if (usage != 0U && !report_has_usage(priv->previous_report, usage)) {
                queue_event(kbd, 1, usage);
            }
        }
        memcpy(priv->previous_report, kbd->last_report, sizeof(priv->previous_report));
    }
    kbd->error_streak = 0U;
    if (submit_urb(kbd) != 0) {
        if (path_exists(kbd->device_path)) {
            ++kbd->error_streak;
            errno = 0;
            return 0;
        }
        {
            int saved_errno = kbd->last_errno;

            aoc_usb_kbd_close(kbd);
            errno = saved_errno;
            return -1;
        }
    }
    return 1;
}
