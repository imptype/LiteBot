/* mtls.h - minimal TLS 1.3 client, built for one job: talking to Discord (Google Trust Services, all P-256).
 *
 * Offers exactly: TLS 1.3, TLS_AES_128_GCM_SHA256, P-256 key share, ecdsa_secp256r1_sha256.
 * Verification (all mandatory):
 *   1. the server's Certificate chain must contain an intermediate whose public key hash is in TLS_PINS,
 *   2. that intermediate's key must verify the leaf certificate's signature (ecdsa-with-SHA256),
 *   3. the leaf's DNS names (SAN) must match the host we connected to,
 *   4. CertificateVerify must verify with the leaf's key, and the server Finished must verify.
 * Not implemented on purpose: expiry checks, revocation, KeyUpdate, resumption, client certs, other suites.
 */
#ifndef MTLS_H
#define MTLS_H
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stddef.h>
#include "mcrypto.h"

/* SHA-256 of the DER SubjectPublicKeyInfo of each trusted intermediate (lowercase hex).
 * Get one with: openssl x509 -pubkey -noout | openssl pkey -pubin -outform DER | sha256sum */
static const char *const TLS_PINS[] = {
    "908769e8d34477cc2cba0632c88605b22d7294c0840f78596d247c645b1afc0e",   /* Google Trust Services WE1 */
    NULL
};

typedef struct {
    int fd;
    uint8_t rk[16], rv[12], wk[16], wv[12];   /* record protection keys / static IVs (read, write) */
    uint64_t rseq, wseq;
    int enc_r, enc_w;
    uint8_t rec[5 + 16640];                   /* one incoming record; decrypted in place */
    size_t off, end;                          /* unread application data inside rec */
} tls_t;

#define TLS_FAIL(m) do { fprintf(stderr, "tls: %s\n", m); return -1; } while (0)

/* ---------- socket io ---------- */
static int tls_rd(int fd, uint8_t *p, size_t n) {
    while (n) { ssize_t r = read(fd, p, n); if (r < 0 && errno == EINTR) continue; if (r <= 0) return -1; p += r; n -= (size_t)r; }
    return 0;
}
static int tls_wr(int fd, const uint8_t *p, size_t n) {
    while (n) { ssize_t r = send(fd, p, n, MSG_NOSIGNAL); if (r < 0 && errno == EINTR) continue; if (r <= 0) return -1; p += r; n -= (size_t)r; }
    return 0;
}

/* ---------- record layer ---------- */
static void tls_nonce(uint8_t iv[12], const uint8_t base[12], uint64_t seq) {
    memcpy(iv, base, 12);
    for (int i = 0; i < 8; i++) iv[4 + i] ^= (uint8_t)(seq >> (56 - 8 * i));
}
static int tls_send_rec(tls_t *t, int type, const uint8_t *d, size_t n) {   /* n <= 1024 */
    uint8_t r[5 + 1024 + 1 + 16], iv[12];
    r[1] = 3; r[2] = 3;
    if (!t->enc_w) {
        r[0] = (uint8_t)type; r[3] = (uint8_t)(n >> 8); r[4] = (uint8_t)n;
        memcpy(r + 5, d, n);
        return tls_wr(t->fd, r, 5 + n);
    }
    size_t l = n + 1 + 16;
    r[0] = 23; r[3] = (uint8_t)(l >> 8); r[4] = (uint8_t)l;
    memcpy(r + 5, d, n); r[5 + n] = (uint8_t)type;
    tls_nonce(iv, t->wv, t->wseq++);
    if (!gcm(1, 16, t->wk, iv, r, 5, r + 5, n + 1, r + 5, r + 5 + n + 1)) return -1;
    return tls_wr(t->fd, r, 5 + l);
}
/* Reads the next record into t->rec+5. Returns the (inner) content type in *type and its length in *len. */
static int tls_read_rec(tls_t *t, int *type, size_t *len) {
    for (;;) {
        uint8_t *h = t->rec;
        if (tls_rd(t->fd, h, 5)) return -1;
        size_t l = (size_t)h[3] << 8 | h[4];
        if (l > 16640) return -1;
        if (tls_rd(t->fd, h + 5, l)) return -1;
        if (h[0] == 20) continue;                                  /* legacy ChangeCipherSpec: ignore */
        if (!t->enc_r) { *type = h[0]; *len = l; return 0; }
        if (h[0] != 23 || l < 17) return -1;
        uint8_t iv[12]; tls_nonce(iv, t->rv, t->rseq++);
        if (!gcm(0, 16, t->rk, iv, h, 5, h + 5, l - 16, h + 5, h + 5 + l - 16)) return -1;
        size_t j = l - 16;
        while (j && h[5 + j - 1] == 0) j--;                        /* strip padding */
        if (!j) return -1;
        *type = h[5 + j - 1]; *len = j - 1;
        return 0;
    }
}

