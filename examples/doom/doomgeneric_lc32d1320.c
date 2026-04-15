#include "doomgeneric.h"
#include "d_loop.h"
#include "doomkeys.h"
#include "aoc/aoc.h"

#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *k_log_path = "/etc/core/doom.log";
static const char *k_ok_path = "/etc/core/doom.ok";
static const char *k_fail_path = "/etc/core/doom.fail";

static struct aoc_fb g_fb;
static struct aoc_input g_input;
static unsigned long g_draw_frame_count = 0;
static int g_input_ready = 0;

static void fail_and_exit(const char *message, int code) {
    aoc_log_line(message);
    aoc_write_text_file(k_fail_path, message);
    _exit(code);
}

static unsigned char aoc_input_to_doom_key(enum aoc_input_action action) {
    switch (action) {
        case AOC_INPUT_FORWARD:
            return KEY_UPARROW;
        case AOC_INPUT_BACKWARD:
            return KEY_DOWNARROW;
        case AOC_INPUT_FIRE:
            return KEY_FIRE;
        case AOC_INPUT_TURN_LEFT:
            return KEY_LEFTARROW;
        case AOC_INPUT_TURN_RIGHT:
            return KEY_RIGHTARROW;
        case AOC_INPUT_USE:
            return KEY_USE;
        default:
            return 0;
    }
}

static int pop_doom_key(int *pressed, unsigned char *doomKey) {
    struct aoc_input_event event;

    while (aoc_input_pop(&g_input, &event)) {
        unsigned char key = aoc_input_to_doom_key(event.action);
        if (key != 0) {
            *pressed = event.pressed;
            *doomKey = key;
            return 1;
        }
    }
    return 0;
}

void DG_Init(void) {
    mkdir("/etc/core", 0777);
    unlink(k_log_path);
    unlink(k_ok_path);
    unlink(k_fail_path);

    aoc_log_init(k_log_path);
    aoc_log_set_fsync(0);
    aoc_log_line("doomfb: DG_Init start");

    if (aoc_fb_open(&g_fb, NULL) != 0) {
        fail_and_exit("doomfb: aoc_fb_open failed", 10);
    }

    if (aoc_input_open(&g_input) == 0) {
        g_input_ready = 1;
    } else {
        aoc_log_line("doomfb: aoc_input_open failed");
    }

    aoc_write_text_file(k_ok_path, "doomfb init ok\n");
    aoc_log_line("doomfb: DG_Init ok");
}

void DG_DrawFrame(void) {
    ++g_draw_frame_count;
    if (g_draw_frame_count <= 5 || (g_draw_frame_count % 60UL) == 0) {
        aoc_log_dec("doomfb: draw frame ", g_draw_frame_count);
    }
    aoc_fb_present_xrgb8888_scaled(&g_fb, (const uint32_t *)DG_ScreenBuffer, DOOMGENERIC_RESX, DOOMGENERIC_RESY);
}

void DG_SleepMs(uint32_t ms) {
    aoc_sleep_ms(ms);
}

uint32_t DG_GetTicksMs(void) {
    return aoc_ticks_ms();
}

int DG_GetKey(int *pressed, unsigned char *doomKey) {
    if (!g_input_ready) {
        return 0;
    }
    if (pop_doom_key(pressed, doomKey)) {
        return 1;
    }
    aoc_input_poll(&g_input, g_draw_frame_count);
    return pop_doom_key(pressed, doomKey);
}

void DG_SetWindowTitle(const char *title) {
    (void)title;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    singletics = true;
    aoc_log_init(k_log_path);
    aoc_log_line("doomfb: singletics enabled");
    aoc_log_line("doomfb: before doomgeneric_Create");
    doomgeneric_Create(argc, argv);

    aoc_log_line("doomfb: after doomgeneric_Create");
    aoc_log_line("doomfb: main loop");
    for (;;) {
        doomgeneric_Tick();
        DG_SleepMs(28);
    }

    return 0;
}
