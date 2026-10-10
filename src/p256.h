/* p256.h - the little P-256 (secp256r1) the bot needs: key generation, ECDH, ECDSA sign / verify.
 *
 * Public keys are raw x||y (64 bytes, big endian), private keys and hashes 32 bytes big endian, signatures raw r||s.
 * Needs rnd() from mcrypto.h (include this file after it). No allocation, no tables, no libc beyond memcpy.
 *
 * Numbers are 4 x 64-bit limbs (little endian limb order) in Montgomery form, one generic Montgomery multiply serves both the
 * field (mod p) and the group order (mod n). Points are projective and use the complete addition formulas of Renes, Costello,
 * Batina (2015, algorithm 4, a = -3): one formula for add and double, no special cases for infinity or P == Q.
 * Scalar multiplication always does the same work for every bit and selects with masks (no secret-dependent branches).
 * Inversion is x^(m-2): the exponent is public, so branching on its bits leaks nothing.
 */
#ifndef P256_H
#define P256_H
#include <stdint.h>
#include <string.h>

typedef uint64_t p_u64;
typedef unsigned __int128 p_u128;
typedef p_u64 p_fe[4];
typedef struct { p_u64 m[4], n0, r2[4]; } p_mod;               /* modulus, -1/m mod 2^64, 2^512 mod m */
typedef struct { p_fe x, y, z; } p_pt;

static const p_mod P256_P = { { 0xffffffffffffffffULL, 0x00000000ffffffffULL, 0x0000000000000000ULL, 0xffffffff00000001ULL }, 1,
                              { 0x0000000000000003ULL, 0xfffffffbffffffffULL, 0xfffffffffffffffeULL, 0x00000004fffffffdULL } };
static const p_mod P256_N = { { 0xf3b9cac2fc632551ULL, 0xbce6faada7179e84ULL, 0xffffffffffffffffULL, 0xffffffff00000000ULL }, 0xccd1c8aaee00bc4fULL,
                              { 0x83244c95be79eea2ULL, 0x4699799c49bd6fa6ULL, 0x2845b2392b6bec59ULL, 0x66e12d94f3d95620ULL } };
static const p_fe P256_BM = { 0xd89cdf6229c4bddfULL, 0xacf005cd78843090ULL, 0xe5a220abf7212ed6ULL, 0xdc30061d04874834ULL };   /* curve b, Montgomery form */
static const uint8_t P256_G[64] = {
    0x6b,0x17,0xd1,0xf2,0xe1,0x2c,0x42,0x47,0xf8,0xbc,0xe6,0xe5,0x63,0xa4,0x40,0xf2,0x77,0x03,0x7d,0x81,0x2d,0xeb,0x33,0xa0,0xf4,0xa1,0x39,0x45,0xd8,0x98,0xc2,0x96,
    0x4f,0xe3,0x42,0xe2,0xfe,0x1a,0x7f,0x9b,0x8e,0xe7,0xeb,0x4a,0x7c,0x0f,0x9e,0x16,0x2b,0xce,0x33,0x57,0x6b,0x31,0x5e,0xce,0xcb,0xb6,0x40,0x68,0x37,0xbf,0x51,0xf5 };