/* ---------- key schedule ---------- */
static void tls_hkdf_label(const uint8_t secret[32], const char *label, const uint8_t *ctx, size_t cl, uint8_t *out, size_t L) {
    uint8_t info[54]; size_t n = 0, ll = strlen(label);
    info[n++] = (uint8_t)(L >> 8); info[n++] = (uint8_t)L; info[n++] = (uint8_t)(6 + ll);
    memcpy(info + n, "tls13 ", 6); n += 6; memcpy(info + n, label, ll); n += ll;
    info[n++] = (uint8_t)cl; if (cl) memcpy(info + n, ctx, cl); n += cl;
    hkdf_expand(secret, 32, info, n, out, L);
}
static void tls_keys(const uint8_t secret[32], uint8_t key[16], uint8_t iv[12]) {
    tls_hkdf_label(secret, "key", NULL, 0, key, 16);
    tls_hkdf_label(secret, "iv", NULL, 0, iv, 12);
}
static void th_hash(const sha_t *th, uint8_t out[32]) { sha_t c = *th; sha_fin(&c, out); }

/* ---------- tiny DER reader / X.509 ---------- */
typedef struct { const uint8_t *p; size_t n; } sl;
static int der_next(sl *in, int *tag, sl *val, sl *full) {         /* read the next TLV and advance */
    if (in->n < 2) return 0;
    const uint8_t *s = in->p; size_t l = s[1], h = 2;
    if (l & 0x80) {
        size_t k = l & 0x7f; if (k < 1 || k > 3 || in->n < 2 + k) return 0;
        l = 0; for (size_t i = 0; i < k; i++) l = l << 8 | s[2 + i];
        h = 2 + k;
    }
    if (in->n < h + l) return 0;
    *tag = s[0]; val->p = s + h; val->n = l; full->p = s; full->n = h + l;
    in->p += h + l; in->n -= h + l;
    return 1;
}
static int ecdsa_raw(const uint8_t *d, size_t n, uint8_t raw[64]) {   /* DER ECDSA signature -> r||s */
    sl in = { d, n }, sq, v, f; int tag;
    memset(raw, 0, 64);
    if (!der_next(&in, &tag, &sq, &f) || tag != 0x30) return -1;
    for (int k = 0; k < 2; k++) {
        if (!der_next(&sq, &tag, &v, &f) || tag != 2 || !v.n) return -1;
        const uint8_t *q = v.p; size_t l = v.n;
        while (l > 32 && *q == 0) { q++; l--; }
        if (l > 32) return -1;
        memcpy(raw + 32 * k + 32 - l, q, l);
    }
    return 0;
}
typedef struct { sl tbs, spki, san; int sha256sig, sig_ok; uint8_t sig[64]; } cert_t;   /* sig_ok: P-256 signature parsed */
static int cert_parse(const uint8_t *p, size_t n, cert_t *c) {
    static const uint8_t OID_SHA256_ECDSA[] = { 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02 };
    sl in = { p, n }, cert, tv, alg, bits, x, full; int tag;
    if (!der_next(&in, &tag, &cert, &full) || tag != 0x30) return -1;
    if (!der_next(&cert, &tag, &tv, &c->tbs) || tag != 0x30) return -1;               /* tbsCertificate */
    if (!der_next(&cert, &tag, &alg, &full) || tag != 0x30) return -1;                 /* signatureAlgorithm */
    c->sha256sig = alg.n == sizeof OID_SHA256_ECDSA && !memcmp(alg.p, OID_SHA256_ECDSA, alg.n);
    if (!der_next(&cert, &tag, &bits, &full) || tag != 3 || bits.n < 1 || bits.p[0]) return -1;
    c->sig_ok = !ecdsa_raw(bits.p + 1, bits.n - 1, c->sig);                            /* intermediates are signed with P-384: ignored */
    if (!der_next(&tv, &tag, &x, &full)) return -1;                                    /* [0] version or serial */
    if (tag == 0xa0 && !der_next(&tv, &tag, &x, &full)) return -1;                     /* serial */
    for (int i = 0; i < 4; i++) if (!der_next(&tv, &tag, &x, &full)) return -1;        /* sigalg, issuer, validity, subject */
    if (!der_next(&tv, &tag, &x, &c->spki) || tag != 0x30) return -1;
    c->san.p = NULL; c->san.n = 0;
    while (der_next(&tv, &tag, &x, &full)) {
        if (tag != 0xa3) continue;                                                     /* [3] extensions */
        sl exts = x, list, one, lf; int t1;
        if (!der_next(&exts, &t1, &list, &lf) || t1 != 0x30) return -1;
        while (der_next(&list, &t1, &one, &lf)) {
            sl oid, val, ef; int t2;
            if (!der_next(&one, &t2, &oid, &ef) || t2 != 6) continue;
            if (oid.n == 3 && !memcmp(oid.p, "\x55\x1d\x11", 3))                     /* subjectAltName */
                while (der_next(&one, &t2, &val, &ef)) if (t2 == 4) { c->san = val; break; }
        }
    }
    return 0;
}
static int spki_key(sl spki, uint8_t pub[64]) {                                         /* P-256 SubjectPublicKeyInfo -> x||y */
    static const uint8_t P256[] = { 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07 };
    sl in = spki, v, alg, bs, f; int tag;
    if (!der_next(&in, &tag, &v, &f) || tag != 0x30) return -1;
    in = v;
    if (!der_next(&in, &tag, &alg, &f) || tag != 0x30 || alg.n < sizeof P256 || memcmp(alg.p + alg.n - sizeof P256, P256, sizeof P256)) return -1;
    if (!der_next(&in, &tag, &bs, &f) || tag != 3 || bs.n != 66 || bs.p[0] != 0 || bs.p[1] != 4) return -1;
    memcpy(pub, bs.p + 2, 64);
    return p256_valid_pub(pub) ? 0 : -1;
}
static int name_match(const char *pat, size_t pl, const char *host) {
    if (pl > 2 && pat[0] == '*' && pat[1] == '.') {                                     /* *.example.com matches one label */
        const char *dot = strchr(host, '.');
        return dot && dot != host && strlen(dot) == pl - 1 && !strncasecmp(dot, pat + 1, pl - 1);
    }
    return pl == strlen(host) && !strncasecmp(pat, host, pl);
}
static int san_match(sl san, const char *host) {
    sl in = san, v, x, f; int tag;
    if (!der_next(&in, &tag, &v, &f) || tag != 0x30) return 0;
    while (der_next(&v, &tag, &x, &f)) if (tag == 0x82 && name_match((const char *)x.p, x.n, host)) return 1;
    return 0;
}
static void hex32(const uint8_t h[32], char out[65]) {
    static const char D[] = "0123456789abcdef";
    for (int i = 0; i < 32; i++) { out[2 * i] = D[h[i] >> 4]; out[2 * i + 1] = D[h[i] & 15]; }
    out[64] = 0;
}
/* Certificate message body -> checks the chain, returns the leaf public key. */
static int tls_check_certs(const uint8_t *m, size_t n, const char *host, uint8_t leafpub[64]) {
    const uint8_t *c[6]; size_t cl[6]; int nc = 0;
    if (n < 4) TLS_FAIL("short Certificate");
    size_t pos = 1 + m[0];
    if (n < pos + 3) TLS_FAIL("short Certificate");
    size_t total = (size_t)m[pos] << 16 | m[pos + 1] << 8 | m[pos + 2]; pos += 3;
    if (pos + total > n) TLS_FAIL("bad Certificate length");
    size_t end = pos + total;
    while (pos < end && nc < 6) {
        if (pos + 3 > end) TLS_FAIL("bad certificate entry");
        size_t l = (size_t)m[pos] << 16 | m[pos + 1] << 8 | m[pos + 2]; pos += 3;
        if (pos + l + 2 > end) TLS_FAIL("bad certificate entry");
        c[nc] = m + pos; cl[nc++] = l; pos += l;
        pos += 2 + ((size_t)m[pos] << 8 | m[pos + 1]);                               /* skip entry extensions */
    }
    if (nc < 2) TLS_FAIL("server sent no intermediate certificate");
    cert_t leaf;
    if (cert_parse(c[0], cl[0], &leaf)) TLS_FAIL("cannot parse leaf certificate");
    if (!leaf.sha256sig || !leaf.sig_ok) TLS_FAIL("leaf is not signed with ecdsa-with-SHA256");
    if (!leaf.san.n || !san_match(leaf.san, host)) TLS_FAIL("certificate does not match host name");
    uint8_t lh[32]; sha256(leaf.tbs.p, leaf.tbs.n, lh);
    int trusted = 0; char first[65] = "";
    for (int i = 1; i < nc && !trusted; i++) {
        cert_t ic;
        if (cert_parse(c[i], cl[i], &ic)) continue;
        uint8_t ph[32]; char hx[65];
        sha256(ic.spki.p, ic.spki.n, ph); hex32(ph, hx);
        if (i == 1) memcpy(first, hx, 65);
        for (int k = 0; TLS_PINS[k]; k++) if (!strcmp(hx, TLS_PINS[k])) {
            uint8_t ipub[64];
            if (spki_key(ic.spki, ipub)) TLS_FAIL("bad intermediate key");
            if (!p256_verify(ipub, lh, leaf.sig)) TLS_FAIL("leaf signature does not verify");
            trusted = 1; break;
        }
    }
    if (!trusted) { fprintf(stderr, "tls: intermediate not pinned (spki sha256 %s): add it to TLS_PINS if you trust it\n", first); return -1; }
    if (spki_key(leaf.spki, leafpub)) TLS_FAIL("leaf key is not P-256");
    return 0;
}

