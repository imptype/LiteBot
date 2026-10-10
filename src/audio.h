// audio.h - the packet format shared by convert.c (writes the .bin) and bot.c (plays it).
// Values come from https://www.opus-codec.org/docs/opus_api-1.6/group__opus__ctlvalues.html

#ifndef AUDIO_H
#define AUDIO_H

// SHARED SETTINGS (used in convert.c and bot.c):
#define SAMPLE_RATE 48000 // Any of 8/12/16/24/48 * 1000. Number of samples per second;
                          // 48000 16-bit values per channel per 1 second.
#define PACKET_MS   100   // Any of 5/10/20/40/60/80/100. Duration of each packet.
                          // 2.5 (float) is not supported, requires changing datatypes.
                          // 120 is not supported by Discord yet. A higher ms means
                          // less CPU/energy, but longer latency drops if bad network.

// AUDIO CONSTANTS (no need to change):
#define PACKET_SAMPLES (SAMPLE_RATE * PACKET_MS / 1000)
#define OPUS_MAX_FRAME_BYTES 1275 // Max bytes in one Opus frame is 1275.
#define OPUS_MAX_FRAME_MS    20   // Max opus frame is 20 ms, so bigger packets packs multiple in one.
#define OPUS_FRAME_OVERHEAD  2    // Per-frame packet header bytes, worst case: 1 TOC byte (+1 frame-count byte
                                  // and up to 2 length bytes per extra frame in multi-frame packets). (?)
#define PACKET_FRAMES ((PACKET_MS + OPUS_MAX_FRAME_MS - 1) / OPUS_MAX_FRAME_MS)
#define MAX_PACKET    (PACKET_FRAMES * (OPUS_MAX_FRAME_BYTES + OPUS_FRAME_OVERHEAD))
#define RTP_STEP (48 * PACKET_MS) // Discord always advances with a sample rate of 48 kHZ so this 
                                  // normalizes the window if it's smaller, there is no loss of quality.

#endif