#include "aoc/aoc.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

static const char *g_log_path = AOC_LOG_DEFAULT_PATH;
static int g_log_fsync = 0;

static int write_all(int fd, const char *buf, size_t len) {
    while (len != 0) {
        ssize_t n = write(fd, buf, len);
        if (n <= 0) {
            return -1;
        }
        buf += (size_t)n;
        len -= (size_t)n;
    }
    return 0;
}

void aoc_log_init(const char *path) {
    if (path != 0 && path[0] != '\0') {
        g_log_path = path;
    } else {
        g_log_path = AOC_LOG_DEFAULT_PATH;
    }
}

void aoc_log_set_fsync(int enabled) {
    g_log_fsync = enabled != 0;
}

int aoc_log_append_raw(const char *text) {
    int fd;

    if (text == 0) {
        return -1;
    }

    fd = open(g_log_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd < 0) {
        return -1;
    }
    if (write_all(fd, text, strlen(text)) != 0) {
        close(fd);
        return -1;
    }
    if (g_log_fsync) {
        fsync(fd);
    }
    close(fd);
    return 0;
}

static void append_dec_value(unsigned long value) {
    char buf[24];
    size_t pos = sizeof(buf);

    buf[--pos] = '\0';
    if (value == 0) {
        buf[--pos] = '0';
    } else {
        while (value != 0 && pos != 0) {
            buf[--pos] = (char)('0' + (value % 10));
            value /= 10;
        }
    }
    aoc_log_append_raw(&buf[pos]);
}

static void format_hex_value(char *buf, uint32_t value) {
    unsigned int i;

    buf[0] = '0';
    buf[1] = 'x';
    for (i = 0; i < 8U; ++i) {
        unsigned int nibble = (value >> ((7U - i) * 4U)) & 0xfU;
        buf[2U + i] = (char)(nibble < 10U ? ('0' + nibble) : ('a' + nibble - 10U));
    }
    buf[10] = '\0';
}

void aoc_log_line(const char *line) {
    if (line == 0) {
        return;
    }
    aoc_log_append_raw(line);
    aoc_log_append_raw("\n");
    write_all(2, line, strlen(line));
    write_all(2, "\n", 1);
}

void aoc_log_dec(const char *prefix, unsigned long value) {
    if (prefix == 0) {
        prefix = "";
    }
    aoc_log_append_raw(prefix);
    append_dec_value(value);
    aoc_log_append_raw("\n");
}

void aoc_log_hex(const char *prefix, uint32_t value) {
    char hexbuf[11];

    if (prefix == 0) {
        prefix = "";
    }
    format_hex_value(hexbuf, value);
    aoc_log_append_raw(prefix);
    aoc_log_append_raw(hexbuf);
    aoc_log_append_raw("\n");
    write_all(2, prefix, strlen(prefix));
    write_all(2, hexbuf, strlen(hexbuf));
    write_all(2, "\n", 1);
}

void aoc_log_errno(const char *prefix) {
    if (prefix == 0) {
        prefix = "";
    }
    aoc_log_append_raw(prefix);
    aoc_log_append_raw(" errno=");
    append_dec_value((unsigned long)errno);
    aoc_log_append_raw("\n");
    write_all(2, prefix, strlen(prefix));
    write_all(2, "\n", 1);
}

int aoc_write_text_file(const char *path, const char *text) {
    int fd;

    if (path == 0 || text == 0) {
        return -1;
    }

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        return -1;
    }
    if (write_all(fd, text, strlen(text)) != 0) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}
