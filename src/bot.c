/* bot.c - minimal Discord voice bot in C.
 *
 * Connects to the gateway, joins one voice channel and plays a pre-encoded track on repeat,
 * but only while somebody else is in that channel.
 * Run from the project folder:  build/bot     (reads config.json from the current directory)
 *
 * No malloc anywhere (fixed buffers, a static receive page, mmap for rare big frames) and no JSON library.
 *
 * Files:  mtls.h      TLS 1.3 client (pinned to Discord's certificate chain)
 *         dave.h      DAVE end-to-end encryption for voice
 *         mcrypto.h   SHA-256 / HKDF / AES-GCM / P-256 / fixed buffers
 *         dns.h       tiny IPv4 DNS resolver (replaces getaddrinfo)
 *         json.h      reads fields out of a JSON message in place (replaces a JSON library)
 *         syslite.h   system calls, memcpy / strlen / ..., program entry: the program links no C library at all
 *         stdlite.h   small replacements for printf, strtoull, strstr
 *         audio.h     packet format shared with convert.c (packet length, max packet size)
 *         convert.c   one-time tool: MP3 -> .bin
 */
#define _DEFAULT_SOURCE
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "syslite.h"                                    /* no C library: system calls, memcpy and friends, _start */
#include "stdlite.h"                                    /* then: it redirects snprintf / fprintf for the headers below */
#include "audio.h"                                      /* PACKET_MS, MAX_PACKET, RTP_STEP: must match what convert.c wrote */
#include "dave.h"
#include "dns.h"
#include "json.h"
#include "mtls.h"

/* ---- settings ---- */
#define CONFIG_FILE  "config.json"            /* token, guild_id, vc_channel_id, mp3_file, optional owner_id */
#define INTENTS      (128 | 512)              /* GUILD_VOICE_STATES + GUILD_MESSAGES (we get content of messages that ping us) */
#define GATEWAY_HOST "gateway.discord.gg"
#define REST_HOST    "discord.com"            /* only used to answer the owner's ping with stats */
#define REFRESH_MS   3600000LL                /* voice connection older than this is refreshed when the first listener joins */
#define MAXM         256                      /* max listeners tracked */
#define RETRY_MS     5000                     /* wait before retrying a failed gateway / voice connection */
#define SKIM_MIN     2048                     /* gateway frames bigger than this are peeked at and dropped unread if ignorable */
#define SKIM_HEAD    80                       /* bytes peeked: enough for {"t":"NAME","s":N,"op":0, */

#define COLD __attribute__((cold))            /* runs rarely: keep it away from the hot code pages */

static char token[256], guild[32], channel[32], audio[256], owner[32];

static int64_t now_ms(void) {
    struct timespec t;
    syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &t);          /* raw syscall: libc's clock_gettime would pull in the vdso lookup (1.4 KB of code and the vdso page in RSS) */
    return t.tv_sec * 1000LL + t.tv_nsec / 1000000;
}
static void soon(int64_t *wake, int64_t t) { if (t && t < *wake) *wake = t; } /* earliest timer wins */
static void die(const char *msg) {
    fprintf(stderr, "error: %s\n", msg);
    exit(1);
}

COLD static void cfg_str(const char *root, const char *key, char *dst, size_t cap, int required) {
    const char *v = j_get(root, key);
    if (j_is_str(v) && j_copy(v, dst, cap)) return;
    if (required || v) { fprintf(stderr, "error: " CONFIG_FILE " needs a \"%s\" string\n", key); exit(1); }
}
COLD static void load_config(void) {
    char json[2048];
    int fd = open(CONFIG_FILE, O_RDONLY);
    if (fd < 0) die("cannot open " CONFIG_FILE);
    ssize_t n = read(fd, json, sizeof json - 1);
    close(fd);
    json[n > 0 ? n : 0] = 0;
    if (*j_ws(json) != '{' || !j_skip(json)) die(CONFIG_FILE " is not valid JSON");
    cfg_str(json, "token", token, sizeof token, 1);
    cfg_str(json, "guild_id", guild, sizeof guild, 1);          /* IDs are strings: they exceed what JSON numbers hold exactly */
    cfg_str(json, "vc_channel_id", channel, sizeof channel, 1);
    char name[200];
    cfg_str(json, "mp3_file", name, sizeof name, 1);           /* base name: "Pinnacle" -> assets/Pinnacle.bin */
    snprintf(audio, sizeof audio, "assets/%s.bin", name);
    cfg_str(json, "owner_id", owner, sizeof owner, 0);         /* who may ping the bot to get the stats (empty = disabled) */
}

/* ============================ WebSocket client over TLS ============================ */
typedef struct {
    tls_t t; int fd;
    uint8_t *fp; size_t fn, fcap; int fmap;    /* message being received: data, length, capacity, 1 = pages from mmap (0 = the shared static page) */
    int fop;                                   /* opcode (text / binary) of that message */
    int skim;                                  /* gateway: big ignorable frames are dropped unread */
} ws_t;
static ws_t gw, vc;                            /* gateway and voice connections (fd = -1 when closed, set in main) */
static int vc_open;

/* Receive buffer: one static page is shared by both connections and covers every normal message. Only a message that
 * does not fit (or arrives while the page is taken by an unfinished one) gets its own mmap pages, returned right after use. */
