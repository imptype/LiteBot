/* dave.h - minimal DAVE (Discord A/V E2EE) for a send-only bot. Pure C on mcrypto.h (no OpenSSL).
 *
 * MLS ciphersuite 2 (DHKEMP256 / AES128GCM / SHA256 / P256).
 * Scope: build a KeyPackage, accept an MLS Welcome, derive the per-sender media key,
 * encrypt Opus frames. Nothing here allocates (fixed stack buffers, streamed hashes). The bot never commits; if it is in a group and a commit arrives
 * it reports it as unprocessable (op 31) and is re-added via a fresh Welcome.
 */
#ifndef DAVE_H
#define DAVE_H
#include "mcrypto.h"

/* ---------- MLS variable-size vectors (writer + reader) ---------- */
static size_t vec_hdr(uint8_t *h, size_t n) {            /* MLS variable-size vector length header; returns its size */
    if (n < 64) { h[0] = (uint8_t)n; return 1; }
    if (n < 16384) { h[0] = (uint8_t)(0x40 | n >> 8); h[1] = (uint8_t)n; return 2; }
    h[0] = (uint8_t)(0x80 | n >> 24); h[1] = (uint8_t)(n >> 16); h[2] = (uint8_t)(n >> 8); h[3] = (uint8_t)n;
    return 4;
}
static void b_vec(buf *b, const void *d, size_t n) {
    uint8_t h[4];
    b_put(b, h, vec_hdr(h, n));
    b_put(b, d, n);
}
static void b_mls(buf *b, const char *label) {          /* vec("MLS 1.0 " || label); labels are short */
    size_t n = strlen(label);
    b_u8(b, (unsigned)(8 + n)); b_put(b, "MLS 1.0 ", 8); b_put(b, label, n);
}
static void sha_vec(sha_t *s, const void *d, size_t n) { uint8_t h[4]; sha_add(s, h, vec_hdr(h, n)); sha_add(s, d, n); }   /* same bytes as b_vec, streamed */
static void sha_mls(sha_t *s, const char *label) {
    size_t n = strlen(label); uint8_t l = (uint8_t)(8 + n);
    sha_add(s, &l, 1); sha_add(s, "MLS 1.0 ", 8); sha_add(s, label, n);
}

typedef struct { const uint8_t *p; size_t n; int bad; } rd;
static uint64_t r_be(rd *r, int k) {
    if (r->n < (size_t)k) { r->bad = 1; r->n = 0; return 0; }
    uint64_t v = 0;
    for (int i = 0; i < k; i++) v = v << 8 | r->p[i];
    r->p += k; r->n -= k;
    return v;
}
static size_t r_len(rd *r) {
    if (!r->n) { r->bad = 1; return 0; }
    int pre = r->p[0] >> 6;
    if (pre == 0) return (size_t)r_be(r, 1);
    if (pre == 1) return (size_t)(r_be(r, 2) & 0x3fff);
    if (pre == 2) return (size_t)(r_be(r, 4) & 0x3fffffff);
    r->bad = 1; return 0;
}
static rd r_vec(rd *r) {
    size_t l = r_len(r);
    rd o = { r->p, 0, 0 };
    if (r->bad || l > r->n) { r->bad = 1; return o; }
    o.n = l; r->p += l; r->n -= l;
    return o;
}

/* ---------- MLS key-schedule helpers (RFC 9420 8, 5.1) ---------- */
static void mls_ewl(const uint8_t *sec, size_t sl, const char *label, const uint8_t *ctx, size_t cl, uint8_t *out, size_t L) {
    FBUF(b, 48);                                               /* KDFLabel = length | vec(label) | vec(context); the context is streamed */
    b_be(&b, L, 2); b_mls(&b, label);
    uint8_t h[4]; size_t hn = vec_hdr(h, cl); b_put(&b, h, hn);
    hkdf_expand2(sec, sl, b.p, b.n, ctx, cl, out, L);
}
static void mls_ds(const uint8_t *sec, const char *label, uint8_t out[32]) { mls_ewl(sec, 32, label, NULL, 0, out, 32); }

/* MLS-Exporter(label, context, L) */
static void mls_export(const uint8_t exporter_secret[32], const char *label, const uint8_t *ctx, size_t cl, uint8_t *out, size_t L) {
    uint8_t d[32], h[32];
    mls_ds(exporter_secret, label, d);
    sha256(ctx, cl, h);
    mls_ewl(d, 32, "exported", h, 32, out, L);
}

