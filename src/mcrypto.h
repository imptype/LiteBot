/* mcrypto.h - the small crypto kit for the bot, no OpenSSL:
 * SHA-256, HMAC, HKDF, AES-128/256-GCM, P-256 (p256.h), random bytes, base64, a fixed-size buffer.
 * Nothing in here allocates: buffers live on the caller's stack, so the program never needs malloc.
 * AES uses table lookups (not constant-time); acceptable for a bot that only talks to Discord.
 */
#ifndef MCRYPTO_H
#define MCRYPTO_H
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/random.h>
#ifdef __x86_64__
#include <cpuid.h>
#include <immintrin.h>
/* AES-NI and PCLMULQDQ do one AES block / one GHASH multiply in a few cycles instead of a few hundred. They use the same
 * round-key bytes and the same data layout as the software code below, so they need no extra memory; the software code stays
 * as the fallback for CPUs without them (and for non-x86 builds, where this block is not compiled at all). */
static int hw = -1;                                            /* bit 0: AES-NI, bit 1: PCLMULQDQ; probed on first use */
static int hw_has(int bit) {
    unsigned a, b, c, d;
    if (hw < 0) hw = __get_cpuid(1, &a, &b, &c, &d) ? (int)(((c >> 25) & 1) | (((c >> 1) & 1) << 1)) : 0;
    return (hw >> bit) & 1;
}
#endif