static uint8_t fbuf[4096] __attribute__((aligned(4096)));
static ws_t *fbuf_owner;
static void frag_release(ws_t *w) {
    if (!w->fp) return;
    if (w->fmap) pg_free(w->fp, w->fcap); else fbuf_owner = NULL;
    w->fp = NULL; w->fn = w->fcap = 0; w->fmap = 0;
}
static int frag_grow(ws_t *w, size_t need) {                   /* room for `need` bytes, keeping what is already there */
    if (w->fcap >= need) return 0;
    if (!w->fp && need <= sizeof fbuf && !fbuf_owner) { fbuf_owner = w; w->fp = fbuf; w->fcap = sizeof fbuf; return 0; }
    size_t cap = need + (w->fn ? need / 2 : 0), fn = w->fn;
    cap = (cap + 4095) & ~(size_t)4095;
    uint8_t *np = pg_alloc(cap);
    if (!np) return -1;
    if (fn) memcpy(np, w->fp, fn);
    frag_release(w);
    w->fp = np; w->fcap = cap; w->fmap = 1; w->fn = fn;
    return 0;
}

static int gw_skim_prefix(const char *head);   /* defined with the gateway code: 1 = ignorable event, sequence number recorded */

static int readn(tls_t *s, uint8_t *p, size_t n) {
    while (n) {
        int r = tls_read(s, p, n);
        if (r <= 0) return -1;
        p += r; n -= (size_t)r;
    }
    return 0;
}
/* TCP connect with 10 s send/receive timeouts (also stops a dead peer from blocking us forever). */
static int tcp_open(const char *host, const char *port) {
    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)dec(port));
    if (resolve4(host, &sa.sin_addr)) return -1;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd >= 0) {
        struct timeval tv = { 10, 0 };
        int one = 1;
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
        if (connect(fd, (struct sockaddr *)&sa, sizeof sa)) { close(fd); fd = -1; }
    }
    return fd;
}
static void ws_close(ws_t *w) {
    if (w->fd >= 0) close(w->fd);
    w->fd = w->t.fd = -1;                      /* t.fd too: a stale number could be reused by another socket */
    w->t.off = w->t.end = 0;
    frag_release(w);                           /* drop a half-received message */
}
static int ws_connect(ws_t *w, const char *host, const char *port, const char *path) {
    w->fd = tcp_open(host, port);
    if (w->fd < 0) return -1;
    if (tls_connect(&w->t, w->fd, host)) goto fail;

    /* HTTP upgrade. Read the reply one byte at a time so no WebSocket frame data is consumed. */
    uint8_t k[16];
    char key[32], req[400], st[12] = {0};      /* st: only the start of the status line is kept */
    uint32_t last = 0;                         /* last 4 bytes seen: finds the blank line without storing the headers */
    int rl = 0;
    rnd(k, 16);
    b64(k, 16, key);
    int n = snprintf(req, sizeof req,
                     "GET %s HTTP/1.1\r\nHost: %s\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                     "Sec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n\r\n", path, host, key);
    tls_write(&w->t, (const uint8_t *)req, (size_t)n);
    for (;;) {
        uint8_t ch;
        if (tls_read(&w->t, &ch, 1) != 1 || rl >= 4096) goto fail;
        if (rl < (int)sizeof st) st[rl] = (char)ch;
        rl++;
        last = last << 8 | ch;
        if (last == 0x0d0a0d0aU) break;
    }
    if (!memcmp(st, "HTTP/1.1 101", 12)) return 0;
fail:
    ws_close(w);
    return -1;
}
static int ws_send(ws_t *w, int op, const void *d, size_t n) {   /* client frames are always masked; n <= 1024 */
    uint8_t b[14 + 1024], m[4];
    int hl = 0;
    b[hl++] = 0x80 | op;
    if (n < 126) b[hl++] = 0x80 | (uint8_t)n;
    else { b[hl++] = 0x80 | 126; b[hl++] = (uint8_t)(n >> 8); b[hl++] = (uint8_t)n; }
    rnd(m, 4);
    memcpy(b + hl, m, 4);
    hl += 4;
    for (size_t i = 0; i < n; i++) b[hl + i] = ((const uint8_t *)d)[i] ^ m[i & 3];
    return tls_write(&w->t, b, hl + n);
}
static int ws_text(ws_t *w, const char *s) { return ws_send(w, 1, s, strlen(s)); }
__attribute__((format(printf, 2, 3))) static int ws_fmt(ws_t *w, const char *fmt, ...) {   /* format a JSON message and send it */
    char s[640];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s, sizeof s, fmt, ap);
    va_end(ap);
    return ws_text(w, s);
}

/* Returns 1 = a complete message is in w->fp[0 .. w->fn) with a NUL after it (*op: 1 text / 2 binary; the caller
 * must frag_release(w) when done), 0 = control frame, partial fragment or a skipped message, -1 = closed.
 * Big gateway dispatches (GUILD_CREATE can be hundreds of KB) are never buffered when we would ignore them anyway:
 * the first SKIM_HEAD bytes are enough to tell, and the rest is read through a small stack buffer and discarded. */