/* SignWithLabel: ECDSA-P256-SHA256, DER signature. The signed bytes are vec(label) | vec(content), hashed as a stream. */
static void mls_hash(const char *label, const uint8_t *content, size_t cl, uint8_t dg[32]) {
    sha_t s; sha_init(&s);
    sha_mls(&s, label); sha_vec(&s, content, cl);
    sha_fin(&s, dg);
}
static void mls_sign(const ec_t *k, const char *label, const uint8_t *content, size_t cl, buf *sig) {
    uint8_t dg[32], raw[64], s[72];
    mls_hash(label, content, cl, dg);
    ec_sign_hash(k, dg, raw);
    b_put(sig, s, ecdsa_der(raw, s));
}

/* ---------- HPKE base mode: DHKEM(P-256,HKDF-SHA256) / HKDF-SHA256 / AES-128-GCM (RFC 9180) ---------- */
static const uint8_t KEM_SUITE[5] = { 'K', 'E', 'M', 0x00, 0x10 };
static const uint8_t HPKE_SUITE[10] = { 'H', 'P', 'K', 'E', 0x00, 0x10, 0x00, 0x01, 0x00, 0x01 };
static void hp_lx(const uint8_t *suite, size_t sl, const uint8_t *salt, size_t saltl, const char *label,
                  const uint8_t *ikm, size_t il, const uint8_t *ikm2, size_t il2, uint8_t out[32]) {   /* ikm = ikm || ikm2 */
    uint8_t z[32] = {0}; hm_t h;
    if (!saltl) { salt = z; saltl = 32; }
    hm_init(&h, salt, saltl);
    hm_add(&h, "HPKE-v1", 7); hm_add(&h, suite, sl); hm_add(&h, label, strlen(label)); hm_add(&h, ikm, il); hm_add(&h, ikm2, il2);
    hm_fin(&h, out);
}
static void hp_le(const uint8_t *suite, size_t sl, const uint8_t prk[32], const char *label,
                  const uint8_t *info, size_t il, uint8_t *out, size_t L) {
    FBUF(b, 48);
    b_be(&b, L, 2); b_put(&b, "HPKE-v1", 7); b_put(&b, suite, sl); b_put(&b, label, strlen(label));
    hkdf_expand2(prk, 32, b.p, b.n, info, il, out, L);
}
/* Decrypt `ct` (ciphertext||tag) with recipient key `sk`; writes ctl-16 bytes to out.
 * The HPKE info is passed in two pieces (info1 || info2) so the caller never has to copy a big GroupInfo. */
static int hpke_open(const ec_t *sk, const uint8_t *enc, size_t enc_l, const uint8_t *info1, size_t info1_l,
                     const uint8_t *info2, size_t info2_l, const uint8_t *ct, size_t ctl, uint8_t *out) {
    if (enc_l != 65 || enc[0] != 4 || ctl < 16) return 0;
    uint8_t dh[32], pkr[65], kctx[130], prk[32], ss[32], pid[32], ih[32], sec[32], key[16], nonce[12], ksc[65];
    if (!p256_shared(enc + 1, sk->priv, dh)) return 0;         /* also rejects a point that is not on the curve */
    ec_pub(sk, pkr);
    memcpy(kctx, enc, 65); memcpy(kctx + 65, pkr, 65);
    hp_lx(KEM_SUITE, 5, NULL, 0, "eae_prk", dh, 32, NULL, 0, prk);
    hp_le(KEM_SUITE, 5, prk, "shared_secret", kctx, 130, ss, 32);
    hp_lx(HPKE_SUITE, 10, NULL, 0, "psk_id_hash", NULL, 0, NULL, 0, pid);
    hp_lx(HPKE_SUITE, 10, NULL, 0, "info_hash", info1, info1_l, info2, info2_l, ih);
    ksc[0] = 0; memcpy(ksc + 1, pid, 32); memcpy(ksc + 33, ih, 32);
    hp_lx(HPKE_SUITE, 10, ss, 32, "secret", NULL, 0, NULL, 0, sec);
    hp_le(HPKE_SUITE, 10, sec, "key", ksc, 65, key, 16);
    hp_le(HPKE_SUITE, 10, sec, "base_nonce", ksc, 65, nonce, 12);
    return gcm(0, 16, key, nonce, NULL, 0, ct, ctl - 16, out, (uint8_t *)ct + ctl - 16);
}

