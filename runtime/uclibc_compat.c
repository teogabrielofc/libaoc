#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

int __printf_chk(int flag, const char *format, ...) {
    int rc;
    va_list ap;
    (void)flag;
    va_start(ap, format);
    rc = vfprintf(stdout, format, ap);
    va_end(ap);
    return rc;
}

int __fprintf_chk(FILE *stream, int flag, const char *format, ...) {
    int rc;
    va_list ap;
    (void)flag;
    va_start(ap, format);
    rc = vfprintf(stream, format, ap);
    va_end(ap);
    return rc;
}

int __vfprintf_chk(FILE *stream, int flag, const char *format, va_list ap) {
    (void)flag;
    return vfprintf(stream, format, ap);
}

int __snprintf_chk(char *s, size_t maxlen, int flag, size_t slen, const char *format, ...) {
    int rc;
    va_list ap;
    (void)flag;
    (void)slen;
    va_start(ap, format);
    rc = vsnprintf(s, maxlen, format, ap);
    va_end(ap);
    return rc;
}

int __vsnprintf_chk(char *s, size_t maxlen, int flag, size_t slen, const char *format, va_list ap) {
    (void)flag;
    (void)slen;
    return vsnprintf(s, maxlen, format, ap);
}

void *__memcpy_chk(void *dest, const void *src, size_t len, size_t destlen) {
    (void)destlen;
    return memcpy(dest, src, len);
}

void *__memset_chk(void *dest, int c, size_t len, size_t destlen) {
    (void)destlen;
    return memset(dest, c, len);
}

char *__strncpy_chk(char *dest, const char *src, size_t len, size_t destlen) {
    (void)destlen;
    return strncpy(dest, src, len);
}

int __isoc99_sscanf(const char *str, const char *format, ...) {
    int rc;
    va_list ap;
    va_start(ap, format);
    rc = vsscanf(str, format, ap);
    va_end(ap);
    return rc;
}

int __isoc99_vsscanf(const char *str, const char *format, va_list ap) {
    return vsscanf(str, format, ap);
}

int __gettimeofday64(void *tv, void *tz) {
    return gettimeofday((struct timeval *)tv, (struct timezone *)tz);
}

const int32_t **__ctype_toupper_loc(void) {
    static int initialized = 0;
    static int32_t table[384];
    static const int32_t *table_ptr = table + 128;
    int i;

    if (!initialized) {
        for (i = -128; i < 256; ++i) {
            unsigned char c = (unsigned char)i;
            if (c >= 'a' && c <= 'z') {
                table[i + 128] = (int32_t)(c - ('a' - 'A'));
            } else {
                table[i + 128] = (int32_t)c;
            }
        }
        initialized = 1;
    }

    return &table_ptr;
}
