#include "aoc/aoc.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define REMOTE_KEY_MENU 0x0010000FU
#define REMOTE_KEY_UP 0x00100007U
#define REMOTE_KEY_DOWN 0x00100006U
#define REMOTE_KEY_LEFT 0x00100008U
#define REMOTE_KEY_RIGHT 0x00100009U

#define REMOTE_KEY_SOCKET_VOL_UP 0x0000003CU
#define REMOTE_KEY_SOCKET_VOL_DOWN 0x0000003DU
#define REMOTE_KEY_SOCKET_MENU 0x00010319U
#define REMOTE_KEY_SOCKET_CH_UP 0x00010316U
#define REMOTE_KEY_SOCKET_CH_DOWN 0x00010317U
#define REMOTE_KEY_SOCKET_INPUT_1 0x00010318U
#define REMOTE_KEY_SOCKET_INPUT_2 0x0000003EU
#define REMOTE_KEY_SOCKET_INPUT_3 0x00100017U

#define DIKS_CURSOR_LEFT 61440U
#define DIKS_CURSOR_RIGHT 61441U
#define DIKS_CURSOR_UP 61442U
#define DIKS_CURSOR_DOWN 61443U
#define DIKS_OK 61451U
#define DIKS_ENTER 13U
#define DIKS_MENU 61458U
#define DIKS_CHANNEL_UP 61510U
#define DIKS_CHANNEL_DOWN 61511U
#define DIKS_VOLUME_UP 61516U
#define DIKS_VOLUME_DOWN 61517U

#define INPUT_LOG_LIMIT 128UL

static int queue_input_event(
    struct aoc_input *input,
    int pressed,
    enum aoc_input_action action,
    uint32_t raw
) {
    unsigned int next = (input->queue_head + 1U) % AOC_INPUT_QUEUE_CAP;

    if (next == input->queue_tail) {
        return 0;
    }
    input->queue[input->queue_head].pressed = pressed;
    input->queue[input->queue_head].action = action;
    input->queue[input->queue_head].raw = raw;
    input->queue_head = next;
    return 1;
}

int aoc_input_pop(struct aoc_input *input, struct aoc_input_event *event) {
    if (input == 0 || event == 0 || input->queue_tail == input->queue_head) {
        return 0;
    }
    *event = input->queue[input->queue_tail];
    input->queue_tail = (input->queue_tail + 1U) % AOC_INPUT_QUEUE_CAP;
    return 1;
}

enum aoc_input_action aoc_input_translate_raw(uint32_t raw) {
    switch (raw) {
        case REMOTE_KEY_SOCKET_VOL_UP:
            return AOC_INPUT_FORWARD;
        case REMOTE_KEY_SOCKET_VOL_DOWN:
            return AOC_INPUT_BACKWARD;
        case REMOTE_KEY_SOCKET_MENU:
            return AOC_INPUT_FIRE;
        case REMOTE_KEY_SOCKET_CH_UP:
            return AOC_INPUT_TURN_LEFT;
        case REMOTE_KEY_SOCKET_CH_DOWN:
            return AOC_INPUT_TURN_RIGHT;
        case REMOTE_KEY_SOCKET_INPUT_1:
        case REMOTE_KEY_SOCKET_INPUT_2:
        case REMOTE_KEY_SOCKET_INPUT_3:
            return AOC_INPUT_USE;
        case REMOTE_KEY_UP:
        case 7U:
        case DIKS_CURSOR_UP:
        case DIKS_CHANNEL_UP:
            return AOC_INPUT_FORWARD;
        case REMOTE_KEY_DOWN:
        case 6U:
        case DIKS_CURSOR_DOWN:
        case DIKS_CHANNEL_DOWN:
            return AOC_INPUT_BACKWARD;
        case REMOTE_KEY_LEFT:
        case 8U:
        case DIKS_CURSOR_LEFT:
        case DIKS_VOLUME_DOWN:
            return AOC_INPUT_TURN_LEFT;
        case REMOTE_KEY_RIGHT:
        case 9U:
        case DIKS_CURSOR_RIGHT:
        case DIKS_VOLUME_UP:
            return AOC_INPUT_TURN_RIGHT;
        case REMOTE_KEY_MENU:
        case 15U:
        case DIKS_MENU:
            return AOC_INPUT_FIRE;
        case DIKS_OK:
        case DIKS_ENTER:
            return AOC_INPUT_USE;
        default:
            return AOC_INPUT_NONE;
    }
}