/* ---------- AES-128-GCM open without writing the input: authenticate, then decrypt chosen slices ---------- */
typedef struct { aes_t a; uint8_t j0[16]; } gcm_slice_t;
static int gcm_open_begin(gcm_slice_t *g, const uint8_t key[16], const uint8_t iv[12],
                          const uint8_t *ct, size_t cl, const uint8_t tag[16]) {   /* 1 = tag ok */
    uint8_t h[16] = {0}, want[16];
    aes_key(&g->a, key, 16); aes_enc(&g->a, h, h);
    memcpy(g->j0, iv, 12); g->j0[12] = g->j0[13] = g->j0[14] = 0; g->j0[15] = 1;
    ghash_tag(&g->a, h, g->j0, NULL, 0, ct, cl, want);
    return !ct_memcmp(want, tag, 16);
}
static int gcm_slice(const gcm_slice_t *g, const uint8_t *ct, size_t cl, size_t off, size_t n, uint8_t *out) {
    if (off > cl || n > cl - off) return 0;
    uint8_t ctr[16], ks[16]; memcpy(ctr, g->j0, 12);
    for (size_t o = 0; o < n; ) {
        size_t pos = off + o, skip = pos % 16;
        uint32_t c = (uint32_t)(pos / 16 + 2);                    /* block 0 uses counter 2 (J0 is 1) */
        ctr[12] = (uint8_t)(c >> 24); ctr[13] = (uint8_t)(c >> 16); ctr[14] = (uint8_t)(c >> 8); ctr[15] = (uint8_t)c;
        aes_enc(&g->a, ctr, ks);
        size_t k = 16 - skip; if (k > n - o) k = n - o;
        for (size_t i = 0; i < k; i++) out[o + i] = ct[pos + i] ^ ks[skip + i];
        o += k;
    }
    return 1;
}

/* ---------- DAVE state ---------- */
typedef struct {
    uint64_t uid;
    ec_t sig;             /* identity (signature) key, lives for the whole process */
    ec_t init; int have_init;   /* init key of the outstanding key package */
    uint8_t kpref[32];    /* KeyPackageRef of the outstanding key package */
    uint8_t ext_sender[512]; size_t ext_len;   /* op 25 payload: ExternalSender */
    uint64_t epoch;
} dave_t;

static void dave_init(dave_t *d, uint64_t uid) {
    memset(d, 0, sizeof *d);
    d->uid = uid; ec_gen(&d->sig);
}

/* Build op-26 payload (opcode byte + raw KeyPackage) and remember init key + ref.
 * `out` must be empty and have room for 1 + 394 bytes: the key package is 390..394 bytes, depending on the DER length of its two signatures. */
__attribute__((noinline)) static void dave_key_package(dave_t *d, buf *out) {   /* noinline: its ~1 KB of buffers must not stay on the stack while the caller sends the result */
    uint8_t spub[65], epub[65], ipub[65], id[8];
    ec_t enc; ec_gen(&enc);
    ec_gen(&d->init); d->have_init = 1;
    ec_pub(&d->sig, spub); ec_pub(&enc, epub); ec_pub(&d->init, ipub);
    for (int i = 0; i < 8; i++) id[i] = (uint8_t)(d->uid >> (56 - 8 * i));

    /* LeafNodeTBS (source = key_package needs no trailing fields): 174 bytes, at most 248 with its signature */
    static const uint8_t CAPS[] = { 2, 0, 1,  2, 0, 2,  0,  0,  2, 0, 1 };   /* versions [mls10], suites [2], no extensions, no proposals, credentials [basic] */
    FBUF(ln, 288); FBUF(sig, 80);
    b_vec(&ln, epub, 65); b_vec(&ln, spub, 65);
    b_be(&ln, 1, 2); b_vec(&ln, id, 8);                        /* basic credential */
    b_put(&ln, CAPS, sizeof CAPS);
    b_u8(&ln, 1);                                              /* leaf_node_source = key_package */
    b_be(&ln, 0, 8); b_be(&ln, ~0ULL, 8);                      /* lifetime 0 .. 2^64-1 */
    b_vec(&ln, NULL, 0);                                       /* leaf extensions */
    mls_sign(&d->sig, "LeafNodeTBS", ln.p, ln.n, &sig);
    b_vec(&ln, sig.p, sig.n);                                  /* -> LeafNode */

    /* KeyPackageTBS: about 320 bytes, at most 394 with its signature. Built in `out` right behind the op byte, so no second buffer is needed. */
    b_u8(out, 26);                                             /* op 26 | raw KeyPackage (no MLSMessage wrapper) */
    size_t kp0 = out->n;
    sig.n = 0;
    b_be(out, 1, 2); b_be(out, 2, 2); b_vec(out, ipub, 65);
    b_put(out, ln.p, ln.n);
    b_vec(out, NULL, 0);                                       /* kp extensions */
    mls_sign(&d->sig, "KeyPackageTBS", out->p + kp0, out->n - kp0, &sig);
    b_vec(out, sig.p, sig.n);                                  /* -> KeyPackage */

    {   /* KeyPackageRef = RefHash("MLS 1.0 KeyPackage Reference", KeyPackage) */
        sha_t h; sha_init(&h);
        sha_mls(&h, "KeyPackage Reference"); sha_vec(&h, out->p + kp0, out->n - kp0);
        sha_fin(&h, d->kpref);
    }
}

