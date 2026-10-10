/* syslite.h - the Linux kernel interface and the few memory / string functions this bot uses, with no C library at all.
 *
 * With this header the program is built without musl or glibc:   gcc -static -nostdlib ... -lgcc
 * The system headers are still included, but only for types and constants (sockaddr_in, pollfd, O_RDONLY, ...): they contain no
 * code. Everything that would otherwise come from the C library is defined below:
 *
 *   - system calls: read write pread lseek open close socket connect send recv setsockopt poll getrandom mmap munmap
 *     syscall exit signal. Each one is a single `syscall` instruction; on failure they return -1 and set errno, like libc.
 *   - memcpy memmove memset memcmp memchr strlen strcmp strncmp strncasecmp strchr strcat (the compiler also calls memcpy,
 *     memset, memmove and memcmp by itself for larger copies)
 *   - _start, the program entry point, which calls main() and exits
 *
 * x86-64 Linux only. On another system drop this header from bot.c and build with the normal C library instead.
 * Include it from bot.c after the system headers and before stdlite.h.
 */
#ifndef SYSLITE_H
#define SYSLITE_H
#ifndef __x86_64__
#error "syslite.h is x86-64 Linux only: remove it from bot.c and link against a normal C library"
#endif
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

/* ---------------- system calls ---------------- */
static inline long sl_sys(long n, long a, long b, long c, long d, long e, long f) {
    register long r10 __asm__("r10") = d, r8 __asm__("r8") = e, r9 __asm__("r9") = f;
    long ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return ret;
}
static int sl_errno;
int *__errno_location(void) { return &sl_errno; }              /* what the `errno` macro in <errno.h> expands to */
static long sl_ret(long r) {                                   /* the kernel returns -errno in -4095..-1 */
    if ((unsigned long)r > (unsigned long)-4096) { sl_errno = (int)-r; return -1; }
    return r;
}
#define SYS(n, a, b, c, d, e, f) sl_ret(sl_sys((n), (long)(a), (long)(b), (long)(c), (long)(d), (long)(e), (long)(f)))

ssize_t read(int fd, void *b, size_t n) { return SYS(SYS_read, fd, b, n, 0, 0, 0); }
ssize_t write(int fd, const void *b, size_t n) { return SYS(SYS_write, fd, b, n, 0, 0, 0); }
ssize_t pread(int fd, void *b, size_t n, off_t o) { return SYS(SYS_pread64, fd, b, n, o, 0, 0); }
off_t lseek(int fd, off_t o, int w) { return SYS(SYS_lseek, fd, o, w, 0, 0, 0); }
int close(int fd) { return (int)SYS(SYS_close, fd, 0, 0, 0, 0, 0); }
int open(const char *p, int flags, ...) {
    va_list ap; va_start(ap, flags);
    unsigned mode = (flags & O_CREAT) ? va_arg(ap, unsigned) : 0;
    va_end(ap);
    return (int)SYS(SYS_open, p, flags, mode, 0, 0, 0);
}
int socket(int d, int t, int p) { return (int)SYS(SYS_socket, d, t, p, 0, 0, 0); }
int connect(int fd, const struct sockaddr *a, socklen_t l) { return (int)SYS(SYS_connect, fd, a, l, 0, 0, 0); }
ssize_t send(int fd, const void *b, size_t n, int fl) { return SYS(SYS_sendto, fd, b, n, fl, 0, 0); }
ssize_t recv(int fd, void *b, size_t n, int fl) { return SYS(SYS_recvfrom, fd, b, n, fl, 0, 0); }
int setsockopt(int fd, int lvl, int name, const void *v, socklen_t l) { return (int)SYS(SYS_setsockopt, fd, lvl, name, v, l, 0); }
int poll(struct pollfd *f, nfds_t n, int ms) { return (int)SYS(SYS_poll, f, n, ms, 0, 0, 0); }
ssize_t getrandom(void *b, size_t n, unsigned fl) { return SYS(SYS_getrandom, b, n, fl, 0, 0, 0); }
void *mmap(void *a, size_t l, int prot, int fl, int fd, off_t o) {
    long r = SYS(SYS_mmap, a, l, prot, fl, fd, o);
    return r == -1 ? MAP_FAILED : (void *)r;
}
int munmap(void *a, size_t l) { return (int)SYS(SYS_munmap, a, l, 0, 0, 0, 0); }
long syscall(long n, ...) {                                    /* bot.c reads the clock with syscall(SYS_clock_gettime, ...) */
    va_list ap; va_start(ap, n);
    long a = va_arg(ap, long), b = va_arg(ap, long), c = va_arg(ap, long), d = va_arg(ap, long), e = va_arg(ap, long), f = va_arg(ap, long);
    va_end(ap);
    return SYS(n, a, b, c, d, e, f);
}
__attribute__((noreturn)) void exit(int code) {
    for (;;) sl_sys(SYS_exit_group, code, 0, 0, 0, 0, 0);
}

