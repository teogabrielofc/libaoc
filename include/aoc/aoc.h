#ifndef AOC_AOC_H
#define AOC_AOC_H

#include <stdint.h>

#define AOC_FB_DEFAULT_DEVICE "/dev/hidtv2dge"
#define AOC_INPUT_SOCKET_PATH "/tmp/hp_dfb_handler"
#define AOC_REMOTE_DEVICE_PATH "/dev/remote"
#define AOC_LOG_DEFAULT_PATH "/etc/core/aoc.log"
#define AOC_INPUT_QUEUE_CAP 32U
#define AOC_USB_KBD_QUEUE_CAP 32U
#define AOC_USB_KBD_PATH_CAP 64U
#define AOC_USB_KBD_NAME_CAP 96U
#define AOC_USB_KBD_REPORT_CAP 8U

enum aoc_input_action {
    AOC_INPUT_NONE = 0,
    AOC_INPUT_VOLUP,
    AOC_INPUT_VOLDOWN,
    AOC_INPUT_MENU,
    AOC_INPUT_CHUP,
    AOC_INPUT_CHDOWN,
    AOC_INPUT_INPUT,
    AOC_INPUT_ACTION_COUNT,
    AOC_INPUT_FORWARD = AOC_INPUT_VOLUP,
    AOC_INPUT_BACKWARD = AOC_INPUT_VOLDOWN,
    AOC_INPUT_FIRE = AOC_INPUT_MENU,
    AOC_INPUT_TURN_LEFT = AOC_INPUT_CHUP,
    AOC_INPUT_TURN_RIGHT = AOC_INPUT_CHDOWN,
    AOC_INPUT_USE = AOC_INPUT_INPUT
};

struct aoc_input_event {
    int pressed;
    enum aoc_input_action action;
    uint32_t raw;
};

struct aoc_input_raw_event {
    int pressed;
    uint32_t raw;
};

struct aoc_usb_kbd_event {
    int pressed;
    uint8_t usage;
    uint8_t modifiers;
};

struct aoc_fb {
    int fd;
    void *map;
    unsigned long map_len;
    unsigned long page_len;
    unsigned int xres;
    unsigned int yres;
    unsigned int xres_virtual;
    unsigned int yres_virtual;
    unsigned int bits_per_pixel;
    unsigned int line_length;
    unsigned int present_pages;
    unsigned int full_refresh_every;
    unsigned long frame_count;
};

struct aoc_input {
    int trid_fd;
    int remote_fd;
    int trid_bind_state;
    int trid_last_errno;
    unsigned long frame_polled;
    unsigned long remote_open_attempted;
    unsigned long trid_open_attempted;
    unsigned long remote_log_count;
    unsigned long trid_log_count;
    unsigned long trid_packet_count;
    uint32_t trid_last_kind;
    uint32_t trid_last_code;
    unsigned long active_until[AOC_INPUT_ACTION_COUNT];
    struct aoc_input_event queue[AOC_INPUT_QUEUE_CAP];
    struct aoc_input_raw_event raw_queue[AOC_INPUT_QUEUE_CAP];
    unsigned int queue_head;
    unsigned int queue_tail;
    unsigned int raw_queue_head;
    unsigned int raw_queue_tail;
    int debug;
};

struct aoc_usb_kbd {
    int fd;
    int iface_claimed;
    int driver_detached;
    int urb_submitted;
    int last_errno;
    unsigned int iface_number;
    unsigned int endpoint;
    unsigned int max_packet;
    unsigned int error_streak;
    unsigned long report_count;
    uint8_t modifiers;
    uint8_t last_report[AOC_USB_KBD_REPORT_CAP];
    char device_path[AOC_USB_KBD_PATH_CAP];
    char driver_name[AOC_USB_KBD_NAME_CAP];
    struct aoc_usb_kbd_event queue[AOC_USB_KBD_QUEUE_CAP];
    unsigned int queue_head;
    unsigned int queue_tail;
    void *priv;
};

void aoc_log_init(const char *path);
void aoc_log_set_fsync(int enabled);
int aoc_log_append_raw(const char *text);
void aoc_log_line(const char *line);
void aoc_log_dec(const char *prefix, unsigned long value);
void aoc_log_hex(const char *prefix, uint32_t value);
void aoc_log_errno(const char *prefix);
int aoc_write_text_file(const char *path, const char *text);

int aoc_raw_ioctl(int fd, unsigned long request, void *arg);
int aoc_raw_fcntl(int fd, unsigned long cmd, unsigned long arg);
void aoc_sleep_ms(uint32_t ms);
uint32_t aoc_ticks_ms(void);

int aoc_fb_open(struct aoc_fb *fb, const char *device_path);
void aoc_fb_close(struct aoc_fb *fb);
unsigned int aoc_fb_page_count(const struct aoc_fb *fb);
unsigned int aoc_fb_configured_present_pages(const struct aoc_fb *fb);
void aoc_fb_present_xrgb8888_scaled(struct aoc_fb *fb, const uint32_t *src, unsigned int src_width, unsigned int src_height);

int aoc_input_open(struct aoc_input *input);
int aoc_input_poll(struct aoc_input *input, unsigned long frame);
int aoc_input_pop(struct aoc_input *input, struct aoc_input_event *event);
int aoc_input_pop_raw(struct aoc_input *input, struct aoc_input_raw_event *event);
int aoc_usb_kbd_open(struct aoc_usb_kbd *kbd);
int aoc_usb_kbd_poll(struct aoc_usb_kbd *kbd);
int aoc_usb_kbd_pop_event(struct aoc_usb_kbd *kbd, struct aoc_usb_kbd_event *event);
int aoc_usb_kbd_translate_event(const struct aoc_usb_kbd_event *event, char *out, unsigned int out_cap);
void aoc_usb_kbd_close(struct aoc_usb_kbd *kbd);
void aoc_input_close(struct aoc_input *input);
enum aoc_input_action aoc_input_translate_raw(uint32_t raw);

#endif