static int ws_recv(ws_t *w, int *op) {
    uint8_t h[2], e[8], c[125];
    uint64_t len;
    if (readn(&w->t, h, 2)) return -1;
    int o = h[0] & 15, fin = h[0] >> 7;
    len = h[1] & 127;
    if (len == 126) {
        if (readn(&w->t, e, 2)) return -1;
        len = e[0] << 8 | e[1];
    } else if (len == 127) {
        if (readn(&w->t, e, 8)) return -1;
        len = 0;
        for (int i = 0; i < 8; i++) len = len << 8 | e[i];
    }
    if (o >= 3) {                                              /* control frames carry at most 125 bytes */
        if (o < 8 || o > 10 || len > 125 || readn(&w->t, c, (size_t)len) || o == 8) return -1;   /* reserved opcode, bad size or close */
        if (o == 9) ws_send(w, 10, c, (size_t)len);            /* ping -> pong */
        return 0;                                              /* pong: ignored */
    }
    if (len > (1u << 24)) return -1;                           /* 16 MB sanity limit */

    size_t pre = 0;                                            /* bytes of this message already read into head */
    uint8_t head[SKIM_HEAD + 1];
    if (w->skim && o == 1 && fin && len > SKIM_MIN) {          /* single big text frame on the gateway */
        if (readn(&w->t, head, SKIM_HEAD)) return -1;
        head[SKIM_HEAD] = 0;
        if (gw_skim_prefix((const char *)head)) {
            uint8_t junk[256];
            for (uint64_t left = len - SKIM_HEAD; left; ) {
                size_t n = left < sizeof junk ? (size_t)left : sizeof junk;
                if (readn(&w->t, junk, n)) return -1;
                left -= n;
            }
            return 0;                                          /* nothing buffered, nothing parsed */
        }
        pre = SKIM_HEAD;                                       /* needed after all: continue below with the head already in hand */
    }

    if (o) { w->fop = o; w->fn = 0; }                          /* text / binary starts a message; 0 = continuation */
    if (frag_grow(w, w->fn + (size_t)len + 1)) return -1;      /* +1 for the NUL after the data */
    if (pre) {
        memcpy(w->fp + w->fn, head, pre);
        w->fn += pre;
        len -= pre;
    }
    if (readn(&w->t, w->fp + w->fn, (size_t)len)) return -1;
    w->fn += (size_t)len;
    if (!fin) return 0;
    w->fp[w->fn] = 0;
    *op = w->fop;
    return 1;
}

/* ============================ one-shot REST call (only for the stats reply) ============================ */
COLD static void rest_post(const char *chan, const char *json) {
    static tls_t t;                                            /* BSS: its 16 KB record buffer costs nothing until used */
    int fd = tcp_open(REST_HOST, "443");
    if (fd < 0) { fprintf(stderr, "rest: connect failed\n"); return; }
    if (!tls_connect(&t, fd, REST_HOST)) {
        char req[384];                                         /* headers only; the body goes out as its own write */
        int n = snprintf(req, sizeof req,
                         "POST /api/v10/channels/%s/messages HTTP/1.1\r\nHost: " REST_HOST "\r\n"
                         "Authorization: Bot %s\r\nUser-Agent: DiscordBot (litebot, 1)\r\n"
                         "Content-Type: application/json\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
                         chan, token, strlen(json));
        if (n > 0 && (size_t)n < sizeof req) {
            uint8_t resp[16];
            tls_write(&t, (const uint8_t *)req, (size_t)n);
            tls_write(&t, (const uint8_t *)json, strlen(json));
            tls_read(&t, resp, sizeof resp);                   /* wait for the reply before hanging up */
        }
    }
    close(fd);
    t.off = t.end = 0;
}

/* ============================ state ============================ */
static long gw_seq = -1, vc_seq = -1;
static int64_t gw_hb_iv, gw_hb_next, vc_hb_iv, vc_hb_next;
static int gw_acked = 1, vc_acked = 1;         /* heartbeat ACK seen since the last heartbeat we sent */
static int64_t gw_hb_sent;                     /* when the last scheduled gateway heartbeat went out (0 = none pending) */
static int gw_ping = -1;                       /* gateway heartbeat round trip in ms (-1 = not measured yet) */
static char bot_id[32], vs_session[128], vs_token[256], vs_endpoint[256];
static int have_vs, have_vsu;                  /* got our VOICE_STATE_UPDATE / VOICE_SERVER_UPDATE */

static char stats_chan[32];                    /* channel to answer the owner's ping in; sent from the main loop, not from inside the parser */
static int gw_ready;                           /* READY seen: safe to join voice */
static int in_chan;                            /* we asked the gateway to put us in the channel */
static int64_t gw_retry, join_at, conn_deadline, vc_since, start_ms;
static unsigned long connects, retries, joins; /* stats */

static uint64_t mem[MAXM];                     /* listeners currently in the channel (not counting us) */
static int nmem;
static int mem_has(uint64_t id) { for (int i = 0; i < nmem; i++) if (mem[i] == id) return 1; return 0; }
static int mem_add(uint64_t id) {              /* 1 = newly added */
    if (mem_has(id) || nmem == MAXM) return 0;
    mem[nmem++] = id;
    return 1;
}
static void mem_del(uint64_t id) {
    for (int i = 0; i < nmem; i++) if (mem[i] == id) { mem[i] = mem[--nmem]; return; }
}

static int udp = -1, session_ok, speaking;
static uint32_t ssrc, rtp_ts, tx_nonce;
static uint16_t rtp_seq;
static uint8_t secret[32];                     /* RTP transport key */

static dave_t dv;                              /* DAVE: MLS identity and key package */
static int in_group, keys_on;
static uint8_t sbase[16], skey[16], pend_base[16];
static uint32_t fnonce, fgen;
static int pend_valid;
static unsigned pend_tid, welcomed_tid = 0xffffffffu;