/* ---------- handshake ---------- */
static void put16(uint8_t *b, size_t *i, unsigned v) { b[(*i)++] = (uint8_t)(v >> 8); b[(*i)++] = (uint8_t)v; }
/* total size (header + body) of the handshake message starting at p; needs 4 readable bytes */
#define HS_LEN(p) (((size_t)(p)[1] << 16 | (size_t)(p)[2] << 8 | (size_t)(p)[3]) + 4)
static int tls_connect(tls_t *t, int fd, const char *host) {
    static const uint8_t HRR[32] = { 0xcf,0x21,0xad,0x74,0xe5,0x9a,0x61,0x11,0xbe,0x1d,0x8c,0x02,0x1e,0x65,0xb8,0x91,
                                     0xc2,0xa2,0x11,0x16,0x7a,0xbb,0x8c,0x5e,0x07,0x9e,0x09,0xe2,0xc8,0xa8,0x33,0x9c };
    /* zero only the header: rec[] (16 KB) is always written before it is read, so skip it to keep its pages out of RAM */
    memset(t, 0, offsetof(tls_t, rec)); t->off = t->end = 0; t->fd = fd;
    size_t hl = strlen(host);
    if (hl > 253) TLS_FAIL("host name too long");
    ec_t eph; ec_gen(&eph);

    /* ClientHello */
    uint8_t ch[600]; size_t i = 0;
    ch[i++] = 1; size_t lenpos = i; i += 3;
    put16(ch, &i, 0x0303); rnd(ch + i, 32); i += 32;
    ch[i++] = 32; rnd(ch + i, 32); i += 32;                        /* legacy session id (compat mode) */
    put16(ch, &i, 2); put16(ch, &i, 0x1301);
    ch[i++] = 1; ch[i++] = 0;
    size_t extpos = i; i += 2;
    put16(ch, &i, 0); put16(ch, &i, (unsigned)hl + 5); put16(ch, &i, (unsigned)hl + 3); ch[i++] = 0; put16(ch, &i, (unsigned)hl); memcpy(ch + i, host, hl); i += hl;
    put16(ch, &i, 10); put16(ch, &i, 4); put16(ch, &i, 2); put16(ch, &i, 0x0017);        /* supported_groups: P-256 */
    put16(ch, &i, 13); put16(ch, &i, 4); put16(ch, &i, 2); put16(ch, &i, 0x0403);        /* signature_algorithms */
    put16(ch, &i, 43); put16(ch, &i, 3); ch[i++] = 2; put16(ch, &i, 0x0304);             /* supported_versions */
    put16(ch, &i, 51); put16(ch, &i, 71); put16(ch, &i, 69); put16(ch, &i, 0x0017); put16(ch, &i, 65);
    ec_pub(&eph, ch + i); i += 65;                                                       /* key_share */
    size_t el = i - extpos - 2; ch[extpos] = (uint8_t)(el >> 8); ch[extpos + 1] = (uint8_t)el;
    size_t bl = i - lenpos - 3; ch[lenpos] = (uint8_t)(bl >> 16); ch[lenpos + 1] = (uint8_t)(bl >> 8); ch[lenpos + 2] = (uint8_t)bl;
    sha_t th; sha_init(&th); sha_add(&th, ch, i);
    if (tls_send_rec(t, 22, ch, i)) TLS_FAIL("cannot send ClientHello");

    /* ServerHello */
    int type; size_t len;
    if (tls_read_rec(t, &type, &len)) TLS_FAIL("no ServerHello");
    if (type == 21) TLS_FAIL("server sent an alert (it rejected our ClientHello)");
    const uint8_t *sh = t->rec + 5;
    if (type != 22 || len < 4 || sh[0] != 2 || (size_t)(sh[1] << 16 | sh[2] << 8 | sh[3]) + 4 != len) TLS_FAIL("bad ServerHello");
    sha_add(&th, sh, len);
    const uint8_t *b = sh + 4, *bend = sh + len;
    if (len < 4 + 2 + 32 + 1) TLS_FAIL("short ServerHello");
    if (!memcmp(b + 2, HRR, 32)) TLS_FAIL("server asked for HelloRetryRequest (unsupported)");
    b += 34; b += 1 + b[0];
    if (b + 5 > bend) TLS_FAIL("short ServerHello");
    if ((b[0] << 8 | b[1]) != 0x1301 || b[2] != 0) TLS_FAIL("server chose an unsupported cipher suite");
    b += 3;
    size_t exl = (size_t)b[0] << 8 | b[1]; b += 2;
    if (b + exl > bend) TLS_FAIL("bad ServerHello extensions");
    const uint8_t *spub = NULL; int v13 = 0;
    for (const uint8_t *e = b, *ee = b + exl; e + 4 <= ee; ) {
        unsigned et = e[0] << 8 | e[1], elen = e[2] << 8 | e[3];
        if (e + 4 + elen > ee) TLS_FAIL("bad ServerHello extension");
        if (et == 43 && elen == 2 && e[4] == 3 && e[5] == 4) v13 = 1;
        if (et == 51 && elen == 69 && e[4] == 0 && e[5] == 0x17 && e[6] == 0 && e[7] == 65 && e[8] == 4) spub = e + 9;
        e += 4 + elen;
    }
    if (!v13 || !spub) TLS_FAIL("server did not negotiate TLS 1.3 with P-256");
    uint8_t shared[32];
    if (!p256_shared(spub, eph.priv, shared)) TLS_FAIL("bad server key share");   /* p256_shared also validates the point */

    /* handshake secrets */
    uint8_t zero[32] = {0}, early[32], eh[32], derived[32], hs[32], h1[32], c_hs[32], s_hs[32];
    sha256("", 0, eh);
    hkdf_extract(NULL, 0, zero, 32, early);
    tls_hkdf_label(early, "derived", eh, 32, derived, 32);
    hkdf_extract(derived, 32, shared, 32, hs);
    th_hash(&th, h1);
    tls_hkdf_label(hs, "c hs traffic", h1, 32, c_hs, 32);
    tls_hkdf_label(hs, "s hs traffic", h1, 32, s_hs, 32);
    tls_keys(s_hs, t->rk, t->rv); t->rseq = 0; t->enc_r = 1;

    /* server flight: EncryptedExtensions, Certificate, CertificateVerify, Finished.
     * Messages are parsed in place from t->rec (cur/cn = unread handshake bytes in the current record).
     * hb is only mapped, and only touched, when a message is split across records; it then holds
     * exactly that one message (we copy no more than the message needs), so hn is 0 between messages.
     * It is whole pages from mmap and goes back with munmap, so it never stays in RAM after the handshake. */
    enum { CAP = 8192 };
    uint8_t *hb = NULL; size_t hn = 0;
    const uint8_t *cur = NULL; size_t cn = 0;
    uint8_t leafpub[64], h_cert[32], h_fin[32]; int got_cert = 0, got_cv = 0, rc = -1;
    for (;;) {
        const uint8_t *m; size_t ml = 0;
        if (hn == 0 && cn >= 4 && (ml = HS_LEN(cur)) <= cn) {                         /* whole message sits in rec */
            m = cur; cur += ml; cn -= ml;
        } else if (cn == 0) {                                                          /* need the next record */
            if (tls_read_rec(t, &type, &len) || type != 22) { fprintf(stderr, "tls: handshake failed (connection closed or alert)\n"); goto out; }
            cur = t->rec + 5; cn = len;
            continue;
        } else {                                                                       /* message split across records: assemble in hb */
            if (hn >= 4 && HS_LEN(hb) > CAP) { fprintf(stderr, "tls: handshake message too large\n"); goto out; }
            if (!hb && !(hb = pg_alloc(CAP))) { fprintf(stderr, "tls: out of memory\n"); goto out; }
            size_t want = hn < 4 ? 4 - hn : HS_LEN(hb) - hn;
            size_t k = cn < want ? cn : want;
            memcpy(hb + hn, cur, k); hn += k; cur += k; cn -= k;
            if (hn < 4 || hn < HS_LEN(hb)) continue;
            m = hb; ml = hn; hn = 0;
        }
        int mt = m[0];
        if (mt == 8) {
            sha_add(&th, m, ml);
        } else if (mt == 11) {
            if (tls_check_certs(m + 4, ml - 4, host, leafpub)) goto out;
            sha_add(&th, m, ml); th_hash(&th, h_cert); got_cert = 1;
        } else if (mt == 15) {
            if (!got_cert || ml < 8 || (m[4] << 8 | m[5]) != 0x0403) { fprintf(stderr, "tls: unsupported CertificateVerify\n"); goto out; }
            size_t sl_ = (size_t)m[6] << 8 | m[7]; uint8_t raw[64], sc[130], dg[32];
            if (8 + sl_ != ml || ecdsa_raw(m + 8, sl_, raw)) { fprintf(stderr, "tls: bad CertificateVerify\n"); goto out; }
            memset(sc, 0x20, 64); memcpy(sc + 64, "TLS 1.3, server CertificateVerify", 33); sc[97] = 0; memcpy(sc + 98, h_cert, 32);
            sha256(sc, 130, dg);
            if (!p256_verify(leafpub, dg, raw)) { fprintf(stderr, "tls: CertificateVerify signature is wrong\n"); goto out; }
            sha_add(&th, m, ml); got_cv = 1;
        } else if (mt == 20) {
            uint8_t fk[32], hh[32], want[32];
            if (!got_cv || ml != 36) { fprintf(stderr, "tls: unexpected Finished\n"); goto out; }
            th_hash(&th, hh);
            tls_hkdf_label(s_hs, "finished", NULL, 0, fk, 32);
            hmac256(fk, 32, hh, 32, want);
            if (ct_memcmp(want, m + 4, 32)) { fprintf(stderr, "tls: server Finished does not verify\n"); goto out; }
            sha_add(&th, m, ml); th_hash(&th, h_fin);
            break;
        } else { fprintf(stderr, "tls: unexpected handshake message %d\n", mt); goto out; }
    }
    rc = 0;
out:
    pg_free(hb, CAP);
    if (rc) return -1;

    /* our Finished, then application keys */
    uint8_t master[32], c_ap[32], s_ap[32], fk[32], fin[36];
    tls_hkdf_label(hs, "derived", eh, 32, derived, 32);
    hkdf_extract(derived, 32, zero, 32, master);
    tls_hkdf_label(master, "c ap traffic", h_fin, 32, c_ap, 32);
    tls_hkdf_label(master, "s ap traffic", h_fin, 32, s_ap, 32);
    tls_hkdf_label(c_hs, "finished", NULL, 0, fk, 32);
    fin[0] = 20; fin[1] = 0; fin[2] = 0; fin[3] = 32;
    hmac256(fk, 32, h_fin, 32, fin + 4);
    { const uint8_t ccs = 1; if (tls_send_rec(t, 20, &ccs, 1)) TLS_FAIL("send failed"); }
    tls_keys(c_hs, t->wk, t->wv); t->wseq = 0; t->enc_w = 1;
    if (tls_send_rec(t, 22, fin, 36)) TLS_FAIL("cannot send Finished");
    tls_keys(c_ap, t->wk, t->wv); t->wseq = 0;
    tls_keys(s_ap, t->rk, t->rv); t->rseq = 0;
    t->off = t->end = 0;
    return 0;
}

/* ---------- application data ---------- */
static int tls_read(tls_t *t, uint8_t *p, size_t n) {              /* >0 bytes, or -1 when closed / failed */
    while (t->off == t->end) {
        int type; size_t len;
        if (tls_read_rec(t, &type, &len)) return -1;
        if (type == 23) { t->off = 5; t->end = 5 + len; }
        else if (type == 21) return -1;                            /* alert, including close_notify */
        /* type 22: NewSessionTicket etc. are ignored */
    }
    size_t k = t->end - t->off; if (k > n) k = n;
    memcpy(p, t->rec + t->off, k); t->off += k;
    return (int)k;
}
static int tls_write(tls_t *t, const uint8_t *p, size_t n) {       /* 0 on success */
    while (n) { size_t k = n > 1024 ? 1024 : n; if (tls_send_rec(t, 23, p, k)) return -1; p += k; n -= k; }
    return 0;
}
static int tls_pending(const tls_t *t) { return t->off < t->end; }
#endif