/* ---------- limb helpers ---------- */
static void p_ld(p_fe r, const uint8_t *b) {                   /* 32 bytes big endian -> limbs */
    for (int i = 0; i < 4; i++) { p_u64 v = 0; for (int j = 0; j < 8; j++) v = v << 8 | b[24 - 8 * i + j]; r[i] = v; }
}
static void p_st(uint8_t *b, const p_fe a) {
    for (int i = 0; i < 4; i++) for (int j = 0; j < 8; j++) b[24 - 8 * i + j] = (uint8_t)(a[i] >> (56 - 8 * j));
}
static p_u64 p_sub(p_fe r, const p_u64 *a, const p_u64 *b) {   /* r = a - b, returns the borrow */
    p_u64 br = 0;
    for (int i = 0; i < 4; i++) { p_u128 x = (p_u128)a[i] - b[i] - br; r[i] = (p_u64)x; br = (p_u64)(x >> 64) & 1; }
    return br;
}
static p_u64 p_add(p_fe r, const p_u64 *a, const p_u64 *b) {   /* r = a + b, returns the carry */
    p_u64 c = 0;
    for (int i = 0; i < 4; i++) { p_u128 x = (p_u128)a[i] + b[i] + c; r[i] = (p_u64)x; c = (p_u64)(x >> 64); }
    return c;
}
static void p_sel(p_fe r, const p_u64 *a, const p_u64 *b, p_u64 bit) {   /* r = bit ? a : b, without branching */
    p_u64 m = -bit;
    for (int i = 0; i < 4; i++) r[i] = (a[i] & m) | (b[i] & ~m);
}
static p_u64 p_zero(const p_u64 *a) { p_u64 v = a[0] | a[1] | a[2] | a[3]; return v == 0; }
static p_u64 p_eq(const p_u64 *a, const p_u64 *b) { p_u64 v = 0; for (int i = 0; i < 4; i++) v |= a[i] ^ b[i]; return v == 0; }
static p_u64 p_lt(const p_u64 *a, const p_u64 *m) { p_fe t; return p_sub(t, a, m); }   /* a < m */

/* ---------- modular arithmetic (inputs and outputs < m) ---------- */
static void p_fadd(p_fe r, const p_u64 *a, const p_u64 *b, const p_mod *M) {
    p_fe s, d; p_u64 c = p_add(s, a, b), br = p_sub(d, s, M->m);
    p_sel(r, d, s, c | (br ^ 1));
}
static void p_fsub(p_fe r, const p_u64 *a, const p_u64 *b, const p_mod *M) {
    p_fe d, s; p_u64 br = p_sub(d, a, b);
    p_add(s, d, M->m);
    p_sel(r, s, d, br);
}
static void p_mm(p_fe r, const p_u64 *a, const p_u64 *b, const p_mod *M) {   /* r = a * b / 2^256 mod m (CIOS) */
    p_u64 t[6] = {0};
    for (int i = 0; i < 4; i++) {
        p_u64 c = 0;
        for (int j = 0; j < 4; j++) { p_u128 x = (p_u128)a[j] * b[i] + t[j] + c; t[j] = (p_u64)x; c = (p_u64)(x >> 64); }
        p_u128 x = (p_u128)t[4] + c; t[4] = (p_u64)x; t[5] = (p_u64)(x >> 64);
        p_u64 q = t[0] * M->n0;
        x = (p_u128)q * M->m[0] + t[0]; c = (p_u64)(x >> 64);
        for (int j = 1; j < 4; j++) { x = (p_u128)q * M->m[j] + t[j] + c; t[j - 1] = (p_u64)x; c = (p_u64)(x >> 64); }
        x = (p_u128)t[4] + c; t[3] = (p_u64)x; t[4] = t[5] + (p_u64)(x >> 64);
    }
    p_fe d; p_u64 br = p_sub(d, t, M->m);
    p_sel(r, d, t, t[4] | (br ^ 1));
}
static void p_tom(p_fe r, const p_u64 *a, const p_mod *M) { p_mm(r, a, M->r2, M); }                          /* into Montgomery form */
static void p_fromm(p_fe r, const p_u64 *a, const p_mod *M) { static const p_fe one = { 1, 0, 0, 0 }; p_mm(r, a, one, M); }
static void p_inv(p_fe r, const p_u64 *a, const p_mod *M) {    /* a in Montgomery form -> a^-1 in Montgomery form */
    static const p_fe two = { 2, 0, 0, 0 };
    p_fe e, one = { 1, 0, 0, 0 };
    p_sub(e, M->m, two);                                       /* exponent m - 2 */
    p_tom(r, one, M);
    for (int i = 255; i >= 0; i--) {
        p_mm(r, r, r, M);
        if (e[i >> 6] >> (i & 63) & 1) p_mm(r, r, a, M);
    }
}
static void p_nmul(p_fe r, const p_u64 *a, const p_u64 *b) {  /* plain a * b mod n */
    p_mm(r, a, b, &P256_N); p_mm(r, r, P256_N.r2, &P256_N);
}
static void p_ninv(p_fe r, const p_u64 *a) {                   /* plain 1/a mod n */
    p_fe t, u; p_tom(t, a, &P256_N); p_inv(u, t, &P256_N); p_fromm(r, u, &P256_N);   /* p_inv's output must not alias its input */
}
static void p_red_n(p_fe r, const p_u64 *a) { p_fe d; p_u64 br = p_sub(d, a, P256_N.m); p_sel(r, a, d, br); }   /* a < 2n -> a mod n */