static int audio_fd = -1;                      /* audio file: [u16 len][opus packet]..., read one packet at a time */
static size_t audio_pos;
static unsigned audio_packet, audio_packets;   /* packets played in this loop / packets in the file (PACKET_MS each) */
static int plays;

/* ============================ voice: DAVE glue ============================ */
static void vc_json_tid(int op, unsigned tid) { ws_fmt(&vc, "{\"op\":%d,\"d\":{\"transition_id\":%u}}", op, tid); }
static void send_key_package(void) {
    FBUF(kp, 448);                                             /* the op byte + a key package of at most 394 bytes */
    dave_key_package(&dv, &kp);
    ws_send(&vc, 2, kp.p, kp.n);
}
static void dave_forget(void) {                /* leave the MLS group state behind */
    in_group = keys_on = pend_valid = 0;
    welcomed_tid = 0xffffffffu;
}
static void dave_reset(void) {                 /* forget the group and ask to be re-added with a new key package */
    dave_forget();
    send_key_package();
}
static void activate(void) {                   /* start encrypting with the key from the accepted Welcome */
    memcpy(sbase, pend_base, 16);
    dave_sender_key(sbase, 0, skey);
    fgen = fnonce = 0;
    keys_on = 1;
    pend_valid = 0;
    fprintf(stderr, "voice: encryption ready\n");
}
static void on_vc_binary(const uint8_t *b, size_t n) {
    if (n < 3) return;
    vc_seq = b[0] << 8 | b[1];
    int op = b[2];
    const uint8_t *p = b + 3;
    size_t pl = n - 3;
    unsigned tid = pl >= 2 ? p[0] << 8 | p[1] : 0;             /* ops 29 / 30 start with the transition id */

    switch (op) {
    case 25:                                                   /* external sender */
        if (pl <= sizeof dv.ext_sender) { memcpy(dv.ext_sender, p, pl); dv.ext_len = pl; }
        break;
    case 29:                                                   /* a commit we cannot process (we never track the tree) */
        if (pl >= 2 && in_group && tid != welcomed_tid) { vc_json_tid(31, tid); dave_reset(); }   /* rejoin via a fresh welcome */
        break;
    case 30: {                                                 /* welcome: we are in the group */
        uint8_t ex[32];
        if (pl >= 2 && dave_welcome(&dv, p + 2, pl - 2, ex) == 0) {
            dave_base_secret(ex, dec(bot_id), pend_base);
            pend_valid = 1; pend_tid = tid; in_group = 1; welcomed_tid = tid;
            vc_json_tid(23, tid);
            if (tid == 0) activate();
        }
    } break;
    }
}

/* ============================ voice: connection lifecycle ============================ */
static void leave_voice(void) {                                /* gateway: disconnect from voice */
    ws_fmt(&gw, "{\"op\":4,\"d\":{\"guild_id\":\"%s\",\"channel_id\":null,\"self_mute\":false,\"self_deaf\":false}}", guild);
}
static void vc_close(void) {                                   /* drop the voice sockets and all per-connection state */
    ws_close(&vc);
    vc_open = 0;
    if (udp >= 0) close(udp);
    udp = -1;
    session_ok = speaking = 0;
    vc_hb_iv = 0; vc_seq = -1; vc_since = 0;
    dave_forget();
    dv.ext_len = 0;
}
static void join_reset(int64_t at) {                           /* forget the channel join; the main loop joins again at `at` (0 = now) */
    in_chan = have_vs = have_vsu = 0;
    conn_deadline = 0;
    join_at = at;
}
/* Tear the voice connection down, leave the channel, and join again after `delay` ms. */
static void vc_restart(int64_t delay) {
    vc_close();
    if (in_chan) leave_voice();
    join_reset(now_ms() + delay);
}
static void vc_fail(void) {                                    /* the voice connection broke or never came up */
    if (!session_ok) retries++;
    fprintf(stderr, "voice: lost, rejoining\n");
    vc_restart(RETRY_MS);
}
static void set_speaking(int on) {
    speaking = on;
    ws_fmt(&vc, "{\"op\":5,\"d\":{\"speaking\":%d,\"delay\":0,\"ssrc\":%u}}", on, ssrc);
}

/* A listener appeared / disappeared. Fed by the gateway (VOICE_STATE_UPDATE) and by the voice socket (ops 11 / 13),
 * whichever tells us first; the set makes the duplicate a no-op. */
static void member_joined(uint64_t uid) {
    if (!mem_add(uid)) return;
    joins++;
    fprintf(stderr, "listeners: %d\n", nmem);
    /* first listener and the voice connection is old: reconnect now, while it is silent and nobody else is in the
     * channel, so the connection is fresh for the whole session */
    if (nmem == 1 && session_ok && vc_since && now_ms() - vc_since > REFRESH_MS) {
        fprintf(stderr, "voice: refreshing old connection\n");
        vc_restart(500);
    }
}
static void member_left(uint64_t uid) {
    if (!mem_has(uid)) return;
    mem_del(uid);
    fprintf(stderr, "listeners: %d\n", nmem);
}

