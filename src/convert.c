// convert.c - MP3 -> .bin ([u16 BE length][opus packet]...). Usage: build/convert assets/in.mp3 assets/out.bin
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opusenc.h"
#include "mp3.h"
#include "audio.h" // SAMPLE_RATE, PACKET_MS, PACKET_SAMPLES, MAX_PACKET: shared with bot.c

// SETTINGS:
static const int BITRATE = 96000; // A number between 40000-510000. Under 40kbps requires SILK, opusenc.h is CELT only.
                                  // 510k is opus's max. Increases audio quality at the cost of network bandwidth. 
static const int COMPLEXITY = 9;  // A number between 0-10. 0 = fastest, 10 = slowest. Controls the CPU effort
                                  // Opus spends to find the best way to encode audio at the target bitrate.
static const int VBR = 1;         // 1 (varies) or 0 (fixed). To vary bitrate to save bandwidth vs predictability.
static const int VBR_CONSTRAINED = 1; // 1 (constrained) or 0 (not constrained). If constrained, a frame avoids going
                                      // above the bitrate, otherwise it more often will as long as the average is under.
                                      // Discord voice channels have a hard kbps cap so constrained makes more sense. 
static const int APPLICATION = OPUS_APPLICATION_AUDIO; // Either AUDIO = best quality, ~2.5 ms more latency.
                                                       // Or RESTRICTED_LOWDELAY = less quality, ~2.5 ms less latency.
                                                       // No VOIP mode because opusenc.h is CELT only.
static const int SIGNAL = OPUS_SIGNAL_MUSIC; // Either _AUTO, SIGNAL_MUSIC or SIGNAL_VOICE. May be better quality if specified.
static const int MAX_BANDWIDTH = OPUS_BANDWIDTH_FULLBAND; // Either _NARROWBAND 4kHz, MEDIUMBAND 6 kHz, WIDEBAND 8 kHz, 
                                                          // SUPERWIDEBAND 12 kHz or FULLBAND 20 kHz. Avoid changing,
                                                          // hearing range is 20Hz-20kHz, anything lower removes detail;
                                                          // waves that cycle faster than the limit are filtered out.
                                                          // Ideally, sample rate should be 2x of frequency bandwidth.

static void die(const char *m) { fprintf(stderr, "error: %s\n", m); exit(1); }

#define CTL(req) do { if (opus_encoder_ctl(e, req) != OPUS_OK) die("cannot set " #req); } while (0)

/* File name without its directory. */
static const char *base(const char *p) { const char *s = strrchr(p, '/'); return s ? s + 1 : p; }

/* Formats "m:ss, X.XMB" into a 48-byte buffer. */
static void fmt(char *b, double secs, double bytes) {
    int t = (int)(secs + 0.5);
    snprintf(b, 48, "%d:%02d, %.1fMB", t / 60, t % 60, bytes / 1e6);
}

/* Returns 1 and fills bytes/secs if the .bin already exists, else 0. Walks the [u16 length][packet] records. */
static int old_bin(const char *p, double *bytes, double *secs) {
    FILE *f = fopen(p, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    *bytes = (double)sz;
    long n = 0; int hi, lo;
    while ((hi = fgetc(f)) != EOF && (lo = fgetc(f)) != EOF) {
        if (fseek(f, (hi << 8) | lo, SEEK_CUR) || ftell(f) > sz) break;
        n++;
    }
    fclose(f);
    *secs = (double)n * PACKET_MS / 1000.0;
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "usage: %s assets/in.mp3 assets/out.bin\n", argv[0]); return 1; }
    const char *path = argv[1], *outp = argv[2];

    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "error: cannot open mp3: %s\n", path); return 1; }
    fseek(f, 0, SEEK_END); long fsz = ftell(f); rewind(f);
    uint8_t *mp3 = malloc(fsz > 0 ? (size_t)fsz : 1);
    if (!mp3 || fsz <= 0 || fread(mp3, 1, (size_t)fsz, f) != (size_t)fsz) die("cannot read mp3");
    fclose(f);

    int16_t *buf; size_t inf; int hz, channels;
    if (mp3_decode_file(mp3, (size_t)fsz, &buf, &inf, &hz, &channels)) die("cannot decode mp3");
    free(mp3);
    size_t frames = (size_t)((uint64_t)inf * SAMPLE_RATE / hz);
    int16_t *pcm = malloc(frames * 4);
    if (!pcm) die("out of memory");
    for (size_t i = 0; i < frames; i++) {  /* linear resample to SAMPLE_RATE stereo */
        double src = (double)i * hz / SAMPLE_RATE; size_t i0 = (size_t)src, i1 = i0 + 1 < inf ? i0 + 1 : inf - 1; double fr = src - i0;
        for (int c = 0; c < 2; c++) {
            int ch = channels == 1 ? 0 : c; double a = buf[i0 * channels + ch], b = buf[i1 * channels + ch];
            pcm[i * 2 + c] = (int16_t)(a + (b - a) * fr);
        }
    }
    free(buf);

    int err; OpusEncoder *e = opus_encoder_create(SAMPLE_RATE, 2, APPLICATION, &err);
    if (!e) die("cannot create opus encoder");
    CTL(OPUS_SET_BITRATE(BITRATE));
    CTL(OPUS_SET_COMPLEXITY(COMPLEXITY));
    CTL(OPUS_SET_VBR(VBR));
    CTL(OPUS_SET_VBR_CONSTRAINT(VBR_CONSTRAINED));
    CTL(OPUS_SET_SIGNAL(SIGNAL));
    CTL(OPUS_SET_MAX_BANDWIDTH(MAX_BANDWIDTH));

    /* Inspect any existing .bin before "wb" truncates it. */
    double old_bytes = 0, old_secs = 0;
    int existed = old_bin(outp, &old_bytes, &old_secs);

    FILE *o = fopen(outp, "wb");
    if (!o) { fprintf(stderr, "error: cannot write output: %s (does the folder exist?)\n", outp); return 1; }
    for (size_t p = 0; p < frames; p += PACKET_SAMPLES) {
        int16_t fr[PACKET_SAMPLES * 2] = {0}; uint8_t pk[MAX_PACKET];
        size_t n = p + PACKET_SAMPLES <= frames ? PACKET_SAMPLES : frames - p;
        memcpy(fr, pcm + p * 2, n * 4);
        int ol = opus_encode(e, fr, PACKET_SAMPLES, pk, sizeof pk);
        if (ol < 0) die("opus_encode failed (BITRATE too low for the CELT-only encoder in opusenc.h?)");
        fputc(ol >> 8, o); fputc(ol & 255, o); fwrite(pk, 1, (size_t)ol, o);
    }
    long new_bytes = ftell(o);
    fclose(o); free(pcm); opus_encoder_destroy(e);

    char mp3s[48], news[48], olds[48];
    fmt(mp3s, (double)inf / hz, (double)fsz);
    fmt(news, frames / (double)SAMPLE_RATE, (double)new_bytes);
    if (existed) {
        fmt(olds, old_secs, old_bytes);
        fprintf(stderr, "Reconverted %s (%s) with %s (%s) -> %s (%s)\n",
                base(outp), olds, base(path), mp3s, base(outp), news);
    } else {
        fprintf(stderr, "Converted %s (%s) -> %s (%s)\n",
                base(path), mp3s, base(outp), news);
    }
    return 0;
}