/* Process a Welcome (the bytes after `transition_id` in op 30). On success fills
 * exporter_secret. Verifies the confirmation tag and the external sender.
 * `w` is only read. The GroupInfo is authenticated in place and only the slices we need are
 * decrypted into small stack buffers (no malloc, no copy of the ratchet tree). */
static int dave_welcome(dave_t *d, const uint8_t *w, size_t wl, uint8_t exporter_secret[32]) {
    rd r = { w, wl, 0 };
    if (r_be(&r, 2) != 2) { fprintf(stderr, "welcome: unexpected ciphersuite\n"); return -1; }
    rd secrets = r_vec(&r), egi = r_vec(&r);
    if (r.bad) return -1;
    rd kem = {0}, ct = {0}; int found = 0;
    while (secrets.n && !secrets.bad) {
        rd ref = r_vec(&secrets), k = r_vec(&secrets), c = r_vec(&secrets);
        if (secrets.bad) return -1;
        if (ref.n == 32 && !memcmp(ref.p, d->kpref, 32)) { kem = k; ct = c; found = 1; break; }
    }
    if (!found || !d->have_init) { fprintf(stderr, "welcome: not for our key package\n"); return -1; }

    FBUF(info, 24);                                                    /* EncryptContext = vec("MLS 1.0 Welcome") | vec(GroupInfo): the first part is built here, */
    b_mls(&info, "Welcome");                                           /* the GroupInfo bytes themselves are hashed straight from the message (no copy) */
    { uint8_t h[4]; b_put(&info, h, vec_hdr(h, egi.n)); }
    uint8_t gs[256];                                              /* GroupSecrets: joiner_secret + optional path_secret + psks */
    int ok = ct.n >= 16 && ct.n - 16 <= sizeof gs && hpke_open(&d->init, kem.p, kem.n, info.p, info.n, egi.p, egi.n, ct.p, ct.n, gs);
    if (!ok) { fprintf(stderr, "welcome: cannot decrypt GroupSecrets\n"); return -1; }
    rd g = { gs, ct.n - 16, 0 };
    rd joiner = r_vec(&g);
    if (joiner.bad || joiner.n != 32) return -1;
    uint8_t joiner_secret[32]; memcpy(joiner_secret, joiner.p, 32);   /* path_secret / psks ignored (never used) */

    uint8_t zero[32] = {0}, member[32], wsec[32], wkey[16], wnonce[12];
    hkdf_extract(joiner_secret, 32, zero, 32, member);           /* psk_secret = 0 */
    mls_ds(member, "welcome", wsec);
    mls_ewl(wsec, 32, "key", NULL, 0, wkey, 16);
    mls_ewl(wsec, 32, "nonce", NULL, 0, wnonce, 12);
    if (egi.n < 16) { fprintf(stderr, "welcome: GroupInfo too short\n"); return -1; }
    size_t gil = egi.n - 16;
    gcm_slice_t gsl;                                             /* authenticate only; w is never written */
    if (!gcm_open_begin(&gsl, wkey, wnonce, egi.p, gil, egi.p + gil)) {
        fprintf(stderr, "welcome: cannot decrypt GroupInfo\n"); return -1;
    }

    /* GroupInfo = GroupContext | extensions<V> | confirmation_tag<V> | signer u32 | signature<V>
     * We only need the GroupContext (at the start) and the confirmation tag (after the ratchet-tree
     * extension), so decrypt just those two slices into small stack buffers. */
    uint8_t pre[512], tb[33];
    size_t pn = gil < sizeof pre ? gil : sizeof pre;
    gcm_slice(&gsl, egi.p, gil, 0, pn, pre);
    rd gr = { pre, pn, 0 };
    const uint8_t *gc_start = gr.p;
    r_be(&gr, 2); r_be(&gr, 2); r_vec(&gr);                         /* version, suite, group_id */
    d->epoch = r_be(&gr, 8);
    r_vec(&gr);                                                     /* tree_hash */
    rd cth = r_vec(&gr);                                            /* confirmed_transcript_hash */
    rd gcext = r_vec(&gr);                                          /* group context extensions */
    size_t gc_len = (size_t)(gr.p - gc_start);
    size_t exl = r_len(&gr);                                        /* GroupInfo extensions (ratchet tree; skipped, never decrypted) */
    if (gr.bad) { fprintf(stderr, "welcome: malformed GroupInfo (or GroupContext over 512 bytes)\n"); return -1; }
    size_t toff = (size_t)(gr.p - pre) + exl;
    if (exl > gil || toff > gil || gil - toff < 33 || !gcm_slice(&gsl, egi.p, gil, toff, 33, tb)) {
        fprintf(stderr, "welcome: malformed GroupInfo\n"); return -1;
    }
    rd tr = { tb, 33, 0 };
    rd tag = r_vec(&tr);
    if (tr.bad || tag.n != 32) { fprintf(stderr, "welcome: malformed GroupInfo\n"); return -1; }

    uint8_t epoch_secret[32], ckey[32], want[32];
    mls_ewl(member, 32, "epoch", gc_start, gc_len, epoch_secret, 32);
    mls_ds(epoch_secret, "confirm", ckey);
    hmac256(ckey, 32, cth.p, cth.n, want);
    if (ct_memcmp(want, tag.p, 32)) { fprintf(stderr, "welcome: confirmation tag mismatch\n"); return -1; }

    /* the group must contain exactly the external sender the gateway gave us (op 25) */
    int ext_ok = 0;
    while (gcext.n && !gcext.bad) {
        unsigned type = (unsigned)r_be(&gcext, 2);
        rd data = r_vec(&gcext);
        if (type == 5) {                                            /* external_senders */
            rd senders = data; rd inner = r_vec(&senders);
            if (!senders.bad && d->ext_len && inner.n == d->ext_len && !memcmp(inner.p, d->ext_sender, d->ext_len)) ext_ok = 1;
        }
    }
    if (!ext_ok) { fprintf(stderr, "welcome: external sender mismatch\n"); return -1; }

    mls_ds(epoch_secret, "exporter", exporter_secret);
    return 0;
}