/* ---------- points ---------- */
#define PM(r, a, b) p_mm(r, a, b, &P256_P)
#define PA(r, a, b) p_fadd(r, a, b, &P256_P)
#define PS(r, a, b) p_fsub(r, a, b, &P256_P)
static void p_padd(p_pt *r, const p_pt *p, const p_pt *q) {    /* complete addition, a = -3 (RCB 2015, algorithm 4) */
    p_fe t0, t1, t2, t3, t4, x3, y3, z3;
    PM(t0, p->x, q->x); PM(t1, p->y, q->y); PM(t2, p->z, q->z);
    PA(t3, p->x, p->y); PA(t4, q->x, q->y); PM(t3, t3, t4); PA(t4, t0, t1); PS(t3, t3, t4);
    PA(t4, p->y, p->z); PA(x3, q->y, q->z); PM(t4, t4, x3); PA(x3, t1, t2); PS(t4, t4, x3);
    PA(x3, p->x, p->z); PA(y3, q->x, q->z); PM(x3, x3, y3); PA(y3, t0, t2); PS(y3, x3, y3);
    PM(z3, P256_BM, t2); PS(x3, y3, z3); PA(z3, x3, x3); PA(x3, x3, z3); PS(z3, t1, x3); PA(x3, t1, x3);
    PM(y3, P256_BM, y3); PA(t1, t2, t2); PA(t2, t1, t2); PS(y3, y3, t2); PS(y3, y3, t0); PA(t1, y3, y3); PA(y3, t1, y3);
    PA(t1, t0, t0); PA(t0, t1, t0); PS(t0, t0, t2);
    PM(t1, t4, y3); PM(t2, t0, y3); PM(y3, x3, z3); PA(y3, y3, t2); PM(x3, t3, x3); PS(x3, x3, t1);
    PM(z3, t4, z3); PM(t1, t3, t0); PA(z3, z3, t1);
    memcpy(r->x, x3, 32); memcpy(r->y, y3, 32); memcpy(r->z, z3, 32);
}
static void p_load(p_pt *r, const uint8_t xy[64]) {            /* affine bytes -> projective, Montgomery form */
    static const p_fe one = { 1, 0, 0, 0 };
    p_fe v;
    p_ld(v, xy); p_tom(r->x, v, &P256_P);
    p_ld(v, xy + 32); p_tom(r->y, v, &P256_P);
    p_tom(r->z, one, &P256_P);
}
static void p_smul(p_pt *r, const uint8_t k[32], const p_pt *b) {   /* r = k * b, the same work for every bit of k */
    static const p_fe one = { 1, 0, 0, 0 };
    p_pt acc, t;
    memset(&acc, 0, sizeof acc); p_tom(acc.y, one, &P256_P);   /* infinity (0 : 1 : 0) */
    for (int i = 255; i >= 0; i--) {
        p_padd(&acc, &acc, &acc);
        p_padd(&t, &acc, b);
        p_u64 bit = k[31 - (i >> 3)] >> (i & 7) & 1;
        p_sel(acc.x, t.x, acc.x, bit); p_sel(acc.y, t.y, acc.y, bit); p_sel(acc.z, t.z, acc.z, bit);
    }
    *r = acc;
}
static int p_affine(uint8_t *x, uint8_t *y, const p_pt *a) {   /* writes x (and y if given); 0 = point at infinity */
    if (p_zero(a->z)) return 0;
    p_fe zi, v;
    p_inv(zi, a->z, &P256_P);
    PM(v, a->x, zi); p_fromm(v, v, &P256_P); p_st(x, v);
    if (y) { PM(v, a->y, zi); p_fromm(v, v, &P256_P); p_st(y, v); }
    return 1;
}