/* signal(): the kernel's sigaction wants a return stub that calls rt_sigreturn (system call 15) */
struct sl_ksigaction { void (*handler)(int); unsigned long flags; void (*restorer)(void); unsigned long mask; };
__attribute__((naked)) static void sl_restore(void) { __asm__ volatile("mov $15, %eax\n\tsyscall"); }
void (*signal(int sig, void (*handler)(int)))(int) {
    struct sl_ksigaction a = { handler, 0x04000000UL /* SA_RESTORER */ | 0x10000000UL /* SA_RESTART */, sl_restore, 0 };
    return SYS(SYS_rt_sigaction, sig, &a, 0, 8, 0, 0) < 0 ? SIG_ERR : SIG_DFL;
}

#ifndef htons                                                  /* with optimisation on, <arpa/inet.h> already makes this a macro */
uint16_t htons(uint16_t v) { return (uint16_t)(v >> 8 | v << 8); }
#endif

/* ---------------- memory and string functions ---------------- */
/* The loops below must stay loops: GCC may turn a copy loop into a call to memcpy, which inside memcpy itself would never end.
 * The Makefile builds with -fno-tree-loop-distribute-patterns, and `make check` looks for such a call. */
void *memcpy(void *d, const void *s, size_t n) {
    void *r = d;
    __asm__ volatile("rep movsb" : "+D"(d), "+S"(s), "+c"(n) : : "memory");
    return r;
}
void *memset(void *d, int c, size_t n) {
    void *r = d;
    __asm__ volatile("rep stosb" : "+D"(d), "+c"(n) : "a"(c) : "memory");
    return r;
}
void *memmove(void *d, const void *s, size_t n) {
    uint8_t *x = d; const uint8_t *y = s;
    if (x <= y || x >= y + n) { while (n--) *x++ = *y++; }
    else { x += n; y += n; while (n--) *--x = *--y; }
    return d;
}
int memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *x = a, *y = b;
    for (; n; n--, x++, y++) if (*x != *y) return *x - *y;
    return 0;
}
void *memchr(const void *s, int c, size_t n) {
    const uint8_t *p = s;
    for (; n; n--, p++) if (*p == (uint8_t)c) return (void *)p;
    return NULL;
}
size_t strlen(const char *s) { const char *p = s; while (*p) p++; return (size_t)(p - s); }
int strcmp(const char *a, const char *b) {
    for (; *a && *a == *b; a++, b++) ;
    return (uint8_t)*a - (uint8_t)*b;
}
int strncmp(const char *a, const char *b, size_t n) {
    for (; n && *a && *a == *b; n--, a++, b++) ;
    return n ? (uint8_t)*a - (uint8_t)*b : 0;
}
int strncasecmp(const char *a, const char *b, size_t n) {      /* ASCII only */
    for (; n; n--, a++, b++) {
        int x = (uint8_t)*a, y = (uint8_t)*b;
        if (x >= 'A' && x <= 'Z') x += 32;
        if (y >= 'A' && y <= 'Z') y += 32;
        if (x != y || !x) return x - y;
    }
    return 0;
}
char *strchr(const char *s, int c) {
    for (;; s++) { if (*s == (char)c) return (char *)s; if (!*s) return NULL; }
}
char *strcat(char *d, const char *s) {
    char *p = d + strlen(d);
    while ((*p++ = *s++)) ;
    return d;
}

/* ---------------- program entry ---------------- */
int main(void);
__attribute__((noreturn, used)) void sl_start(void) { exit(main()); }
__asm__(".text\n"
        ".globl _start\n"
        "_start:\n\t"
        "xor %ebp, %ebp\n\t"                                   /* mark the outermost frame */
        "and $-16, %rsp\n\t"                                   /* the ABI wants a 16-byte aligned stack at every call */
        "call sl_start\n\t"
        "hlt\n");
#endif