static uint32_t be32_from_bytes(const unsigned char *buf) {
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) |
           (uint32_t)buf[3];
}

static uint32_t le32_from_bytes(const unsigned char *buf) {
    return ((uint32_t)buf[3] << 24) |
           ((uint32_t)buf[2] << 16) |
           ((uint32_t)buf[1] << 8) |
           (uint32_t)buf[0];
}

static void note_action(
    struct aoc_input *input,
    enum aoc_input_action action,
    uint32_t raw,
    unsigned long frame
) {
    if (action <= AOC_INPUT_NONE || action >= AOC_INPUT_ACTION_COUNT) {
        return;
    }
    if (input->active_until[action] == 0) {
        queue_input_event(input, 1, action, raw);
    }
    input->active_until[action] = frame + 5UL;
}

static void note_remote_word(struct aoc_input *input, uint32_t raw, unsigned long frame) {
    enum aoc_input_action action;

    if (raw == 0) {
        return;
    }
    if (input->debug && input->remote_log_count < INPUT_LOG_LIMIT) {
        aoc_log_hex("aoc_input: remote raw=", raw);
        ++input->remote_log_count;
    }

    action = aoc_input_translate_raw(raw);
    if (action == AOC_INPUT_NONE) {
        action = aoc_input_translate_raw(raw & 0xffffU);
    }
    if (action == AOC_INPUT_NONE) {
        action = aoc_input_translate_raw(raw & 0xffU);
    }
    if (input->debug && action != AOC_INPUT_NONE &&
        input->remote_log_count < INPUT_LOG_LIMIT) {
        aoc_log_hex("aoc_input: remote action=", (uint32_t)action);
    }
    note_action(input, action, raw, frame);
}

static void process_remote_bytes(
    struct aoc_input *input,
    const unsigned char *buf,
    ssize_t len,
    unsigned long frame
) {
    ssize_t i;

    if (len == 1) {
        note_remote_word(input, (uint32_t)buf[0], frame);
        return;
    }

    for (i = 0; i + 3 < len; i += 4) {
        note_remote_word(input, be32_from_bytes(buf + i), frame);
        note_remote_word(input, le32_from_bytes(buf + i), frame);
    }
}

static void expire_actions(struct aoc_input *input, unsigned long frame) {
    unsigned int i;

    for (i = 1U; i < (unsigned int)AOC_INPUT_ACTION_COUNT; ++i) {
        if (input->active_until[i] != 0 && input->active_until[i] <= frame) {
            input->active_until[i] = 0;
            queue_input_event(input, 0, (enum aoc_input_action)i, 0);
        }
    }
}

static void trace_trid_packet(struct aoc_input *input, uint32_t kind, uint32_t code) {
    if (!input->debug || input->trid_log_count >= INPUT_LOG_LIMIT) {
        return;
    }
    aoc_log_hex("aoc_input: trid kind=", kind);
    aoc_log_hex("aoc_input: trid code=", code);
    ++input->trid_log_count;
}

static void note_trid_packet(
    struct aoc_input *input,
    uint32_t kind,
    uint32_t code,
    unsigned long frame
) {
    trace_trid_packet(input, kind, code);
    if (kind == 1U) {
        note_remote_word(input, code, frame);
    }
}

