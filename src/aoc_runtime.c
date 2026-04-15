#include "aoc/aoc.h"

#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>

extern long syscall(long number, ...);

int aoc_raw_ioctl(int fd, unsigned long request, void *arg) {
    return (int)syscall(4054, fd, request, arg);
}

int aoc_raw_fcntl(int fd, unsigned long cmd, unsigned long arg) {
    return (int)syscall(4055, fd, cmd, arg);
}

void aoc_sleep_ms(uint32_t ms) {
    usleep((useconds_t)(ms * 1000U));
}

uint32_t aoc_ticks_ms(void) {
    struct timeval tv;

    gettimeofday(&tv, 0);
    return (uint32_t)(tv.tv_sec * 1000UL + tv.tv_usec / 1000UL);
}