/* ============================ voice: JSON ops ============================ */
COLD static void ip_discovery_and_select(const char *d) {
    char ip[48];
    long long sv = j_int(j_get(d, "ssrc"), -1), port = j_int(j_get(d, "port"), 0);
    struct sockaddr_in sa = {0};
    if (sv < 0 || port <= 0 || !j_copy(j_get(d, "ip"), ip, sizeof ip) || !ip4(ip, &sa.sin_addr)) {
        fprintf(stderr, "voice: bad ready message\n"); vc_fail(); return;
    }
    ssrc = (uint32_t)sv;
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)port);
    udp = socket(AF_INET, SOCK_DGRAM, 0);
    connect(udp, (struct sockaddr *)&sa, sizeof sa);
    struct timeval tv = { 5, 0 };
    setsockopt(udp, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);

    uint8_t pk[74] = {0}, rp[74];                              /* IP discovery: learn our public address */
    pk[1] = 1; pk[3] = 70;
    pk[4] = ssrc >> 24; pk[5] = ssrc >> 16; pk[6] = ssrc >> 8; pk[7] = ssrc;
    send(udp, pk, 74, 0);
    if (recv(udp, rp, 74, 0) != 74) { fprintf(stderr, "voice: ip discovery failed\n"); vc_fail(); return; }

    ws_fmt(&vc, "{\"op\":1,\"d\":{\"protocol\":\"udp\",\"data\":{\"address\":\"%s\",\"port\":%d,\"mode\":\"aead_aes256_gcm_rtpsize\"}}}",
           (char *)rp + 8, rp[72] << 8 | rp[73]);
}
static void on_vc_text(const char *txt) {
    const char *vop = j_get(txt, "op");
    if (!vop) return;
    vc_seq = (long)j_int(j_get(txt, "seq"), vc_seq);
    int op = (int)j_int(vop, -1);
    const char *d = j_get(txt, "d");

    switch (op) {
    case 8: {                                                  /* hello -> identify */
        long long iv = j_int(j_get(d, "heartbeat_interval"), 0);
        if (iv <= 0) break;
        vc_hb_iv = iv;
        vc_hb_next = now_ms() + vc_hb_iv / 2;
        vc_acked = 1;
        ws_fmt(&vc, "{\"op\":0,\"d\":{\"server_id\":\"%s\",\"user_id\":\"%s\",\"session_id\":\"%s\",\"token\":\"%s\",\"max_dave_protocol_version\":1}}",
               guild, bot_id, vs_session, vs_token);
    } break;
    case 6: vc_acked = 1; break;                               /* heartbeat ack */
    case 11:                                                   /* clients connect: also lists who is already in the channel */
        for (const char *u = j_first(j_get(d, "user_ids")); u; u = j_next(u)) {
            char id[32];
            if (j_copy(u, id, sizeof id) && strcmp(id, bot_id)) member_joined(dec(id));
        }
        break;
    case 13: {                                                 /* client disconnect */
        char id[32];
        if (j_copy(j_get(d, "user_id"), id, sizeof id)) member_left(dec(id));
    } break;
    case 2: ip_discovery_and_select(d); break;                 /* ready */
    case 4: {                                                  /* session description */
        int i = 0;
        for (const char *k = j_first(j_get(d, "secret_key")); k && i < 32; k = j_next(k)) secret[i++] = (uint8_t)j_int(k, 0);
        session_ok = 1;                                        /* speaking is switched on by the main loop when there is a listener */
        speaking = 0;
        connects++;
        conn_deadline = 0;
        vc_since = now_ms();
        fprintf(stderr, "voice: connected\n");
        send_key_package();
    } break;
    case 21: {                                                 /* DAVE: prepare transition */
        const char *tv = j_get(d, "transition_id");
        if (!tv) break;
        unsigned tid = (unsigned)j_int(tv, 0);
        if (tid == 0) keys_on = 0;                             /* sole-member reset */
        else vc_json_tid(23, tid);
    } break;
    case 22: {                                                 /* DAVE: execute transition */
        const char *tv = j_get(d, "transition_id");
        if (tv && pend_valid && (unsigned)j_int(tv, 0) == pend_tid) activate();
    } break;
    case 24:                                                   /* DAVE: prepare epoch */
        if (j_int(j_get(d, "epoch"), 0) == 1) dave_reset();    /* new group being created */
        break;
    }
}