/* ---------- public API ---------- */
static int p256_valid_pub(const uint8_t pub[64]) {             /* 1 = x, y < p and y^2 = x^3 - 3x + b */
    p_fe x, y, xm, ym, l, rr, t;
    p_ld(x, pub); p_ld(y, pub + 32);
    if (!p_lt(x, P256_P.m) || !p_lt(y, P256_P.m)) return 0;
    p_tom(xm, x, &P256_P); p_tom(ym, y, &P256_P);
    PM(l, ym, ym);
    PM(rr, xm, xm); PM(rr, rr, xm);
    PA(t, xm, xm); PA(t, t, xm); PS(rr, rr, t); PA(rr, rr, P256_BM);
    return (int)p_eq(l, rr);
}
static int p_scalar_ok(const p_fe k) { return !p_zero(k) && p_lt(k, P256_N.m); }   /* 1 <= k < n */
static void p_rand_scalar(uint8_t k[32]) {                     /* uniform in [1, n-1] by rejection */
    p_fe v;
    do { rnd(k, 32); p_ld(v, k); } while (!p_scalar_ok(v));
}
static int p256_make_key(uint8_t pub[64], uint8_t priv[32]) {
    p_pt g, r;
    p_rand_scalar(priv);
    p_load(&g, P256_G); p_smul(&r, priv, &g);
    return p_affine(pub, pub + 32, &r);
}
static int p256_shared(const uint8_t pub[64], const uint8_t priv[32], uint8_t out[32]) {   /* ECDH: x coordinate of priv * pub */
    p_pt q, r;
    if (!p256_valid_pub(pub)) return 0;
    p_load(&q, pub); p_smul(&r, priv, &q);
    return p_affine(out, NULL, &r);
}
static int p256_sign(const uint8_t priv[32], const uint8_t hash[32], uint8_t sig[64]) {
    p_pt g, r; p_fe z, d, k, rr, s, t; uint8_t kb[32], rx[32];
    p_ld(z, hash); p_red_n(z, z); p_ld(d, priv);
    p_load(&g, P256_G);
    for (;;) {
        p_rand_scalar(kb); p_ld(k, kb);
        p_smul(&r, kb, &g);
        if (!p_affine(rx, NULL, &r)) continue;
        p_ld(rr, rx); p_red_n(rr, rr);
        if (p_zero(rr)) continue;
        p_nmul(t, rr, d); p_fadd(t, t, z, &P256_N);            /* z + r*d */
        p_ninv(s, k); p_nmul(s, s, t);                         /* k^-1 * (z + r*d) */
        if (p_zero(s)) continue;
        p_st(sig, rr); p_st(sig + 32, s);
        memset(kb, 0, 32); memset(k, 0, 32);
        return 1;
    }
}
static int p256_verify(const uint8_t pub[64], const uint8_t hash[32], const uint8_t sig[64]) {
    p_pt g, q, a, b; p_fe z, r, s, w, u1, u2, xr; uint8_t u1b[32], u2b[32], x[32];
    p_ld(r, sig); p_ld(s, sig + 32);
    if (!p_scalar_ok(r) || !p_scalar_ok(s) || !p256_valid_pub(pub)) return 0;
    p_ld(z, hash); p_red_n(z, z);
    p_ninv(w, s); p_nmul(u1, z, w); p_nmul(u2, r, w);
    p_st(u1b, u1); p_st(u2b, u2);
    p_load(&g, P256_G); p_load(&q, pub);
    p_smul(&a, u1b, &g); p_smul(&b, u2b, &q);
    p_padd(&a, &a, &b);
    if (!p_affine(x, NULL, &a)) return 0;
    p_ld(xr, x); p_red_n(xr, xr);
    return (int)p_eq(xr, r);
}
#undef PM
#undef PA
#undef PS
#endif