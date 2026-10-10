/* dns.h - minimal DNS resolver for the bot: IPv4 only, first nameserver of /etc/resolv.conf, one A query over UDP.
 * Replaces getaddrinfo, which pulls about 15 KB of musl resolver code into the static binary.
 * Not supported on purpose: IPv6, /etc/hosts, search domains, a second nameserver, TCP fallback.
 */
#ifndef DNS_H
#define DNS_H
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include "mcrypto.h"

/* dotted quad -> *a. 1 = ok. Same rules as inet_pton(AF_INET), which would cost about 900 bytes of libc. */
static int ip4(const char *s, struct in_addr *a) {
    uint8_t b[4];
    for (int i = 0; i < 4; i++) {
        unsigned v = 0, j = 0;
        while (j < 3 && s[j] >= '0' && s[j] <= '9') v = v * 10 + (unsigned)(s[j++] - '0');
        if (!j || (j > 1 && s[0] == '0') || v > 255) return 0;
        b[i] = (uint8_t)v; s += j;
        if (i < 3 && *s++ != '.') return 0;
    }
    if (*s) return 0;
    memcpy(a, b, 4);
    return 1;
}
static int dns_skip(const uint8_t *r, size_t m, size_t *pos) {   /* step over a (possibly compressed) name */
    while (*pos < m) {
        uint8_t c = r[*pos];
        if (c == 0) { (*pos)++; return 0; }
        if (c >= 0xC0) { *pos += 2; return 0; }                    /* a compression pointer ends the name */
        *pos += 1 + c;
    }
    return -1;
}
static int dns_parse(const uint8_t *q, const uint8_t *r, size_t m, struct in_addr *ip) {   /* first A record of the reply */
    if (m < 12 || r[0] != q[0] || r[1] != q[1] || !(r[2] & 0x80) || (r[3] & 15)) return -1;   /* other id, not a reply, or an error */
    size_t pos = 12, qd = r[4] << 8 | r[5], an = r[6] << 8 | r[7];
    while (qd--) { if (dns_skip(r, m, &pos)) return -1; pos += 4; }
    while (an--) {                                                 /* CNAME records come first and are skipped */
        if (dns_skip(r, m, &pos) || pos + 10 > m) return -1;
        size_t type = r[pos] << 8 | r[pos + 1], rdl = r[pos + 8] << 8 | r[pos + 9];
        pos += 10;
        if (pos + rdl > m) return -1;
        if (type == 1 && rdl == 4) { memcpy(ip, r + pos, 4); return 0; }
        pos += rdl;
    }
    return -1;
}
static int nameserver(struct in_addr *ns) {                        /* first IPv4 "nameserver" line */
    char c[1024], *l = c;
    int fd = open("/etc/resolv.conf", O_RDONLY);
    if (fd < 0) return -1;
    ssize_t n = read(fd, c, sizeof c - 1);
    close(fd);
    if (n <= 0) return -1;
    c[n] = 0;
    while (l) {
        char *e = strchr(l, '\n');
        if (e) *e = 0;
        if (!strncmp(l, "nameserver", 10) && (l[10] == ' ' || l[10] == '\t')) {
            char *a = l + 11, *t;
            while (*a == ' ' || *a == '\t') a++;
            for (t = a; *t && *t != ' ' && *t != '\t' && *t != '\r'; t++) ;
            *t = 0;
            if (ip4(a, ns)) return 0;
        }
        l = e ? e + 1 : NULL;
    }
    return -1;
}
static int resolve4(const char *host, struct in_addr *ip) {        /* 0 = *ip filled in */
    struct sockaddr_in ns = {0};
    if (ip4(host, ip)) return 0;                                   /* already a numeric address */
    ns.sin_family = AF_INET;
    ns.sin_port = htons(53);
    if (nameserver(&ns.sin_addr)) return -1;

    uint8_t q[300], r[512];
    size_t n = 12;
    memset(q, 0, 12);
    rnd(q, 2);                                                     /* query id */
    q[2] = 1; q[5] = 1;                                            /* recursion desired, one question */
    for (const char *s = host; *s; ) {                             /* host name as length-prefixed labels */
        const char *e = strchr(s, '.');
        size_t l = e ? (size_t)(e - s) : strlen(s);
        if (!l || l > 63 || n + l + 6 > sizeof q) return -1;
        q[n++] = (uint8_t)l;
        memcpy(q + n, s, l);
        n += l;
        s += l + (e != NULL);
    }
    memcpy(q + n, "\0\0\1\0\1", 5);                                /* root label, type A, class IN */
    n += 5;

    int fd = socket(AF_INET, SOCK_DGRAM, 0), res = -1;
    if (fd < 0) return -1;
    struct timeval tv = { 2, 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    if (!connect(fd, (struct sockaddr *)&ns, sizeof ns))
        for (int tries = 0; tries < 3 && res; tries++)             /* resend on timeout */
            if (send(fd, q, n, 0) == (ssize_t)n) {
                ssize_t m = recv(fd, r, sizeof r, 0);
                if (m > 0) res = dns_parse(q, r, (size_t)m, ip);
            }
    close(fd);
    return res;
}
#endif