/* ============================ gateway ============================ */
static void voice_connect(void) {                              /* once we have both voice events, open the voice socket */
    if (!have_vs || !have_vsu || vc_open) return;
    char host[256], port[16] = "443";
    snprintf(host, sizeof host, "%s", vs_endpoint);
    char *c = strchr(host, ':');
    if (c) { *c = 0; snprintf(port, sizeof port, "%s", c + 1); }
    if (ws_connect(&vc, host, port, "/?v=8")) { fprintf(stderr, "voice: websocket connect failed\n"); vc_fail(); return; }
    vc_open = 1;
}
static void join_voice(void) {
    ws_fmt(&gw, "{\"op\":4,\"d\":{\"guild_id\":\"%s\",\"channel_id\":\"%s\",\"self_mute\":false,\"self_deaf\":true}}", guild, channel);
}
static void gw_heartbeat(void) {
    if (gw_seq < 0) ws_text(&gw, "{\"op\":1,\"d\":null}");
    else ws_fmt(&gw, "{\"op\":1,\"d\":%ld}", gw_seq);
}
static void gw_fail(void) {                                    /* gateway died: drop everything, main loop reconnects */
    fprintf(stderr, "gateway: lost, reconnecting\n");
    ws_close(&gw);
    vc_close();
    gw_ready = nmem = 0;
    join_reset(0);
    gw_hb_iv = 0; gw_seq = -1;
    gw_hb_sent = 0; gw_ping = -1;
    gw_retry = now_ms() + RETRY_MS;
}
static void fmt_dur(int64_t ms, char *o, size_t n) {
    long long s = ms / 1000;
    snprintf(o, n, "%lldd %lldh %lldm %llds", s / 86400, s % 86400 / 3600, s % 3600 / 60, s % 60);
}
static int can_send(void);                                     /* defined in the audio section */
static void fmt_clock(unsigned packets, char *o, size_t n) {   /* PACKET_MS packets -> m:ss */
    unsigned s = packets * PACKET_MS / 1000;
    snprintf(o, n, "%u:%02u", s / 60, s % 60);
}
COLD static void send_stats(const char *chan) {
    char up[48], age[48] = "n/a", pos[16], len[16], ping[16] = "n/a", ptime[48], js[512];
    int64_t now = now_ms();
    fmt_dur(now - start_ms, up, sizeof up);
    if (session_ok && vc_since) fmt_dur(now - vc_since, age, sizeof age);
    if (gw_ping >= 0) snprintf(ping, sizeof ping, "%dms", gw_ping);
    fmt_clock(audio_packet, pos, sizeof pos);
    fmt_clock(audio_packets, len, sizeof len);
    /* total audio played: finished loops * track length + position in the current loop (PACKET_MS per packet) */
    int64_t played = plays > 0 ? (int64_t)(plays - 1) * audio_packets + audio_packet : 0;
    fmt_dur(played * PACKET_MS, ptime, sizeof ptime);
    snprintf(js, sizeof js,
             "{\"content\":\"```js\\nping: %s\\nuptime: %s\\nvc_age: %s\\nplaytime: %s\\nconnects: %lu\\nretries: %lu\\njoins: %lu\\nplays: %d\\n"
             "playing: %s\\nlisteners: %d\\ntimestamp: %s/%s\\n```\"}",
             ping, up, age, ptime, connects, retries, joins, plays, can_send() ? "True" : "False", nmem, pos, len);
    rest_post(chan, js);
}

/* Dispatches look like {"t":"NAME","s":N,"op":0,"d":...}. For events we never act on (GUILD_CREATE alone can be hundreds
 * of KB, plus every chat message in the guild) only the sequence number is needed, so skip building a JSON tree.
 * Returns 1 if the message was fully handled here; anything unexpected returns 0 and is parsed normally.
 * txt is NUL-terminated. whole = 1: it is the complete message; whole = 0: only its first bytes (the big-frame peek in
 * ws_recv), so a check that needs the full text (is this chat message pinging us?) cannot be made and we play safe. */
static int is_t(const char *t, size_t tl, const char *n) { return strlen(n) == tl && !memcmp(t, n, tl); }
static int gw_skippable_core(const char *txt, int whole) {
    if (strncmp(txt, "{\"t\":\"", 6)) return 0;
    const char *t = txt + 6, *e = strchr(t, '"');
    if (!e || strncmp(e, "\",\"s\":", 6)) return 0;
    size_t tl = (size_t)(e - t);
    if (is_t(t, tl, "READY") || is_t(t, tl, "VOICE_STATE_UPDATE") || is_t(t, tl, "VOICE_SERVER_UPDATE")) return 0;
    if (is_t(t, tl, "MESSAGE_CREATE") && *owner && (!whole || has(txt, bot_id))) return 0;   /* only a message that could ping us matters */
    const char *end = e + 6;
    long s = 0;
    while (*end >= '0' && *end <= '9') s = s * 10 + (*end++ - '0');
    if (end == e + 6 || strncmp(end, ",\"op\":0,", 8)) return 0;
    gw_seq = s;
    return 1;
}
static int gw_skippable(const char *txt) { return gw_skippable_core(txt, 1); }
static int gw_skim_prefix(const char *head) { return gw_skippable_core(head, 0); }