/* ---------- media keys and frames ---------- */
static void dave_base_secret(const uint8_t exporter_secret[32], uint64_t uid, uint8_t out[16]) {
    uint8_t ctx[8];
    for (int i = 0; i < 8; i++) ctx[i] = (uint8_t)(uid >> (8 * i));     /* little-endian user id */
    mls_export(exporter_secret, "Discord Secure Frames v0", ctx, 8, out, 16);
}
static void dave_sender_key(const uint8_t base[16], uint32_t gen, uint8_t key[16]) {
    uint8_t cur[32], nxt[32]; size_t cl = 16;
    memcpy(cur, base, 16);
    for (uint32_t g = 0; g <= gen; g++) {
        uint8_t c[4] = { (uint8_t)(g >> 24), (uint8_t)(g >> 16), (uint8_t)(g >> 8), (uint8_t)g };
        mls_ewl(cur, cl, "key", c, 4, key, 16);
        if (g < gen) { mls_ewl(cur, cl, "secret", c, 4, nxt, 32); memcpy(cur, nxt, 32); cl = 32; }
    }
}
/* Opus frame -> protocol frame: ciphertext | tag[8] | uleb(nonce) | size | 0xFAFA. Returns the frame length. */
static size_t dave_encrypt_frame(const uint8_t key[16], uint32_t nonce, const uint8_t *in, size_t n, uint8_t *out) {
    uint8_t iv[12] = {0}, tag[16], leb[5]; size_t ln = 0;
    iv[8] = (uint8_t)nonce; iv[9] = (uint8_t)(nonce >> 8); iv[10] = (uint8_t)(nonce >> 16); iv[11] = (uint8_t)(nonce >> 24);
    gcm(1, 16, key, iv, NULL, 0, in, n, out, tag);
    memcpy(out + n, tag, 8);
    uint32_t v = nonce;
    while (v >= 0x80) { leb[ln++] = 0x80 | (v & 0x7f); v >>= 7; }
    leb[ln++] = (uint8_t)v;
    memcpy(out + n + 8, leb, ln);
    size_t o = n + 8 + ln;
    out[o++] = (uint8_t)(8 + ln + 1 + 2);        /* supplemental data size */
    out[o++] = 0xFA; out[o++] = 0xFA;
    return o;
}
#endif