static void rnd(void *p, size_t n) {
    uint8_t *b = p;
    while (n) {
        ssize_t r = getrandom(b, n, 0);
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) { fprintf(stderr, "error: getrandom failed\n"); exit(1); }
        b += r; n -= (size_t)r;
    }
}
#include "p256.h"                                              /* P-256: needs rnd() above */
static int ct_memcmp(const void *a, const void *b, size_t n) {   /* 0 = equal */
    const uint8_t *x = a, *y = b; uint8_t d = 0;
    while (n--) d |= *x++ ^ *y++;
    return d;
}
static void b64(const uint8_t *in, size_t n, char *out) {        /* standard base64 with padding, NUL-terminated */
    static const char A[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t o = 0;
    for (size_t i = 0; i < n; i += 3) {
        uint32_t v = (uint32_t)in[i] << 16 | (i + 1 < n ? in[i + 1] << 8 : 0) | (i + 2 < n ? in[i + 2] : 0);
        out[o++] = A[v >> 18 & 63]; out[o++] = A[v >> 12 & 63];
        out[o++] = i + 1 < n ? A[v >> 6 & 63] : '='; out[o++] = i + 2 < n ? A[v & 63] : '=';
    }
    out[o] = 0;
}

/* ---------- byte buffer with caller-provided storage (never grows, never allocates) ---------- */
typedef struct { uint8_t *p; size_t n, cap; } buf;
#define FBUF(name, size) uint8_t name##_s[size]; buf name = { name##_s, 0, size }   /* declare a buf backed by a stack array */
static void b_put(buf *b, const void *d, size_t n) {
    if (b->n + n > b->cap) { fprintf(stderr, "error: internal buffer too small\n"); exit(1); }   /* every use has a fixed worst case */
    if (n) memcpy(b->p + b->n, d, n);
    b->n += n;
}
static void b_u8(buf *b, unsigned v) { uint8_t x = (uint8_t)v; b_put(b, &x, 1); }
static void b_be(buf *b, uint64_t v, int k) { for (int i = k - 1; i >= 0; i--) b_u8(b, (unsigned)(v >> (8 * i)) & 0xff); }

/* whole pages straight from the kernel for the rare big transient block (a split handshake message, a big
 * websocket frame); given back with munmap, so it never stays in RAM like a heap high-water mark would */
static void *pg_alloc(size_t n) {
    void *p = mmap(NULL, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return p == MAP_FAILED ? NULL : p;
}
static void pg_free(void *p, size_t n) { if (p) munmap(p, n); }


/* ---------- SHA-256 (incremental; copy the struct to snapshot a running hash) ---------- */
typedef struct { uint32_t h[8]; uint64_t len; uint8_t b[64]; unsigned bl; } sha_t;
static const uint32_t SHA_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};
#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
static void sha_block(sha_t *s, const uint8_t *p) {
    uint32_t w[64], v[8];
    for (int i = 0; i < 16; i++) w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 | (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = ROR(w[i - 15], 7) ^ ROR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = ROR(w[i - 2], 17) ^ ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    memcpy(v, s->h, 32);
    for (int i = 0; i < 64; i++) {                            /* v = a b c d e f g h */
        uint32_t a = v[0], e = v[4];
        uint32_t t1 = v[7] + (ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25)) + ((e & v[5]) ^ (~e & v[6])) + SHA_K[i] + w[i];
        uint32_t t2 = (ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22)) + ((a & v[1]) ^ (a & v[2]) ^ (v[1] & v[2]));
        memmove(v + 1, v, 28);
        v[4] += t1; v[0] = t1 + t2;
    }
    for (int i = 0; i < 8; i++) s->h[i] += v[i];
}
static void sha_init(sha_t *s) {
    static const uint32_t H0[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    memcpy(s->h, H0, 32); s->len = 0; s->bl = 0;
}
static void sha_add(sha_t *s, const void *d, size_t n) {
    const uint8_t *p = d; s->len += n;
    while (n) {
        size_t k = 64 - s->bl; if (k > n) k = n;
        memcpy(s->b + s->bl, p, k); s->bl += (unsigned)k; p += k; n -= k;
        if (s->bl == 64) { sha_block(s, s->b); s->bl = 0; }
    }
}
static void sha_fin(sha_t *s, uint8_t out[32]) {              /* destroys s */
    uint64_t bits = s->len * 8; uint8_t pad = 0x80, l[8];
    sha_add(s, &pad, 1); pad = 0;
    while (s->bl != 56) sha_add(s, &pad, 1);
    for (int i = 0; i < 8; i++) l[i] = (uint8_t)(bits >> (56 - 8 * i));
    sha_add(s, l, 8);
    for (int i = 0; i < 8; i++) { out[4 * i] = (uint8_t)(s->h[i] >> 24); out[4 * i + 1] = (uint8_t)(s->h[i] >> 16); out[4 * i + 2] = (uint8_t)(s->h[i] >> 8); out[4 * i + 3] = (uint8_t)s->h[i]; }
}
static void sha256(const void *d, size_t n, uint8_t out[32]) { sha_t s; sha_init(&s); sha_add(&s, d, n); sha_fin(&s, out); }
/* incremental HMAC: hm_init(key), hm_add(...) any number of pieces, hm_fin() */
typedef struct { sha_t s; uint8_t ok[64]; } hm_t;
static void hm_init(hm_t *h, const void *k, size_t kl) {
    uint8_t kb[64] = {0};
    if (kl > 64) sha256(k, kl, kb); else if (kl) memcpy(kb, k, kl);
    for (int i = 0; i < 64; i++) { h->ok[i] = kb[i] ^ 0x5c; kb[i] ^= 0x36; }
    sha_init(&h->s); sha_add(&h->s, kb, 64);
}
static void hm_add(hm_t *h, const void *d, size_t n) { sha_add(&h->s, d, n); }
static void hm_fin(hm_t *h, uint8_t out[32]) {
    uint8_t in[32];
    sha_fin(&h->s, in);
    sha_init(&h->s); sha_add(&h->s, h->ok, 64); sha_add(&h->s, in, 32); sha_fin(&h->s, out);
}
static void hmac256(const void *k, size_t kl, const void *d, size_t dl, uint8_t out[32]) {
    hm_t h; hm_init(&h, k, kl); hm_add(&h, d, dl); hm_fin(&h, out);
}

/* ---------- HKDF (SHA-256) ---------- */
static void hkdf_extract(const uint8_t *salt, size_t sl, const uint8_t *ikm, size_t il, uint8_t out[32]) {
    uint8_t z[32] = {0};
    if (!sl) { salt = z; sl = 32; }
    hmac256(salt, sl, ikm, il, out);
}
static void hkdf_expand2(const uint8_t *prk, size_t pl, const uint8_t *info, size_t il, const uint8_t *info2, size_t il2,
                         uint8_t *out, size_t L) {              /* info = info || info2, so callers need no scratch buffer */
    uint8_t t[32]; size_t tl = 0, o = 0; uint8_t c = 1;
    while (o < L) {
        hm_t h; hm_init(&h, prk, pl);
        hm_add(&h, t, tl); hm_add(&h, info, il); hm_add(&h, info2, il2); hm_add(&h, &c, 1); c++;
        hm_fin(&h, t);
        tl = 32;
        size_t k = L - o < 32 ? L - o : 32;
        memcpy(out + o, t, k); o += k;
    }
}
static void hkdf_expand(const uint8_t *prk, size_t pl, const uint8_t *info, size_t il, uint8_t *out, size_t L) {
    hkdf_expand2(prk, pl, info, il, NULL, 0, out, L);
}

/* ---------- AES (encrypt only, that is all GCM needs) ---------- */
static const uint8_t SB[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};
typedef struct { uint8_t rk[240]; int nr; } aes_t;
static uint8_t xt(uint8_t x) { return (uint8_t)((x << 1) ^ ((x >> 7) * 0x1b)); }
static void aes_key(aes_t *a, const uint8_t *key, int klen) {
    int nk = klen / 4, total; uint8_t rc = 1;
    a->nr = nk + 6; total = 4 * (a->nr + 1);
    memcpy(a->rk, key, klen);
    for (int i = nk; i < total; i++) {
        uint8_t t[4]; memcpy(t, a->rk + 4 * (i - 1), 4);
        if (i % nk == 0) { uint8_t x = t[0]; t[0] = SB[t[1]] ^ rc; t[1] = SB[t[2]]; t[2] = SB[t[3]]; t[3] = SB[x]; rc = xt(rc); }
        else if (nk > 6 && i % nk == 4) for (int j = 0; j < 4; j++) t[j] = SB[t[j]];
        for (int j = 0; j < 4; j++) a->rk[4 * i + j] = a->rk[4 * (i - nk) + j] ^ t[j];
    }
}
static void aes_enc_sw(const aes_t *a, const uint8_t in[16], uint8_t out[16]) {
    uint8_t s[16], t[16];
    for (int i = 0; i < 16; i++) s[i] = in[i] ^ a->rk[i];
    for (int r = 1; r <= a->nr; r++) {
        for (int c = 0; c < 4; c++) for (int row = 0; row < 4; row++) t[4 * c + row] = SB[s[4 * ((c + row) & 3) + row]];   /* SubBytes + ShiftRows */
        if (r < a->nr) for (int c = 0; c < 4; c++) {                                                                      /* MixColumns */
            const uint8_t *x = t + 4 * c; uint8_t *y = s + 4 * c;
            y[0] = xt(x[0] ^ x[1]) ^ x[1] ^ x[2] ^ x[3];
            y[1] = xt(x[1] ^ x[2]) ^ x[2] ^ x[3] ^ x[0];
            y[2] = xt(x[2] ^ x[3]) ^ x[3] ^ x[0] ^ x[1];
            y[3] = xt(x[3] ^ x[0]) ^ x[0] ^ x[1] ^ x[2];
        } else memcpy(s, t, 16);
        for (int i = 0; i < 16; i++) s[i] ^= a->rk[16 * r + i];
    }
    memcpy(out, s, 16);
}
#ifdef __x86_64__
__attribute__((target("aes"))) static void aes_enc_ni(const aes_t *a, const uint8_t in[16], uint8_t out[16]) {
    __m128i s = _mm_xor_si128(_mm_loadu_si128((const __m128i *)in), _mm_loadu_si128((const __m128i *)a->rk));
    for (int r = 1; r < a->nr; r++) s = _mm_aesenc_si128(s, _mm_loadu_si128((const __m128i *)(a->rk + 16 * r)));
    _mm_storeu_si128((__m128i *)out, _mm_aesenclast_si128(s, _mm_loadu_si128((const __m128i *)(a->rk + 16 * a->nr))));
}
#endif
static void aes_enc(const aes_t *a, const uint8_t in[16], uint8_t out[16]) {   /* in == out is fine */
#ifdef __x86_64__
    if (hw_has(0)) { aes_enc_ni(a, in, out); return; }
#endif
    aes_enc_sw(a, in, out);
}

/* ---------- AES-GCM (12-byte IV); in == out is allowed. Decrypt returns 0 on a bad tag and writes nothing. ---------- */
static uint64_t ld64(const uint8_t *p) { uint64_t v = 0; for (int i = 0; i < 8; i++) v = v << 8 | p[i]; return v; }
static void st64(uint8_t *p, uint64_t v) { for (int i = 7; i >= 0; i--) { p[i] = (uint8_t)v; v >>= 8; } }
/* x = x * h in GF(2^128), GCM bit order. The 128 bits are handled as two 64-bit words, with the same constant-time masking as a
 * byte-wise version but about 17x fewer instructions (it was 85-90% of the CPU spent encrypting an audio packet). No tables. */
static void gmul_sw(uint8_t x[16], const uint8_t h[16]) {
    uint64_t vh = ld64(h), vl = ld64(h + 8), zh = 0, zl = 0, xw[2] = { ld64(x), ld64(x + 8) };
    for (int i = 0; i < 128; i++) {
        uint64_t m = -((xw[i >> 6] >> (63 - (i & 63))) & 1);     /* all ones when bit i of x (MSB first) is set */
        zh ^= vh & m; zl ^= vl & m;
        uint64_t lsb = vl & 1;
        vl = vl >> 1 | vh << 63;
        vh = vh >> 1 ^ (0xe100000000000000ULL & -lsb);           /* reduce by x^128 + x^7 + x^2 + x + 1 */
    }
    st64(x, zh); st64(x + 8, zl);
}
#ifdef __x86_64__
/* Same product with carry-less multiplies (Intel white paper "Carry-Less Multiplication and Its Usage for Computing the GCM
 * Mode", algorithm 5): the operands are used byte-reversed, the 256-bit result is shifted left one bit, then reduced. */
#define XOR _mm_xor_si128
__attribute__((target("pclmul"))) static void gmul_ni(uint8_t x[16], const uint8_t h[16]) {
    __m128i a = _mm_set_epi64x((long long)ld64(x), (long long)ld64(x + 8)), b = _mm_set_epi64x((long long)ld64(h), (long long)ld64(h + 8));
    __m128i t3 = _mm_clmulepi64_si128(a, b, 0x00), t4 = _mm_clmulepi64_si128(a, b, 0x10);
    __m128i t5 = _mm_clmulepi64_si128(a, b, 0x01), t6 = _mm_clmulepi64_si128(a, b, 0x11);
    t4 = XOR(t4, t5);
    t3 = XOR(t3, _mm_slli_si128(t4, 8)); t6 = XOR(t6, _mm_srli_si128(t4, 8));
    __m128i t7 = _mm_srli_epi32(t3, 31), t8 = _mm_srli_epi32(t6, 31);          /* shift [t6:t3] left by one bit */
    t3 = _mm_slli_epi32(t3, 1); t6 = _mm_slli_epi32(t6, 1);
    t3 = _mm_or_si128(t3, _mm_slli_si128(t7, 4));
    t6 = _mm_or_si128(_mm_or_si128(t6, _mm_slli_si128(t8, 4)), _mm_srli_si128(t7, 12));
    t7 = XOR(XOR(_mm_slli_epi32(t3, 31), _mm_slli_epi32(t3, 30)), _mm_slli_epi32(t3, 25));   /* reduce by x^128 + x^7 + x^2 + x + 1 */
    t8 = _mm_srli_si128(t7, 4);
    t3 = XOR(t3, _mm_slli_si128(t7, 12));
    t3 = XOR(t3, XOR(XOR(_mm_srli_epi32(t3, 1), _mm_srli_epi32(t3, 2)), XOR(_mm_srli_epi32(t3, 7), t8)));
    t6 = XOR(t6, t3);
    st64(x, (uint64_t)_mm_cvtsi128_si64(_mm_srli_si128(t6, 8))); st64(x + 8, (uint64_t)_mm_cvtsi128_si64(t6));
}
#undef XOR
#endif
static void gmul(uint8_t x[16], const uint8_t h[16]) {
#ifdef __x86_64__
    if (hw_has(1)) { gmul_ni(x, h); return; }
#endif
    gmul_sw(x, h);
}
static void ghash_upd(uint8_t y[16], const uint8_t h[16], const uint8_t *d, size_t n) {
    while (n) { size_t k = n < 16 ? n : 16; for (size_t i = 0; i < k; i++) y[i] ^= d[i]; gmul(y, h); d += k; n -= k; }
}
static void ghash_tag(const aes_t *a, const uint8_t h[16], const uint8_t j0[16], const uint8_t *aad, size_t al,
                      const uint8_t *ct, size_t cl, uint8_t tag[16]) {
    uint8_t y[16] = {0}, l[16], e[16];
    ghash_upd(y, h, aad, al); ghash_upd(y, h, ct, cl);
    for (int i = 0; i < 8; i++) { l[i] = (uint8_t)(((uint64_t)al * 8) >> (56 - 8 * i)); l[8 + i] = (uint8_t)(((uint64_t)cl * 8) >> (56 - 8 * i)); }
    ghash_upd(y, h, l, 16);
    aes_enc(a, j0, e);
    for (int i = 0; i < 16; i++) tag[i] = y[i] ^ e[i];
}
static int gcm(int enc, int klen, const uint8_t *key, const uint8_t iv[12], const uint8_t *aad, size_t al,
               const uint8_t *in, size_t il, uint8_t *out, uint8_t tag[16]) {
    aes_t a; aes_key(&a, key, klen);
    uint8_t h[16] = {0}, j0[16], ctr[16], ks[16], want[16];
    aes_enc(&a, h, h);
    memcpy(j0, iv, 12); j0[12] = j0[13] = j0[14] = 0; j0[15] = 1;
    if (!enc) {                                                /* authenticate first, decrypt only if the tag is good */
        ghash_tag(&a, h, j0, aad, al, in, il, want);
        if (ct_memcmp(want, tag, 16)) return 0;
    }
    memcpy(ctr, j0, 16);
    for (size_t o = 0; o < il; o += 16) {
        for (int i = 15; i >= 12 && ++ctr[i] == 0; i--) ;      /* inc32 */
        aes_enc(&a, ctr, ks);
        size_t k = il - o < 16 ? il - o : 16;
        for (size_t i = 0; i < k; i++) out[o + i] = in[o + i] ^ ks[i];
    }
    if (enc) ghash_tag(&a, h, j0, aad, al, out, il, tag);
    return 1;
}

/* ---------- P-256 (p256.h). Public keys are raw x||y (64 bytes). ---------- */
typedef struct { uint8_t priv[32], pub[64]; } ec_t;
static void ec_gen(ec_t *k) {
    if (!p256_make_key(k->pub, k->priv)) { fprintf(stderr, "error: key generation failed\n"); exit(1); }
}
static void ec_pub(const ec_t *k, uint8_t out[65]) { out[0] = 4; memcpy(out + 1, k->pub, 64); }   /* uncompressed SEC1 point */
static void ec_sign_hash(const ec_t *k, const uint8_t h[32], uint8_t raw[64]) {
    if (!p256_sign(k->priv, h, raw)) { fprintf(stderr, "error: sign failed\n"); exit(1); }
}
static size_t der_int(uint8_t *o, const uint8_t v[32]) {       /* one DER INTEGER from a 32-byte big-endian value */
    int i = 0; while (i < 31 && v[i] == 0) i++;
    int pad = (v[i] & 0x80) != 0; size_t k = 2;
    o[0] = 2; o[1] = (uint8_t)(32 - i + pad);
    if (pad) o[k++] = 0;
    memcpy(o + k, v + i, 32 - i);
    return k + 32 - i;
}
static size_t ecdsa_der(const uint8_t raw[64], uint8_t *out) { /* raw r||s -> DER SEQUENCE; out needs 72 bytes */
    uint8_t t[70]; size_t n = der_int(t, raw); n += der_int(t + n, raw + 32);
    out[0] = 0x30; out[1] = (uint8_t)n; memcpy(out + 2, t, n);
    return n + 2;
}
#endif