static void on_gw_text(const char *txt) {
    if (gw_skippable(txt)) return;
    const char *vop = j_get(txt, "op");
    if (!vop) return;
    gw_seq = (long)j_int(j_get(txt, "s"), gw_seq);             /* "s" is null on everything but dispatches */
    int op = (int)j_int(vop, -1);
    const char *d = j_get(txt, "d");

    switch (op) {
    case 10: {                                                 /* hello -> identify */
        long long iv = j_int(j_get(d, "heartbeat_interval"), 0);
        if (iv <= 0) break;
        gw_hb_iv = iv;
        gw_hb_next = now_ms() + gw_hb_iv / 2;
        gw_acked = 1;
        ws_fmt(&gw, "{\"op\":2,\"d\":{\"token\":\"%s\",\"intents\":%d,\"properties\":{\"os\":\"linux\",\"browser\":\"c\",\"device\":\"c\"}}}",
               token, INTENTS);
    } break;
    case 11:                                                   /* heartbeat ack */
        gw_acked = 1;
        if (gw_hb_sent) { gw_ping = (int)(now_ms() - gw_hb_sent); gw_hb_sent = 0; }
        break;
    case 1: gw_heartbeat(); break;
    case 7: case 9: gw_fail(); break;                          /* reconnect / invalid session: start over */
    case 0: {
        const char *t = j_get(txt, "t");
        if (j_eq(t, "READY")) {
            if (!j_copy(j_get(j_get(d, "user"), "id"), bot_id, sizeof bot_id)) break;
            dave_init(&dv, dec(bot_id));
            gw_ready = 1;                                      /* joining does not depend on who is in the channel */
            fprintf(stderr, "gateway: connected\n");
        } else if (j_eq(t, "VOICE_STATE_UPDATE")) {            /* someone (or us) joined, left or moved */
            char uid[32];
            if (j_copy(j_get(d, "user_id"), uid, sizeof uid)) {
                int here = j_eq(j_get(d, "channel_id"), channel);
                if (!strcmp(uid, bot_id)) {
                    if (here && j_copy(j_get(d, "session_id"), vs_session, sizeof vs_session)) {   /* carries our voice session id */
                        have_vs = 1;
                    } else if (!here && in_chan && have_vs) {  /* kicked or moved: come back */
                        fprintf(stderr, "voice: removed from channel\n");
                        vc_restart(1500);
                    }
                } else {
                    if (here) member_joined(dec(uid));
                    else member_left(dec(uid));
                }
            }
        } else if (j_eq(t, "VOICE_SERVER_UPDATE")) {           /* carries the voice server and its token */
            const char *tk = j_get(d, "token"), *ep = j_get(d, "endpoint");
            if (j_is_str(tk) && j_is_str(ep)) {                /* endpoint is null while Discord has no voice server for us yet */
                if (vc_open || session_ok) { vc_close(); conn_deadline = now_ms() + 20000; }   /* server moved: reconnect */
                if (j_copy(tk, vs_token, sizeof vs_token) && j_copy(ep, vs_endpoint, sizeof vs_endpoint)) have_vsu = 1;
            }
        } else if (j_eq(t, "MESSAGE_CREATE") && *owner) {      /* owner pings us: reply with the stats */
            char cid[32];
            if (j_eq(j_get(j_get(d, "author"), "id"), owner) && j_copy(j_get(d, "channel_id"), cid, sizeof cid))
                for (const char *m = j_first(j_get(d, "mentions")); m; m = j_next(m))   /* the bot must be among the mentioned users */
                    if (j_eq(j_get(m, "id"), bot_id)) { snprintf(stats_chan, sizeof stats_chan, "%s", cid); break; }
        }
    } break;
    }
}