static void process_trid_bytes(
    struct aoc_input *input,
    const unsigned char *buf,
    ssize_t len,
    unsigned long frame
) {
    uint32_t be_kind;
    uint32_t be_code;
    uint32_t le_kind;
    uint32_t le_code;

    if (len != 8) {
        if (input->debug) {
            aoc_log_dec("aoc_input: trid short packet len=", (unsigned long)len);
        }
        process_remote_bytes(input, buf, len, frame);
        return;
    }

    be_kind = be32_from_bytes(buf);
    be_code = be32_from_bytes(buf + 4);
    le_kind = le32_from_bytes(buf);
    le_code = le32_from_bytes(buf + 4);

    note_trid_packet(input, be_kind, be_code, frame);
    if (le_kind != be_kind || le_code != be_code) {
        note_trid_packet(input, le_kind, le_code, frame);
    }
}

static void ensure_trid_input_open(struct aoc_input *input) {
    struct sockaddr_un addr;

    if (input->trid_fd >= 0 || input->trid_open_attempted) {
        return;
    }

    input->trid_open_attempted = 1;
    input->trid_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (input->trid_fd < 0) {
        if (input->debug) {
            aoc_log_errno("aoc_input: trid socket failed");
        }
        return;
    }

    aoc_raw_fcntl(input->trid_fd, F_SETFL, O_NONBLOCK);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, AOC_INPUT_SOCKET_PATH, sizeof(addr.sun_path) - 1U);

    unlink(AOC_INPUT_SOCKET_PATH);
    if (bind(input->trid_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        if (input->debug) {
            aoc_log_errno("aoc_input: trid bind failed");
        }
        close(input->trid_fd);
        input->trid_fd = -1;
        return;
    }

    chmod(AOC_INPUT_SOCKET_PATH, 0666);
    aoc_log_line("aoc_input: trid bind ok");
}

static void ensure_remote_open(struct aoc_input *input) {
    if (input->remote_fd >= 0 || input->remote_open_attempted) {
        return;
    }

    input->remote_open_attempted = 1;
    input->remote_fd = open(AOC_REMOTE_DEVICE_PATH, O_RDONLY | O_NONBLOCK);
    if (input->remote_fd < 0) {
        if (input->debug) {
            aoc_log_errno("aoc_input: remote open failed");
        }
        return;
    }

    if (input->debug) {
        aoc_log_line("aoc_input: remote open ok");
    }
    aoc_raw_ioctl(input->remote_fd, 0x1009, (void *)1UL);
    aoc_raw_ioctl(input->remote_fd, 0x1005, (void *)0xffUL);
}

int aoc_input_open(struct aoc_input *input) {
    const char *debug_env;

    if (input == 0) {
        return -1;
    }
    memset(input, 0, sizeof(*input));
    input->trid_fd = -1;
    input->remote_fd = -1;
    input->frame_polled = ~0UL;

    debug_env = getenv("AOC_INPUT_DEBUG");
    input->debug = debug_env != 0 && strcmp(debug_env, "1") == 0;
    ensure_trid_input_open(input);
    return 0;
}

int aoc_input_poll(struct aoc_input *input, unsigned long frame) {
    unsigned int reads = 0;

    if (input == 0) {
        return -1;
    }
    if (input->frame_polled == frame) {
        return 0;
    }
    input->frame_polled = frame;

    ensure_trid_input_open(input);
    if (input->trid_fd >= 0) {
        while (reads < 8U) {
            unsigned char buf[64];
            ssize_t n = recv(input->trid_fd, buf, sizeof(buf), 0);
            if (n <= 0) {
                break;
            }
            process_trid_bytes(input, buf, n, frame);
            ++reads;
        }
    }

    reads = 0;
    ensure_remote_open(input);
    if (input->remote_fd >= 0) {
        while (reads < 8U) {
            unsigned char buf[32];
            ssize_t n = read(input->remote_fd, buf, sizeof(buf));
            if (n <= 0) {
                break;
            }
            process_remote_bytes(input, buf, n, frame);
            ++reads;
        }
    }

    expire_actions(input, frame);
    return 0;
}

void aoc_input_close(struct aoc_input *input) {
    if (input == 0) {
        return;
    }
    if (input->trid_fd >= 0) {
        close(input->trid_fd);
    }
    if (input->remote_fd >= 0) {
        close(input->remote_fd);
    }
    input->trid_fd = -1;
    input->remote_fd = -1;
}
