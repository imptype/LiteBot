/* stdlite.h - tiny replacements for the few libc functions this bot uses, so a C library is not needed for them.
 *
 * Why: every page that holds code which has run once stays in RSS. The C library's printf (vfprintf + printf_core + fmt_fp), its stdio
 * write/read/close path, strtol/strtod, strstr and inet_pton add up to about 20 KB of code, and the heartbeat, the logging and the
 * message parsing run them all regularly. These few lines do the same job for the formats this program uses.
 *
 * Include this BEFORE the other project headers: it redirects snprintf / vsnprintf / fprintf with macros, so the
 * fprintf(stderr, ...) calls inside dave.h, mcrypto.h and mtls.h use the small version too.
 *
 * Supported: %s %c %d %i %u %x %X %%, flag 0, a width, length l / ll / z (LP64: all 64-bit).
 * Not supported on purpose: floats, %p, precision, left-justify. Logging always goes to stderr (one write() per message).
 */
#ifndef STDLITE_H
#define STDLITE_H
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PUT(ch) do { if (n + 1 < cap) d[n] = (ch); n++; } while (0)
static int vfmt(char *d, size_t cap, const char *f, va_list ap) {
    size_t n = 0;
    for (; *f; f++) {
        if (*f != '%') { PUT(*f); continue; }
        f++;
        int zero = 0, w = 0, l = 0, neg = 0;
        if (*f == '0') { zero = 1; f++; }
        while (*f >= '0' && *f <= '9') w = w * 10 + (*f++ - '0');
        while (*f == 'l' || *f == 'z') { l++; f++; }
        char c = *f, t[24], *s = t;
        size_t len;
        if (!c) break;
        if (c == '%') { PUT('%'); continue; }
        if (c == 's') { s = va_arg(ap, char *); if (!s) s = "(null)"; len = strlen(s); }
        else if (c == 'c') { t[0] = (char)va_arg(ap, int); len = 1; }
        else if (c == 'd' || c == 'i' || c == 'u' || c == 'x' || c == 'X') {
            unsigned long long v;
            unsigned b = (c == 'x' || c == 'X') ? 16 : 10;
            if (c == 'd' || c == 'i') {
                long long x = l ? va_arg(ap, long long) : va_arg(ap, int);
                neg = x < 0;
                v = neg ? 0ULL - (unsigned long long)x : (unsigned long long)x;
            } else v = l ? va_arg(ap, unsigned long long) : va_arg(ap, unsigned);
            s = t + sizeof t;
            do { *--s = (c == 'X' ? "0123456789ABCDEF" : "0123456789abcdef")[v % b]; v /= b; } while (v);
            len = (size_t)(t + sizeof t - s);
        } else { PUT('%'); PUT(c); continue; }                 /* unsupported conversion: show it as written */
        size_t pad = (size_t)w > len + (size_t)neg ? (size_t)w - len - (size_t)neg : 0;
        int zp = zero && c != 's' && c != 'c';
        if (!zp) for (; pad; pad--) PUT(' ');
        if (neg) PUT('-');
        if (zp) for (; pad; pad--) PUT('0');
        for (size_t i = 0; i < len; i++) PUT(s[i]);
    }
    if (cap) d[n < cap ? n : cap - 1] = 0;
    return (int)n;
}
__attribute__((format(printf, 3, 4))) static int mini_snprintf(char *d, size_t cap, const char *f, ...) {
    va_list ap;
    va_start(ap, f);
    int n = vfmt(d, cap, f, ap);
    va_end(ap);
    return n;
}
__attribute__((format(printf, 1, 2))) static void elog(const char *f, ...) {   /* one write() per message, like unbuffered stderr */
    char b[200];
    va_list ap;
    va_start(ap, f);
    int n = vfmt(b, sizeof b, f, ap);
    va_end(ap);
    if (write(2, b, (size_t)n < sizeof b ? (size_t)n : sizeof b - 1) < 0) { }
}
#undef PUT
#define snprintf  mini_snprintf
#define vsnprintf vfmt
#define fprintf(f, ...) elog(__VA_ARGS__)

/* ---- small string helpers that replace strtoull / strtoul / strtol / strstr ---- */
static uint64_t dec(const char *s) {                           /* decimal string -> number (Discord ids) */
    uint64_t v = 0;
    while (*s >= '0' && *s <= '9') v = v * 10 + (uint64_t)(*s++ - '0');
    return v;
}

static int has(const char *h, const char *n) {                 /* n occurs in h (strstr would pull in about 1.3 KB of two-way search code) */
    size_t l = strlen(n);
    for (; *h; h++) if (*h == *n && !strncmp(h, n, l)) return 1;
    return !l;
}
#endif