/* ============================ audio out ============================ */
static void put32(uint8_t *p, uint32_t v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

/* pkt+12 holds n payload bytes; the RTP packet is built and encrypted in place (gcm allows in == out). */
static void send_rtp(uint8_t *pkt, size_t n) {
    uint8_t iv[12] = {0}, tag[16];
    pkt[0] = 0x80; pkt[1] = 0x78;
    pkt[2] = rtp_seq >> 8; pkt[3] = (uint8_t)rtp_seq;
    put32(pkt + 4, rtp_ts);
    put32(pkt + 8, ssrc);
    rtp_seq++;
    rtp_ts += RTP_STEP;                                        /* Discord's RTP clock is 48 kHz whatever SAMPLE_RATE is */
    put32(iv, ++tx_nonce);                                     /* AES-256-GCM: nonce = counter || zeros, AAD = RTP header */
    if (!gcm(1, 32, secret, iv, pkt, 12, pkt + 12, n, pkt + 12, tag)) return;
    memcpy(pkt + 12 + n, tag, 16);
    put32(pkt + 28 + n, tx_nonce);
    send(udp, pkt, 32 + n, 0);
}
static void audio_tick(void) {                                 /* called every PACKET_MS: send the next Opus packet */
    uint8_t pkt[12 + MAX_PACKET + 88];                         /* one buffer: RTP header | Opus -> DAVE frame -> encrypted | tag | nonce (88 = room for the DAVE overhead, tag and nonce) */
    size_t ol = 0;
    for (int pass = 0; pass < 2 && !ol; pass++) {
        ssize_t got = pread(audio_fd, pkt + 10, 2 + MAX_PACKET, (off_t)audio_pos);   /* [u16 len] lands in pkt[10..11], Opus data at pkt+12 */
        ol = got >= 2 ? (size_t)(pkt[10] << 8 | pkt[11]) : 0;
        if (!ol || 2 + ol > (size_t)got) { ol = 0; audio_pos = 0; audio_packet = 0; }   /* end of the track (or bad data): start over */
    }
    if (!ol) return;
    if (!audio_pos) fprintf(stderr, "playing %d\n", ++plays);
    audio_pos += 2 + ol;
    audio_packet++;

    if ((fnonce + 1) >> 24 != fgen) {                          /* end-to-end encrypt the frame, then send it */
        fgen = (fnonce + 1) >> 24;
        dave_sender_key(sbase, fgen, skey);
    }
    size_t ln = dave_encrypt_frame(skey, ++fnonce, pkt + 12, ol, pkt + 12);   /* in place */
    if (ln) send_rtp(pkt, ln);
}
/* play only while the voice session and DAVE keys are ready AND somebody is listening */
static int can_send(void) { return session_ok && udp >= 0 && keys_on && nmem > 0; }

COLD static void load_audio(void) {                            /* open the packet file; packets are read on demand, nothing is mapped */
    off_t size = 0;
    audio_fd = open(audio, O_RDONLY);
    if (audio_fd < 0 || (size = lseek(audio_fd, 0, SEEK_END)) < 3) die("cannot open the .bin audio file (make creates it from the .mp3)");
    uint8_t h[2];                                              /* walk the packet headers once to get the track length */
    for (off_t p = 0; pread(audio_fd, h, 2, p) == 2; ) {
        size_t l = h[0] << 8 | h[1];
        if (!l || p + 2 + (off_t)l > size) break;
        p += 2 + (off_t)l;
        audio_packets++;
    }
}

/* ============================ main loop ============================ */
static void pump(ws_t *w, int is_gateway) {                    /* handle every message already waiting on a connection */
    int op;
    do {
        int r = ws_recv(w, &op);
        if (r < 0) { if (is_gateway) gw_fail(); else vc_fail(); break; }
        if (r == 1) {
            if (is_gateway) on_gw_text((char *)w->fp);
            else if (op == 1) on_vc_text((char *)w->fp);
            else on_vc_binary(w->fp, w->fn);
            frag_release(w);                                   /* a handler may already have closed (and released) the connection */
        }
    } while (w->fd >= 0 && tls_pending(&w->t));
}
/* timers that decide when to join / give up on a join */
static void tend(int64_t now) {
    if (!gw_ready) return;
    if (!in_chan && !join_at) join_at = now;
    if (join_at && now >= join_at) {
        join_at = 0;
        in_chan = 1;
        conn_deadline = now + 20000;
        join_voice();
    }
    if (in_chan && !session_ok && conn_deadline && now >= conn_deadline) {   /* join never completed: retry */
        conn_deadline = 0;
        vc_fail();
    }
}
static volatile sig_atomic_t quit;                             /* set by Ctrl+C / SIGTERM */
static void on_signal(int sig) { (void)sig; quit = 1; }

int main(void) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    load_config();
    load_audio();
    gw.fd = gw.t.fd = vc.fd = vc.t.fd = -1;
    gw.skim = 1;                                               /* only the gateway carries huge dispatches we may drop */
    start_ms = now_ms();

    int64_t next_tick = 0;
    while (!quit) {
        int64_t now = now_ms(), wake = now + 1000;

        if (gw.fd < 0 && now >= gw_retry) {                    /* (re)connect the gateway; retry after RETRY_MS on failure */
            if (ws_connect(&gw, GATEWAY_HOST, "443", "/?v=10&encoding=json")) {
                fprintf(stderr, "gateway: connect failed, retrying\n");
                gw_retry = now_ms() + RETRY_MS;
            }
        }
        tend(now);
        voice_connect();                                       /* top level: the handshake stack starts from main, not from pump > on_gw_text */
        if (stats_chan[0]) { send_stats(stats_chan); stats_chan[0] = 0; }

        /* sleep until something is readable or a timer is due */
        soon(&wake, gw_hb_iv ? gw_hb_next : 0);
        soon(&wake, vc_open && vc_hb_iv ? vc_hb_next : 0);
        soon(&wake, gw.fd < 0 ? gw_retry : 0);
        soon(&wake, join_at);
        soon(&wake, in_chan && !session_ok ? conn_deadline : 0);
        int sending = can_send();
        if (sending && next_tick < wake) wake = next_tick;

        int rg = gw.fd >= 0 && tls_pending(&gw.t), rv = vc_open && tls_pending(&vc.t);
        if (!rg && !rv) {
            struct pollfd pf[2] = { { gw.fd, POLLIN, 0 }, { vc_open ? vc.fd : -1, POLLIN, 0 } };
            poll(pf, 2, wake > now ? (int)(wake - now) : 0);
            rg = pf[0].revents & (POLLIN | POLLHUP | POLLERR);
            rv = vc_open && (pf[1].revents & (POLLIN | POLLHUP | POLLERR));
        }
        if (rg && gw.fd >= 0) pump(&gw, 1);
        if (rv && vc_open) pump(&vc, 0);

        now = now_ms();
        if (gw_hb_iv && now >= gw_hb_next) {
            if (!gw_acked) gw_fail();                          /* no ACK for a whole interval: connection is dead */
            else { gw_acked = 0; gw_hb_sent = now; gw_heartbeat(); gw_hb_next = now + gw_hb_iv; }
        }
        if (vc_open && vc_hb_iv && now >= vc_hb_next) {
            if (!vc_acked) vc_fail();
            else {
                vc_acked = 0;
                ws_fmt(&vc, "{\"op\":3,\"d\":{\"t\":%lld,\"seq_ack\":%ld}}", (long long)now, vc_seq);
                vc_hb_next = now + vc_hb_iv;
            }
        }
        int cs = can_send();
        if (vc_open && session_ok && cs != speaking) set_speaking(cs);   /* speaking flag follows "is anyone listening" */
        if (cs) {                                              /* pace audio at exactly PACKET_MS per packet */
            if (!sending) next_tick = now;                     /* just became ready: start pacing from now */
            for (int i = 0; i < 5 && now >= next_tick; i++) {
                audio_tick();
                next_tick += PACKET_MS;
            }
            if (now - next_tick > 200) next_tick = now;
        }
    }

    /* shutting down: leave the voice channel, then close the gateway politely */
    if (gw.fd >= 0) {
        if (in_chan) leave_voice();
        ws_send(&gw, 8, "\x03\xe8", 2);                          /* close frame, 1000 normal */
        ws_close(&gw);
    }
    ws_close(&vc);
    fprintf(stderr, "bye\n");
    return 0;
}