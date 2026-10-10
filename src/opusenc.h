/* opusenc.h - minimal single-file Opus encoder (libopus 1.5.2, float build)
 *
 * WHAT THIS IS
 *   The encoder half of libopus, reduced to the code reachable from
 *       opus_encoder_create / opus_encoder_ctl / opus_encode / opus_encoder_destroy
 *   plus the public OPUS_xxx constants and OPUS_SET_xxx and OPUS_GET_xxx macros. It was
 *   generated mechanically from the unmodified libopus v1.5.2 sources (one
 *   translation unit, preprocessor conditionals resolved, unreferenced code
 *   removed), not rewritten. It carries no settings of its own: choose bitrate,
 *   complexity, VBR, ... with opus_encoder_ctl(), as with the real library.
 *
 * OUTPUT
 *   Byte-identical to libopus 1.5.2 built from plain C (no SSE/AVX/NEON
 *   intrinsics), checked on 20 s of mixed test audio at 40-510 kbps, complexity
 *   0/5/9/10, and 10/20/40 ms frames. A libopus built with SIMD kernels (most
 *   distro x86 packages) sums floats in a different order, so it can differ
 *   in the low bits of many packets (same sizes, same sound).
 *
 * LIMITS
 *   - SILK (the speech codec) and hybrid mode are NOT included. libopus only
 *     picks them at low bitrates or for speech-like input (below roughly 40 kbps
 *     for stereo music). Where it would, opus_encode() returns OPUS_INTERNAL_ERROR (-3).
 *   - Only the encoder and what it needs; no decoder, multistream, DRED or custom modes.
 *   - Preprocessor choices were frozen for GCC/Clang, C99 or newer, lrintf().
 *
 * BUILD
 *   Link with -lm and compile with -ffp-contract=off (otherwise a compiler
 *   that fuses multiply-adds, e.g. on aarch64, changes the output).
 *   Include it BEFORE your other project headers: it #undefs its internal
 *   macros at the end, so it cannot clash with them afterwards.
 *
 * LICENSE (from libopus COPYING)
 *   Copyright 2001-2023 Xiph.Org, Skype Limited, Octasic,
 *                       Jean-Marc Valin, Timothy B. Terriberry,
 *                       CSIRO, Gregory Maxwell, Mark Borgerding,
 *                       Erik de Castro Lopo, Mozilla, Amazon
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *   - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   - Neither the name of Internet Society, IETF or IETF Trust, nor the
 *   names of specific contributors, may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 *   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *   Opus is subject to the royalty-free patent licenses specified at
 *   https://datatracker.ietf.org/ipr/1524/ (Xiph.Org),
 *   https://datatracker.ietf.org/ipr/1914/ (Microsoft) and
 *   https://datatracker.ietf.org/ipr/1526/ (Broadcom).
 */
#ifndef OPUSENC_H
#define OPUSENC_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define opus_int int
#define opus_int64 long long
#define opus_int8 signed char
/* Use the real stdint.h if it's there (taken from Paul Hsieh's pstdint.h) */
#undef opus_int64
#undef opus_int8
typedef int8_t opus_int8;
typedef int16_t opus_int16;
typedef int32_t opus_int32;
typedef uint32_t opus_uint32;
typedef int64_t opus_int64;
/* (C) COPYRIGHT 1994-2002 Xiph.Org Foundation */
/* Modified by Jean-Marc Valin */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

- Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/* opus_types.h based on ogg_types.h from libogg */

#define OPUS_OK 0
#define OPUS_BAD_ARG -1
#define OPUS_BUFFER_TOO_SMALL -2
#define OPUS_INTERNAL_ERROR -3
#define OPUS_INVALID_PACKET -4
#define OPUS_UNIMPLEMENTED -5
#define OPUS_INVALID_STATE -6
#define OPUS_ALLOC_FAIL -7

#define OPUS_EXPORT __attribute__ ((visibility ("default")))
#define OPUS_RESTRICT restrict
#define OPUS_INLINE inline
#define OPUS_WARN_UNUSED_RESULT __attribute__ ((__warn_unused_result__))
#define OPUS_ARG_NONNULL(_x) 
#define OPUS_SET_APPLICATION_REQUEST 4000
#define OPUS_GET_APPLICATION_REQUEST 4001
#define OPUS_SET_BITRATE_REQUEST 4002
#define OPUS_GET_BITRATE_REQUEST 4003
#define OPUS_SET_MAX_BANDWIDTH_REQUEST 4004
#define OPUS_GET_MAX_BANDWIDTH_REQUEST 4005
#define OPUS_SET_VBR_REQUEST 4006
#define OPUS_GET_VBR_REQUEST 4007
#define OPUS_SET_BANDWIDTH_REQUEST 4008
#define OPUS_GET_BANDWIDTH_REQUEST 4009
#define OPUS_SET_COMPLEXITY_REQUEST 4010
#define OPUS_GET_COMPLEXITY_REQUEST 4011
#define OPUS_SET_INBAND_FEC_REQUEST 4012
#define OPUS_GET_INBAND_FEC_REQUEST 4013
#define OPUS_SET_PACKET_LOSS_PERC_REQUEST 4014
#define OPUS_GET_PACKET_LOSS_PERC_REQUEST 4015
#define OPUS_SET_DTX_REQUEST 4016
#define OPUS_GET_DTX_REQUEST 4017
#define OPUS_SET_VBR_CONSTRAINT_REQUEST 4020
#define OPUS_GET_VBR_CONSTRAINT_REQUEST 4021
#define OPUS_SET_FORCE_CHANNELS_REQUEST 4022
#define OPUS_GET_FORCE_CHANNELS_REQUEST 4023
#define OPUS_SET_SIGNAL_REQUEST 4024
#define OPUS_GET_SIGNAL_REQUEST 4025
#define OPUS_GET_LOOKAHEAD_REQUEST 4027
/* #define OPUS_RESET_STATE 4028 */
#define OPUS_GET_SAMPLE_RATE_REQUEST 4029
#define OPUS_GET_FINAL_RANGE_REQUEST 4031
#define OPUS_GET_PITCH_REQUEST 4033
#define OPUS_SET_GAIN_REQUEST 4034
#define OPUS_GET_GAIN_REQUEST 4045
#define OPUS_SET_LSB_DEPTH_REQUEST 4036
#define OPUS_GET_LSB_DEPTH_REQUEST 4037
#define OPUS_GET_LAST_PACKET_DURATION_REQUEST 4039
#define OPUS_SET_EXPERT_FRAME_DURATION_REQUEST 4040
#define OPUS_GET_EXPERT_FRAME_DURATION_REQUEST 4041
#define OPUS_SET_PREDICTION_DISABLED_REQUEST 4042
#define OPUS_GET_PREDICTION_DISABLED_REQUEST 4043
/* Don't use 4045, it's already taken by OPUS_GET_GAIN_REQUEST */
#define OPUS_SET_PHASE_INVERSION_DISABLED_REQUEST 4046
#define OPUS_GET_PHASE_INVERSION_DISABLED_REQUEST 4047
#define OPUS_GET_IN_DTX_REQUEST 4049
#define OPUS_SET_DRED_DURATION_REQUEST 4050
#define OPUS_GET_DRED_DURATION_REQUEST 4051
#define OPUS_SET_DNN_BLOB_REQUEST 4052
/* Macros to trigger compilation errors when the wrong types are provided to a CTL */
#define __opus_check_int(x) (((void)((x) == (opus_int32)0)), (opus_int32)(x))
#define __opus_check_int_ptr(ptr) ((ptr) + ((ptr) - (opus_int32*)(ptr)))
#define __opus_check_uint_ptr(ptr) ((ptr) + ((ptr) - (opus_uint32*)(ptr)))
#define __opus_check_val16_ptr(ptr) ((ptr) + ((ptr) - (opus_val16*)(ptr)))
#define __opus_check_void_ptr(x) ((void)((void *)0 == (x)), (x))

/* Values for the various encoder CTLs */
#define OPUS_AUTO -1000
#define OPUS_BITRATE_MAX -1
#define OPUS_APPLICATION_VOIP 2048
#define OPUS_APPLICATION_AUDIO 2049
#define OPUS_APPLICATION_RESTRICTED_LOWDELAY 2051
#define OPUS_SIGNAL_VOICE 3001
#define OPUS_SIGNAL_MUSIC 3002
#define OPUS_BANDWIDTH_NARROWBAND 1101
#define OPUS_BANDWIDTH_MEDIUMBAND 1102
#define OPUS_BANDWIDTH_WIDEBAND 1103
#define OPUS_BANDWIDTH_SUPERWIDEBAND 1104
#define OPUS_BANDWIDTH_FULLBAND 1105
#define OPUS_FRAMESIZE_ARG 5000
#define OPUS_FRAMESIZE_2_5_MS 5001
#define OPUS_FRAMESIZE_5_MS 5002
#define OPUS_FRAMESIZE_10_MS 5003
#define OPUS_FRAMESIZE_20_MS 5004
#define OPUS_FRAMESIZE_40_MS 5005
#define OPUS_FRAMESIZE_60_MS 5006
#define OPUS_FRAMESIZE_80_MS 5007
#define OPUS_FRAMESIZE_100_MS 5008
#define OPUS_FRAMESIZE_120_MS 5009

#define OPUS_SET_COMPLEXITY(x) OPUS_SET_COMPLEXITY_REQUEST, __opus_check_int(x)
#define OPUS_GET_COMPLEXITY(x) OPUS_GET_COMPLEXITY_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_BITRATE(x) OPUS_SET_BITRATE_REQUEST, __opus_check_int(x)
#define OPUS_GET_BITRATE(x) OPUS_GET_BITRATE_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_VBR(x) OPUS_SET_VBR_REQUEST, __opus_check_int(x)
#define OPUS_GET_VBR(x) OPUS_GET_VBR_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_VBR_CONSTRAINT(x) OPUS_SET_VBR_CONSTRAINT_REQUEST, __opus_check_int(x)
#define OPUS_GET_VBR_CONSTRAINT(x) OPUS_GET_VBR_CONSTRAINT_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_FORCE_CHANNELS(x) OPUS_SET_FORCE_CHANNELS_REQUEST, __opus_check_int(x)
#define OPUS_GET_FORCE_CHANNELS(x) OPUS_GET_FORCE_CHANNELS_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_MAX_BANDWIDTH(x) OPUS_SET_MAX_BANDWIDTH_REQUEST, __opus_check_int(x)
#define OPUS_GET_MAX_BANDWIDTH(x) OPUS_GET_MAX_BANDWIDTH_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_BANDWIDTH(x) OPUS_SET_BANDWIDTH_REQUEST, __opus_check_int(x)
#define OPUS_SET_SIGNAL(x) OPUS_SET_SIGNAL_REQUEST, __opus_check_int(x)
#define OPUS_GET_SIGNAL(x) OPUS_GET_SIGNAL_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_APPLICATION(x) OPUS_SET_APPLICATION_REQUEST, __opus_check_int(x)
#define OPUS_GET_APPLICATION(x) OPUS_GET_APPLICATION_REQUEST, __opus_check_int_ptr(x)
#define OPUS_GET_LOOKAHEAD(x) OPUS_GET_LOOKAHEAD_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_INBAND_FEC(x) OPUS_SET_INBAND_FEC_REQUEST, __opus_check_int(x)
#define OPUS_GET_INBAND_FEC(x) OPUS_GET_INBAND_FEC_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_PACKET_LOSS_PERC(x) OPUS_SET_PACKET_LOSS_PERC_REQUEST, __opus_check_int(x)
#define OPUS_GET_PACKET_LOSS_PERC(x) OPUS_GET_PACKET_LOSS_PERC_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_DTX(x) OPUS_SET_DTX_REQUEST, __opus_check_int(x)
#define OPUS_GET_DTX(x) OPUS_GET_DTX_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_LSB_DEPTH(x) OPUS_SET_LSB_DEPTH_REQUEST, __opus_check_int(x)
#define OPUS_GET_LSB_DEPTH(x) OPUS_GET_LSB_DEPTH_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_EXPERT_FRAME_DURATION(x) OPUS_SET_EXPERT_FRAME_DURATION_REQUEST, __opus_check_int(x)
#define OPUS_GET_EXPERT_FRAME_DURATION(x) OPUS_GET_EXPERT_FRAME_DURATION_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_PREDICTION_DISABLED(x) OPUS_SET_PREDICTION_DISABLED_REQUEST, __opus_check_int(x)
#define OPUS_GET_PREDICTION_DISABLED(x) OPUS_GET_PREDICTION_DISABLED_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_DRED_DURATION(x) OPUS_SET_DRED_DURATION_REQUEST, __opus_check_int(x)
#define OPUS_GET_DRED_DURATION(x) OPUS_GET_DRED_DURATION_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_DNN_BLOB(data,len) OPUS_SET_DNN_BLOB_REQUEST, __opus_check_void_ptr(data), __opus_check_int(len)

#define OPUS_RESET_STATE 4028
#define OPUS_GET_FINAL_RANGE(x) OPUS_GET_FINAL_RANGE_REQUEST, __opus_check_uint_ptr(x)
#define OPUS_GET_BANDWIDTH(x) OPUS_GET_BANDWIDTH_REQUEST, __opus_check_int_ptr(x)
#define OPUS_GET_SAMPLE_RATE(x) OPUS_GET_SAMPLE_RATE_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_PHASE_INVERSION_DISABLED(x) OPUS_SET_PHASE_INVERSION_DISABLED_REQUEST, __opus_check_int(x)
#define OPUS_GET_PHASE_INVERSION_DISABLED(x) OPUS_GET_PHASE_INVERSION_DISABLED_REQUEST, __opus_check_int_ptr(x)
#define OPUS_GET_IN_DTX(x) OPUS_GET_IN_DTX_REQUEST, __opus_check_int_ptr(x)

#define OPUS_SET_GAIN(x) OPUS_SET_GAIN_REQUEST, __opus_check_int(x)
#define OPUS_GET_GAIN(x) OPUS_GET_GAIN_REQUEST, __opus_check_int_ptr(x)
#define OPUS_GET_LAST_PACKET_DURATION(x) OPUS_GET_LAST_PACKET_DURATION_REQUEST, __opus_check_int_ptr(x)
#define OPUS_GET_PITCH(x) OPUS_GET_PITCH_REQUEST, __opus_check_int_ptr(x)
#define opus_unlikely(x) (__builtin_expect(!!(x), 0))
#define CELT_SIG_SCALE 32768.f
#define celt_assert(cond) 
#define celt_assert2(cond,message) 
#define celt_sig_assert(cond) 
#define IMUL32(a,b) ((a)*(b))
#define MIN16(a,b) ((a) < (b) ? (a) : (b))
#define MAX16(a,b) ((a) > (b) ? (a) : (b))
#define MIN32(a,b) ((a) < (b) ? (a) : (b))
#define MAX32(a,b) ((a) > (b) ? (a) : (b))
#define IMIN(a,b) ((a) < (b) ? (a) : (b))
#define IMAX(a,b) ((a) > (b) ? (a) : (b))
typedef float opus_val16;
typedef float opus_val32;
typedef float opus_val64;
typedef float celt_sig;
typedef float celt_norm;
typedef float celt_ener;
#define celt_isnan(x) ((x)!=(x))
#define Q15ONE 1.0f
#define NORM_SCALING 1.f
#define EPSILON 1e-15f
#define VERY_SMALL 1e-30f
#define Q15_ONE ((opus_val16)1.f)
/* This appears to be the same speed as C99's fabsf() but it's more portable. */
#define ABS16(x) ((float)fabs(x))
#define ABS32(x) ((float)fabs(x))
#define QCONST16(x,bits) (x)
#define QCONST32(x,bits) (x)
#define NEG16(x) (-(x))
#define NEG32_ovflw(x) (-(x))
#define EXTRACT16(x) (x)
#define EXTEND32(x) (x)
#define SHR16(a,shift) (a)
#define SHL16(a,shift) (a)
#define SHR32(a,shift) (a)
#define SHL32(a,shift) (a)
#define PSHR32(a,shift) (a)
#define VSHR32(a,shift) (a)
#define SATURATE(x,a) (x)
#define ROUND16(a,shift) (a)
#define SROUND16(a,shift) (a)
#define HALF16(x) (.5f*(x))
#define HALF32(x) (.5f*(x))
#define ADD16(a,b) ((a)+(b))
#define SUB16(a,b) ((a)-(b))
#define ADD32(a,b) ((a)+(b))
#define SUB32(a,b) ((a)-(b))
#define ADD32_ovflw(a,b) ((a)+(b))
#define SUB32_ovflw(a,b) ((a)-(b))
#define MULT16_16(a,b) ((opus_val32)(a)*(opus_val32)(b))
#define MAC16_16(c,a,b) ((c)+(opus_val32)(a)*(opus_val32)(b))
#define MULT16_32_Q15(a,b) ((a)*(b))
#define MULT16_32_Q16(a,b) ((a)*(b))
#define MULT32_32_Q31(a,b) ((a)*(b))
#define MAC16_32_Q15(c,a,b) ((c)+(a)*(b))
#define MULT16_16_Q14(a,b) ((a)*(b))
#define MULT16_16_Q15(a,b) ((a)*(b))
#define MULT16_16_P15(a,b) ((a)*(b))
#define DIV32_16(a,b) (((opus_val32)(a))/(opus_val16)(b))
#define DIV32(a,b) (((opus_val32)(a))/(opus_val32)(b))
#define SCALEIN(a) ((a)*CELT_SIG_SCALE)
/* Copyright (c) 2010-2011 Xiph.Org Foundation, Skype Limited
   Written by Jean-Marc Valin and Koen Vos */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

- Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define OPUS_CUSTOM_EXPORT 
#define OPUS_CUSTOM_EXPORT_STATIC static OPUS_INLINE

typedef struct OpusCustomEncoder OpusCustomEncoder;
typedef struct OpusCustomMode OpusCustomMode;
OPUS_CUSTOM_EXPORT OPUS_WARN_UNUSED_RESULT OpusCustomMode *opus_custom_mode_create(opus_int32 Fs, int frame_size, int *error);
/* Encoder */
OPUS_CUSTOM_EXPORT_STATIC OPUS_WARN_UNUSED_RESULT int opus_custom_encoder_get_size(
    const OpusCustomMode *mode,
    int channels
) OPUS_ARG_NONNULL(1);
OPUS_CUSTOM_EXPORT int opus_custom_encoder_ctl(OpusCustomEncoder * OPUS_RESTRICT st, int request, ...) OPUS_ARG_NONNULL(1);
/*Some specific platforms may have optimized intrinsic or OPUS_INLINE assembly
   versions of these functions which can substantially improve performance.
  We define macros for them to allow easy incorporation of these non-ANSI
   features.*/

/*Modern gcc (4.x) can compile the naive versions of min and max with cmov if
   given an appropriate architecture, but the branchless bit-twiddling versions
   are just as fast, and do not require any special target architecture.
  Earlier gcc versions (3.x) compiled both code to the same assembly
   instructions, because of the way they represented ((_b)>(_a)) internally.*/
#define EC_MINI(_a,_b) ((_a)+(((_b)-(_a))&-((_b)<(_a))))
/*Count leading zeros.
  This macro should only be used for implementing ec_ilog(), if it is defined.
  All other code should use EC_ILOG() instead.*/
#define EC_CLZ0 ((int)sizeof(unsigned)*CHAR_BIT)
#define EC_CLZ(_x) (__builtin_clz(_x))
/*Note that __builtin_clz is not defined when _x==0, according to the gcc
   documentation (and that of the BSR instruction that implements it on x86).
  The majority of the time we can never pass it zero.
  When we need to, it can be special cased.*/
#define EC_ILOG(_x) (EC_CLZ0-EC_CLZ(_x))
/*OPT: ec_window must be at least 32 bits, but if you have fast arithmetic on a
   larger type, you can speed up the decoder by using it here.*/
typedef opus_uint32           ec_window;
typedef struct ec_ctx         ec_ctx;
typedef struct ec_ctx         ec_enc;
typedef struct ec_ctx         ec_dec;
#define EC_WINDOW_SIZE ((int)sizeof(ec_window)*CHAR_BIT)
/*The number of bits to use for the range-coded part of unsigned integers.*/
#define EC_UINT_BITS (8)
/*The resolution of fractional-precision bit usage measurements, i.e.,
   3 => 1/8th bits.*/
#define BITRES 3
/*The entropy encoder/decoder context.
  We use the same structure for both, so that common functions like ec_tell()
   can be used on either one.*/
struct ec_ctx{
   /*Buffered input/output.*/
   unsigned char *buf;
   /*The size of the buffer.*/
   opus_uint32    storage;
   /*The offset at which the last byte containing raw bits was read/written.*/
   opus_uint32    end_offs;
   /*Bits that will be read from/written at the end.*/
   ec_window      end_window;
   /*Number of valid bits in end_window.*/
   int            nend_bits;
   /*The total number of whole bits read/written.
     This does not include partial bits currently in the range coder.*/
   int            nbits_total;
   /*The offset at which the next range coder byte will be read/written.*/
   opus_uint32    offs;
   /*The number of values in the current range.*/
   opus_uint32    rng;
   /*In the decoder: the difference between the top of the current range and
      the input value, minus one.
     In the encoder: the low end of the current range.*/
   opus_uint32    val;
   /*In the decoder: the saved normalization factor from ec_decode().
     In the encoder: the number of oustanding carry propagating symbols.*/
   opus_uint32    ext;
   /*A buffered input/output symbol, awaiting carry propagation.*/
   int            rem;
   /*Nonzero if an error occurred.*/
   int            error;
};
static OPUS_INLINE opus_uint32 ec_range_bytes(ec_ctx *_this){
  return _this->offs;
}
static OPUS_INLINE unsigned char *ec_get_buffer(ec_ctx *_this){
  return _this->buf;
}
static OPUS_INLINE int ec_get_error(ec_ctx *_this){
  return _this->error;
}
/*Returns the number of bits "used" by the encoded or decoded symbols so far.
  This same number can be computed in either the encoder or the decoder, and is
   suitable for making coding decisions.
  Return: The number of bits.
          This will always be slightly larger than the exact value (e.g., all
           rounding error is in the positive direction).*/
static OPUS_INLINE int ec_tell(ec_ctx *_this){
  return _this->nbits_total-EC_ILOG(_this->rng);
}
/*Returns the number of bits "used" by the encoded or decoded symbols so far.
  This same number can be computed in either the encoder or the decoder, and is
   suitable for making coding decisions.
  Return: The number of bits scaled by 2**BITRES.
          This will always be slightly larger than the exact value (e.g., all
           rounding error is in the positive direction).*/
opus_uint32 ec_tell_frac(ec_ctx *_this);
/* Tested exhaustively for all n and for 1<=d<=256 */
static OPUS_INLINE opus_uint32 celt_udiv(opus_uint32 n, opus_uint32 d) {
   celt_sig_assert(d>0);
   return n/d;
}
static OPUS_INLINE opus_int32 celt_sudiv(opus_int32 n, opus_int32 d) {
   celt_sig_assert(d>0);
   return n/d;
}
/*Initializes the encoder.
  _buf:  The buffer to store output bytes in.
  _size: The size of the buffer, in chars.*/
void ec_enc_init(ec_enc *_this,unsigned char *_buf,opus_uint32 _size);
/*Encodes a symbol given its frequency information.
  The frequency information must be discernable by the decoder, assuming it
   has read only the previous symbols from the stream.
  It is allowable to change the frequency information, or even the entire
   source alphabet, so long as the decoder can tell from the context of the
   previously encoded information that it is supposed to do so as well.
  _fl: The cumulative frequency of all symbols that come before the one to be
        encoded.
  _fh: The cumulative frequency of all symbols up to and including the one to
        be encoded.
       Together with _fl, this defines the range [_fl,_fh) in which the
        decoded value will fall.
  _ft: The sum of the frequencies of all the symbols*/
void ec_encode(ec_enc *_this,unsigned _fl,unsigned _fh,unsigned _ft);
/*Equivalent to ec_encode() with _ft==1<<_bits.*/
void ec_encode_bin(ec_enc *_this,unsigned _fl,unsigned _fh,unsigned _bits);
/* Encode a bit that has a 1/(1<<_logp) probability of being a one */
void ec_enc_bit_logp(ec_enc *_this,int _val,unsigned _logp);
/*Encodes a symbol given an "inverse" CDF table.
  _s:    The index of the symbol to encode.
  _icdf: The "inverse" CDF, such that symbol _s falls in the range
          [_s>0?ft-_icdf[_s-1]:0,ft-_icdf[_s]), where ft=1<<_ftb.
         The values must be monotonically non-increasing, and the last value
          must be 0.
  _ftb: The number of bits of precision in the cumulative distribution.*/
void ec_enc_icdf(ec_enc *_this,int _s,const unsigned char *_icdf,unsigned _ftb);
/*Encodes a raw unsigned integer in the stream.
  _fl: The integer to encode.
  _ft: The number of integers that can be encoded (one more than the max).
       This must be at least 2, and no more than 2**32-1.*/
void ec_enc_uint(ec_enc *_this,opus_uint32 _fl,opus_uint32 _ft);
/*Encodes a sequence of raw bits in the stream.
  _fl:  The bits to encode.
  _ftb: The number of bits to encode.
        This must be between 1 and 25, inclusive.*/
void ec_enc_bits(ec_enc *_this,opus_uint32 _fl,unsigned _ftb);
/*Compacts the data to fit in the target size.
  This moves up the raw bits at the end of the current buffer so they are at
   the end of the new buffer size.
  The caller must ensure that the amount of data that's already been written
   will fit in the new size.
  _size: The number of bytes in the new buffer.
         This must be large enough to contain the bits already written, and
          must be no larger than the existing size.*/
void ec_enc_shrink(ec_enc *_this,opus_uint32 _size);
/*Indicates that there are no more symbols to encode.
  All reamining output bytes are flushed to the output buffer.
  ec_enc_init() must be called before the encoder can be used again.*/
void ec_enc_done(ec_enc *_this);
/*Calculates the cumulative frequency for the next symbol.
  This can then be fed into the probability model to determine what that
   symbol is, and the additional frequency information required to advance to
   the next symbol.
  This function cannot be called more than once without a corresponding call to
   ec_dec_update(), or decoding will not proceed correctly.
  _ft: The total frequency of the symbols in the alphabet the next symbol was
        encoded with.
  Return: A cumulative frequency representing the encoded symbol.
          If the cumulative frequency of all the symbols before the one that
           was encoded was fl, and the cumulative frequency of all the symbols
           up to and including the one encoded is fh, then the returned value
           will fall in the range [fl,fh).*/
unsigned ec_decode(ec_dec *_this,unsigned _ft);
/*Advance the decoder past the next symbol using the frequency information the
   symbol was encoded with.
  Exactly one call to ec_decode() must have been made so that all necessary
   intermediate calculations are performed.
  _fl:  The cumulative frequency of all symbols that come before the symbol
         decoded.
  _fh:  The cumulative frequency of all symbols up to and including the symbol
         decoded.
        Together with _fl, this defines the range [_fl,_fh) in which the value
         returned above must fall.
  _ft:  The total frequency of the symbols in the alphabet the symbol decoded
         was encoded in.
        This must be the same as passed to the preceding call to ec_decode().*/
void ec_dec_update(ec_dec *_this,unsigned _fl,unsigned _fh,unsigned _ft);
/* Decode a bit that has a 1/(1<<_logp) probability of being a one */
int ec_dec_bit_logp(ec_dec *_this,unsigned _logp);
/*Extracts a raw unsigned integer with a non-power-of-2 range from the stream.
  The bits must have been encoded with ec_enc_uint().
  No call to ec_dec_update() is necessary after this call.
  _ft: The number of integers that can be decoded (one more than the max).
       This must be at least 2, and no more than 2**32-1.
  Return: The decoded bits.*/
opus_uint32 ec_dec_uint(ec_dec *_this,opus_uint32 _ft);
/*Extracts a sequence of raw bits from the stream.
  The bits must have been encoded with ec_enc_bits().
  No call to ec_dec_update() is necessary after this call.
  _ftb: The number of bits to extract.
        This must be between 0 and 25, inclusive.
  Return: The decoded bits.*/
opus_uint32 ec_dec_bits(ec_dec *_this,unsigned _ftb);
#define CELTEncoder OpusCustomEncoder
#define CELTMode OpusCustomMode
#define LEAK_BANDS 19
typedef struct {
   int valid;
   float tonality;
   float tonality_slope;
   float noisiness;
   float activity;
   float music_prob;
   float music_prob_min;
   float music_prob_max;
   int   bandwidth;
   float activity_probability;
   float max_pitch_ratio;
   /* Store as Q6 char to save space. */
   unsigned char leak_boost[LEAK_BANDS];
} AnalysisInfo;
typedef struct {
   int signalType;
   int offset;
} SILKInfo;
#define __celt_check_mode_ptr_ptr(ptr) ((ptr) + ((ptr) - (const CELTMode**)(ptr)))
#define __celt_check_analysis_ptr(ptr) ((ptr) + ((ptr) - (const AnalysisInfo*)(ptr)))
#define __celt_check_silkinfo_ptr(ptr) ((ptr) + ((ptr) - (const SILKInfo*)(ptr)))
/* Encoder/decoder Requests */
#define CELT_SET_PREDICTION_REQUEST 10002
#define CELT_SET_PREDICTION(x) CELT_SET_PREDICTION_REQUEST, __opus_check_int(x)
#define CELT_SET_CHANNELS_REQUEST 10008
#define CELT_SET_CHANNELS(x) CELT_SET_CHANNELS_REQUEST, __opus_check_int(x)
/* Internal */
#define CELT_SET_START_BAND_REQUEST 10010
#define CELT_SET_START_BAND(x) CELT_SET_START_BAND_REQUEST, __opus_check_int(x)
#define CELT_SET_END_BAND_REQUEST 10012
#define CELT_SET_END_BAND(x) CELT_SET_END_BAND_REQUEST, __opus_check_int(x)
#define CELT_GET_MODE_REQUEST 10015
#define CELT_GET_MODE(x) CELT_GET_MODE_REQUEST, __celt_check_mode_ptr_ptr(x)
#define CELT_SET_SIGNALLING_REQUEST 10016
#define CELT_SET_SIGNALLING(x) CELT_SET_SIGNALLING_REQUEST, __opus_check_int(x)
#define CELT_SET_ANALYSIS_REQUEST 10022
#define CELT_SET_ANALYSIS(x) CELT_SET_ANALYSIS_REQUEST, __celt_check_analysis_ptr(x)
#define OPUS_SET_LFE_REQUEST 10024
#define OPUS_SET_LFE(x) OPUS_SET_LFE_REQUEST, __opus_check_int(x)
#define OPUS_SET_ENERGY_MASK_REQUEST 10026
#define OPUS_SET_ENERGY_MASK(x) OPUS_SET_ENERGY_MASK_REQUEST, __opus_check_val16_ptr(x)
#define CELT_SET_SILK_INFO_REQUEST 10028
#define CELT_SET_SILK_INFO(x) CELT_SET_SILK_INFO_REQUEST, __celt_check_silkinfo_ptr(x)
/* Encoder stuff */
int celt_encoder_get_size(int channels);
int celt_encode_with_ec(OpusCustomEncoder * OPUS_RESTRICT st, const opus_val16 * pcm, int frame_size, unsigned char *compressed, int nbCompressedBytes, ec_enc *enc);
int celt_encoder_init(CELTEncoder *st, opus_int32 sampling_rate, int channels,
                      int arch);
#define celt_encoder_ctl opus_custom_encoder_ctl
#define OPUS_CUSTOM_NOSTATIC static OPUS_INLINE
static const unsigned char trim_icdf[11] = {126, 124, 119, 109, 87, 41, 19, 9, 4, 2, 0};
/* Probs: NONE: 21.875%, LIGHT: 6.25%, NORMAL: 65.625%, AGGRESSIVE: 6.25% */
static const unsigned char spread_icdf[4] = {25, 23, 2, 0};
static const unsigned char tapset_icdf[3]={2,1,0};
#define COMBFILTER_MAXPERIOD 1024
#define COMBFILTER_MINPERIOD 15
extern const signed char tf_select_table[4][8];
int resampling_factor(opus_int32 rate);
void celt_preemphasis(const opus_val16 * OPUS_RESTRICT pcmp, celt_sig * OPUS_RESTRICT inp,
                        int N, int CC, int upsample, const opus_val16 *coef, celt_sig *mem, int clip);
void comb_filter(opus_val32 *y, opus_val32 *x, int T0, int T1, int N,
      opus_val16 g0, opus_val16 g1, int tapset0, int tapset1,
      const opus_val16 *window, int overlap, int arch);
void init_caps(const CELTMode *m,int *cap,int LM,int C);
static OPUS_INLINE int opus_select_arch(void)
{
  return 0;
}
/*  default is float */
#define kiss_fft_scalar float
#define kiss_twiddle_scalar float
#define KISS_TWIDDLE_CPX_ALIGNED 
typedef struct {
    kiss_fft_scalar r;
    kiss_fft_scalar i;
}kiss_fft_cpx;
typedef struct {
   kiss_twiddle_scalar r;
   kiss_twiddle_scalar i;
} KISS_TWIDDLE_CPX_ALIGNED kiss_twiddle_cpx;
#define MAXFACTORS 8
/* e.g. an fft of length 128 has 4 factors
 as far as kissfft is concerned
 4*4*4*2
 */
typedef struct arch_fft_state{
   int is_supported;
   void *priv;
} arch_fft_state;
typedef struct kiss_fft_state{
    int nfft;
    opus_val16 scale;
    int shift;
    opus_int16 factors[2*MAXFACTORS];
    const opus_int16 *bitrev;
    const kiss_twiddle_cpx *twiddles;
    arch_fft_state *arch_fft;
} kiss_fft_state;
void opus_fft_c(const kiss_fft_state *cfg,const kiss_fft_cpx *fin,kiss_fft_cpx *fout);
void opus_fft_impl(const kiss_fft_state *st,kiss_fft_cpx *fout);
#define opus_fft(_cfg,_fin,_fout,arch) ((void)(arch), opus_fft_c(_cfg, _fin, _fout))
typedef struct {
   int n;
   int maxshift;
   const kiss_fft_state *kfft[4];
   const kiss_twiddle_scalar * OPUS_RESTRICT trig;
} mdct_lookup;
void clt_mdct_forward_c(const mdct_lookup *l, kiss_fft_scalar *in,
                        kiss_fft_scalar * OPUS_RESTRICT out,
                        const opus_val16 *window, int overlap,
                        int shift, int stride, int arch);
/* Is run-time CPU detection enabled on this platform? */
#define clt_mdct_forward(_l,_in,_out,_window,_overlap,_shift,_stride,_arch) clt_mdct_forward_c(_l, _in, _out, _window, _overlap, _shift, _stride, _arch)
typedef struct {
   int size;
   const opus_int16 *index;
   const unsigned char *bits;
   const unsigned char *caps;
} PulseCache;
struct OpusCustomMode {
   opus_int32 Fs;
   int          overlap;

   int          nbEBands;
   int          effEBands;
   opus_val16    preemph[4];
   const opus_int16   *eBands;   
   int         maxLM;
   int         nbShortMdcts;
   int         shortMdctSize;

   int          nbAllocVectors;    const unsigned char   *allocVectors;      const opus_int16 *logN;

   const opus_val16 *window;
   mdct_lookup mdct;
   PulseCache cache;
};
#define LOG_MAX_PSEUDO 6
#define MAX_FINE_BITS 8
#define FINE_OFFSET 21
#define QTHETA_OFFSET 4
#define QTHETA_OFFSET_TWOPHASE 16

#define VARDECL(type,var) 
#define ALLOC(var,size,type) type var[size]
#define SAVE_STACK 
#define RESTORE_STACK 
#define ALLOC_STACK 
/* C99 does not allow VLAs of size zero */
#define ALLOC_NONE 1
void encode_pulses(const int *_y, int N, int K, ec_enc *enc);
opus_val32 decode_pulses(int *_y, int N, int K, ec_dec *dec);
static OPUS_INLINE int get_pulses(int i)
{
   return i<8 ? i : (8 + (i&7)) << ((i>>3)-1);
}
static OPUS_INLINE int bits2pulses(const CELTMode *m, int band, int LM, int bits)
{
   int i;
   int lo, hi;
   const unsigned char *cache;

   LM++;
   cache = m->cache.bits + m->cache.index[LM*m->nbEBands+band];

   lo = 0;
   hi = cache[0];
   bits--;
   for (i=0;i<LOG_MAX_PSEUDO;i++)
   {
      int mid = (lo+hi+1)>>1;
      /* OPT: Make sure this is implemented with a conditional move */
      if ((int)cache[mid] >= bits)
         hi = mid;
      else
         lo = mid;
   }
   if (bits- (lo == 0 ? -1 : (int)cache[lo]) <= (int)cache[hi]-bits)
      return lo;
   else
      return hi;
}
static OPUS_INLINE int pulses2bits(const CELTMode *m, int band, int LM, int pulses)
{
   const unsigned char *cache;

   LM++;
   cache = m->cache.bits + m->cache.index[LM*m->nbEBands+band];
   return pulses == 0 ? 0 : cache[pulses]+1;
}
int clt_compute_allocation(const CELTMode *m, int start, int end, const int *offsets, const int *cap, int alloc_trim, int *intensity, int *dual_stereo,
      opus_int32 total, opus_int32 *balance, int *pulses, int *ebits, int *fine_priority, int C, int LM, ec_ctx *ec, int encode, int prev, int signalBandwidth);
opus_int16 bitexact_cos(opus_int16 x);
int bitexact_log2tan(int isin,int icos);
void compute_band_energies(const CELTMode *m, const celt_sig *X, celt_ener *bandE, int end, int C, int LM, int arch);
/*void compute_noise_energies(const CELTMode *m, const celt_sig *X, const opus_val16 *tonality, celt_ener *bandE);*/

void normalise_bands(const CELTMode *m, const celt_sig * OPUS_RESTRICT freq, celt_norm * OPUS_RESTRICT X, const celt_ener *bandE, int end, int C, int M);
#define SPREAD_NONE (0)
#define SPREAD_LIGHT (1)
#define SPREAD_NORMAL (2)
#define SPREAD_AGGRESSIVE (3)
int spreading_decision(const CELTMode *m, const celt_norm *X, int *average,
      int last_decision, int *hf_average, int *tapset_decision, int update_hf,
      int end, int C, int M, const int *spread_weight);
void haar1(celt_norm *X, int N0, int stride);
void quant_all_bands(int encode, const CELTMode *m, int start, int end,
      celt_norm * X, celt_norm * Y, unsigned char *collapse_masks,
      const celt_ener *bandE, int *pulses, int shortBlocks, int spread,
      int dual_stereo, int intensity, int *tf_res, opus_int32 total_bits,
      opus_int32 balance, ec_ctx *ec, int M, int codedBands, opus_uint32 *seed,
      int complexity, int arch, int disable_inv);
opus_uint32 celt_lcg_rand(opus_uint32 seed);
int hysteresis_decision(opus_val16 val, const opus_val16 *thresholds, const opus_val16 *hysteresis, int N, int prev);
void exp_rotation(celt_norm *X, int len, int dir, int stride, int K, int spread);
opus_val16 op_pvq_search_c(celt_norm *X, int *iy, int K, int N, int arch);
#define op_pvq_search(x,iy,K,N,arch) (op_pvq_search_c(x, iy, K, N, arch))
unsigned alg_quant(celt_norm *X, int N, int K, int spread, int B, ec_enc *enc,
      opus_val16 gain, int resynth, int arch);
unsigned alg_unquant(celt_norm *X, int N, int K, int spread, int B,
      ec_dec *dec, opus_val16 gain);
void renormalise_vector(celt_norm *X, int N, opus_val16 gain, int arch);
int stereo_itheta(const celt_norm *X, const celt_norm *Y, int stereo, int N, int arch);
static OPUS_INLINE void *opus_alloc (size_t size)
{
   return malloc(size);
}
static OPUS_INLINE void opus_free (void *ptr)
{
   free(ptr);
}
#define OPUS_COPY(dst,src,n) (memcpy((dst), (src), (n)*sizeof(*(dst)) + 0*((dst)-(src)) ))
#define OPUS_MOVE(dst,src,n) (memmove((dst), (src), (n)*sizeof(*(dst)) + 0*((dst)-(src)) ))
#define OPUS_CLEAR(dst,n) (memset((dst), 0, (n)*sizeof(*(dst))))
/* Copyright (c) 2001-2011 Timothy B. Terriberry
   Copyright (c) 2008-2009 Xiph.Org Foundation */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

- Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#define PI 3.141592653f
/* Multiplies two 16-bit fractional values. Bit-exactness of this macro is important */
#define FRAC_MUL16(a,b) ((16384+((opus_int32)(opus_int16)(a)*(opus_int16)(b)))>>15)
unsigned isqrt32(opus_uint32 _val);
/* CELT doesn't need it for fixed-point, by analysis.c does. */
#define cA 0.43157974f
#define cB 0.67848403f
#define cC 0.08595542f
#define cE ((float)PI/2)
static OPUS_INLINE float fast_atan2f(float y, float x) {
   float x2, y2;
   x2 = x*x;
   y2 = y*y;
   /* For very small values, we don't care about the answer, so
      we can just return 0. */
   if (x2 + y2 < 1e-18f)
   {
      return 0;
   }
   if(x2<y2){
      float den = (y2 + cB*x2) * (y2 + cC*x2);
      return -x*y*(y2 + cA*x2) / den + (y<0 ? -cE : cE);
   }else{
      float den = (x2 + cB*y2) * (x2 + cC*y2);
      return  x*y*(x2 + cA*y2) / den + (y<0 ? -cE : cE) - (x*y<0 ? -cE : cE);
   }
}
#undef cA
#undef cB
#undef cC
#undef cE
static OPUS_INLINE opus_val32 celt_maxabs16(const opus_val16 *x, int len)
{
   int i;
   opus_val16 maxval = 0;
   opus_val16 minval = 0;
   for (i=0;i<len;i++)
   {
      maxval = MAX16(maxval, x[i]);
      minval = MIN16(minval, x[i]);
   }
   return MAX32(EXTEND32(maxval),-EXTEND32(minval));
}
#define celt_sqrt(x) ((float)sqrt(x))
#define celt_rsqrt(x) (1.f/celt_sqrt(x))
#define celt_rsqrt_norm(x) (celt_rsqrt(x))
#define celt_cos_norm(x) ((float)cos((.5f*PI)*(x)))
#define celt_rcp(x) (1.f/(x))
#define celt_div(a,b) ((a)/(b))
#define frac_div32(a,b) ((float)(a)/(b))
#define celt_log2(x) ((float)(1.442695040888963387*log(x)))
#define celt_exp2(x) ((float)exp(0.6931471805599453094*(x)))
extern const opus_val16 eMeans[25];
void amp2Log2(const CELTMode *m, int effEnd, int end,
      celt_ener *bandE, opus_val16 *bandLogE, int C);
void quant_coarse_energy(const CELTMode *m, int start, int end, int effEnd,
      const opus_val16 *eBands, opus_val16 *oldEBands, opus_uint32 budget,
      opus_val16 *error, ec_enc *enc, int C, int LM,
      int nbAvailableBytes, int force_intra, opus_val32 *delayedIntra,
      int two_pass, int loss_rate, int lfe);
void quant_fine_energy(const CELTMode *m, int start, int end, opus_val16 *oldEBands, opus_val16 *error, int *fine_quant, ec_enc *enc, int C);
void quant_energy_finalise(const CELTMode *m, int start, int end, opus_val16 *oldEBands, opus_val16 *error, int *fine_quant, int *fine_priority, int bits_left, ec_enc *enc, int C);
void pitch_downsample(celt_sig * OPUS_RESTRICT x[], opus_val16 * OPUS_RESTRICT x_lp,
      int len, int C, int arch);
void pitch_search(const opus_val16 * OPUS_RESTRICT x_lp, opus_val16 * OPUS_RESTRICT y,
                  int len, int max_pitch, int *pitch, int arch);
opus_val16 remove_doubling(opus_val16 *x, int maxperiod, int minperiod,
      int N, int *T0, int prev_period, opus_val16 prev_gain, int arch);
/* OPT: This is the kernel you really want to optimize. It gets used a lot
   by the prefilter and by the PLC. */
static OPUS_INLINE void xcorr_kernel_c(const opus_val16 * x, const opus_val16 * y, opus_val32 sum[4], int len)
{
   int j;
   opus_val16 y_0, y_1, y_2, y_3;
   celt_assert(len>=3);
   y_3=0; /* gcc doesn't realize that y_3 can't be used uninitialized */
   y_0=*y++;
   y_1=*y++;
   y_2=*y++;
   for (j=0;j<len-3;j+=4)
   {
      opus_val16 tmp;
      tmp = *x++;
      y_3=*y++;
      sum[0] = MAC16_16(sum[0],tmp,y_0);
      sum[1] = MAC16_16(sum[1],tmp,y_1);
      sum[2] = MAC16_16(sum[2],tmp,y_2);
      sum[3] = MAC16_16(sum[3],tmp,y_3);
      tmp=*x++;
      y_0=*y++;
      sum[0] = MAC16_16(sum[0],tmp,y_1);
      sum[1] = MAC16_16(sum[1],tmp,y_2);
      sum[2] = MAC16_16(sum[2],tmp,y_3);
      sum[3] = MAC16_16(sum[3],tmp,y_0);
      tmp=*x++;
      y_1=*y++;
      sum[0] = MAC16_16(sum[0],tmp,y_2);
      sum[1] = MAC16_16(sum[1],tmp,y_3);
      sum[2] = MAC16_16(sum[2],tmp,y_0);
      sum[3] = MAC16_16(sum[3],tmp,y_1);
      tmp=*x++;
      y_2=*y++;
      sum[0] = MAC16_16(sum[0],tmp,y_3);
      sum[1] = MAC16_16(sum[1],tmp,y_0);
      sum[2] = MAC16_16(sum[2],tmp,y_1);
      sum[3] = MAC16_16(sum[3],tmp,y_2);
   }
   if (j++<len)
   {
      opus_val16 tmp = *x++;
      y_3=*y++;
      sum[0] = MAC16_16(sum[0],tmp,y_0);
      sum[1] = MAC16_16(sum[1],tmp,y_1);
      sum[2] = MAC16_16(sum[2],tmp,y_2);
      sum[3] = MAC16_16(sum[3],tmp,y_3);
   }
   if (j++<len)
   {
      opus_val16 tmp=*x++;
      y_0=*y++;
      sum[0] = MAC16_16(sum[0],tmp,y_1);
      sum[1] = MAC16_16(sum[1],tmp,y_2);
      sum[2] = MAC16_16(sum[2],tmp,y_3);
      sum[3] = MAC16_16(sum[3],tmp,y_0);
   }
   if (j<len)
   {
      opus_val16 tmp=*x++;
      y_1=*y++;
      sum[0] = MAC16_16(sum[0],tmp,y_2);
      sum[1] = MAC16_16(sum[1],tmp,y_3);
      sum[2] = MAC16_16(sum[2],tmp,y_0);
      sum[3] = MAC16_16(sum[3],tmp,y_1);
   }
}
#define xcorr_kernel(x,y,sum,len,arch) ((void)(arch),xcorr_kernel_c(x, y, sum, len))
static OPUS_INLINE void dual_inner_prod_c(const opus_val16 *x, const opus_val16 *y01, const opus_val16 *y02,
      int N, opus_val32 *xy1, opus_val32 *xy2)
{
   int i;
   opus_val32 xy01=0;
   opus_val32 xy02=0;
   for (i=0;i<N;i++)
   {
      xy01 = MAC16_16(xy01, x[i], y01[i]);
      xy02 = MAC16_16(xy02, x[i], y02[i]);
   }
   *xy1 = xy01;
   *xy2 = xy02;
}
#define dual_inner_prod(x,y01,y02,N,xy1,xy2,arch) ((void)(arch),dual_inner_prod_c(x, y01, y02, N, xy1, xy2))
/*We make sure a C version is always available for cases where the overhead of
  vectorization and passing around an arch flag aren't worth it.*/
static OPUS_INLINE opus_val32 celt_inner_prod_c(const opus_val16 *x,
      const opus_val16 *y, int N)
{
   int i;
   opus_val32 xy=0;
   for (i=0;i<N;i++)
      xy = MAC16_16(xy, x[i], y[i]);
   return xy;
}
#define celt_inner_prod(x,y,N,arch) ((void)(arch),celt_inner_prod_c(x, y, N))
void
celt_pitch_xcorr_c(const opus_val16 *_x, const opus_val16 *_y,
      opus_val32 *xcorr, int len, int max_pitch, int arch);
#define celt_pitch_xcorr celt_pitch_xcorr_c
#define comb_filter_const(y,x,T,N,g10,g11,g12,arch) ((void)(arch),comb_filter_const_c(y, x, T, N, g10, g11, g12))
int hysteresis_decision(opus_val16 val, const opus_val16 *thresholds, const opus_val16 *hysteresis, int N, int prev)
{
   int i;
   for (i=0;i<N;i++)
   {
      if (val < thresholds[i])
         break;
   }
   if (i>prev && val < thresholds[prev]+hysteresis[prev])
      i=prev;
   if (i<prev && val > thresholds[prev-1]-hysteresis[prev-1])
      i=prev;
   return i;
}
opus_uint32 celt_lcg_rand(opus_uint32 seed)
{
   return 1664525 * seed + 1013904223;
}
/* This is a cos() approximation designed to be bit-exact on any platform. Bit exactness
   with this approximation is important because it has an impact on the bit allocation */
opus_int16 bitexact_cos(opus_int16 x)
{
   opus_int32 tmp;
   opus_int16 x2;
   tmp = (4096+((opus_int32)(x)*(x)))>>13;
   celt_sig_assert(tmp<=32767);
   x2 = tmp;
   x2 = (32767-x2) + FRAC_MUL16(x2, (-7651 + FRAC_MUL16(x2, (8277 + FRAC_MUL16(-626, x2)))));
   celt_sig_assert(x2<=32766);
   return 1+x2;
}
int bitexact_log2tan(int isin,int icos)
{
   int lc;
   int ls;
   lc=EC_ILOG(icos);
   ls=EC_ILOG(isin);
   icos<<=15-lc;
   isin<<=15-ls;
   return (ls-lc)*(1<<11)
         +FRAC_MUL16(isin, FRAC_MUL16(isin, -2597) + 7932)
         -FRAC_MUL16(icos, FRAC_MUL16(icos, -2597) + 7932);
}
/* Compute the amplitude (sqrt energy) in each of the bands */
void compute_band_energies(const CELTMode *m, const celt_sig *X, celt_ener *bandE, int end, int C, int LM, int arch)
{
   int i, c, N;
   const opus_int16 *eBands = m->eBands;
   N = m->shortMdctSize<<LM;
   c=0; do {
      for (i=0;i<end;i++)
      {
         opus_val32 sum;
         sum = 1e-27f + celt_inner_prod(&X[c*N+(eBands[i]<<LM)], &X[c*N+(eBands[i]<<LM)], (eBands[i+1]-eBands[i])<<LM, arch);
         bandE[i+c*m->nbEBands] = celt_sqrt(sum);
         /*printf ("%f ", bandE[i+c*m->nbEBands]);*/
      }
   } while (++c<C);
   /*printf ("\n");*/
}
/* Normalise each band such that the energy is one. */
void normalise_bands(const CELTMode *m, const celt_sig * OPUS_RESTRICT freq, celt_norm * OPUS_RESTRICT X, const celt_ener *bandE, int end, int C, int M)
{
   int i, c, N;
   const opus_int16 *eBands = m->eBands;
   N = M*m->shortMdctSize;
   c=0; do {
      for (i=0;i<end;i++)
      {
         int j;
         opus_val16 g = 1.f/(1e-27f+bandE[i+c*m->nbEBands]);
         for (j=M*eBands[i];j<M*eBands[i+1];j++)
            X[j+c*N] = freq[j+c*N]*g;
      }
   } while (++c<C);
}
/* Compute the weights to use for optimizing normalized distortion across
   channels. We use the amplitude to weight square distortion, which means
   that we use the square root of the value we would have been using if we
   wanted to minimize the MSE in the non-normalized domain. This roughly
   corresponds to some quick-and-dirty perceptual experiments I ran to
   measure inter-aural masking (there doesn't seem to be any published data
   on the topic). */
static void compute_channel_weights(celt_ener Ex, celt_ener Ey, opus_val16 w[2])
{
   celt_ener minE;
   minE = MIN32(Ex, Ey);
   /* Adjustment to make the weights a bit more conservative. */
   Ex = ADD32(Ex, minE/3);
   Ey = ADD32(Ey, minE/3);
   w[0] = VSHR32(Ex, shift);
   w[1] = VSHR32(Ey, shift);
}
static void intensity_stereo(const CELTMode *m, celt_norm * OPUS_RESTRICT X, const celt_norm * OPUS_RESTRICT Y, const celt_ener *bandE, int bandID, int N)
{
   int i = bandID;
   int j;
   opus_val16 a1, a2;
   opus_val16 left, right;
   opus_val16 norm;
   left = VSHR32(bandE[i],shift);
   right = VSHR32(bandE[i+m->nbEBands],shift);
   norm = EPSILON + celt_sqrt(EPSILON+MULT16_16(left,left)+MULT16_16(right,right));
   a1 = DIV32_16(SHL32(EXTEND32(left),14),norm);
   a2 = DIV32_16(SHL32(EXTEND32(right),14),norm);
   for (j=0;j<N;j++)
   {
      celt_norm r, l;
      l = X[j];
      r = Y[j];
      X[j] = EXTRACT16(SHR32(MAC16_16(MULT16_16(a1, l), a2, r), 14));
      /* Side is not encoded, no need to calculate */
   }
}
static void stereo_split(celt_norm * OPUS_RESTRICT X, celt_norm * OPUS_RESTRICT Y, int N)
{
   int j;
   for (j=0;j<N;j++)
   {
      opus_val32 r, l;
      l = MULT16_16(QCONST16(.70710678f, 15), X[j]);
      r = MULT16_16(QCONST16(.70710678f, 15), Y[j]);
      X[j] = EXTRACT16(SHR32(ADD32(l, r), 15));
      Y[j] = EXTRACT16(SHR32(SUB32(r, l), 15));
   }
}
static void stereo_merge(celt_norm * OPUS_RESTRICT X, celt_norm * OPUS_RESTRICT Y, opus_val16 mid, int N, int arch)
{
   int j;
   opus_val32 xp=0, side=0;
   opus_val32 El, Er;
   opus_val16 mid2;
   opus_val32 t, lgain, rgain;

   /* Compute the norm of X+Y and X-Y as |X|^2 + |Y|^2 +/- sum(xy) */
   dual_inner_prod(Y, X, Y, N, &xp, &side, arch);
   /* Compensating for the mid normalization */
   xp = MULT16_32_Q15(mid, xp);
   /* mid and side are in Q15, not Q14 like X and Y */
   mid2 = SHR16(mid, 1);
   El = MULT16_16(mid2, mid2) + side - 2*xp;
   Er = MULT16_16(mid2, mid2) + side + 2*xp;
   if (Er < QCONST32(6e-4f, 28) || El < QCONST32(6e-4f, 28))
   {
      OPUS_COPY(Y, X, N);
      return;
   }

   t = VSHR32(El, (kl-7)<<1);
   lgain = celt_rsqrt_norm(t);
   t = VSHR32(Er, (kr-7)<<1);
   rgain = celt_rsqrt_norm(t);

   for (j=0;j<N;j++)
   {
      celt_norm r, l;
      /* Apply mid scaling (side is already scaled) */
      l = MULT16_16_P15(mid, X[j]);
      r = Y[j];
      X[j] = EXTRACT16(PSHR32(MULT16_16(lgain, SUB16(l,r)), kl+1));
      Y[j] = EXTRACT16(PSHR32(MULT16_16(rgain, ADD16(l,r)), kr+1));
   }
}
/* Decide whether we should spread the pulses in the current frame */
int spreading_decision(const CELTMode *m, const celt_norm *X, int *average,
      int last_decision, int *hf_average, int *tapset_decision, int update_hf,
      int end, int C, int M, const int *spread_weight)
{
   int i, c, N0;
   int sum = 0, nbBands=0;
   const opus_int16 * OPUS_RESTRICT eBands = m->eBands;
   int decision;
   int hf_sum=0;

   celt_assert(end>0);

   N0 = M*m->shortMdctSize;

   if (M*(eBands[end]-eBands[end-1]) <= 8)
      return SPREAD_NONE;
   c=0; do {
      for (i=0;i<end;i++)
      {
         int j, N, tmp=0;
         int tcount[3] = {0,0,0};
         const celt_norm * OPUS_RESTRICT x = X+M*eBands[i]+c*N0;
         N = M*(eBands[i+1]-eBands[i]);
         if (N<=8)
            continue;
         /* Compute rough CDF of |x[j]| */
         for (j=0;j<N;j++)
         {
            opus_val32 x2N; /* Q13 */

            x2N = MULT16_16(MULT16_16_Q15(x[j], x[j]), N);
            if (x2N < QCONST16(0.25f,13))
               tcount[0]++;
            if (x2N < QCONST16(0.0625f,13))
               tcount[1]++;
            if (x2N < QCONST16(0.015625f,13))
               tcount[2]++;
         }

         /* Only include four last bands (8 kHz and up) */
         if (i>m->nbEBands-4)
            hf_sum += celt_udiv(32*(tcount[1]+tcount[0]), N);
         tmp = (2*tcount[2] >= N) + (2*tcount[1] >= N) + (2*tcount[0] >= N);
         sum += tmp*spread_weight[i];
         nbBands+=spread_weight[i];
      }
   } while (++c<C);

   if (update_hf)
   {
      if (hf_sum)
         hf_sum = celt_udiv(hf_sum, C*(4-m->nbEBands+end));
      *hf_average = (*hf_average+hf_sum)>>1;
      hf_sum = *hf_average;
      if (*tapset_decision==2)
         hf_sum += 4;
      else if (*tapset_decision==0)
         hf_sum -= 4;
      if (hf_sum > 22)
         *tapset_decision=2;
      else if (hf_sum > 18)
         *tapset_decision=1;
      else
         *tapset_decision=0;
   }
   /*printf("%d %d %d\n", hf_sum, *hf_average, *tapset_decision);*/
   celt_assert(nbBands>0); /* end has to be non-zero */
   celt_assert(sum>=0);
   sum = celt_udiv((opus_int32)sum<<8, nbBands);
   /* Recursive averaging */
   sum = (sum+*average)>>1;
   *average = sum;
   /* Hysteresis */
   sum = (3*sum + (((3-last_decision)<<7) + 64) + 2)>>2;
   if (sum < 80)
   {
      decision = SPREAD_AGGRESSIVE;
   } else if (sum < 256)
   {
      decision = SPREAD_NORMAL;
   } else if (sum < 384)
   {
      decision = SPREAD_LIGHT;
   } else {
      decision = SPREAD_NONE;
   }
   return decision;
}
/* Indexing table for converting from natural Hadamard to ordery Hadamard
   This is essentially a bit-reversed Gray, on top of which we've added
   an inversion of the order because we want the DC at the end rather than
   the beginning. The lines are for N=2, 4, 8, 16 */
static const int ordery_table[] = {
       1,  0,
       3,  0,  2,  1,
       7,  0,  4,  3,  6,  1,  5,  2,
      15,  0,  8,  7, 12,  3, 11,  4, 14,  1,  9,  6, 13,  2, 10,  5,
};
static void deinterleave_hadamard(celt_norm *X, int N0, int stride, int hadamard)
{
   int i,j;
   VARDECL(celt_norm, tmp);
   int N;
   SAVE_STACK;
   N = N0*stride;
   ALLOC(tmp, N, celt_norm);
   celt_assert(stride>0);
   if (hadamard)
   {
      const int *ordery = ordery_table+stride-2;
      for (i=0;i<stride;i++)
      {
         for (j=0;j<N0;j++)
            tmp[ordery[i]*N0+j] = X[j*stride+i];
      }
   } else {
      for (i=0;i<stride;i++)
         for (j=0;j<N0;j++)
            tmp[i*N0+j] = X[j*stride+i];
   }
   OPUS_COPY(X, tmp, N);
   RESTORE_STACK;
}
static void interleave_hadamard(celt_norm *X, int N0, int stride, int hadamard)
{
   int i,j;
   VARDECL(celt_norm, tmp);
   int N;
   SAVE_STACK;
   N = N0*stride;
   ALLOC(tmp, N, celt_norm);
   if (hadamard)
   {
      const int *ordery = ordery_table+stride-2;
      for (i=0;i<stride;i++)
         for (j=0;j<N0;j++)
            tmp[j*stride+i] = X[ordery[i]*N0+j];
   } else {
      for (i=0;i<stride;i++)
         for (j=0;j<N0;j++)
            tmp[j*stride+i] = X[i*N0+j];
   }
   OPUS_COPY(X, tmp, N);
   RESTORE_STACK;
}
void haar1(celt_norm *X, int N0, int stride)
{
   int i, j;
   N0 >>= 1;
   for (i=0;i<stride;i++)
      for (j=0;j<N0;j++)
      {
         opus_val32 tmp1, tmp2;
         tmp1 = MULT16_16(QCONST16(.70710678f,15), X[stride*2*j+i]);
         tmp2 = MULT16_16(QCONST16(.70710678f,15), X[stride*(2*j+1)+i]);
         X[stride*2*j+i] = EXTRACT16(PSHR32(ADD32(tmp1, tmp2), 15));
         X[stride*(2*j+1)+i] = EXTRACT16(PSHR32(SUB32(tmp1, tmp2), 15));
      }
}
static int compute_qn(int N, int b, int offset, int pulse_cap, int stereo)
{
   static const opus_int16 exp2_table8[8] =
      {16384, 17866, 19483, 21247, 23170, 25267, 27554, 30048};
   int qn, qb;
   int N2 = 2*N-1;
   if (stereo && N==2)
      N2--;
   /* The upper limit ensures that in a stereo split with itheta==16384, we'll
       always have enough bits left over to code at least one pulse in the
       side; otherwise it would collapse, since it doesn't get folded. */
   qb = celt_sudiv(b+N2*offset, N2);
   qb = IMIN(b-pulse_cap-(4<<BITRES), qb);

   qb = IMIN(8<<BITRES, qb);

   if (qb<(1<<BITRES>>1)) {
      qn = 1;
   } else {
      qn = exp2_table8[qb&0x7]>>(14-(qb>>BITRES));
      qn = (qn+1)>>1<<1;
   }
   celt_assert(qn <= 256);
   return qn;
}
struct band_ctx {
   int encode;
   int resynth;
   const CELTMode *m;
   int i;
   int intensity;
   int spread;
   int tf_change;
   ec_ctx *ec;
   opus_int32 remaining_bits;
   const celt_ener *bandE;
   opus_uint32 seed;
   int arch;
   int theta_round;
   int disable_inv;
   int avoid_split_noise;
};
struct split_ctx {
   int inv;
   int imid;
   int iside;
   int delta;
   int itheta;
   int qalloc;
};
static void compute_theta(struct band_ctx *ctx, struct split_ctx *sctx,
      celt_norm *X, celt_norm *Y, int N, int *b, int B, int B0,
      int LM,
      int stereo, int *fill)
{
   int qn;
   int itheta=0;
   int delta;
   int imid, iside;
   int qalloc;
   int pulse_cap;
   int offset;
   opus_int32 tell;
   int inv=0;
   int encode;
   const CELTMode *m;
   int i;
   int intensity;
   ec_ctx *ec;
   const celt_ener *bandE;

   encode = ctx->encode;
   m = ctx->m;
   i = ctx->i;
   intensity = ctx->intensity;
   ec = ctx->ec;
   bandE = ctx->bandE;

   /* Decide on the resolution to give to the split parameter theta */
   pulse_cap = m->logN[i]+LM*(1<<BITRES);
   offset = (pulse_cap>>1) - (stereo&&N==2 ? QTHETA_OFFSET_TWOPHASE : QTHETA_OFFSET);
   qn = compute_qn(N, *b, offset, pulse_cap, stereo);
   if (stereo && i>=intensity)
      qn = 1;
   if (encode)
   {
      /* theta is the atan() of the ratio between the (normalized)
         side and mid. With just that parameter, we can re-scale both
         mid and side because we know that 1) they have unit norm and
         2) they are orthogonal. */
      itheta = stereo_itheta(X, Y, stereo, N, ctx->arch);
   }
   tell = ec_tell_frac(ec);
   if (qn!=1)
   {
      if (encode)
      {
         if (!stereo || ctx->theta_round == 0)
         {
            itheta = (itheta*(opus_int32)qn+8192)>>14;
            if (!stereo && ctx->avoid_split_noise && itheta > 0 && itheta < qn)
            {
               /* Check if the selected value of theta will cause the bit allocation
                  to inject noise on one side. If so, make sure the energy of that side
                  is zero. */
               int unquantized = celt_udiv((opus_int32)itheta*16384, qn);
               imid = bitexact_cos((opus_int16)unquantized);
               iside = bitexact_cos((opus_int16)(16384-unquantized));
               delta = FRAC_MUL16((N-1)<<7,bitexact_log2tan(iside,imid));
               if (delta > *b)
                  itheta = qn;
               else if (delta < -*b)
                  itheta = 0;
            }
         } else {
            int down;
            /* Bias quantization towards itheta=0 and itheta=16384. */
            int bias = itheta > 8192 ? 32767/qn : -32767/qn;
            down = IMIN(qn-1, IMAX(0, (itheta*(opus_int32)qn + bias)>>14));
            if (ctx->theta_round < 0)
               itheta = down;
            else
               itheta = down+1;
         }
      }
      /* Entropy coding of the angle. We use a uniform pdf for the
         time split, a step for stereo, and a triangular one for the rest. */
      if (stereo && N>2)
      {
         int p0 = 3;
         int x = itheta;
         int x0 = qn/2;
         int ft = p0*(x0+1) + x0;
         /* Use a probability of p0 up to itheta=8192 and then use 1 after */
         if (encode)
         {
            ec_encode(ec,x<=x0?p0*x:(x-1-x0)+(x0+1)*p0,x<=x0?p0*(x+1):(x-x0)+(x0+1)*p0,ft);
         } else {
            int fs;
            fs=ec_decode(ec,ft);
            if (fs<(x0+1)*p0)
               x=fs/p0;
            else
               x=x0+1+(fs-(x0+1)*p0);
            ec_dec_update(ec,x<=x0?p0*x:(x-1-x0)+(x0+1)*p0,x<=x0?p0*(x+1):(x-x0)+(x0+1)*p0,ft);
            itheta = x;
         }
      } else if (B0>1 || stereo) {
         /* Uniform pdf */
         if (encode)
            ec_enc_uint(ec, itheta, qn+1);
         else
            itheta = ec_dec_uint(ec, qn+1);
      } else {
         int fs=1, ft;
         ft = ((qn>>1)+1)*((qn>>1)+1);
         if (encode)
         {
            int fl;

            fs = itheta <= (qn>>1) ? itheta + 1 : qn + 1 - itheta;
            fl = itheta <= (qn>>1) ? itheta*(itheta + 1)>>1 :
             ft - ((qn + 1 - itheta)*(qn + 2 - itheta)>>1);

            ec_encode(ec, fl, fl+fs, ft);
         } else {
            /* Triangular pdf */
            int fl=0;
            int fm;
            fm = ec_decode(ec, ft);

            if (fm < ((qn>>1)*((qn>>1) + 1)>>1))
            {
               itheta = (isqrt32(8*(opus_uint32)fm + 1) - 1)>>1;
               fs = itheta + 1;
               fl = itheta*(itheta + 1)>>1;
            }
            else
            {
               itheta = (2*(qn + 1)
                - isqrt32(8*(opus_uint32)(ft - fm - 1) + 1))>>1;
               fs = qn + 1 - itheta;
               fl = ft - ((qn + 1 - itheta)*(qn + 2 - itheta)>>1);
            }

            ec_dec_update(ec, fl, fl+fs, ft);
         }
      }
      celt_assert(itheta>=0);
      itheta = celt_udiv((opus_int32)itheta*16384, qn);
      if (encode && stereo)
      {
         if (itheta==0)
            intensity_stereo(m, X, Y, bandE, i, N);
         else
            stereo_split(X, Y, N);
      }
      /* NOTE: Renormalising X and Y *may* help fixed-point a bit at very high rate.
               Let's do that at higher complexity */
   } else if (stereo) {
      if (encode)
      {
         inv = itheta > 8192 && !ctx->disable_inv;
         if (inv)
         {
            int j;
            for (j=0;j<N;j++)
               Y[j] = -Y[j];
         }
         intensity_stereo(m, X, Y, bandE, i, N);
      }
      if (*b>2<<BITRES && ctx->remaining_bits > 2<<BITRES)
      {
         if (encode)
            ec_enc_bit_logp(ec, inv, 2);
         else
            inv = ec_dec_bit_logp(ec, 2);
      } else
         inv = 0;
      /* inv flag override to avoid problems with downmixing. */
      if (ctx->disable_inv)
         inv = 0;
      itheta = 0;
   }
   qalloc = ec_tell_frac(ec) - tell;
   *b -= qalloc;

   if (itheta == 0)
   {
      imid = 32767;
      iside = 0;
      *fill &= (1<<B)-1;
      delta = -16384;
   } else if (itheta == 16384)
   {
      imid = 0;
      iside = 32767;
      *fill &= ((1<<B)-1)<<B;
      delta = 16384;
   } else {
      imid = bitexact_cos((opus_int16)itheta);
      iside = bitexact_cos((opus_int16)(16384-itheta));
      /* This is the mid vs side allocation that minimizes squared error
         in that band. */
      delta = FRAC_MUL16((N-1)<<7,bitexact_log2tan(iside,imid));
   }

   sctx->inv = inv;
   sctx->imid = imid;
   sctx->iside = iside;
   sctx->delta = delta;
   sctx->itheta = itheta;
   sctx->qalloc = qalloc;
}
static unsigned quant_band_n1(struct band_ctx *ctx, celt_norm *X, celt_norm *Y,
      celt_norm *lowband_out)
{
   int c;
   int stereo;
   celt_norm *x = X;
   int encode;
   ec_ctx *ec;

   encode = ctx->encode;
   ec = ctx->ec;

   stereo = Y != NULL;
   c=0; do {
      int sign=0;
      if (ctx->remaining_bits>=1<<BITRES)
      {
         if (encode)
         {
            sign = x[0]<0;
            ec_enc_bits(ec, sign, 1);
         } else {
            sign = ec_dec_bits(ec, 1);
         }
         ctx->remaining_bits -= 1<<BITRES;
      }
      if (ctx->resynth)
         x[0] = sign ? -NORM_SCALING : NORM_SCALING;
      x = Y;
   } while (++c<1+stereo);
   if (lowband_out)
      lowband_out[0] = SHR16(X[0],4);
   return 1;
}
/* This function is responsible for encoding and decoding a mono partition.
   It can split the band in two and transmit the energy difference with
   the two half-bands. It can be called recursively so bands can end up being
   split in 8 parts. */
static unsigned quant_partition(struct band_ctx *ctx, celt_norm *X,
      int N, int b, int B, celt_norm *lowband,
      int LM,
      opus_val16 gain, int fill)
{
   const unsigned char *cache;
   int q;
   int curr_bits;
   int imid=0, iside=0;
   int B0=B;
   opus_val16 mid=0, side=0;
   unsigned cm=0;
   celt_norm *Y=NULL;
   int encode;
   const CELTMode *m;
   int i;
   int spread;
   ec_ctx *ec;

   encode = ctx->encode;
   m = ctx->m;
   i = ctx->i;
   spread = ctx->spread;
   ec = ctx->ec;

   /* If we need 1.5 more bit than we can produce, split the band in two. */
   cache = m->cache.bits + m->cache.index[(LM+1)*m->nbEBands+i];
   if (LM != -1 && b > cache[cache[0]]+12 && N>2)
   {
      int mbits, sbits, delta;
      int itheta;
      int qalloc;
      struct split_ctx sctx;
      celt_norm *next_lowband2=NULL;
      opus_int32 rebalance;

      N >>= 1;
      Y = X+N;
      LM -= 1;
      if (B==1)
         fill = (fill&1)|(fill<<1);
      B = (B+1)>>1;

      compute_theta(ctx, &sctx, X, Y, N, &b, B, B0, LM, 0, &fill);
      imid = sctx.imid;
      iside = sctx.iside;
      delta = sctx.delta;
      itheta = sctx.itheta;
      qalloc = sctx.qalloc;
      mid = (1.f/32768)*imid;
      side = (1.f/32768)*iside;

      /* Give more bits to low-energy MDCTs than they would otherwise deserve */
      if (B0>1 && (itheta&0x3fff))
      {
         if (itheta > 8192)
            /* Rough approximation for pre-echo masking */
            delta -= delta>>(4-LM);
         else
            /* Corresponds to a forward-masking slope of 1.5 dB per 10 ms */
            delta = IMIN(0, delta + (N<<BITRES>>(5-LM)));
      }
      mbits = IMAX(0, IMIN(b, (b-delta)/2));
      sbits = b-mbits;
      ctx->remaining_bits -= qalloc;

      if (lowband)
         next_lowband2 = lowband+N; /* >32-bit split case */

      rebalance = ctx->remaining_bits;
      if (mbits >= sbits)
      {
         cm = quant_partition(ctx, X, N, mbits, B, lowband, LM,
               MULT16_16_P15(gain,mid), fill);
         rebalance = mbits - (rebalance-ctx->remaining_bits);
         if (rebalance > 3<<BITRES && itheta!=0)
            sbits += rebalance - (3<<BITRES);
         cm |= quant_partition(ctx, Y, N, sbits, B, next_lowband2, LM,
               MULT16_16_P15(gain,side), fill>>B)<<(B0>>1);
      } else {
         cm = quant_partition(ctx, Y, N, sbits, B, next_lowband2, LM,
               MULT16_16_P15(gain,side), fill>>B)<<(B0>>1);
         rebalance = sbits - (rebalance-ctx->remaining_bits);
         if (rebalance > 3<<BITRES && itheta!=16384)
            mbits += rebalance - (3<<BITRES);
         cm |= quant_partition(ctx, X, N, mbits, B, lowband, LM,
               MULT16_16_P15(gain,mid), fill);
      }
   } else {
      /* This is the basic no-split case */
      q = bits2pulses(m, i, LM, b);
      curr_bits = pulses2bits(m, i, LM, q);
      ctx->remaining_bits -= curr_bits;

      /* Ensures we can never bust the budget */
      while (ctx->remaining_bits < 0 && q > 0)
      {
         ctx->remaining_bits += curr_bits;
         q--;
         curr_bits = pulses2bits(m, i, LM, q);
         ctx->remaining_bits -= curr_bits;
      }

      if (q!=0)
      {
         int K = get_pulses(q);

         /* Finally do the actual quantization */
         if (encode)
         {
            cm = alg_quant(X, N, K, spread, B, ec, gain, ctx->resynth, ctx->arch);
         } else {
            cm = alg_unquant(X, N, K, spread, B, ec, gain);
         }
      } else {
         /* If there's no pulse, fill the band anyway */
         int j;
         if (ctx->resynth)
         {
            unsigned cm_mask;
            /* B can be as large as 16, so this shift might overflow an int on a
               16-bit platform; use a long to get defined behavior.*/
            cm_mask = (unsigned)(1UL<<B)-1;
            fill &= cm_mask;
            if (!fill)
            {
               OPUS_CLEAR(X, N);
            } else {
               if (lowband == NULL)
               {
                  /* Noise */
                  for (j=0;j<N;j++)
                  {
                     ctx->seed = celt_lcg_rand(ctx->seed);
                     X[j] = (celt_norm)((opus_int32)ctx->seed>>20);
                  }
                  cm = cm_mask;
               } else {
                  /* Folded spectrum */
                  for (j=0;j<N;j++)
                  {
                     opus_val16 tmp;
                     ctx->seed = celt_lcg_rand(ctx->seed);
                     /* About 48 dB below the "normal" folding level */
                     tmp = QCONST16(1.0f/256, 10);
                     tmp = (ctx->seed)&0x8000 ? tmp : -tmp;
                     X[j] = lowband[j]+tmp;
                  }
                  cm = fill;
               }
               renormalise_vector(X, N, gain, ctx->arch);
            }
         }
      }
   }

   return cm;
}
/* This function is responsible for encoding and decoding a band for the mono case. */
static unsigned quant_band(struct band_ctx *ctx, celt_norm *X,
      int N, int b, int B, celt_norm *lowband,
      int LM, celt_norm *lowband_out,
      opus_val16 gain, celt_norm *lowband_scratch, int fill)
{
   int N0=N;
   int N_B=N;
   int N_B0;
   int B0=B;
   int time_divide=0;
   int recombine=0;
   int longBlocks;
   unsigned cm=0;
   int k;
   int encode;
   int tf_change;

   encode = ctx->encode;
   tf_change = ctx->tf_change;

   longBlocks = B0==1;

   N_B = celt_udiv(N_B, B);

   /* Special case for one sample */
   if (N==1)
   {
      return quant_band_n1(ctx, X, NULL, lowband_out);
   }

   if (tf_change>0)
      recombine = tf_change;
   /* Band recombining to increase frequency resolution */

   if (lowband_scratch && lowband && (recombine || ((N_B&1) == 0 && tf_change<0) || B0>1))
   {
      OPUS_COPY(lowband_scratch, lowband, N);
      lowband = lowband_scratch;
   }

   for (k=0;k<recombine;k++)
   {
      static const unsigned char bit_interleave_table[16]={
            0,1,1,1,2,3,3,3,2,3,3,3,2,3,3,3
      };
      if (encode)
         haar1(X, N>>k, 1<<k);
      if (lowband)
         haar1(lowband, N>>k, 1<<k);
      fill = bit_interleave_table[fill&0xF]|bit_interleave_table[fill>>4]<<2;
   }
   B>>=recombine;
   N_B<<=recombine;

   /* Increasing the time resolution */
   while ((N_B&1) == 0 && tf_change<0)
   {
      if (encode)
         haar1(X, N_B, B);
      if (lowband)
         haar1(lowband, N_B, B);
      fill |= fill<<B;
      B <<= 1;
      N_B >>= 1;
      time_divide++;
      tf_change++;
   }
   B0=B;
   N_B0 = N_B;

   /* Reorganize the samples in time order instead of frequency order */
   if (B0>1)
   {
      if (encode)
         deinterleave_hadamard(X, N_B>>recombine, B0<<recombine, longBlocks);
      if (lowband)
         deinterleave_hadamard(lowband, N_B>>recombine, B0<<recombine, longBlocks);
   }

   cm = quant_partition(ctx, X, N, b, B, lowband, LM, gain, fill);

   /* This code is used by the decoder and by the resynthesis-enabled encoder */
   if (ctx->resynth)
   {
      /* Undo the sample reorganization going from time order to frequency order */
      if (B0>1)
         interleave_hadamard(X, N_B>>recombine, B0<<recombine, longBlocks);

      /* Undo time-freq changes that we did earlier */
      N_B = N_B0;
      B = B0;
      for (k=0;k<time_divide;k++)
      {
         B >>= 1;
         N_B <<= 1;
         cm |= cm>>B;
         haar1(X, N_B, B);
      }

      for (k=0;k<recombine;k++)
      {
         static const unsigned char bit_deinterleave_table[16]={
               0x00,0x03,0x0C,0x0F,0x30,0x33,0x3C,0x3F,
               0xC0,0xC3,0xCC,0xCF,0xF0,0xF3,0xFC,0xFF
         };
         cm = bit_deinterleave_table[cm];
         haar1(X, N0>>k, 1<<k);
      }
      B<<=recombine;

      /* Scale output for later folding */
      if (lowband_out)
      {
         int j;
         opus_val16 n;
         n = celt_sqrt(SHL32(EXTEND32(N0),22));
         for (j=0;j<N0;j++)
            lowband_out[j] = MULT16_16_Q15(n,X[j]);
      }
      cm &= (1<<B)-1;
   }
   return cm;
}
/* This function is responsible for encoding and decoding a band for the stereo case. */
static unsigned quant_band_stereo(struct band_ctx *ctx, celt_norm *X, celt_norm *Y,
      int N, int b, int B, celt_norm *lowband,
      int LM, celt_norm *lowband_out,
      celt_norm *lowband_scratch, int fill)
{
   int imid=0, iside=0;
   int inv = 0;
   opus_val16 mid=0, side=0;
   unsigned cm=0;
   int mbits, sbits, delta;
   int itheta;
   int qalloc;
   struct split_ctx sctx;
   int orig_fill;
   int encode;
   ec_ctx *ec;

   encode = ctx->encode;
   ec = ctx->ec;

   /* Special case for one sample */
   if (N==1)
   {
      return quant_band_n1(ctx, X, Y, lowband_out);
   }

   orig_fill = fill;

   compute_theta(ctx, &sctx, X, Y, N, &b, B, B, LM, 1, &fill);
   inv = sctx.inv;
   imid = sctx.imid;
   iside = sctx.iside;
   delta = sctx.delta;
   itheta = sctx.itheta;
   qalloc = sctx.qalloc;
   mid = (1.f/32768)*imid;
   side = (1.f/32768)*iside;

   /* This is a special case for N=2 that only works for stereo and takes
      advantage of the fact that mid and side are orthogonal to encode
      the side with just one bit. */
   if (N==2)
   {
      int c;
      int sign=0;
      celt_norm *x2, *y2;
      mbits = b;
      sbits = 0;
      /* Only need one bit for the side. */
      if (itheta != 0 && itheta != 16384)
         sbits = 1<<BITRES;
      mbits -= sbits;
      c = itheta > 8192;
      ctx->remaining_bits -= qalloc+sbits;

      x2 = c ? Y : X;
      y2 = c ? X : Y;
      if (sbits)
      {
         if (encode)
         {
            /* Here we only need to encode a sign for the side. */
            sign = x2[0]*y2[1] - x2[1]*y2[0] < 0;
            ec_enc_bits(ec, sign, 1);
         } else {
            sign = ec_dec_bits(ec, 1);
         }
      }
      sign = 1-2*sign;
      /* We use orig_fill here because we want to fold the side, but if
         itheta==16384, we'll have cleared the low bits of fill. */
      cm = quant_band(ctx, x2, N, mbits, B, lowband, LM, lowband_out, Q15ONE,
            lowband_scratch, orig_fill);
      /* We don't split N=2 bands, so cm is either 1 or 0 (for a fold-collapse),
         and there's no need to worry about mixing with the other channel. */
      y2[0] = -sign*x2[1];
      y2[1] = sign*x2[0];
      if (ctx->resynth)
      {
         celt_norm tmp;
         X[0] = MULT16_16_Q15(mid, X[0]);
         X[1] = MULT16_16_Q15(mid, X[1]);
         Y[0] = MULT16_16_Q15(side, Y[0]);
         Y[1] = MULT16_16_Q15(side, Y[1]);
         tmp = X[0];
         X[0] = SUB16(tmp,Y[0]);
         Y[0] = ADD16(tmp,Y[0]);
         tmp = X[1];
         X[1] = SUB16(tmp,Y[1]);
         Y[1] = ADD16(tmp,Y[1]);
      }
   } else {
      /* "Normal" split code */
      opus_int32 rebalance;

      mbits = IMAX(0, IMIN(b, (b-delta)/2));
      sbits = b-mbits;
      ctx->remaining_bits -= qalloc;

      rebalance = ctx->remaining_bits;
      if (mbits >= sbits)
      {
         /* In stereo mode, we do not apply a scaling to the mid because we need the normalized
            mid for folding later. */
         cm = quant_band(ctx, X, N, mbits, B, lowband, LM, lowband_out, Q15ONE,
               lowband_scratch, fill);
         rebalance = mbits - (rebalance-ctx->remaining_bits);
         if (rebalance > 3<<BITRES && itheta!=0)
            sbits += rebalance - (3<<BITRES);

         /* For a stereo split, the high bits of fill are always zero, so no
            folding will be done to the side. */
         cm |= quant_band(ctx, Y, N, sbits, B, NULL, LM, NULL, side, NULL, fill>>B);
      } else {
         /* For a stereo split, the high bits of fill are always zero, so no
            folding will be done to the side. */
         cm = quant_band(ctx, Y, N, sbits, B, NULL, LM, NULL, side, NULL, fill>>B);
         rebalance = sbits - (rebalance-ctx->remaining_bits);
         if (rebalance > 3<<BITRES && itheta!=16384)
            mbits += rebalance - (3<<BITRES);
         /* In stereo mode, we do not apply a scaling to the mid because we need the normalized
            mid for folding later. */
         cm |= quant_band(ctx, X, N, mbits, B, lowband, LM, lowband_out, Q15ONE,
               lowband_scratch, fill);
      }
   }

   /* This code is used by the decoder and by the resynthesis-enabled encoder */
   if (ctx->resynth)
   {
      if (N!=2)
         stereo_merge(X, Y, mid, N, ctx->arch);
      if (inv)
      {
         int j;
         for (j=0;j<N;j++)
            Y[j] = -Y[j];
      }
   }
   return cm;
}
static void special_hybrid_folding(const CELTMode *m, celt_norm *norm, celt_norm *norm2, int start, int M, int dual_stereo)
{
   int n1, n2;
   const opus_int16 * OPUS_RESTRICT eBands = m->eBands;
   n1 = M*(eBands[start+1]-eBands[start]);
   n2 = M*(eBands[start+2]-eBands[start+1]);
   /* Duplicate enough of the first band folding data to be able to fold the second band.
      Copies no data for CELT-only mode. */
   OPUS_COPY(&norm[n1], &norm[2*n1 - n2], n2-n1);
   if (dual_stereo)
      OPUS_COPY(&norm2[n1], &norm2[2*n1 - n2], n2-n1);
}
void quant_all_bands(int encode, const CELTMode *m, int start, int end,
      celt_norm *X_, celt_norm *Y_, unsigned char *collapse_masks,
      const celt_ener *bandE, int *pulses, int shortBlocks, int spread,
      int dual_stereo, int intensity, int *tf_res, opus_int32 total_bits,
      opus_int32 balance, ec_ctx *ec, int LM, int codedBands,
      opus_uint32 *seed, int complexity, int arch, int disable_inv)
{
   int i;
   opus_int32 remaining_bits;
   const opus_int16 * OPUS_RESTRICT eBands = m->eBands;
   celt_norm * OPUS_RESTRICT norm, * OPUS_RESTRICT norm2;
   VARDECL(celt_norm, _norm);
   VARDECL(celt_norm, _lowband_scratch);
   VARDECL(celt_norm, X_save);
   VARDECL(celt_norm, Y_save);
   VARDECL(celt_norm, X_save2);
   VARDECL(celt_norm, Y_save2);
   VARDECL(celt_norm, norm_save2);
   int resynth_alloc;
   celt_norm *lowband_scratch;
   int B;
   int M;
   int lowband_offset;
   int update_lowband = 1;
   int C = Y_ != NULL ? 2 : 1;
   int norm_offset;
   int theta_rdo = encode && Y_!=NULL && !dual_stereo && complexity>=8;
   int resynth = !encode || theta_rdo;
   struct band_ctx ctx;
   SAVE_STACK;

   M = 1<<LM;
   B = shortBlocks ? M : 1;
   norm_offset = M*eBands[start];
   /* No need to allocate norm for the last band because we don't need an
      output in that band. */
   ALLOC(_norm, C*(M*eBands[m->nbEBands-1]-norm_offset), celt_norm);
   norm = _norm;
   norm2 = norm + M*eBands[m->nbEBands-1]-norm_offset;

   /* For decoding, we can use the last band as scratch space because we don't need that
      scratch space for the last band and we don't care about the data there until we're
      decoding the last band. */
   if (encode && resynth)
      resynth_alloc = M*(eBands[m->nbEBands]-eBands[m->nbEBands-1]);
   else
      resynth_alloc = ALLOC_NONE;
   ALLOC(_lowband_scratch, resynth_alloc, celt_norm);
   if (encode && resynth)
      lowband_scratch = _lowband_scratch;
   else
      lowband_scratch = X_+M*eBands[m->effEBands-1];
   ALLOC(X_save, resynth_alloc, celt_norm);
   ALLOC(Y_save, resynth_alloc, celt_norm);
   ALLOC(X_save2, resynth_alloc, celt_norm);
   ALLOC(Y_save2, resynth_alloc, celt_norm);
   ALLOC(norm_save2, resynth_alloc, celt_norm);

   lowband_offset = 0;
   ctx.bandE = bandE;
   ctx.ec = ec;
   ctx.encode = encode;
   ctx.intensity = intensity;
   ctx.m = m;
   ctx.seed = *seed;
   ctx.spread = spread;
   ctx.arch = arch;
   ctx.disable_inv = disable_inv;
   ctx.resynth = resynth;
   ctx.theta_round = 0;
   /* Avoid injecting noise in the first band on transients. */
   ctx.avoid_split_noise = B > 1;
   for (i=start;i<end;i++)
   {
      opus_int32 tell;
      int b;
      int N;
      opus_int32 curr_balance;
      int effective_lowband=-1;
      celt_norm * OPUS_RESTRICT X, * OPUS_RESTRICT Y;
      int tf_change=0;
      unsigned x_cm;
      unsigned y_cm;
      int last;

      ctx.i = i;
      last = (i==end-1);

      X = X_+M*eBands[i];
      if (Y_!=NULL)
         Y = Y_+M*eBands[i];
      else
         Y = NULL;
      N = M*eBands[i+1]-M*eBands[i];
      celt_assert(N > 0);
      tell = ec_tell_frac(ec);

      /* Compute how many bits we want to allocate to this band */
      if (i != start)
         balance -= tell;
      remaining_bits = total_bits-tell-1;
      ctx.remaining_bits = remaining_bits;
      if (i <= codedBands-1)
      {
         curr_balance = celt_sudiv(balance, IMIN(3, codedBands-i));
         b = IMAX(0, IMIN(16383, IMIN(remaining_bits+1,pulses[i]+curr_balance)));
      } else {
         b = 0;
      }

      if (resynth && (M*eBands[i]-N >= M*eBands[start] || i==start+1) && (update_lowband || lowband_offset==0))
            lowband_offset = i;
      if (i == start+1)
         special_hybrid_folding(m, norm, norm2, start, M, dual_stereo);

      tf_change = tf_res[i];
      ctx.tf_change = tf_change;
      if (i>=m->effEBands)
      {
         X=norm;
         if (Y_!=NULL)
            Y = norm;
         lowband_scratch = NULL;
      }
      if (last && !theta_rdo)
         lowband_scratch = NULL;

      /* Get a conservative estimate of the collapse_mask's for the bands we're
         going to be folding from. */
      if (lowband_offset != 0 && (spread!=SPREAD_AGGRESSIVE || B>1 || tf_change<0))
      {
         int fold_start;
         int fold_end;
         int fold_i;
         /* This ensures we never repeat spectral content within one band */
         effective_lowband = IMAX(0, M*eBands[lowband_offset]-norm_offset-N);
         fold_start = lowband_offset;
         while(M*eBands[--fold_start] > effective_lowband+norm_offset);
         fold_end = lowband_offset-1;
         while(++fold_end < i && M*eBands[fold_end] < effective_lowband+norm_offset+N);
         x_cm = y_cm = 0;
         fold_i = fold_start; do {
           x_cm |= collapse_masks[fold_i*C+0];
           y_cm |= collapse_masks[fold_i*C+C-1];
         } while (++fold_i<fold_end);
      }
      /* Otherwise, we'll be using the LCG to fold, so all blocks will (almost
         always) be non-zero. */
      else
         x_cm = y_cm = (1<<B)-1;

      if (dual_stereo && i==intensity)
      {
         int j;

         /* Switch off dual stereo to do intensity. */
         dual_stereo = 0;
         if (resynth)
            for (j=0;j<M*eBands[i]-norm_offset;j++)
               norm[j] = HALF32(norm[j]+norm2[j]);
      }
      if (dual_stereo)
      {
         x_cm = quant_band(&ctx, X, N, b/2, B,
               effective_lowband != -1 ? norm+effective_lowband : NULL, LM,
               last?NULL:norm+M*eBands[i]-norm_offset, Q15ONE, lowband_scratch, x_cm);
         y_cm = quant_band(&ctx, Y, N, b/2, B,
               effective_lowband != -1 ? norm2+effective_lowband : NULL, LM,
               last?NULL:norm2+M*eBands[i]-norm_offset, Q15ONE, lowband_scratch, y_cm);
      } else {
         if (Y!=NULL)
         {
            if (theta_rdo && i < intensity)
            {
               ec_ctx ec_save, ec_save2;
               struct band_ctx ctx_save, ctx_save2;
               opus_val32 dist0, dist1;
               unsigned cm, cm2;
               int nstart_bytes, nend_bytes, save_bytes;
               unsigned char *bytes_buf;
               unsigned char bytes_save[1275];
               opus_val16 w[2];
               compute_channel_weights(bandE[i], bandE[i+m->nbEBands], w);
               /* Make a copy. */
               cm = x_cm|y_cm;
               ec_save = *ec;
               ctx_save = ctx;
               OPUS_COPY(X_save, X, N);
               OPUS_COPY(Y_save, Y, N);
               /* Encode and round down. */
               ctx.theta_round = -1;
               x_cm = quant_band_stereo(&ctx, X, Y, N, b, B,
                     effective_lowband != -1 ? norm+effective_lowband : NULL, LM,
                     last?NULL:norm+M*eBands[i]-norm_offset, lowband_scratch, cm);
               dist0 = MULT16_32_Q15(w[0], celt_inner_prod(X_save, X, N, arch)) + MULT16_32_Q15(w[1], celt_inner_prod(Y_save, Y, N, arch));

               /* Save first result. */
               cm2 = x_cm;
               ec_save2 = *ec;
               ctx_save2 = ctx;
               OPUS_COPY(X_save2, X, N);
               OPUS_COPY(Y_save2, Y, N);
               if (!last)
                  OPUS_COPY(norm_save2, norm+M*eBands[i]-norm_offset, N);
               nstart_bytes = ec_save.offs;
               nend_bytes = ec_save.storage;
               bytes_buf = ec_save.buf+nstart_bytes;
               save_bytes = nend_bytes-nstart_bytes;
               OPUS_COPY(bytes_save, bytes_buf, save_bytes);

               /* Restore */
               *ec = ec_save;
               ctx = ctx_save;
               OPUS_COPY(X, X_save, N);
               OPUS_COPY(Y, Y_save, N);
               if (i == start+1)
                  special_hybrid_folding(m, norm, norm2, start, M, dual_stereo);
               /* Encode and round up. */
               ctx.theta_round = 1;
               x_cm = quant_band_stereo(&ctx, X, Y, N, b, B,
                     effective_lowband != -1 ? norm+effective_lowband : NULL, LM,
                     last?NULL:norm+M*eBands[i]-norm_offset, lowband_scratch, cm);
               dist1 = MULT16_32_Q15(w[0], celt_inner_prod(X_save, X, N, arch)) + MULT16_32_Q15(w[1], celt_inner_prod(Y_save, Y, N, arch));
               if (dist0 >= dist1) {
                  x_cm = cm2;
                  *ec = ec_save2;
                  ctx = ctx_save2;
                  OPUS_COPY(X, X_save2, N);
                  OPUS_COPY(Y, Y_save2, N);
                  if (!last)
                     OPUS_COPY(norm+M*eBands[i]-norm_offset, norm_save2, N);
                  OPUS_COPY(bytes_buf, bytes_save, save_bytes);
               }
            } else {
               ctx.theta_round = 0;
               x_cm = quant_band_stereo(&ctx, X, Y, N, b, B,
                     effective_lowband != -1 ? norm+effective_lowband : NULL, LM,
                     last?NULL:norm+M*eBands[i]-norm_offset, lowband_scratch, x_cm|y_cm);
            }
         } else {
            x_cm = quant_band(&ctx, X, N, b, B,
                  effective_lowband != -1 ? norm+effective_lowband : NULL, LM,
                  last?NULL:norm+M*eBands[i]-norm_offset, Q15ONE, lowband_scratch, x_cm|y_cm);
         }
         y_cm = x_cm;
      }
      collapse_masks[i*C+0] = (unsigned char)x_cm;
      collapse_masks[i*C+C-1] = (unsigned char)y_cm;
      balance += pulses[i] + tell;

      /* Update the folding position only as long as we have 1 bit/sample depth. */
      update_lowband = b>(N<<BITRES);
      /* We only need to avoid noise on a split for the first band. After that, we
         have folding. */
      ctx.avoid_split_noise = 0;
   }
   *seed = ctx.seed;

   RESTORE_STACK;
}
#define float2int(x) lrintf(x)
static OPUS_INLINE opus_int16 FLOAT2INT16(float x)
{
   x = x*CELT_SIG_SCALE;
   x = MAX32(x, -32768);
   x = MIN32(x, 32767);
   return (opus_int16)float2int(x);
}
void _celt_lpc(opus_val16 *_lpc, const opus_val32 *ac, int p);
int _celt_autocorr(const opus_val16 *x, opus_val32 *ac,
         const opus_val16 *window, int overlap, int lag, int n, int arch);
int resampling_factor(opus_int32 rate)
{
   int ret;
   switch (rate)
   {
   case 48000:
      ret = 1;
      break;
   case 24000:
      ret = 2;
      break;
   case 16000:
      ret = 3;
      break;
   case 12000:
      ret = 4;
      break;
   case 8000:
      ret = 6;
      break;
   default:
      celt_assert(0);
      ret = 0;
      break;
   }
   return ret;
}
/* This version should be faster on ARM */
static
void comb_filter_const_c(opus_val32 *y, opus_val32 *x, int T, int N,
      opus_val16 g10, opus_val16 g11, opus_val16 g12)
{
   opus_val32 x0, x1, x2, x3, x4;
   int i;
   x4 = x[-T-2];
   x3 = x[-T-1];
   x2 = x[-T];
   x1 = x[-T+1];
   for (i=0;i<N;i++)
   {
      x0=x[i-T+2];
      y[i] = x[i]
               + MULT16_32_Q15(g10,x2)
               + MULT16_32_Q15(g11,ADD32(x1,x3))
               + MULT16_32_Q15(g12,ADD32(x0,x4));
      y[i] = SATURATE(y[i], SIG_SAT);
      x4=x3;
      x3=x2;
      x2=x1;
      x1=x0;
   }

}
void comb_filter(opus_val32 *y, opus_val32 *x, int T0, int T1, int N,
      opus_val16 g0, opus_val16 g1, int tapset0, int tapset1,
      const opus_val16 *window, int overlap, int arch)
{
   int i;
   /* printf ("%d %d %f %f\n", T0, T1, g0, g1); */
   opus_val16 g00, g01, g02, g10, g11, g12;
   opus_val32 x0, x1, x2, x3, x4;
   static const opus_val16 gains[3][3] = {
         {QCONST16(0.3066406250f, 15), QCONST16(0.2170410156f, 15), QCONST16(0.1296386719f, 15)},
         {QCONST16(0.4638671875f, 15), QCONST16(0.2680664062f, 15), QCONST16(0.f, 15)},
         {QCONST16(0.7998046875f, 15), QCONST16(0.1000976562f, 15), QCONST16(0.f, 15)}};

   if (g0==0 && g1==0)
   {
      /* OPT: Happens to work without the OPUS_MOVE(), but only because the current encoder already copies x to y */
      if (x!=y)
         OPUS_MOVE(y, x, N);
      return;
   }
   /* When the gain is zero, T0 and/or T1 is set to zero. We need
      to have then be at least 2 to avoid processing garbage data. */
   T0 = IMAX(T0, COMBFILTER_MINPERIOD);
   T1 = IMAX(T1, COMBFILTER_MINPERIOD);
   g00 = MULT16_16_P15(g0, gains[tapset0][0]);
   g01 = MULT16_16_P15(g0, gains[tapset0][1]);
   g02 = MULT16_16_P15(g0, gains[tapset0][2]);
   g10 = MULT16_16_P15(g1, gains[tapset1][0]);
   g11 = MULT16_16_P15(g1, gains[tapset1][1]);
   g12 = MULT16_16_P15(g1, gains[tapset1][2]);
   x1 = x[-T1+1];
   x2 = x[-T1  ];
   x3 = x[-T1-1];
   x4 = x[-T1-2];
   /* If the filter didn't change, we don't need the overlap */
   if (g0==g1 && T0==T1 && tapset0==tapset1)
      overlap=0;
   for (i=0;i<overlap;i++)
   {
      opus_val16 f;
      x0=x[i-T1+2];
      f = MULT16_16_Q15(window[i],window[i]);
      y[i] = x[i]
               + MULT16_32_Q15(MULT16_16_Q15((Q15ONE-f),g00),x[i-T0])
               + MULT16_32_Q15(MULT16_16_Q15((Q15ONE-f),g01),ADD32(x[i-T0+1],x[i-T0-1]))
               + MULT16_32_Q15(MULT16_16_Q15((Q15ONE-f),g02),ADD32(x[i-T0+2],x[i-T0-2]))
               + MULT16_32_Q15(MULT16_16_Q15(f,g10),x2)
               + MULT16_32_Q15(MULT16_16_Q15(f,g11),ADD32(x1,x3))
               + MULT16_32_Q15(MULT16_16_Q15(f,g12),ADD32(x0,x4));
      y[i] = SATURATE(y[i], SIG_SAT);
      x4=x3;
      x3=x2;
      x2=x1;
      x1=x0;

   }
   if (g1==0)
   {
      /* OPT: Happens to work without the OPUS_MOVE(), but only because the current encoder already copies x to y */
      if (x!=y)
         OPUS_MOVE(y+overlap, x+overlap, N-overlap);
      return;
   }

   /* Compute the part with the constant filter. */
   comb_filter_const(y+i, x+i, T1, N-i, g10, g11, g12, arch);
}
/* TF change table. Positive values mean better frequency resolution (longer
   effective window), whereas negative values mean better time resolution
   (shorter effective window). The second index is computed as:
   4*isTransient + 2*tf_select + per_band_flag */
const signed char tf_select_table[4][8] = {
    /*isTransient=0     isTransient=1 */
      {0, -1, 0, -1,    0,-1, 0,-1}, /* 2.5 ms */
      {0, -1, 0, -2,    1, 0, 1,-1}, /* 5 ms */
      {0, -2, 0, -3,    2, 0, 1,-1}, /* 10 ms */
      {0, -2, 0, -3,    3, 0, 1,-1}, /* 20 ms */
};
void init_caps(const CELTMode *m,int *cap,int LM,int C)
{
   int i;
   for (i=0;i<m->nbEBands;i++)
   {
      int N;
      N=(m->eBands[i+1]-m->eBands[i])<<LM;
      cap[i] = (m->cache.caps[m->nbEBands*(2*LM+C-1)+i]+64)*C*N>>2;
   }
}
/* Copyright (c) 2001-2011 Timothy B. Terriberry
   Copyright (c) 2008-2009 Xiph.Org Foundation */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

- Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

struct OpusCustomEncoder {
   const OpusCustomMode *mode;        int channels;
   int stream_channels;

   int force_intra;
   int clip;
   int disable_pf;
   int complexity;
   int upsample;
   int start, end;

   opus_int32 bitrate;
   int vbr;
   int signalling;
   int constrained_vbr;      /* If zero, VBR can do whatever it likes with the rate */
   int loss_rate;
   int lsb_depth;
   int lfe;
   int disable_inv;
   int arch;

   /* Everything beyond this point gets cleared on a reset */
#define ENCODER_RESET_START rng

   opus_uint32 rng;
   int spread_decision;
   opus_val32 delayedIntra;
   int tonal_average;
   int lastCodedBands;
   int hf_average;
   int tapset_decision;

   int prefilter_period;
   opus_val16 prefilter_gain;
   int prefilter_tapset;
   int consec_transient;
   AnalysisInfo analysis;
   SILKInfo silk_info;

   opus_val32 preemph_memE[2];
   opus_val32 preemph_memD[2];

   /* VBR-related parameters */
   opus_int32 vbr_reservoir;
   opus_int32 vbr_drift;
   opus_int32 vbr_offset;
   opus_int32 vbr_count;
   opus_val32 overlap_max;
   opus_val16 stereo_saving;
   int intensity;
   opus_val16 *energy_mask;
   opus_val16 spec_avg;

   celt_sig in_mem[1]; /* Size = channels*mode->overlap */
   /* celt_sig prefilter_mem[],  Size = channels*COMBFILTER_MAXPERIOD */
   /* opus_val16 oldBandE[],     Size = channels*mode->nbEBands */
   /* opus_val16 oldLogE[],      Size = channels*mode->nbEBands */
   /* opus_val16 oldLogE2[],     Size = channels*mode->nbEBands */
   /* opus_val16 energyError[],  Size = channels*mode->nbEBands */
};
int celt_encoder_get_size(int channels)
{
   CELTMode *mode = opus_custom_mode_create(48000, 960, NULL);
   return opus_custom_encoder_get_size(mode, channels);
}
OPUS_CUSTOM_NOSTATIC int opus_custom_encoder_get_size(const CELTMode *mode, int channels)
{
   int size = sizeof(struct CELTEncoder)
         + (channels*mode->overlap-1)*sizeof(celt_sig)    /* celt_sig in_mem[channels*mode->overlap]; */
         + channels*COMBFILTER_MAXPERIOD*sizeof(celt_sig) /* celt_sig prefilter_mem[channels*COMBFILTER_MAXPERIOD]; */
         + 4*channels*mode->nbEBands*sizeof(opus_val16);  /* opus_val16 oldBandE[channels*mode->nbEBands]; */
                                                          /* opus_val16 oldLogE[channels*mode->nbEBands]; */
                                                          /* opus_val16 oldLogE2[channels*mode->nbEBands]; */
                                                          /* opus_val16 energyError[channels*mode->nbEBands]; */
   return size;
}
static int opus_custom_encoder_init_arch(CELTEncoder *st, const CELTMode *mode,
                                         int channels, int arch)
{
   if (channels < 0 || channels > 2)
      return OPUS_BAD_ARG;

   if (st==NULL || mode==NULL)
      return OPUS_ALLOC_FAIL;

   OPUS_CLEAR((char*)st, opus_custom_encoder_get_size(mode, channels));

   st->mode = mode;
   st->stream_channels = st->channels = channels;

   st->upsample = 1;
   st->start = 0;
   st->end = st->mode->effEBands;
   st->signalling = 1;
   st->arch = arch;

   st->constrained_vbr = 1;
   st->clip = 1;

   st->bitrate = OPUS_BITRATE_MAX;
   st->vbr = 0;
   st->force_intra  = 0;
   st->complexity = 5;
   st->lsb_depth=24;

   opus_custom_encoder_ctl(st, OPUS_RESET_STATE);

   return OPUS_OK;
}
int celt_encoder_init(CELTEncoder *st, opus_int32 sampling_rate, int channels,
                      int arch)
{
   int ret;
   ret = opus_custom_encoder_init_arch(st,
           opus_custom_mode_create(48000, 960, NULL), channels, arch);
   if (ret != OPUS_OK)
      return ret;
   st->upsample = resampling_factor(sampling_rate);
   return OPUS_OK;
}
static int transient_analysis(const opus_val32 * OPUS_RESTRICT in, int len, int C,
                              opus_val16 *tf_estimate, int *tf_chan, int allow_weak_transients,
                              int *weak_transient)
{
   int i;
   VARDECL(opus_val16, tmp);
   opus_val32 mem0,mem1;
   int is_transient = 0;
   opus_int32 mask_metric = 0;
   int c;
   opus_val16 tf_max;
   int len2;
   /* Forward masking: 6.7 dB/ms. */
   opus_val16 forward_decay = QCONST16(.0625f,15);
   /* Table of 6*64/x, trained on real data to minimize the average error */
   static const unsigned char inv_table[128] = {
         255,255,156,110, 86, 70, 59, 51, 45, 40, 37, 33, 31, 28, 26, 25,
          23, 22, 21, 20, 19, 18, 17, 16, 16, 15, 15, 14, 13, 13, 12, 12,
          12, 12, 11, 11, 11, 10, 10, 10,  9,  9,  9,  9,  9,  9,  8,  8,
           8,  8,  8,  7,  7,  7,  7,  7,  7,  6,  6,  6,  6,  6,  6,  6,
           6,  6,  6,  6,  6,  6,  6,  6,  6,  5,  5,  5,  5,  5,  5,  5,
           5,  5,  5,  5,  5,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
           4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  3,  3,
           3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  2,
   };
   SAVE_STACK;
   ALLOC(tmp, len, opus_val16);

   *weak_transient = 0;
   /* For lower bitrates, let's be more conservative and have a forward masking
      decay of 3.3 dB/ms. This avoids having to code transients at very low
      bitrate (mostly for hybrid), which can result in unstable energy and/or
      partial collapse. */
   if (allow_weak_transients)
   {
      forward_decay = QCONST16(.03125f,15);
   }
   len2=len/2;
   for (c=0;c<C;c++)
   {
      opus_val32 mean;
      opus_int32 unmask=0;
      opus_val32 norm;
      opus_val16 maxE;
      mem0=0;
      mem1=0;
      /* High-pass filter: (1 - 2*z^-1 + z^-2) / (1 - z^-1 + .5*z^-2) */
      for (i=0;i<len;i++)
      {
         float mem00;
         opus_val32 x,y;
         x = SHR32(in[i+c*len],SIG_SHIFT);
         y = ADD32(mem0, x);
         /* Original code:
         mem0 = mem1 + y - 2*x;
         mem1 = x - .5f*y;
         Modified code to shorten dependency chains: */
         mem00=mem0;
         mem0 = mem0 - x + .5f*mem1;
         mem1 =  x - mem00;
         tmp[i] = SROUND16(y, 2);
         /*printf("%f ", tmp[i]);*/
      }
      /*printf("\n");*/
      /* First few samples are bad because we don't propagate the memory */
      OPUS_CLEAR(tmp, 12);

      mean=0;
      mem0=0;
      /* Grouping by two to reduce complexity */
      /* Forward pass to compute the post-echo threshold*/
      for (i=0;i<len2;i++)
      {
         opus_val16 x2 = PSHR32(MULT16_16(tmp[2*i],tmp[2*i]) + MULT16_16(tmp[2*i+1],tmp[2*i+1]),16);
         mean += x2;
         mem0 = x2 + (1.f-forward_decay)*mem0;
         tmp[i] = forward_decay*mem0;
      }

      mem0=0;
      maxE=0;
      /* Backward pass to compute the pre-echo threshold */
      for (i=len2-1;i>=0;i--)
      {
         /* Backward masking: 13.9 dB/ms. */
         mem0 = tmp[i] + 0.875f*mem0;
         tmp[i] = 0.125f*mem0;
         maxE = MAX16(maxE, 0.125f*mem0);
      }
      /*for (i=0;i<len2;i++)printf("%f ", tmp[i]/mean);printf("\n");*/

      /* Compute the ratio of the "frame energy" over the harmonic mean of the energy.
         This essentially corresponds to a bitrate-normalized temporal noise-to-mask
         ratio */

      /* As a compromise with the old transient detector, frame energy is the
         geometric mean of the energy and half the max */
      mean = celt_sqrt(mean * maxE*.5*len2);
      /* Inverse of the mean energy in Q15+6 */
      norm = SHL32(EXTEND32(len2),6+14)/ADD32(EPSILON,SHR32(mean,1));
      /* Compute harmonic mean discarding the unreliable boundaries
         The data is smooth, so we only take 1/4th of the samples */
      unmask=0;
      /* We should never see NaNs here. If we find any, then something really bad happened and we better abort
         before it does any damage later on. If these asserts are disabled (no hardening), then the table
         lookup a few lines below (id = ...) is likely to crash dur to an out-of-bounds read. DO NOT FIX
         that crash on NaN since it could result in a worse issue later on. */
      celt_assert(!celt_isnan(tmp[0]));
      celt_assert(!celt_isnan(norm));
      for (i=12;i<len2-5;i+=4)
      {
         int id;
         id = (int)MAX32(0,MIN32(127,floor(64*norm*(tmp[i]+EPSILON)))); /* Do not round to nearest */
         unmask += inv_table[id];
      }
      /*printf("%d\n", unmask);*/
      /* Normalize, compensate for the 1/4th of the sample and the factor of 6 in the inverse table */
      unmask = 64*unmask*4/(6*(len2-17));
      if (unmask>mask_metric)
      {
         *tf_chan = c;
         mask_metric = unmask;
      }
   }
   is_transient = mask_metric>200;
   /* For low bitrates, define "weak transients" that need to be
      handled differently to avoid partial collapse. */
   if (allow_weak_transients && is_transient && mask_metric<600) {
      is_transient = 0;
      *weak_transient = 1;
   }
   /* Arbitrary metric for VBR boost */
   tf_max = MAX16(0,celt_sqrt(27*mask_metric)-42);
   /* *tf_estimate = 1 + MIN16(1, sqrt(MAX16(0, tf_max-30))/20); */
   *tf_estimate = celt_sqrt(MAX32(0, SHL32(MULT16_16(QCONST16(0.0069,14),MIN16(163,tf_max)),14)-QCONST32(0.139,28)));
   /*printf("%d %f\n", tf_max, mask_metric);*/
   RESTORE_STACK;
   /*printf("%d %f %d\n", is_transient, (float)*tf_estimate, tf_max);*/
   return is_transient;
}
/* Looks for sudden increases of energy to decide whether we need to patch
   the transient decision */
static int patch_transient_decision(opus_val16 *newE, opus_val16 *oldE, int nbEBands,
      int start, int end, int C)
{
   int i, c;
   opus_val32 mean_diff=0;
   opus_val16 spread_old[26];
   /* Apply an aggressive (-6 dB/Bark) spreading function to the old frame to
      avoid false detection caused by irrelevant bands */
   if (C==1)
   {
      spread_old[start] = oldE[start];
      for (i=start+1;i<end;i++)
         spread_old[i] = MAX16(spread_old[i-1]-QCONST16(1.0f, DB_SHIFT), oldE[i]);
   } else {
      spread_old[start] = MAX16(oldE[start],oldE[start+nbEBands]);
      for (i=start+1;i<end;i++)
         spread_old[i] = MAX16(spread_old[i-1]-QCONST16(1.0f, DB_SHIFT),
                               MAX16(oldE[i],oldE[i+nbEBands]));
   }
   for (i=end-2;i>=start;i--)
      spread_old[i] = MAX16(spread_old[i], spread_old[i+1]-QCONST16(1.0f, DB_SHIFT));
   /* Compute mean increase */
   c=0; do {
      for (i=IMAX(2,start);i<end-1;i++)
      {
         opus_val16 x1, x2;
         x1 = MAX16(0, newE[i + c*nbEBands]);
         x2 = MAX16(0, spread_old[i]);
         mean_diff = ADD32(mean_diff, EXTEND32(MAX16(0, SUB16(x1, x2))));
      }
   } while (++c<C);
   mean_diff = DIV32(mean_diff, C*(end-1-IMAX(2,start)));
   /*printf("%f %f %d\n", mean_diff, max_diff, count);*/
   return mean_diff > QCONST16(1.f, DB_SHIFT);
}
static void compute_mdcts(const CELTMode *mode, int shortBlocks, celt_sig * OPUS_RESTRICT in,
                          celt_sig * OPUS_RESTRICT out, int C, int CC, int LM, int upsample,
                          int arch)
{
   const int overlap = mode->overlap;
   int N;
   int B;
   int shift;
   int i, b, c;
   if (shortBlocks)
   {
      B = shortBlocks;
      N = mode->shortMdctSize;
      shift = mode->maxLM;
   } else {
      B = 1;
      N = mode->shortMdctSize<<LM;
      shift = mode->maxLM-LM;
   }
   c=0; do {
      for (b=0;b<B;b++)
      {
         /* Interleaving the sub-frames while doing the MDCTs */
         clt_mdct_forward(&mode->mdct, in+c*(B*N+overlap)+b*N,
                          &out[b+c*N*B], mode->window, overlap, shift, B,
                          arch);
      }
   } while (++c<CC);
   if (CC==2&&C==1)
   {
      for (i=0;i<B*N;i++)
         out[i] = ADD32(HALF32(out[i]), HALF32(out[B*N+i]));
   }
   if (upsample != 1)
   {
      c=0; do
      {
         int bound = B*N/upsample;
         for (i=0;i<bound;i++)
            out[c*B*N+i] *= upsample;
         OPUS_CLEAR(&out[c*B*N+bound], B*N-bound);
      } while (++c<C);
   }
}
void celt_preemphasis(const opus_val16 * OPUS_RESTRICT pcmp, celt_sig * OPUS_RESTRICT inp,
                        int N, int CC, int upsample, const opus_val16 *coef, celt_sig *mem, int clip)
{
   int i;
   opus_val16 coef0;
   celt_sig m;
   int Nu;

   coef0 = coef[0];
   m = *mem;

   /* Fast path for the normal 48kHz case and no clipping */
   if (coef[1] == 0 && upsample == 1 && !clip)
   {
      for (i=0;i<N;i++)
      {
         opus_val16 x;
         x = SCALEIN(pcmp[CC*i]);
         /* Apply pre-emphasis */
         inp[i] = SHL32(x, SIG_SHIFT) - m;
         m = SHR32(MULT16_16(coef0, x), 15-SIG_SHIFT);
      }
      *mem = m;
      return;
   }

   Nu = N/upsample;
   if (upsample!=1)
   {
      OPUS_CLEAR(inp, N);
   }
   for (i=0;i<Nu;i++)
      inp[i*upsample] = SCALEIN(pcmp[CC*i]);

   if (clip)
   {
      /* Clip input to avoid encoding non-portable files */
      for (i=0;i<Nu;i++)
         inp[i*upsample] = MAX32(-65536.f, MIN32(65536.f,inp[i*upsample]));
   }
   {
      for (i=0;i<N;i++)
      {
         opus_val16 x;
         x = inp[i];
         /* Apply pre-emphasis */
         inp[i] = SHL32(x, SIG_SHIFT) - m;
         m = SHR32(MULT16_16(coef0, x), 15-SIG_SHIFT);
      }
   }
   *mem = m;
}
static opus_val32 l1_metric(const celt_norm *tmp, int N, int LM, opus_val16 bias)
{
   int i;
   opus_val32 L1;
   L1 = 0;
   for (i=0;i<N;i++)
      L1 += EXTEND32(ABS16(tmp[i]));
   /* When in doubt, prefer good freq resolution */
   L1 = MAC16_32_Q15(L1, LM*bias, L1);
   return L1;

}
static int tf_analysis(const CELTMode *m, int len, int isTransient,
      int *tf_res, int lambda, celt_norm *X, int N0, int LM,
      opus_val16 tf_estimate, int tf_chan, int *importance)
{
   int i;
   VARDECL(int, metric);
   int cost0;
   int cost1;
   VARDECL(int, path0);
   VARDECL(int, path1);
   VARDECL(celt_norm, tmp);
   VARDECL(celt_norm, tmp_1);
   int sel;
   int selcost[2];
   int tf_select=0;
   opus_val16 bias;

   SAVE_STACK;
   bias = MULT16_16_Q14(QCONST16(.04f,15), MAX16(-QCONST16(.25f,14), QCONST16(.5f,14)-tf_estimate));
   /*printf("%f ", bias);*/

   ALLOC(metric, len, int);
   ALLOC(tmp, (m->eBands[len]-m->eBands[len-1])<<LM, celt_norm);
   ALLOC(tmp_1, (m->eBands[len]-m->eBands[len-1])<<LM, celt_norm);
   ALLOC(path0, len, int);
   ALLOC(path1, len, int);

   for (i=0;i<len;i++)
   {
      int k, N;
      int narrow;
      opus_val32 L1, best_L1;
      int best_level=0;
      N = (m->eBands[i+1]-m->eBands[i])<<LM;
      /* band is too narrow to be split down to LM=-1 */
      narrow = (m->eBands[i+1]-m->eBands[i])==1;
      OPUS_COPY(tmp, &X[tf_chan*N0 + (m->eBands[i]<<LM)], N);
      /* Just add the right channel if we're in stereo */
      /*if (C==2)
         for (j=0;j<N;j++)
            tmp[j] = ADD16(SHR16(tmp[j], 1),SHR16(X[N0+j+(m->eBands[i]<<LM)], 1));*/
      L1 = l1_metric(tmp, N, isTransient ? LM : 0, bias);
      best_L1 = L1;
      /* Check the -1 case for transients */
      if (isTransient && !narrow)
      {
         OPUS_COPY(tmp_1, tmp, N);
         haar1(tmp_1, N>>LM, 1<<LM);
         L1 = l1_metric(tmp_1, N, LM+1, bias);
         if (L1<best_L1)
         {
            best_L1 = L1;
            best_level = -1;
         }
      }
      /*printf ("%f ", L1);*/
      for (k=0;k<LM+!(isTransient||narrow);k++)
      {
         int B;

         if (isTransient)
            B = (LM-k-1);
         else
            B = k+1;

         haar1(tmp, N>>k, 1<<k);

         L1 = l1_metric(tmp, N, B, bias);

         if (L1 < best_L1)
         {
            best_L1 = L1;
            best_level = k+1;
         }
      }
      /*printf ("%d ", isTransient ? LM-best_level : best_level);*/
      /* metric is in Q1 to be able to select the mid-point (-0.5) for narrower bands */
      if (isTransient)
         metric[i] = 2*best_level;
      else
         metric[i] = -2*best_level;
      /* For bands that can't be split to -1, set the metric to the half-way point to avoid
         biasing the decision */
      if (narrow && (metric[i]==0 || metric[i]==-2*LM))
         metric[i]-=1;
      /*printf("%d ", metric[i]/2 + (!isTransient)*LM);*/
   }
   /*printf("\n");*/
   /* Search for the optimal tf resolution, including tf_select */
   tf_select = 0;
   for (sel=0;sel<2;sel++)
   {
      cost0 = importance[0]*abs(metric[0]-2*tf_select_table[LM][4*isTransient+2*sel+0]);
      cost1 = importance[0]*abs(metric[0]-2*tf_select_table[LM][4*isTransient+2*sel+1]) + (isTransient ? 0 : lambda);
      for (i=1;i<len;i++)
      {
         int curr0, curr1;
         curr0 = IMIN(cost0, cost1 + lambda);
         curr1 = IMIN(cost0 + lambda, cost1);
         cost0 = curr0 + importance[i]*abs(metric[i]-2*tf_select_table[LM][4*isTransient+2*sel+0]);
         cost1 = curr1 + importance[i]*abs(metric[i]-2*tf_select_table[LM][4*isTransient+2*sel+1]);
      }
      cost0 = IMIN(cost0, cost1);
      selcost[sel]=cost0;
   }
   /* For now, we're conservative and only allow tf_select=1 for transients.
    * If tests confirm it's useful for non-transients, we could allow it. */
   if (selcost[1]<selcost[0] && isTransient)
      tf_select=1;
   cost0 = importance[0]*abs(metric[0]-2*tf_select_table[LM][4*isTransient+2*tf_select+0]);
   cost1 = importance[0]*abs(metric[0]-2*tf_select_table[LM][4*isTransient+2*tf_select+1]) + (isTransient ? 0 : lambda);
   /* Viterbi forward pass */
   for (i=1;i<len;i++)
   {
      int curr0, curr1;
      int from0, from1;

      from0 = cost0;
      from1 = cost1 + lambda;
      if (from0 < from1)
      {
         curr0 = from0;
         path0[i]= 0;
      } else {
         curr0 = from1;
         path0[i]= 1;
      }

      from0 = cost0 + lambda;
      from1 = cost1;
      if (from0 < from1)
      {
         curr1 = from0;
         path1[i]= 0;
      } else {
         curr1 = from1;
         path1[i]= 1;
      }
      cost0 = curr0 + importance[i]*abs(metric[i]-2*tf_select_table[LM][4*isTransient+2*tf_select+0]);
      cost1 = curr1 + importance[i]*abs(metric[i]-2*tf_select_table[LM][4*isTransient+2*tf_select+1]);
   }
   tf_res[len-1] = cost0 < cost1 ? 0 : 1;
   /* Viterbi backward pass to check the decisions */
   for (i=len-2;i>=0;i--)
   {
      if (tf_res[i+1] == 1)
         tf_res[i] = path1[i+1];
      else
         tf_res[i] = path0[i+1];
   }
   /*printf("%d %f\n", *tf_sum, tf_estimate);*/
   RESTORE_STACK;
   return tf_select;
}
static void tf_encode(int start, int end, int isTransient, int *tf_res, int LM, int tf_select, ec_enc *enc)
{
   int curr, i;
   int tf_select_rsv;
   int tf_changed;
   int logp;
   opus_uint32 budget;
   opus_uint32 tell;
   budget = enc->storage*8;
   tell = ec_tell(enc);
   logp = isTransient ? 2 : 4;
   /* Reserve space to code the tf_select decision. */
   tf_select_rsv = LM>0 && tell+logp+1 <= budget;
   budget -= tf_select_rsv;
   curr = tf_changed = 0;
   for (i=start;i<end;i++)
   {
      if (tell+logp<=budget)
      {
         ec_enc_bit_logp(enc, tf_res[i] ^ curr, logp);
         tell = ec_tell(enc);
         curr = tf_res[i];
         tf_changed |= curr;
      }
      else
         tf_res[i] = curr;
      logp = isTransient ? 4 : 5;
   }
   /* Only code tf_select if it would actually make a difference. */
   if (tf_select_rsv &&
         tf_select_table[LM][4*isTransient+0+tf_changed]!=
         tf_select_table[LM][4*isTransient+2+tf_changed])
      ec_enc_bit_logp(enc, tf_select, 1);
   else
      tf_select = 0;
   for (i=start;i<end;i++)
      tf_res[i] = tf_select_table[LM][4*isTransient+2*tf_select+tf_res[i]];
   /*for(i=0;i<end;i++)printf("%d ", isTransient ? tf_res[i] : LM+tf_res[i]);printf("\n");*/
}
static int alloc_trim_analysis(const CELTMode *m, const celt_norm *X,
      const opus_val16 *bandLogE, int end, int LM, int C, int N0,
      AnalysisInfo *analysis, opus_val16 *stereo_saving, opus_val16 tf_estimate,
      int intensity, opus_val16 surround_trim, opus_int32 equiv_rate, int arch)
{
   int i;
   opus_val32 diff=0;
   int c;
   int trim_index;
   opus_val16 trim = QCONST16(5.f, 8);
   opus_val16 logXC, logXC2;
   /* At low bitrate, reducing the trim seems to help. At higher bitrates, it's less
      clear what's best, so we're keeping it as it was before, at least for now. */
   if (equiv_rate < 64000) {
      trim = QCONST16(4.f, 8);
   } else if (equiv_rate < 80000) {
      opus_int32 frac = (equiv_rate-64000) >> 10;
      trim = QCONST16(4.f, 8) + QCONST16(1.f/16.f, 8)*frac;
   }
   if (C==2)
   {
      opus_val16 sum = 0; /* Q10 */
      opus_val16 minXC; /* Q10 */
      /* Compute inter-channel correlation for low frequencies */
      for (i=0;i<8;i++)
      {
         opus_val32 partial;
         partial = celt_inner_prod(&X[m->eBands[i]<<LM], &X[N0+(m->eBands[i]<<LM)],
               (m->eBands[i+1]-m->eBands[i])<<LM, arch);
         sum = ADD16(sum, EXTRACT16(SHR32(partial, 18)));
      }
      sum = MULT16_16_Q15(QCONST16(1.f/8, 15), sum);
      sum = MIN16(QCONST16(1.f, 10), ABS16(sum));
      minXC = sum;
      for (i=8;i<intensity;i++)
      {
         opus_val32 partial;
         partial = celt_inner_prod(&X[m->eBands[i]<<LM], &X[N0+(m->eBands[i]<<LM)],
               (m->eBands[i+1]-m->eBands[i])<<LM, arch);
         minXC = MIN16(minXC, ABS16(EXTRACT16(SHR32(partial, 18))));
      }
      minXC = MIN16(QCONST16(1.f, 10), ABS16(minXC));
      /*printf ("%f\n", sum);*/
      /* mid-side savings estimations based on the LF average*/
      logXC = celt_log2(QCONST32(1.001f, 20)-MULT16_16(sum, sum));
      /* mid-side savings estimations based on min correlation */
      logXC2 = MAX16(HALF16(logXC), celt_log2(QCONST32(1.001f, 20)-MULT16_16(minXC, minXC)));

      trim += MAX16(-QCONST16(4.f, 8), MULT16_16_Q15(QCONST16(.75f,15),logXC));
      *stereo_saving = MIN16(*stereo_saving + QCONST16(0.25f, 8), -HALF16(logXC2));
   }

   /* Estimate spectral tilt */
   c=0; do {
      for (i=0;i<end-1;i++)
      {
         diff += bandLogE[i+c*m->nbEBands]*(opus_int32)(2+2*i-end);
      }
   } while (++c<C);
   diff /= C*(end-1);
   /*printf("%f\n", diff);*/
   trim -= MAX32(-QCONST16(2.f, 8), MIN32(QCONST16(2.f, 8), SHR32(diff+QCONST16(1.f, DB_SHIFT),DB_SHIFT-8)/6 ));
   trim -= SHR16(surround_trim, DB_SHIFT-8);
   trim -= 2*SHR16(tf_estimate, 14-8);
   if (analysis->valid)
   {
      trim -= MAX16(-QCONST16(2.f, 8), MIN16(QCONST16(2.f, 8),
            (opus_val16)(QCONST16(2.f, 8)*(analysis->tonality_slope+.05f))));
   }

   trim_index = (int)floor(.5f+trim);
   trim_index = IMAX(0, IMIN(10, trim_index));
   /*printf("%d\n", trim_index);*/
   return trim_index;
}
static int stereo_analysis(const CELTMode *m, const celt_norm *X,
      int LM, int N0)
{
   int i;
   int thetas;
   opus_val32 sumLR = EPSILON, sumMS = EPSILON;

   /* Use the L1 norm to model the entropy of the L/R signal vs the M/S signal */
   for (i=0;i<13;i++)
   {
      int j;
      for (j=m->eBands[i]<<LM;j<m->eBands[i+1]<<LM;j++)
      {
         opus_val32 L, R, M, S;
         /* We cast to 32-bit first because of the -32768 case */
         L = EXTEND32(X[j]);
         R = EXTEND32(X[N0+j]);
         M = ADD32(L, R);
         S = SUB32(L, R);
         sumLR = ADD32(sumLR, ADD32(ABS32(L), ABS32(R)));
         sumMS = ADD32(sumMS, ADD32(ABS32(M), ABS32(S)));
      }
   }
   sumMS = MULT16_32_Q15(QCONST16(0.707107f, 15), sumMS);
   thetas = 13;
   /* We don't need thetas for lower bands with LM<=1 */
   if (LM<=1)
      thetas -= 8;
   return MULT16_32_Q15((m->eBands[13]<<(LM+1))+thetas, sumMS)
         > MULT16_32_Q15(m->eBands[13]<<(LM+1), sumLR);
}
#define MSWAP(a,b) do {opus_val16 tmp = a;a=b;b=tmp;} while(0)
static opus_val16 median_of_5(const opus_val16 *x)
{
   opus_val16 t0, t1, t2, t3, t4;
   t2 = x[2];
   if (x[0] > x[1])
   {
      t0 = x[1];
      t1 = x[0];
   } else {
      t0 = x[0];
      t1 = x[1];
   }
   if (x[3] > x[4])
   {
      t3 = x[4];
      t4 = x[3];
   } else {
      t3 = x[3];
      t4 = x[4];
   }
   if (t0 > t3)
   {
      MSWAP(t0, t3);
      MSWAP(t1, t4);
   }
   if (t2 > t1)
   {
      if (t1 < t3)
         return MIN16(t2, t3);
      else
         return MIN16(t4, t1);
   } else {
      if (t2 < t3)
         return MIN16(t1, t3);
      else
         return MIN16(t2, t4);
   }
}
static opus_val16 median_of_3(const opus_val16 *x)
{
   opus_val16 t0, t1, t2;
   if (x[0] > x[1])
   {
      t0 = x[1];
      t1 = x[0];
   } else {
      t0 = x[0];
      t1 = x[1];
   }
   t2 = x[2];
   if (t1 < t2)
      return t1;
   else if (t0 < t2)
      return t2;
   else
      return t0;
}
static opus_val16 dynalloc_analysis(const opus_val16 *bandLogE, const opus_val16 *bandLogE2, const opus_val16 *oldBandE,
      int nbEBands, int start, int end, int C, int *offsets, int lsb_depth, const opus_int16 *logN,
      int isTransient, int vbr, int constrained_vbr, const opus_int16 *eBands, int LM,
      int effectiveBytes, opus_int32 *tot_boost_, int lfe, opus_val16 *surround_dynalloc,
      AnalysisInfo *analysis, int *importance, int *spread_weight)
{
   int i, c;
   opus_int32 tot_boost=0;
   opus_val16 maxDepth;
   VARDECL(opus_val16, follower);
   VARDECL(opus_val16, noise_floor);
   VARDECL(opus_val16, bandLogE3);
   SAVE_STACK;
   ALLOC(follower, C*nbEBands, opus_val16);
   ALLOC(noise_floor, C*nbEBands, opus_val16);
   ALLOC(bandLogE3, nbEBands, opus_val16);
   OPUS_CLEAR(offsets, nbEBands);
   /* Dynamic allocation code */
   maxDepth=-QCONST16(31.9f, DB_SHIFT);
   for (i=0;i<end;i++)
   {
      /* Noise floor must take into account eMeans, the depth, the width of the bands
         and the preemphasis filter (approx. square of bark band ID) */
      noise_floor[i] = MULT16_16(QCONST16(0.0625f, DB_SHIFT),logN[i])
            +QCONST16(.5f,DB_SHIFT)+SHL16(9-lsb_depth,DB_SHIFT)-SHL16(eMeans[i],6)
            +MULT16_16(QCONST16(.0062,DB_SHIFT),(i+5)*(i+5));
   }
   c=0;do
   {
      for (i=0;i<end;i++)
         maxDepth = MAX16(maxDepth, bandLogE[c*nbEBands+i]-noise_floor[i]);
   } while (++c<C);
   {
      /* Compute a really simple masking model to avoid taking into account completely masked
         bands when computing the spreading decision. */
      VARDECL(opus_val16, mask);
      VARDECL(opus_val16, sig);
      ALLOC(mask, nbEBands, opus_val16);
      ALLOC(sig, nbEBands, opus_val16);
      for (i=0;i<end;i++)
         mask[i] = bandLogE[i]-noise_floor[i];
      if (C==2)
      {
         for (i=0;i<end;i++)
            mask[i] = MAX16(mask[i], bandLogE[nbEBands+i]-noise_floor[i]);
      }
      OPUS_COPY(sig, mask, end);
      for (i=1;i<end;i++)
         mask[i] = MAX16(mask[i], mask[i-1] - QCONST16(2.f, DB_SHIFT));
      for (i=end-2;i>=0;i--)
         mask[i] = MAX16(mask[i], mask[i+1] - QCONST16(3.f, DB_SHIFT));
      for (i=0;i<end;i++)
      {
         /* Compute SMR: Mask is never more than 72 dB below the peak and never below the noise floor.*/
         opus_val16 smr = sig[i]-MAX16(MAX16(0, maxDepth-QCONST16(12.f, DB_SHIFT)), mask[i]);
         /* Clamp SMR to make sure we're not shifting by something negative or too large. */
         int shift = IMIN(5, IMAX(0, -(int)floor(.5f + smr)));
         spread_weight[i] = 32 >> shift;
      }
      /*for (i=0;i<end;i++)
         printf("%d ", spread_weight[i]);
      printf("\n");*/
   }
   /* Make sure that dynamic allocation can't make us bust the budget.
      We enable the feature starting at 24 kb/s for 20-ms frames
      and 96 kb/s for 2.5 ms frames.  */
   if (effectiveBytes >= (30 + 5*LM) && !lfe)
   {
      int last=0;
      c=0;do
      {
         opus_val16 offset;
         opus_val16 tmp;
         opus_val16 *f;
         OPUS_COPY(bandLogE3, &bandLogE2[c*nbEBands], end);
         if (LM==0) {
            /* For 2.5 ms frames, the first 8 bands have just one bin, so the
               energy is highly unreliable (high variance). For that reason,
               we take the max with the previous energy so that at least 2 bins
               are getting used. */
            for (i=0;i<IMIN(8,end);i++) bandLogE3[i] = MAX16(bandLogE2[c*nbEBands+i], oldBandE[c*nbEBands+i]);
         }
         f = &follower[c*nbEBands];
         f[0] = bandLogE3[0];
         for (i=1;i<end;i++)
         {
            /* The last band to be at least 3 dB higher than the previous one
               is the last we'll consider. Otherwise, we run into problems on
               bandlimited signals. */
            if (bandLogE3[i] > bandLogE3[i-1]+QCONST16(.5f,DB_SHIFT))
               last=i;
            f[i] = MIN16(f[i-1]+QCONST16(1.5f,DB_SHIFT), bandLogE3[i]);
         }
         for (i=last-1;i>=0;i--)
            f[i] = MIN16(f[i], MIN16(f[i+1]+QCONST16(2.f,DB_SHIFT), bandLogE3[i]));

         /* Combine with a median filter to avoid dynalloc triggering unnecessarily.
            The "offset" value controls how conservative we are -- a higher offset
            reduces the impact of the median filter and makes dynalloc use more bits. */
         offset = QCONST16(1.f, DB_SHIFT);
         for (i=2;i<end-2;i++)
            f[i] = MAX16(f[i], median_of_5(&bandLogE3[i-2])-offset);
         tmp = median_of_3(&bandLogE3[0])-offset;
         f[0] = MAX16(f[0], tmp);
         f[1] = MAX16(f[1], tmp);
         tmp = median_of_3(&bandLogE3[end-3])-offset;
         f[end-2] = MAX16(f[end-2], tmp);
         f[end-1] = MAX16(f[end-1], tmp);

         for (i=0;i<end;i++)
            f[i] = MAX16(f[i], noise_floor[i]);
      } while (++c<C);
      if (C==2)
      {
         for (i=start;i<end;i++)
         {
            /* Consider 24 dB "cross-talk" */
            follower[nbEBands+i] = MAX16(follower[nbEBands+i], follower[         i]-QCONST16(4.f,DB_SHIFT));
            follower[         i] = MAX16(follower[         i], follower[nbEBands+i]-QCONST16(4.f,DB_SHIFT));
            follower[i] = HALF16(MAX16(0, bandLogE[i]-follower[i]) + MAX16(0, bandLogE[nbEBands+i]-follower[nbEBands+i]));
         }
      } else {
         for (i=start;i<end;i++)
         {
            follower[i] = MAX16(0, bandLogE[i]-follower[i]);
         }
      }
      for (i=start;i<end;i++)
         follower[i] = MAX16(follower[i], surround_dynalloc[i]);
      for (i=start;i<end;i++)
      {
         importance[i] = (int)floor(.5f+13*celt_exp2(MIN16(follower[i], QCONST16(4.f, DB_SHIFT))));
      }
      /* For non-transient CBR/CVBR frames, halve the dynalloc contribution */
      if ((!vbr || constrained_vbr)&&!isTransient)
      {
         for (i=start;i<end;i++)
            follower[i] = HALF16(follower[i]);
      }
      for (i=start;i<end;i++)
      {
         if (i<8)
            follower[i] *= 2;
         if (i>=12)
            follower[i] = HALF16(follower[i]);
      }
      if (analysis->valid)
      {
         for (i=start;i<IMIN(LEAK_BANDS, end);i++)
            follower[i] = follower[i] +  QCONST16(1.f/64.f, DB_SHIFT)*analysis->leak_boost[i];
      }
      for (i=start;i<end;i++)
      {
         int width;
         int boost;
         int boost_bits;

         follower[i] = MIN16(follower[i], QCONST16(4, DB_SHIFT));

         width = C*(eBands[i+1]-eBands[i])<<LM;
         if (width<6)
         {
            boost = (int)SHR32(EXTEND32(follower[i]),DB_SHIFT);
            boost_bits = boost*width<<BITRES;
         } else if (width > 48) {
            boost = (int)SHR32(EXTEND32(follower[i])*8,DB_SHIFT);
            boost_bits = (boost*width<<BITRES)/8;
         } else {
            boost = (int)SHR32(EXTEND32(follower[i])*width/6,DB_SHIFT);
            boost_bits = boost*6<<BITRES;
         }
         /* For CBR and non-transient CVBR frames, limit dynalloc to 2/3 of the bits */
         if ((!vbr || (constrained_vbr&&!isTransient))
               && (tot_boost+boost_bits)>>BITRES>>3 > 2*effectiveBytes/3)
         {
            opus_int32 cap = ((2*effectiveBytes/3)<<BITRES<<3);
            offsets[i] = cap-tot_boost;
            tot_boost = cap;
            break;
         } else {
            offsets[i] = boost;
            tot_boost += boost_bits;
         }
      }
   } else {
      for (i=start;i<end;i++)
         importance[i] = 13;
   }
   *tot_boost_ = tot_boost;
   RESTORE_STACK;
   return maxDepth;
}
static int run_prefilter(CELTEncoder *st, celt_sig *in, celt_sig *prefilter_mem, int CC, int N,
      int prefilter_tapset, int *pitch, opus_val16 *gain, int *qgain, int enabled, int nbAvailableBytes, AnalysisInfo *analysis)
{
   int c;
   VARDECL(celt_sig, _pre);
   celt_sig *pre[2];
   const CELTMode *mode;
   int pitch_index;
   opus_val16 gain1;
   opus_val16 pf_threshold;
   int pf_on;
   int qg;
   int overlap;
   SAVE_STACK;

   mode = st->mode;
   overlap = mode->overlap;
   ALLOC(_pre, CC*(N+COMBFILTER_MAXPERIOD), celt_sig);

   pre[0] = _pre;
   pre[1] = _pre + (N+COMBFILTER_MAXPERIOD);

   c=0; do {
      OPUS_COPY(pre[c], prefilter_mem+c*COMBFILTER_MAXPERIOD, COMBFILTER_MAXPERIOD);
      OPUS_COPY(pre[c]+COMBFILTER_MAXPERIOD, in+c*(N+overlap)+overlap, N);
   } while (++c<CC);

   if (enabled)
   {
      VARDECL(opus_val16, pitch_buf);
      ALLOC(pitch_buf, (COMBFILTER_MAXPERIOD+N)>>1, opus_val16);

      pitch_downsample(pre, pitch_buf, COMBFILTER_MAXPERIOD+N, CC, st->arch);
      /* Don't search for the fir last 1.5 octave of the range because
         there's too many false-positives due to short-term correlation */
      pitch_search(pitch_buf+(COMBFILTER_MAXPERIOD>>1), pitch_buf, N,
            COMBFILTER_MAXPERIOD-3*COMBFILTER_MINPERIOD, &pitch_index,
            st->arch);
      pitch_index = COMBFILTER_MAXPERIOD-pitch_index;

      gain1 = remove_doubling(pitch_buf, COMBFILTER_MAXPERIOD, COMBFILTER_MINPERIOD,
            N, &pitch_index, st->prefilter_period, st->prefilter_gain, st->arch);
      if (pitch_index > COMBFILTER_MAXPERIOD-2)
         pitch_index = COMBFILTER_MAXPERIOD-2;
      gain1 = MULT16_16_Q15(QCONST16(.7f,15),gain1);
      /*printf("%d %d %f %f\n", pitch_change, pitch_index, gain1, st->analysis.tonality);*/
      if (st->loss_rate>2)
         gain1 = HALF32(gain1);
      if (st->loss_rate>4)
         gain1 = HALF32(gain1);
      if (st->loss_rate>8)
         gain1 = 0;
   } else {
      gain1 = 0;
      pitch_index = COMBFILTER_MINPERIOD;
   }
   if (analysis->valid)
      gain1 = (opus_val16)(gain1 * analysis->max_pitch_ratio);
   /* Gain threshold for enabling the prefilter/postfilter */
   pf_threshold = QCONST16(.2f,15);

   /* Adjusting the threshold based on rate and continuity */
   if (abs(pitch_index-st->prefilter_period)*10>pitch_index)
      pf_threshold += QCONST16(.2f,15);
   if (nbAvailableBytes<25)
      pf_threshold += QCONST16(.1f,15);
   if (nbAvailableBytes<35)
      pf_threshold += QCONST16(.1f,15);
   if (st->prefilter_gain > QCONST16(.4f,15))
      pf_threshold -= QCONST16(.1f,15);
   if (st->prefilter_gain > QCONST16(.55f,15))
      pf_threshold -= QCONST16(.1f,15);

   /* Hard threshold at 0.2 */
   pf_threshold = MAX16(pf_threshold, QCONST16(.2f,15));
   if (gain1<pf_threshold)
   {
      gain1 = 0;
      pf_on = 0;
      qg = 0;
   } else {
      /*This block is not gated by a total bits check only because
        of the nbAvailableBytes check above.*/
      if (ABS16(gain1-st->prefilter_gain)<QCONST16(.1f,15))
         gain1=st->prefilter_gain;

      qg = (int)floor(.5f+gain1*32/3)-1;
      qg = IMAX(0, IMIN(7, qg));
      gain1 = QCONST16(0.09375f,15)*(qg+1);
      pf_on = 1;
   }
   /*printf("%d %f\n", pitch_index, gain1);*/

   c=0; do {
      int offset = mode->shortMdctSize-overlap;
      st->prefilter_period=IMAX(st->prefilter_period, COMBFILTER_MINPERIOD);
      OPUS_COPY(in+c*(N+overlap), st->in_mem+c*(overlap), overlap);
      if (offset)
         comb_filter(in+c*(N+overlap)+overlap, pre[c]+COMBFILTER_MAXPERIOD,
               st->prefilter_period, st->prefilter_period, offset, -st->prefilter_gain, -st->prefilter_gain,
               st->prefilter_tapset, st->prefilter_tapset, NULL, 0, st->arch);

      comb_filter(in+c*(N+overlap)+overlap+offset, pre[c]+COMBFILTER_MAXPERIOD+offset,
            st->prefilter_period, pitch_index, N-offset, -st->prefilter_gain, -gain1,
            st->prefilter_tapset, prefilter_tapset, mode->window, overlap, st->arch);
      OPUS_COPY(st->in_mem+c*(overlap), in+c*(N+overlap)+N, overlap);

      if (N>COMBFILTER_MAXPERIOD)
      {
         OPUS_COPY(prefilter_mem+c*COMBFILTER_MAXPERIOD, pre[c]+N, COMBFILTER_MAXPERIOD);
      } else {
         OPUS_MOVE(prefilter_mem+c*COMBFILTER_MAXPERIOD, prefilter_mem+c*COMBFILTER_MAXPERIOD+N, COMBFILTER_MAXPERIOD-N);
         OPUS_COPY(prefilter_mem+c*COMBFILTER_MAXPERIOD+COMBFILTER_MAXPERIOD-N, pre[c]+COMBFILTER_MAXPERIOD, N);
      }
   } while (++c<CC);

   RESTORE_STACK;
   *gain = gain1;
   *pitch = pitch_index;
   *qgain = qg;
   return pf_on;
}
static int compute_vbr(const CELTMode *mode, AnalysisInfo *analysis, opus_int32 base_target,
      int LM, opus_int32 bitrate, int lastCodedBands, int C, int intensity,
      int constrained_vbr, opus_val16 stereo_saving, int tot_boost,
      opus_val16 tf_estimate, int pitch_change, opus_val16 maxDepth,
      int lfe, int has_surround_mask, opus_val16 surround_masking,
      opus_val16 temporal_vbr)
{
   /* The target rate in 8th bits per frame */
   opus_int32 target;
   int coded_bins;
   int coded_bands;
   opus_val16 tf_calibration;
   int nbEBands;
   const opus_int16 *eBands;

   nbEBands = mode->nbEBands;
   eBands = mode->eBands;

   coded_bands = lastCodedBands ? lastCodedBands : nbEBands;
   coded_bins = eBands[coded_bands]<<LM;
   if (C==2)
      coded_bins += eBands[IMIN(intensity, coded_bands)]<<LM;

   target = base_target;

   /*printf("%f %f %f %f %d %d ", st->analysis.activity, st->analysis.tonality, tf_estimate, st->stereo_saving, tot_boost, coded_bands);*/
   if (analysis->valid && analysis->activity<.4)
      target -= (opus_int32)((coded_bins<<BITRES)*(.4f-analysis->activity));
   /* Stereo savings */
   if (C==2)
   {
      int coded_stereo_bands;
      int coded_stereo_dof;
      opus_val16 max_frac;
      coded_stereo_bands = IMIN(intensity, coded_bands);
      coded_stereo_dof = (eBands[coded_stereo_bands]<<LM)-coded_stereo_bands;
      /* Maximum fraction of the bits we can save if the signal is mono. */
      max_frac = DIV32_16(MULT16_16(QCONST16(0.8f, 15), coded_stereo_dof), coded_bins);
      stereo_saving = MIN16(stereo_saving, QCONST16(1.f, 8));
      /*printf("%d %d %d ", coded_stereo_dof, coded_bins, tot_boost);*/
      target -= (opus_int32)MIN32(MULT16_32_Q15(max_frac,target),
                      SHR32(MULT16_16(stereo_saving-QCONST16(0.1f,8),(coded_stereo_dof<<BITRES)),8));
   }
   /* Boost the rate according to dynalloc (minus the dynalloc average for calibration). */
   target += tot_boost-(19<<LM);
   /* Apply transient boost, compensating for average boost. */
   tf_calibration = QCONST16(0.044f,14);
   target += (opus_int32)SHL32(MULT16_32_Q15(tf_estimate-tf_calibration, target),1);

   /* Apply tonality boost */
   if (analysis->valid && !lfe)
   {
      opus_int32 tonal_target;
      float tonal;

      /* Tonality boost (compensating for the average). */
      tonal = MAX16(0.f,analysis->tonality-.15f)-0.12f;
      tonal_target = target + (opus_int32)((coded_bins<<BITRES)*1.2f*tonal);
      if (pitch_change)
         tonal_target +=  (opus_int32)((coded_bins<<BITRES)*.8f);
      /*printf("%f %f ", analysis->tonality, tonal);*/
      target = tonal_target;
   }

   if (has_surround_mask&&!lfe)
   {
      opus_int32 surround_target = target + (opus_int32)SHR32(MULT16_16(surround_masking,coded_bins<<BITRES), DB_SHIFT);
      /*printf("%f %d %d %d %d %d %d ", surround_masking, coded_bins, st->end, st->intensity, surround_target, target, st->bitrate);*/
      target = IMAX(target/4, surround_target);
   }

   {
      opus_int32 floor_depth;
      int bins;
      bins = eBands[nbEBands-2]<<LM;
      /*floor_depth = SHR32(MULT16_16((C*bins<<BITRES),celt_log2(SHL32(MAX16(1,sample_max),13))), DB_SHIFT);*/
      floor_depth = (opus_int32)SHR32(MULT16_16((C*bins<<BITRES),maxDepth), DB_SHIFT);
      floor_depth = IMAX(floor_depth, target>>2);
      target = IMIN(target, floor_depth);
      /*printf("%f %d\n", maxDepth, floor_depth);*/
   }

   /* Make VBR less aggressive for constrained VBR because we can't keep a higher bitrate
      for long. Needs tuning. */
   if ((!has_surround_mask||lfe) && constrained_vbr)
   {
      target = base_target + (opus_int32)MULT16_32_Q15(QCONST16(0.67f, 15), target-base_target);
   }

   if (!has_surround_mask && tf_estimate < QCONST16(.2f, 14))
   {
      opus_val16 amount;
      opus_val16 tvbr_factor;
      amount = MULT16_16_Q15(QCONST16(.0000031f, 30), IMAX(0, IMIN(32000, 96000-bitrate)));
      tvbr_factor = SHR32(MULT16_16(temporal_vbr, amount), DB_SHIFT);
      target += (opus_int32)MULT16_32_Q15(tvbr_factor, target);
   }

   /* Don't allow more than doubling the rate */
   target = IMIN(2*base_target, target);

   return target;
}
int celt_encode_with_ec(CELTEncoder * OPUS_RESTRICT st, const opus_val16 * pcm, int frame_size, unsigned char *compressed, int nbCompressedBytes, ec_enc *enc)
{
   int i, c, N;
   opus_int32 bits;
   ec_enc _enc;
   VARDECL(celt_sig, in);
   VARDECL(celt_sig, freq);
   VARDECL(celt_norm, X);
   VARDECL(celt_ener, bandE);
   VARDECL(opus_val16, bandLogE);
   VARDECL(opus_val16, bandLogE2);
   VARDECL(int, fine_quant);
   VARDECL(opus_val16, error);
   VARDECL(int, pulses);
   VARDECL(int, cap);
   VARDECL(int, offsets);
   VARDECL(int, importance);
   VARDECL(int, spread_weight);
   VARDECL(int, fine_priority);
   VARDECL(int, tf_res);
   VARDECL(unsigned char, collapse_masks);
   celt_sig *prefilter_mem;
   opus_val16 *oldBandE, *oldLogE, *oldLogE2, *energyError;
   int shortBlocks=0;
   int isTransient=0;
   const int CC = st->channels;
   const int C = st->stream_channels;
   int LM, M;
   int tf_select;
   int nbFilledBytes, nbAvailableBytes;
   int start;
   int end;
   int effEnd;
   int codedBands;
   int alloc_trim;
   int pitch_index=COMBFILTER_MINPERIOD;
   opus_val16 gain1 = 0;
   int dual_stereo=0;
   int effectiveBytes;
   int dynalloc_logp;
   opus_int32 vbr_rate;
   opus_int32 total_bits;
   opus_int32 total_boost;
   opus_int32 balance;
   opus_int32 tell;
   opus_int32 tell0_frac;
   int prefilter_tapset=0;
   int pf_on;
   int anti_collapse_rsv;
   int anti_collapse_on=0;
   int silence=0;
   int tf_chan = 0;
   opus_val16 tf_estimate;
   int pitch_change=0;
   opus_int32 tot_boost;
   opus_val32 sample_max;
   opus_val16 maxDepth;
   const OpusCustomMode *mode;
   int nbEBands;
   int overlap;
   const opus_int16 *eBands;
   int secondMdct;
   int signalBandwidth;
   int transient_got_disabled=0;
   opus_val16 surround_masking=0;
   opus_val16 temporal_vbr=0;
   opus_val16 surround_trim = 0;
   opus_int32 equiv_rate;
   int hybrid;
   int weak_transient = 0;
   int enable_tf_analysis;
   VARDECL(opus_val16, surround_dynalloc);
   ALLOC_STACK;

   mode = st->mode;
   nbEBands = mode->nbEBands;
   overlap = mode->overlap;
   eBands = mode->eBands;
   start = st->start;
   end = st->end;
   hybrid = start != 0;
   tf_estimate = 0;
   if (nbCompressedBytes<2 || pcm==NULL)
   {
      RESTORE_STACK;
      return OPUS_BAD_ARG;
   }

   frame_size *= st->upsample;
   for (LM=0;LM<=mode->maxLM;LM++)
      if (mode->shortMdctSize<<LM==frame_size)
         break;
   if (LM>mode->maxLM)
   {
      RESTORE_STACK;
      return OPUS_BAD_ARG;
   }
   M=1<<LM;
   N = M*mode->shortMdctSize;

   prefilter_mem = st->in_mem+CC*(overlap);
   oldBandE = (opus_val16*)(st->in_mem+CC*(overlap+COMBFILTER_MAXPERIOD));
   oldLogE = oldBandE + CC*nbEBands;
   oldLogE2 = oldLogE + CC*nbEBands;
   energyError = oldLogE2 + CC*nbEBands;

   if (enc==NULL)
   {
      tell0_frac=tell=1;
      nbFilledBytes=0;
   } else {
      tell0_frac=ec_tell_frac(enc);
      tell=ec_tell(enc);
      nbFilledBytes=(tell+4)>>3;
   }

   celt_assert(st->signalling==0);

   /* Can't produce more than 1275 output bytes */
   nbCompressedBytes = IMIN(nbCompressedBytes,1275);
   nbAvailableBytes = nbCompressedBytes - nbFilledBytes;

   if (st->vbr && st->bitrate!=OPUS_BITRATE_MAX)
   {
      opus_int32 den=mode->Fs>>BITRES;
      vbr_rate=(st->bitrate*frame_size+(den>>1))/den;
      effectiveBytes = vbr_rate>>(3+BITRES);
   } else {
      opus_int32 tmp;
      vbr_rate = 0;
      tmp = st->bitrate*frame_size;
      if (tell>1)
         tmp += tell*mode->Fs;
      if (st->bitrate!=OPUS_BITRATE_MAX)
      {
         nbCompressedBytes = IMAX(2, IMIN(nbCompressedBytes,
               (tmp+4*mode->Fs)/(8*mode->Fs)-!!st->signalling));
         ec_enc_shrink(enc, nbCompressedBytes);
      }
      effectiveBytes = nbCompressedBytes - nbFilledBytes;
   }
   equiv_rate = ((opus_int32)nbCompressedBytes*8*50 << (3-LM)) - (40*C+20)*((400>>LM) - 50);
   if (st->bitrate != OPUS_BITRATE_MAX)
      equiv_rate = IMIN(equiv_rate, st->bitrate - (40*C+20)*((400>>LM) - 50));

   if (enc==NULL)
   {
      ec_enc_init(&_enc, compressed, nbCompressedBytes);
      enc = &_enc;
   }

   if (vbr_rate>0)
   {
      /* Computes the max bit-rate allowed in VBR mode to avoid violating the
          target rate and buffering.
         We must do this up front so that bust-prevention logic triggers
          correctly if we don't have enough bits. */
      if (st->constrained_vbr)
      {
         opus_int32 vbr_bound;
         opus_int32 max_allowed;
         /* We could use any multiple of vbr_rate as bound (depending on the
             delay).
            This is clamped to ensure we use at least two bytes if the encoder
             was entirely empty, but to allow 0 in hybrid mode. */
         vbr_bound = vbr_rate;
         max_allowed = IMIN(IMAX(tell==1?2:0,
               (vbr_rate+vbr_bound-st->vbr_reservoir)>>(BITRES+3)),
               nbAvailableBytes);
         if(max_allowed < nbAvailableBytes)
         {
            nbCompressedBytes = nbFilledBytes+max_allowed;
            nbAvailableBytes = max_allowed;
            ec_enc_shrink(enc, nbCompressedBytes);
         }
      }
   }
   total_bits = nbCompressedBytes*8;

   effEnd = end;
   if (effEnd > mode->effEBands)
      effEnd = mode->effEBands;

   ALLOC(in, CC*(N+overlap), celt_sig);

   sample_max=MAX32(st->overlap_max, celt_maxabs16(pcm, C*(N-overlap)/st->upsample));
   st->overlap_max=celt_maxabs16(pcm+C*(N-overlap)/st->upsample, C*overlap/st->upsample);
   sample_max=MAX32(sample_max, st->overlap_max);
   silence = (sample_max <= (opus_val16)1/(1<<st->lsb_depth));
   if (tell==1)
      ec_enc_bit_logp(enc, silence, 15);
   else
      silence=0;
   if (silence)
   {
      /*In VBR mode there is no need to send more than the minimum. */
      if (vbr_rate>0)
      {
         effectiveBytes=nbCompressedBytes=IMIN(nbCompressedBytes, nbFilledBytes+2);
         total_bits=nbCompressedBytes*8;
         nbAvailableBytes=2;
         ec_enc_shrink(enc, nbCompressedBytes);
      }
      /* Pretend we've filled all the remaining bits with zeros
            (that's what the initialiser did anyway) */
      tell = nbCompressedBytes*8;
      enc->nbits_total+=tell-ec_tell(enc);
   }
   c=0; do {
      int need_clip=0;
      need_clip = st->clip && sample_max>65536.f;
      celt_preemphasis(pcm+c, in+c*(N+overlap)+overlap, N, CC, st->upsample,
                  mode->preemph, st->preemph_memE+c, need_clip);
   } while (++c<CC);

   /* Find pitch period and gain */
   {
      int enabled;
      int qg;
      enabled = ((st->lfe&&nbAvailableBytes>3) || nbAvailableBytes>12*C) && !hybrid && !silence && !st->disable_pf
            && st->complexity >= 5;

      prefilter_tapset = st->tapset_decision;
      pf_on = run_prefilter(st, in, prefilter_mem, CC, N, prefilter_tapset, &pitch_index, &gain1, &qg, enabled, nbAvailableBytes, &st->analysis);
      if ((gain1 > QCONST16(.4f,15) || st->prefilter_gain > QCONST16(.4f,15)) && (!st->analysis.valid || st->analysis.tonality > .3)
            && (pitch_index > 1.26*st->prefilter_period || pitch_index < .79*st->prefilter_period))
         pitch_change = 1;
      if (pf_on==0)
      {
         if(!hybrid && tell+16<=total_bits)
            ec_enc_bit_logp(enc, 0, 1);
      } else {
         /*This block is not gated by a total bits check only because
           of the nbAvailableBytes check above.*/
         int octave;
         ec_enc_bit_logp(enc, 1, 1);
         pitch_index += 1;
         octave = EC_ILOG(pitch_index)-5;
         ec_enc_uint(enc, octave, 6);
         ec_enc_bits(enc, pitch_index-(16<<octave), 4+octave);
         pitch_index -= 1;
         ec_enc_bits(enc, qg, 3);
         ec_enc_icdf(enc, prefilter_tapset, tapset_icdf, 2);
      }
   }

   isTransient = 0;
   shortBlocks = 0;
   if (st->complexity >= 1 && !st->lfe)
   {
      /* Reduces the likelihood of energy instability on fricatives at low bitrate
         in hybrid mode. It seems like we still want to have real transients on vowels
         though (small SILK quantization offset value). */
      int allow_weak_transients = hybrid && effectiveBytes<15 && st->silk_info.signalType != 2;
      isTransient = transient_analysis(in, N+overlap, CC,
            &tf_estimate, &tf_chan, allow_weak_transients, &weak_transient);
   }
   if (LM>0 && ec_tell(enc)+3<=total_bits)
   {
      if (isTransient)
         shortBlocks = M;
   } else {
      isTransient = 0;
      transient_got_disabled=1;
   }

   ALLOC(freq, CC*N, celt_sig);    ALLOC(bandE,nbEBands*CC, celt_ener);
   ALLOC(bandLogE,nbEBands*CC, opus_val16);

   secondMdct = shortBlocks && st->complexity>=8;
   ALLOC(bandLogE2, C*nbEBands, opus_val16);
   if (secondMdct)
   {
      compute_mdcts(mode, 0, in, freq, C, CC, LM, st->upsample, st->arch);
      compute_band_energies(mode, freq, bandE, effEnd, C, LM, st->arch);
      amp2Log2(mode, effEnd, end, bandE, bandLogE2, C);
      for (c=0;c<C;c++)
      {
         for (i=0;i<end;i++)
            bandLogE2[nbEBands*c+i] += HALF16(SHL16(LM, DB_SHIFT));
      }
   }

   compute_mdcts(mode, shortBlocks, in, freq, C, CC, LM, st->upsample, st->arch);
   /* This should catch any NaN in the CELT input. Since we're not supposed to see any (they're filtered
      at the Opus layer), just abort. */
   celt_assert(!celt_isnan(freq[0]) && (C==1 || !celt_isnan(freq[N])));
   if (CC==2&&C==1)
      tf_chan = 0;
   compute_band_energies(mode, freq, bandE, effEnd, C, LM, st->arch);

   if (st->lfe)
   {
      for (i=2;i<end;i++)
      {
         bandE[i] = IMIN(bandE[i], MULT16_32_Q15(QCONST16(1e-4f,15),bandE[0]));
         bandE[i] = MAX32(bandE[i], EPSILON);
      }
   }
   amp2Log2(mode, effEnd, end, bandE, bandLogE, C);

   ALLOC(surround_dynalloc, C*nbEBands, opus_val16);
   OPUS_CLEAR(surround_dynalloc, end);
   /* This computes how much masking takes place between surround channels */
   if (!hybrid&&st->energy_mask&&!st->lfe)
   {
      int mask_end;
      int midband;
      int count_dynalloc;
      opus_val32 mask_avg=0;
      opus_val32 diff=0;
      int count=0;
      mask_end = IMAX(2,st->lastCodedBands);
      for (c=0;c<C;c++)
      {
         for(i=0;i<mask_end;i++)
         {
            opus_val16 mask;
            mask = MAX16(MIN16(st->energy_mask[nbEBands*c+i],
                   QCONST16(.25f, DB_SHIFT)), -QCONST16(2.0f, DB_SHIFT));
            if (mask > 0)
               mask = HALF16(mask);
            mask_avg += MULT16_16(mask, eBands[i+1]-eBands[i]);
            count += eBands[i+1]-eBands[i];
            diff += MULT16_16(mask, 1+2*i-mask_end);
         }
      }
      celt_assert(count>0);
      mask_avg = DIV32_16(mask_avg,count);
      mask_avg += QCONST16(.2f, DB_SHIFT);
      diff = diff*6/(C*(mask_end-1)*(mask_end+1)*mask_end);
      /* Again, being conservative */
      diff = HALF32(diff);
      diff = MAX32(MIN32(diff, QCONST32(.031f, DB_SHIFT)), -QCONST32(.031f, DB_SHIFT));
      /* Find the band that's in the middle of the coded spectrum */
      for (midband=0;eBands[midband+1] < eBands[mask_end]/2;midband++);
      count_dynalloc=0;
      for(i=0;i<mask_end;i++)
      {
         opus_val32 lin;
         opus_val16 unmask;
         lin = mask_avg + diff*(i-midband);
         if (C==2)
            unmask = MAX16(st->energy_mask[i], st->energy_mask[nbEBands+i]);
         else
            unmask = st->energy_mask[i];
         unmask = MIN16(unmask, QCONST16(.0f, DB_SHIFT));
         unmask -= lin;
         if (unmask > QCONST16(.25f, DB_SHIFT))
         {
            surround_dynalloc[i] = unmask - QCONST16(.25f, DB_SHIFT);
            count_dynalloc++;
         }
      }
      if (count_dynalloc>=3)
      {
         /* If we need dynalloc in many bands, it's probably because our
            initial masking rate was too low. */
         mask_avg += QCONST16(.25f, DB_SHIFT);
         if (mask_avg>0)
         {
            /* Something went really wrong in the original calculations,
               disabling masking. */
            mask_avg = 0;
            diff = 0;
            OPUS_CLEAR(surround_dynalloc, mask_end);
         } else {
            for(i=0;i<mask_end;i++)
               surround_dynalloc[i] = MAX16(0, surround_dynalloc[i]-QCONST16(.25f, DB_SHIFT));
         }
      }
      mask_avg += QCONST16(.2f, DB_SHIFT);
      /* Convert to 1/64th units used for the trim */
      surround_trim = 64*diff;
      /*printf("%d %d ", mask_avg, surround_trim);*/
      surround_masking = mask_avg;
   }
   /* Temporal VBR (but not for LFE) */
   if (!st->lfe)
   {
      opus_val16 follow=-QCONST16(10.0f,DB_SHIFT);
      opus_val32 frame_avg=0;
      opus_val16 offset = shortBlocks?HALF16(SHL16(LM, DB_SHIFT)):0;
      for(i=start;i<end;i++)
      {
         follow = MAX16(follow-QCONST16(1.f, DB_SHIFT), bandLogE[i]-offset);
         if (C==2)
            follow = MAX16(follow, bandLogE[i+nbEBands]-offset);
         frame_avg += follow;
      }
      frame_avg /= (end-start);
      temporal_vbr = SUB16(frame_avg,st->spec_avg);
      temporal_vbr = MIN16(QCONST16(3.f, DB_SHIFT), MAX16(-QCONST16(1.5f, DB_SHIFT), temporal_vbr));
      st->spec_avg += MULT16_16_Q15(QCONST16(.02f, 15), temporal_vbr);
   }
   /*for (i=0;i<21;i++)
      printf("%f ", bandLogE[i]);
   printf("\n");*/

   if (!secondMdct)
   {
      OPUS_COPY(bandLogE2, bandLogE, C*nbEBands);
   }

   /* Last chance to catch any transient we might have missed in the
      time-domain analysis */
   if (LM>0 && ec_tell(enc)+3<=total_bits && !isTransient && st->complexity>=5 && !st->lfe && !hybrid)
   {
      if (patch_transient_decision(bandLogE, oldBandE, nbEBands, start, end, C))
      {
         isTransient = 1;
         shortBlocks = M;
         compute_mdcts(mode, shortBlocks, in, freq, C, CC, LM, st->upsample, st->arch);
         compute_band_energies(mode, freq, bandE, effEnd, C, LM, st->arch);
         amp2Log2(mode, effEnd, end, bandE, bandLogE, C);
         /* Compensate for the scaling of short vs long mdcts */
         for (c=0;c<C;c++)
         {
            for (i=0;i<end;i++)
               bandLogE2[nbEBands*c+i] += HALF16(SHL16(LM, DB_SHIFT));
         }
         tf_estimate = QCONST16(.2f,14);
      }
   }

   if (LM>0 && ec_tell(enc)+3<=total_bits)
      ec_enc_bit_logp(enc, isTransient, 3);

   ALLOC(X, C*N, celt_norm);         
   /* Band normalisation */
   normalise_bands(mode, freq, X, bandE, effEnd, C, M);

   enable_tf_analysis = effectiveBytes>=15*C && !hybrid && st->complexity>=2 && !st->lfe;

   ALLOC(offsets, nbEBands, int);
   ALLOC(importance, nbEBands, int);
   ALLOC(spread_weight, nbEBands, int);

   maxDepth = dynalloc_analysis(bandLogE, bandLogE2, oldBandE, nbEBands, start, end, C, offsets,
         st->lsb_depth, mode->logN, isTransient, st->vbr, st->constrained_vbr,
         eBands, LM, effectiveBytes, &tot_boost, st->lfe, surround_dynalloc, &st->analysis, importance, spread_weight);

   ALLOC(tf_res, nbEBands, int);
   /* Disable variable tf resolution for hybrid and at very low bitrate */
   if (enable_tf_analysis)
   {
      int lambda;
      lambda = IMAX(80, 20480/effectiveBytes + 2);
      tf_select = tf_analysis(mode, effEnd, isTransient, tf_res, lambda, X, N, LM, tf_estimate, tf_chan, importance);
      for (i=effEnd;i<end;i++)
         tf_res[i] = tf_res[effEnd-1];
   } else if (hybrid && weak_transient)
   {
      /* For weak transients, we rely on the fact that improving time resolution using
         TF on a long window is imperfect and will not result in an energy collapse at
         low bitrate. */
      for (i=0;i<end;i++)
         tf_res[i] = 1;
      tf_select=0;
   } else if (hybrid && effectiveBytes<15 && st->silk_info.signalType != 2)
   {
      /* For low bitrate hybrid, we force temporal resolution to 5 ms rather than 2.5 ms. */
      for (i=0;i<end;i++)
         tf_res[i] = 0;
      tf_select=isTransient;
   } else {
      for (i=0;i<end;i++)
         tf_res[i] = isTransient;
      tf_select=0;
   }

   ALLOC(error, C*nbEBands, opus_val16);
   c=0;
   do {
      for (i=start;i<end;i++)
      {
         /* When the energy is stable, slightly bias energy quantization towards
            the previous error to make the gain more stable (a constant offset is
            better than fluctuations). */
         if (ABS32(SUB32(bandLogE[i+c*nbEBands], oldBandE[i+c*nbEBands])) < QCONST16(2.f, DB_SHIFT))
         {
            bandLogE[i+c*nbEBands] -= MULT16_16_Q15(energyError[i+c*nbEBands], QCONST16(0.25f, 15));
         }
      }
   } while (++c < C);
   quant_coarse_energy(mode, start, end, effEnd, bandLogE,
         oldBandE, total_bits, error, enc,
         C, LM, nbAvailableBytes, st->force_intra,
         &st->delayedIntra, st->complexity >= 4, st->loss_rate, st->lfe);

   tf_encode(start, end, isTransient, tf_res, LM, tf_select, enc);

   if (ec_tell(enc)+4<=total_bits)
   {
      if (st->lfe)
      {
         st->tapset_decision = 0;
         st->spread_decision = SPREAD_NORMAL;
      } else if (hybrid)
      {
         if (st->complexity == 0)
            st->spread_decision = SPREAD_NONE;
         else if (isTransient)
            st->spread_decision = SPREAD_NORMAL;
         else
            st->spread_decision = SPREAD_AGGRESSIVE;
      } else if (shortBlocks || st->complexity < 3 || nbAvailableBytes < 10*C)
      {
         if (st->complexity == 0)
            st->spread_decision = SPREAD_NONE;
         else
            st->spread_decision = SPREAD_NORMAL;
      } else {
         /* Disable new spreading+tapset estimator until we can show it works
            better than the old one. So far it seems like spreading_decision()
            works best. */
         {
            st->spread_decision = spreading_decision(mode, X,
                  &st->tonal_average, st->spread_decision, &st->hf_average,
                  &st->tapset_decision, pf_on&&!shortBlocks, effEnd, C, M, spread_weight);
         }
         /*printf("%d %d\n", st->tapset_decision, st->spread_decision);*/
         /*printf("%f %d %f %d\n\n", st->analysis.tonality, st->spread_decision, st->analysis.tonality_slope, st->tapset_decision);*/
      }
      ec_enc_icdf(enc, st->spread_decision, spread_icdf, 5);
   }

   /* For LFE, everything interesting is in the first band */
   if (st->lfe)
      offsets[0] = IMIN(8, effectiveBytes/3);
   ALLOC(cap, nbEBands, int);
   init_caps(mode,cap,LM,C);

   dynalloc_logp = 6;
   total_bits<<=BITRES;
   total_boost = 0;
   tell = ec_tell_frac(enc);
   for (i=start;i<end;i++)
   {
      int width, quanta;
      int dynalloc_loop_logp;
      int boost;
      int j;
      width = C*(eBands[i+1]-eBands[i])<<LM;
      /* quanta is 6 bits, but no more than 1 bit/sample
         and no less than 1/8 bit/sample */
      quanta = IMIN(width<<BITRES, IMAX(6<<BITRES, width));
      dynalloc_loop_logp = dynalloc_logp;
      boost = 0;
      for (j = 0; tell+(dynalloc_loop_logp<<BITRES) < total_bits-total_boost
            && boost < cap[i]; j++)
      {
         int flag;
         flag = j<offsets[i];
         ec_enc_bit_logp(enc, flag, dynalloc_loop_logp);
         tell = ec_tell_frac(enc);
         if (!flag)
            break;
         boost += quanta;
         total_boost += quanta;
         dynalloc_loop_logp = 1;
      }
      /* Making dynalloc more likely */
      if (j)
         dynalloc_logp = IMAX(2, dynalloc_logp-1);
      offsets[i] = boost;
   }

   if (C==2)
   {
      static const opus_val16 intensity_thresholds[21]=
      /* 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19  20  off*/
        {  1, 2, 3, 4, 5, 6, 7, 8,16,24,36,44,50,56,62,67,72,79,88,106,134};
      static const opus_val16 intensity_histeresis[21]=
        {  1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 4, 5, 6,  8, 8};

      /* Always use MS for 2.5 ms frames until we can do a better analysis */
      if (LM!=0)
         dual_stereo = stereo_analysis(mode, X, LM, N);

      st->intensity = hysteresis_decision((opus_val16)(equiv_rate/1000),
            intensity_thresholds, intensity_histeresis, 21, st->intensity);
      st->intensity = IMIN(end,IMAX(start, st->intensity));
   }

   alloc_trim = 5;
   if (tell+(6<<BITRES) <= total_bits - total_boost)
   {
      if (start > 0 || st->lfe)
      {
         st->stereo_saving = 0;
         alloc_trim = 5;
      } else {
         alloc_trim = alloc_trim_analysis(mode, X, bandLogE,
            end, LM, C, N, &st->analysis, &st->stereo_saving, tf_estimate,
            st->intensity, surround_trim, equiv_rate, st->arch);
      }
      ec_enc_icdf(enc, alloc_trim, trim_icdf, 7);
      tell = ec_tell_frac(enc);
   }

   /* Variable bitrate */
   if (vbr_rate>0)
   {
     opus_val16 alpha;
     opus_int32 delta;
     /* The target rate in 8th bits per frame */
     opus_int32 target, base_target;
     opus_int32 min_allowed;
     int lm_diff = mode->maxLM - LM;

     /* Don't attempt to use more than 510 kb/s, even for frames smaller than 20 ms.
        The CELT allocator will just not be able to use more than that anyway. */
     nbCompressedBytes = IMIN(nbCompressedBytes,1275>>(3-LM));
     if (!hybrid)
     {
        base_target = vbr_rate - ((40*C+20)<<BITRES);
     } else {
        base_target = IMAX(0, vbr_rate - ((9*C+4)<<BITRES));
     }

     if (st->constrained_vbr)
        base_target += (st->vbr_offset>>lm_diff);

     if (!hybrid)
     {
        target = compute_vbr(mode, &st->analysis, base_target, LM, equiv_rate,
           st->lastCodedBands, C, st->intensity, st->constrained_vbr,
           st->stereo_saving, tot_boost, tf_estimate, pitch_change, maxDepth,
           st->lfe, st->energy_mask!=NULL, surround_masking,
           temporal_vbr);
     } else {
        target = base_target;
        /* Tonal frames (offset<100) need more bits than noisy (offset>100) ones. */
        if (st->silk_info.offset < 100) target += 12 << BITRES >> (3-LM);
        if (st->silk_info.offset > 100) target -= 18 << BITRES >> (3-LM);
        /* Boosting bitrate on transients and vowels with significant temporal
           spikes. */
        target += (opus_int32)MULT16_16_Q14(tf_estimate-QCONST16(.25f,14), (50<<BITRES));
        /* If we have a strong transient, let's make sure it has enough bits to code
           the first two bands, so that it can use folding rather than noise. */
        if (tf_estimate > QCONST16(.7f,14))
           target = IMAX(target, 50<<BITRES);
     }
     /* The current offset is removed from the target and the space used
        so far is added*/
     target=target+tell;
     /* In VBR mode the frame size must not be reduced so much that it would
         result in the encoder running out of bits.
        The margin of 2 bytes ensures that none of the bust-prevention logic
         in the decoder will have triggered so far. */
     min_allowed = ((tell+total_boost+(1<<(BITRES+3))-1)>>(BITRES+3)) + 2;
     /* Take into account the 37 bits we need to have left in the packet to
        signal a redundant frame in hybrid mode. Creating a shorter packet would
        create an entropy coder desync. */
     if (hybrid)
        min_allowed = IMAX(min_allowed, (tell0_frac+(37<<BITRES)+total_boost+(1<<(BITRES+3))-1)>>(BITRES+3));

     nbAvailableBytes = (target+(1<<(BITRES+2)))>>(BITRES+3);
     nbAvailableBytes = IMAX(min_allowed,nbAvailableBytes);
     nbAvailableBytes = IMIN(nbCompressedBytes,nbAvailableBytes);

     /* By how much did we "miss" the target on that frame */
     delta = target - vbr_rate;

     target=nbAvailableBytes<<(BITRES+3);

     /*If the frame is silent we don't adjust our drift, otherwise
       the encoder will shoot to very high rates after hitting a
       span of silence, but we do allow the bitres to refill.
       This means that we'll undershoot our target in CVBR/VBR modes
       on files with lots of silence. */
     if(silence)
     {
       nbAvailableBytes = 2;
       target = 2*8<<BITRES;
       delta = 0;
     }

     if (st->vbr_count < 970)
     {
        st->vbr_count++;
        alpha = celt_rcp(SHL32(EXTEND32(st->vbr_count+20),16));
     } else
        alpha = QCONST16(.001f,15);
     /* How many bits have we used in excess of what we're allowed */
     if (st->constrained_vbr)
        st->vbr_reservoir += target - vbr_rate;
     /*printf ("%d\n", st->vbr_reservoir);*/

     /* Compute the offset we need to apply in order to reach the target */
     if (st->constrained_vbr)
     {
        st->vbr_drift += (opus_int32)MULT16_32_Q15(alpha,(delta*(1<<lm_diff))-st->vbr_offset-st->vbr_drift);
        st->vbr_offset = -st->vbr_drift;
     }
     /*printf ("%d\n", st->vbr_drift);*/

     if (st->constrained_vbr && st->vbr_reservoir < 0)
     {
        /* We're under the min value -- increase rate */
        int adjust = (-st->vbr_reservoir)/(8<<BITRES);
        /* Unless we're just coding silence */
        nbAvailableBytes += silence?0:adjust;
        st->vbr_reservoir = 0;
        /*printf ("+%d\n", adjust);*/
     }
     nbCompressedBytes = IMIN(nbCompressedBytes,nbAvailableBytes);
     /*printf("%d\n", nbCompressedBytes*50*8);*/
     /* This moves the raw bits to take into account the new compressed size */
     ec_enc_shrink(enc, nbCompressedBytes);
   }

   /* Bit allocation */
   ALLOC(fine_quant, nbEBands, int);
   ALLOC(pulses, nbEBands, int);
   ALLOC(fine_priority, nbEBands, int);

   /* bits =           packet size                    - where we are - safety*/
   bits = (((opus_int32)nbCompressedBytes*8)<<BITRES) - ec_tell_frac(enc) - 1;
   anti_collapse_rsv = isTransient&&LM>=2&&bits>=((LM+2)<<BITRES) ? (1<<BITRES) : 0;
   bits -= anti_collapse_rsv;
   signalBandwidth = end-1;
   if (st->analysis.valid)
   {
      int min_bandwidth;
      if (equiv_rate < (opus_int32)32000*C)
         min_bandwidth = 13;
      else if (equiv_rate < (opus_int32)48000*C)
         min_bandwidth = 16;
      else if (equiv_rate < (opus_int32)60000*C)
         min_bandwidth = 18;
      else  if (equiv_rate < (opus_int32)80000*C)
         min_bandwidth = 19;
      else
         min_bandwidth = 20;
      signalBandwidth = IMAX(st->analysis.bandwidth, min_bandwidth);
   }
   if (st->lfe)
      signalBandwidth = 1;
   codedBands = clt_compute_allocation(mode, start, end, offsets, cap,
         alloc_trim, &st->intensity, &dual_stereo, bits, &balance, pulses,
         fine_quant, fine_priority, C, LM, enc, 1, st->lastCodedBands, signalBandwidth);
   if (st->lastCodedBands)
      st->lastCodedBands = IMIN(st->lastCodedBands+1,IMAX(st->lastCodedBands-1,codedBands));
   else
      st->lastCodedBands = codedBands;

   quant_fine_energy(mode, start, end, oldBandE, error, fine_quant, enc, C);

   /* Residual quantisation */
   ALLOC(collapse_masks, C*nbEBands, unsigned char);
   quant_all_bands(1, mode, start, end, X, C==2 ? X+N : NULL, collapse_masks,
         bandE, pulses, shortBlocks, st->spread_decision,
         dual_stereo, st->intensity, tf_res, nbCompressedBytes*(8<<BITRES)-anti_collapse_rsv,
         balance, enc, LM, codedBands, &st->rng, st->complexity, st->arch, st->disable_inv);

   if (anti_collapse_rsv > 0)
   {
      anti_collapse_on = st->consec_transient<2;
      ec_enc_bits(enc, anti_collapse_on, 1);
   }
   quant_energy_finalise(mode, start, end, oldBandE, error, fine_quant, fine_priority, nbCompressedBytes*8-ec_tell(enc), enc, C);
   OPUS_CLEAR(energyError, nbEBands*CC);
   c=0;
   do {
      for (i=start;i<end;i++)
      {
         energyError[i+c*nbEBands] = MAX16(-QCONST16(0.5f, 15), MIN16(QCONST16(0.5f, 15), error[i+c*nbEBands]));
      }
   } while (++c < C);

   if (silence)
   {
      for (i=0;i<C*nbEBands;i++)
         oldBandE[i] = -QCONST16(28.f,DB_SHIFT);
   }

   st->prefilter_period = pitch_index;
   st->prefilter_gain = gain1;
   st->prefilter_tapset = prefilter_tapset;

   if (CC==2&&C==1) {
      OPUS_COPY(&oldBandE[nbEBands], oldBandE, nbEBands);
   }

   if (!isTransient)
   {
      OPUS_COPY(oldLogE2, oldLogE, CC*nbEBands);
      OPUS_COPY(oldLogE, oldBandE, CC*nbEBands);
   } else {
      for (i=0;i<CC*nbEBands;i++)
         oldLogE[i] = MIN16(oldLogE[i], oldBandE[i]);
   }
   /* In case start or end were to change */
   c=0; do
   {
      for (i=0;i<start;i++)
      {
         oldBandE[c*nbEBands+i]=0;
         oldLogE[c*nbEBands+i]=oldLogE2[c*nbEBands+i]=-QCONST16(28.f,DB_SHIFT);
      }
      for (i=end;i<nbEBands;i++)
      {
         oldBandE[c*nbEBands+i]=0;
         oldLogE[c*nbEBands+i]=oldLogE2[c*nbEBands+i]=-QCONST16(28.f,DB_SHIFT);
      }
   } while (++c<CC);

   if (isTransient || transient_got_disabled)
      st->consec_transient++;
   else
      st->consec_transient=0;
   st->rng = enc->rng;

   /* If there's any room left (can only happen for very high rates),
      it's already filled with zeros */
   ec_enc_done(enc);

   RESTORE_STACK;
   if (ec_get_error(enc))
      return OPUS_INTERNAL_ERROR;
   else
      return nbCompressedBytes;
}
int opus_custom_encoder_ctl(CELTEncoder * OPUS_RESTRICT st, int request, ...)
{
   va_list ap;

   va_start(ap, request);
   switch (request)
   {
      case OPUS_SET_COMPLEXITY_REQUEST:
      {
         int value = va_arg(ap, opus_int32);
         if (value<0 || value>10)
            goto bad_arg;
         st->complexity = value;
      }
      break;
      case CELT_SET_START_BAND_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         if (value<0 || value>=st->mode->nbEBands)
            goto bad_arg;
         st->start = value;
      }
      break;
      case CELT_SET_END_BAND_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         if (value<1 || value>st->mode->nbEBands)
            goto bad_arg;
         st->end = value;
      }
      break;
      case CELT_SET_PREDICTION_REQUEST:
      {
         int value = va_arg(ap, opus_int32);
         if (value<0 || value>2)
            goto bad_arg;
         st->disable_pf = value<=1;
         st->force_intra = value==0;
      }
      break;
      case OPUS_SET_PACKET_LOSS_PERC_REQUEST:
      {
         int value = va_arg(ap, opus_int32);
         if (value<0 || value>100)
            goto bad_arg;
         st->loss_rate = value;
      }
      break;
      case OPUS_SET_VBR_CONSTRAINT_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         st->constrained_vbr = value;
      }
      break;
      case OPUS_SET_VBR_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         st->vbr = value;
      }
      break;
      case OPUS_SET_BITRATE_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         if (value<=500 && value!=OPUS_BITRATE_MAX)
            goto bad_arg;
         value = IMIN(value, 260000*st->channels);
         st->bitrate = value;
      }
      break;
      case CELT_SET_CHANNELS_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         if (value<1 || value>2)
            goto bad_arg;
         st->stream_channels = value;
      }
      break;
      case OPUS_SET_LSB_DEPTH_REQUEST:
      {
          opus_int32 value = va_arg(ap, opus_int32);
          if (value<8 || value>24)
             goto bad_arg;
          st->lsb_depth=value;
      }
      break;
      case OPUS_GET_LSB_DEPTH_REQUEST:
      {
          opus_int32 *value = va_arg(ap, opus_int32*);
          *value=st->lsb_depth;
      }
      break;
      case OPUS_SET_PHASE_INVERSION_DISABLED_REQUEST:
      {
          opus_int32 value = va_arg(ap, opus_int32);
          if(value<0 || value>1)
          {
             goto bad_arg;
          }
          st->disable_inv = value;
      }
      break;
      case OPUS_GET_PHASE_INVERSION_DISABLED_REQUEST:
      {
          opus_int32 *value = va_arg(ap, opus_int32*);
          if (!value)
          {
             goto bad_arg;
          }
          *value = st->disable_inv;
      }
      break;
      case OPUS_RESET_STATE:
      {
         int i;
         opus_val16 *oldBandE, *oldLogE, *oldLogE2;
         oldBandE = (opus_val16*)(st->in_mem+st->channels*(st->mode->overlap+COMBFILTER_MAXPERIOD));
         oldLogE = oldBandE + st->channels*st->mode->nbEBands;
         oldLogE2 = oldLogE + st->channels*st->mode->nbEBands;
         OPUS_CLEAR((char*)&st->ENCODER_RESET_START,
               opus_custom_encoder_get_size(st->mode, st->channels)-
               ((char*)&st->ENCODER_RESET_START - (char*)st));
         for (i=0;i<st->channels*st->mode->nbEBands;i++)
            oldLogE[i]=oldLogE2[i]=-QCONST16(28.f,DB_SHIFT);
         st->vbr_offset = 0;
         st->delayedIntra = 1;
         st->spread_decision = SPREAD_NORMAL;
         st->tonal_average = 256;
         st->hf_average = 0;
         st->tapset_decision = 0;
      }
      break;
      case CELT_SET_SIGNALLING_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         st->signalling = value;
      }
      break;
      case CELT_SET_ANALYSIS_REQUEST:
      {
         AnalysisInfo *info = va_arg(ap, AnalysisInfo *);
         if (info)
            OPUS_COPY(&st->analysis, info, 1);
      }
      break;
      case CELT_SET_SILK_INFO_REQUEST:
      {
         SILKInfo *info = va_arg(ap, SILKInfo *);
         if (info)
            OPUS_COPY(&st->silk_info, info, 1);
      }
      break;
      case CELT_GET_MODE_REQUEST:
      {
         const CELTMode ** value = va_arg(ap, const CELTMode**);
         if (value==0)
            goto bad_arg;
         *value=st->mode;
      }
      break;
      case OPUS_GET_FINAL_RANGE_REQUEST:
      {
         opus_uint32 * value = va_arg(ap, opus_uint32 *);
         if (value==0)
            goto bad_arg;
         *value=st->rng;
      }
      break;
      case OPUS_SET_LFE_REQUEST:
      {
          opus_int32 value = va_arg(ap, opus_int32);
          st->lfe = value;
      }
      break;
      case OPUS_SET_ENERGY_MASK_REQUEST:
      {
          opus_val16 *value = va_arg(ap, opus_val16*);
          st->energy_mask = value;
      }
      break;
      default:
         goto bad_request;
   }
   va_end(ap);
   return OPUS_OK;
bad_arg:
   va_end(ap);
   return OPUS_BAD_ARG;
bad_request:
   va_end(ap);
   return OPUS_UNIMPLEMENTED;
}
void _celt_lpc(
      opus_val16       *_lpc, /* out: [0...p-1] LPC coefficients      */
const opus_val32 *ac,  /* in:  [0...p] autocorrelation values  */
int          p
)
{
   int i, j;
   opus_val32 r;
   opus_val32 error = ac[0];
   float *lpc = _lpc;

   OPUS_CLEAR(lpc, p);
   if (ac[0] > 1e-10f)
   {
      for (i = 0; i < p; i++) {
         /* Sum up this iteration's reflection coefficient */
         opus_val32 rr = 0;
         for (j = 0; j < i; j++)
            rr += MULT32_32_Q31(lpc[j],ac[i - j]);
         rr += SHR32(ac[i + 1],6);
         r = -frac_div32(SHL32(rr,6), error);
         /*  Update LPC coefficients and total error */
         lpc[i] = SHR32(r,6);
         for (j = 0; j < (i+1)>>1; j++)
         {
            opus_val32 tmp1, tmp2;
            tmp1 = lpc[j];
            tmp2 = lpc[i-1-j];
            lpc[j]     = tmp1 + MULT32_32_Q31(r,tmp2);
            lpc[i-1-j] = tmp2 + MULT32_32_Q31(r,tmp1);
         }

         error = error - MULT32_32_Q31(MULT32_32_Q31(r,r),error);
         /* Bail out once we get 30 dB gain */
         if (error<=.001f*ac[0])
            break;
      }
   }
}
int _celt_autocorr(
                   const opus_val16 *x,   /*  in: [0...n-1] samples x   */
                   opus_val32       *ac,  /* out: [0...lag-1] ac values */
                   const opus_val16       *window,
                   int          overlap,
                   int          lag,
                   int          n,
                   int          arch
                  )
{
   opus_val32 d;
   int i, k;
   int fastN=n-lag;
   int shift;
   const opus_val16 *xptr;
   VARDECL(opus_val16, xx);
   SAVE_STACK;
   ALLOC(xx, n, opus_val16);
   celt_assert(n>0);
   celt_assert(overlap>=0);
   if (overlap == 0)
   {
      xptr = x;
   } else {
      for (i=0;i<n;i++)
         xx[i] = x[i];
      for (i=0;i<overlap;i++)
      {
         xx[i] = MULT16_16_Q15(x[i],window[i]);
         xx[n-i-1] = MULT16_16_Q15(x[n-i-1],window[i]);
      }
      xptr = xx;
   }
   shift=0;
   celt_pitch_xcorr(xptr, xptr, ac, fastN, lag+1, arch);
   for (k=0;k<=lag;k++)
   {
      for (i = k+fastN, d = 0; i < n; i++)
         d = MAC16_16(d, xptr[i], xptr[i-k]);
      ac[k] += d;
   }

   RESTORE_STACK;
   return shift;
}
/*Although derived separately, the pulse vector coding scheme is equivalent to
   a Pyramid Vector Quantizer \cite{Fis86}.
  Some additional notes about an early version appear at
   https://people.xiph.org/~tterribe/notes/cwrs.html, but the codebook ordering
   and the definitions of some terms have evolved since that was written.

The conversion from a pulse vector to an integer index (encoding) and back
   (decoding) is governed by two related functions, V(N,K) and U(N,K).

V(N,K) = the number of combinations, with replacement, of N items, taken K
   at a time, when a sign bit is added to each item taken at least once (i.e.,
   the number of N-dimensional unit pulse vectors with K pulses).
  One way to compute this is via
    V(N,K) = K>0 ? sum(k=1...K,2**k*choose(N,k)*choose(K-1,k-1)) : 1,
   where choose() is the binomial function.
  A table of values for N<10 and K<10 looks like:
  V[10][10] = {
    {1,  0,   0,    0,    0,     0,     0,      0,      0,       0},
    {1,  2,   2,    2,    2,     2,     2,      2,      2,       2},
    {1,  4,   8,   12,   16,    20,    24,     28,     32,      36},
    {1,  6,  18,   38,   66,   102,   146,    198,    258,     326},
    {1,  8,  32,   88,  192,   360,   608,    952,   1408,    1992},
    {1, 10,  50,  170,  450,  1002,  1970,   3530,   5890,    9290},
    {1, 12,  72,  292,  912,  2364,  5336,  10836,  20256,   35436},
    {1, 14,  98,  462, 1666,  4942, 12642,  28814,  59906,  115598},
    {1, 16, 128,  688, 2816,  9424, 27008,  68464, 157184,  332688},
    {1, 18, 162,  978, 4482, 16722, 53154, 148626, 374274,  864146}
  };

U(N,K) = the number of such combinations wherein N-1 objects are taken at
   most K-1 at a time.
  This is given by
    U(N,K) = sum(k=0...K-1,V(N-1,k))
           = K>0 ? (V(N-1,K-1) + V(N,K-1))/2 : 0.
  The latter expression also makes clear that U(N,K) is half the number of such
   combinations wherein the first object is taken at least once.
  Although it may not be clear from either of these definitions, U(N,K) is the
   natural function to work with when enumerating the pulse vector codebooks,
   not V(N,K).
  U(N,K) is not well-defined for N=0, but with the extension
    U(0,K) = K>0 ? 0 : 1,
   the function becomes symmetric: U(N,K) = U(K,N), with a similar table:
  U[10][10] = {
    {1, 0,  0,   0,    0,    0,     0,     0,      0,      0},
    {0, 1,  1,   1,    1,    1,     1,     1,      1,      1},
    {0, 1,  3,   5,    7,    9,    11,    13,     15,     17},
    {0, 1,  5,  13,   25,   41,    61,    85,    113,    145},
    {0, 1,  7,  25,   63,  129,   231,   377,    575,    833},
    {0, 1,  9,  41,  129,  321,   681,  1289,   2241,   3649},
    {0, 1, 11,  61,  231,  681,  1683,  3653,   7183,  13073},
    {0, 1, 13,  85,  377, 1289,  3653,  8989,  19825,  40081},
    {0, 1, 15, 113,  575, 2241,  7183, 19825,  48639, 108545},
    {0, 1, 17, 145,  833, 3649, 13073, 40081, 108545, 265729}
  };

With this extension, V(N,K) may be written in terms of U(N,K):
    V(N,K) = U(N,K) + U(N,K+1)
   for all N>=0, K>=0.
  Thus U(N,K+1) represents the number of combinations where the first element
   is positive or zero, and U(N,K) represents the number of combinations where
   it is negative.
  With a large enough table of U(N,K) values, we could write O(N) encoding
   and O(min(N*log(K),N+K)) decoding routines, but such a table would be
   prohibitively large for small embedded devices (K may be as large as 32767
   for small N, and N may be as large as 200).

Both functions obey the same recurrence relation:
    V(N,K) = V(N-1,K) + V(N,K-1) + V(N-1,K-1),
    U(N,K) = U(N-1,K) + U(N,K-1) + U(N-1,K-1),
   for all N>0, K>0, with different initial conditions at N=0 or K=0.
  This allows us to construct a row of one of the tables above given the
   previous row or the next row.
  Thus we can derive O(NK) encoding and decoding routines with O(K) memory
   using only addition and subtraction.

When encoding, we build up from the U(2,K) row and work our way forwards.
  When decoding, we need to start at the U(N,K) row and work our way backwards,
   which requires a means of computing U(N,K).
  U(N,K) may be computed from two previous values with the same N:
    U(N,K) = ((2*N-1)*U(N,K-1) - U(N,K-2))/(K-1) + U(N,K-2)
   for all N>1, and since U(N,K) is symmetric, a similar relation holds for two
   previous values with the same K:
    U(N,K>1) = ((2*K-1)*U(N-1,K) - U(N-2,K))/(N-1) + U(N-2,K)
   for all K>1.
  This allows us to construct an arbitrary row of the U(N,K) table by starting
   with the first two values, which are constants.
  This saves roughly 2/3 the work in our O(NK) decoding routine, but costs O(K)
   multiplications.
  Similar relations can be derived for V(N,K), but are not used here.

For N>0 and K>0, U(N,K) and V(N,K) take on the form of an (N-1)-degree
   polynomial for fixed N.
  The first few are
    U(1,K) = 1,
    U(2,K) = 2*K-1,
    U(3,K) = (2*K-2)*K+1,
    U(4,K) = (((4*K-6)*K+8)*K-3)/3,
    U(5,K) = ((((2*K-4)*K+10)*K-8)*K+3)/3,
   and
    V(1,K) = 2,
    V(2,K) = 4*K,
    V(3,K) = 4*K*K+2,
    V(4,K) = 8*(K*K+2)*K/3,
    V(5,K) = ((4*K*K+20)*K*K+6)/3,
   for all K>0.
  This allows us to derive O(N) encoding and O(N*log(K)) decoding routines for
   small N (and indeed decoding is also O(N) for N<3).

@ARTICLE{Fis86,
    author="Thomas R. Fischer",
    title="A Pyramid Vector Quantizer",
    journal="IEEE Transactions on Information Theory",
    volume="IT-32",
    number=4,
    pages="568--583",
    month=Jul,
    year=1986
  }*/

/*U(N,K) = U(K,N) := N>0?K>0?U(N-1,K)+U(N,K-1)+U(N-1,K-1):0:K>0?1:0*/
#define CELT_PVQ_U(_n,_k) (CELT_PVQ_U_ROW[IMIN(_n,_k)][IMAX(_n,_k)])
/*V(N,K) := U(N,K)+U(N,K+1) = the number of PVQ codewords for a band of size N
   with K pulses allocated to it.*/
#define CELT_PVQ_V(_n,_k) (CELT_PVQ_U(_n,_k)+CELT_PVQ_U(_n,(_k)+1))
/*For each V(N,K) supported, we will access element U(min(N,K+1),max(N,K+1)).
  Thus, the number of entries in row I is the larger of the maximum number of
   pulses we will ever allocate for a given N=I (K=128, or however many fit in
   32 bits, whichever is smaller), plus one, and the maximum N for which
   K=I-1 pulses fit in 32 bits.
  The largest band size in an Opus Custom mode is 208.
  Otherwise, we can limit things to the set of N which can be achieved by
   splitting a band from a standard Opus mode: 176, 144, 96, 88, 72, 64, 48,
   44, 36, 32, 24, 22, 18, 16, 8, 4, 2).*/
static const opus_uint32 CELT_PVQ_U_DATA[1272]={
  /*N=0, K=0...176:*/
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /*N=1, K=1...176:*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /*N=2, K=2...176:*/
  3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41,
  43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79,
  81, 83, 85, 87, 89, 91, 93, 95, 97, 99, 101, 103, 105, 107, 109, 111, 113,
  115, 117, 119, 121, 123, 125, 127, 129, 131, 133, 135, 137, 139, 141, 143,
  145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165, 167, 169, 171, 173,
  175, 177, 179, 181, 183, 185, 187, 189, 191, 193, 195, 197, 199, 201, 203,
  205, 207, 209, 211, 213, 215, 217, 219, 221, 223, 225, 227, 229, 231, 233,
  235, 237, 239, 241, 243, 245, 247, 249, 251, 253, 255, 257, 259, 261, 263,
  265, 267, 269, 271, 273, 275, 277, 279, 281, 283, 285, 287, 289, 291, 293,
  295, 297, 299, 301, 303, 305, 307, 309, 311, 313, 315, 317, 319, 321, 323,
  325, 327, 329, 331, 333, 335, 337, 339, 341, 343, 345, 347, 349, 351,
  /*N=3, K=3...176:*/
  13, 25, 41, 61, 85, 113, 145, 181, 221, 265, 313, 365, 421, 481, 545, 613,
  685, 761, 841, 925, 1013, 1105, 1201, 1301, 1405, 1513, 1625, 1741, 1861,
  1985, 2113, 2245, 2381, 2521, 2665, 2813, 2965, 3121, 3281, 3445, 3613, 3785,
  3961, 4141, 4325, 4513, 4705, 4901, 5101, 5305, 5513, 5725, 5941, 6161, 6385,
  6613, 6845, 7081, 7321, 7565, 7813, 8065, 8321, 8581, 8845, 9113, 9385, 9661,
  9941, 10225, 10513, 10805, 11101, 11401, 11705, 12013, 12325, 12641, 12961,
  13285, 13613, 13945, 14281, 14621, 14965, 15313, 15665, 16021, 16381, 16745,
  17113, 17485, 17861, 18241, 18625, 19013, 19405, 19801, 20201, 20605, 21013,
  21425, 21841, 22261, 22685, 23113, 23545, 23981, 24421, 24865, 25313, 25765,
  26221, 26681, 27145, 27613, 28085, 28561, 29041, 29525, 30013, 30505, 31001,
  31501, 32005, 32513, 33025, 33541, 34061, 34585, 35113, 35645, 36181, 36721,
  37265, 37813, 38365, 38921, 39481, 40045, 40613, 41185, 41761, 42341, 42925,
  43513, 44105, 44701, 45301, 45905, 46513, 47125, 47741, 48361, 48985, 49613,
  50245, 50881, 51521, 52165, 52813, 53465, 54121, 54781, 55445, 56113, 56785,
  57461, 58141, 58825, 59513, 60205, 60901, 61601,
  /*N=4, K=4...176:*/
  63, 129, 231, 377, 575, 833, 1159, 1561, 2047, 2625, 3303, 4089, 4991, 6017,
  7175, 8473, 9919, 11521, 13287, 15225, 17343, 19649, 22151, 24857, 27775,
  30913, 34279, 37881, 41727, 45825, 50183, 54809, 59711, 64897, 70375, 76153,
  82239, 88641, 95367, 102425, 109823, 117569, 125671, 134137, 142975, 152193,
  161799, 171801, 182207, 193025, 204263, 215929, 228031, 240577, 253575,
  267033, 280959, 295361, 310247, 325625, 341503, 357889, 374791, 392217,
  410175, 428673, 447719, 467321, 487487, 508225, 529543, 551449, 573951,
  597057, 620775, 645113, 670079, 695681, 721927, 748825, 776383, 804609,
  833511, 863097, 893375, 924353, 956039, 988441, 1021567, 1055425, 1090023,
  1125369, 1161471, 1198337, 1235975, 1274393, 1313599, 1353601, 1394407,
  1436025, 1478463, 1521729, 1565831, 1610777, 1656575, 1703233, 1750759,
  1799161, 1848447, 1898625, 1949703, 2001689, 2054591, 2108417, 2163175,
  2218873, 2275519, 2333121, 2391687, 2451225, 2511743, 2573249, 2635751,
  2699257, 2763775, 2829313, 2895879, 2963481, 3032127, 3101825, 3172583,
  3244409, 3317311, 3391297, 3466375, 3542553, 3619839, 3698241, 3777767,
  3858425, 3940223, 4023169, 4107271, 4192537, 4278975, 4366593, 4455399,
  4545401, 4636607, 4729025, 4822663, 4917529, 5013631, 5110977, 5209575,
  5309433, 5410559, 5512961, 5616647, 5721625, 5827903, 5935489, 6044391,
  6154617, 6266175, 6379073, 6493319, 6608921, 6725887, 6844225, 6963943,
  7085049, 7207551,
  /*N=5, K=5...176:*/
  321, 681, 1289, 2241, 3649, 5641, 8361, 11969, 16641, 22569, 29961, 39041,
  50049, 63241, 78889, 97281, 118721, 143529, 172041, 204609, 241601, 283401,
  330409, 383041, 441729, 506921, 579081, 658689, 746241, 842249, 947241,
  1061761, 1186369, 1321641, 1468169, 1626561, 1797441, 1981449, 2179241,
  2391489, 2618881, 2862121, 3121929, 3399041, 3694209, 4008201, 4341801,
  4695809, 5071041, 5468329, 5888521, 6332481, 6801089, 7295241, 7815849,
  8363841, 8940161, 9545769, 10181641, 10848769, 11548161, 12280841, 13047849,
  13850241, 14689089, 15565481, 16480521, 17435329, 18431041, 19468809,
  20549801, 21675201, 22846209, 24064041, 25329929, 26645121, 28010881,
  29428489, 30899241, 32424449, 34005441, 35643561, 37340169, 39096641,
  40914369, 42794761, 44739241, 46749249, 48826241, 50971689, 53187081,
  55473921, 57833729, 60268041, 62778409, 65366401, 68033601, 70781609,
  73612041, 76526529, 79526721, 82614281, 85790889, 89058241, 92418049,
  95872041, 99421961, 103069569, 106816641, 110664969, 114616361, 118672641,
  122835649, 127107241, 131489289, 135983681, 140592321, 145317129, 150160041,
  155123009, 160208001, 165417001, 170752009, 176215041, 181808129, 187533321,
  193392681, 199388289, 205522241, 211796649, 218213641, 224775361, 231483969,
  238341641, 245350569, 252512961, 259831041, 267307049, 274943241, 282741889,
  290705281, 298835721, 307135529, 315607041, 324252609, 333074601, 342075401,
  351257409, 360623041, 370174729, 379914921, 389846081, 399970689, 410291241,
  420810249, 431530241, 442453761, 453583369, 464921641, 476471169, 488234561,
  500214441, 512413449, 524834241, 537479489, 550351881, 563454121, 576788929,
  590359041, 604167209, 618216201, 632508801,
  /*N=6, K=6...96:*/
  1683, 3653, 7183, 13073, 22363, 36365, 56695, 85305, 124515, 177045, 246047,
  335137, 448427, 590557, 766727, 982729, 1244979, 1560549, 1937199, 2383409,
  2908411, 3522221, 4235671, 5060441, 6009091, 7095093, 8332863, 9737793,
  11326283, 13115773, 15124775, 17372905, 19880915, 22670725, 25765455,
  29189457, 32968347, 37129037, 41699767, 46710137, 52191139, 58175189,
  64696159, 71789409, 79491819, 87841821, 96879431, 106646281, 117185651,
  128542501, 140763503, 153897073, 167993403, 183104493, 199284183, 216588185,
  235074115, 254801525, 275831935, 298228865, 322057867, 347386557, 374284647,
  402823977, 433078547, 465124549, 499040399, 534906769, 572806619, 612825229,
  655050231, 699571641, 746481891, 795875861, 847850911, 902506913, 959946283,
  1020274013, 1083597703, 1150027593, 1219676595, 1292660325, 1369097135,
  1449108145, 1532817275, 1620351277, 1711839767, 1807415257, 1907213187,
  2011371957, 2120032959,
  /*N=7, K=7...54*/
  8989, 19825, 40081, 75517, 134245, 227305, 369305, 579125, 880685, 1303777,
  1884961, 2668525, 3707509, 5064793, 6814249, 9041957, 11847485, 15345233,
  19665841, 24957661, 31388293, 39146185, 48442297, 59511829, 72616013,
  88043969, 106114625, 127178701, 151620757, 179861305, 212358985, 249612805,
  292164445, 340600625, 395555537, 457713341, 527810725, 606639529, 695049433,
  793950709, 904317037, 1027188385, 1163673953, 1314955181, 1482288821,
  1667010073, 1870535785, 2094367717,
  /*N=8, K=8...37*/
  48639, 108545, 224143, 433905, 795455, 1392065, 2340495, 3800305, 5984767,
  9173505, 13726991, 20103025, 28875327, 40754369, 56610575, 77500017,
  104692735, 139703809, 184327311, 240673265, 311207743, 398796225, 506750351,
  638878193, 799538175, 993696769, 1226990095, 1505789553, 1837271615,
  2229491905U,
  /*N=9, K=9...28:*/
  265729, 598417, 1256465, 2485825, 4673345, 8405905, 14546705, 24331777,
  39490049, 62390545, 96220561, 145198913, 214828609, 312193553, 446304145,
  628496897, 872893441, 1196924561, 1621925137, 2173806145U,
  /*N=10, K=10...24:*/
  1462563, 3317445, 7059735, 14218905, 27298155, 50250765, 89129247, 152951073,
  254831667, 413442773, 654862247, 1014889769, 1541911931, 2300409629U,
  3375210671U,
  /*N=11, K=11...19:*/
  8097453, 18474633, 39753273, 81270333, 158819253, 298199265, 540279585,
  948062325, 1616336765,
  /*N=12, K=12...18:*/
  45046719, 103274625, 224298231, 464387817, 921406335, 1759885185,
  3248227095U,
  /*N=13, K=13...16:*/
  251595969, 579168825, 1267854873, 2653649025U,
  /*N=14, K=14:*/
  1409933619
};
static const opus_uint32 *const CELT_PVQ_U_ROW[15]={
  CELT_PVQ_U_DATA+   0,CELT_PVQ_U_DATA+ 176,CELT_PVQ_U_DATA+ 351,
  CELT_PVQ_U_DATA+ 525,CELT_PVQ_U_DATA+ 698,CELT_PVQ_U_DATA+ 870,
  CELT_PVQ_U_DATA+1041,CELT_PVQ_U_DATA+1131,CELT_PVQ_U_DATA+1178,
  CELT_PVQ_U_DATA+1207,CELT_PVQ_U_DATA+1226,CELT_PVQ_U_DATA+1240,
  CELT_PVQ_U_DATA+1248,CELT_PVQ_U_DATA+1254,CELT_PVQ_U_DATA+1257
};
static opus_uint32 icwrs(int _n,const int *_y){
  opus_uint32 i;
  int         j;
  int         k;
  celt_assert(_n>=2);
  j=_n-1;
  i=_y[j]<0;
  k=abs(_y[j]);
  do{
    j--;
    i+=CELT_PVQ_U(_n-j,k);
    k+=abs(_y[j]);
    if(_y[j]<0)i+=CELT_PVQ_U(_n-j,k+1);
  }
  while(j>0);
  return i;
}
void encode_pulses(const int *_y,int _n,int _k,ec_enc *_enc){
  celt_assert(_k>0);
  ec_enc_uint(_enc,icwrs(_n,_y),CELT_PVQ_V(_n,_k));
}
static opus_val32 cwrsi(int _n,int _k,opus_uint32 _i,int *_y){
  opus_uint32 p;
  int         s;
  int         k0;
  opus_int16  val;
  opus_val32  yy=0;
  celt_assert(_k>0);
  celt_assert(_n>1);
  while(_n>2){
    opus_uint32 q;
    /*Lots of pulses case:*/
    if(_k>=_n){
      const opus_uint32 *row;
      row=CELT_PVQ_U_ROW[_n];
      /*Are the pulses in this dimension negative?*/
      p=row[_k+1];
      s=-(_i>=p);
      _i-=p&s;
      /*Count how many pulses were placed in this dimension.*/
      k0=_k;
      q=row[_n];
      if(q>_i){
        celt_sig_assert(p>q);
        _k=_n;
        do p=CELT_PVQ_U_ROW[--_k][_n];
        while(p>_i);
      }
      else for(p=row[_k];p>_i;p=row[_k])_k--;
      _i-=p;
      val=(k0-_k+s)^s;
      *_y++=val;
      yy=MAC16_16(yy,val,val);
    }
    /*Lots of dimensions case:*/
    else{
      /*Are there any pulses in this dimension at all?*/
      p=CELT_PVQ_U_ROW[_k][_n];
      q=CELT_PVQ_U_ROW[_k+1][_n];
      if(p<=_i&&_i<q){
        _i-=p;
        *_y++=0;
      }
      else{
        /*Are the pulses in this dimension negative?*/
        s=-(_i>=q);
        _i-=q&s;
        /*Count how many pulses were placed in this dimension.*/
        k0=_k;
        do p=CELT_PVQ_U_ROW[--_k][_n];
        while(p>_i);
        _i-=p;
        val=(k0-_k+s)^s;
        *_y++=val;
        yy=MAC16_16(yy,val,val);
      }
    }
    _n--;
  }
  /*_n==2*/
  p=2*_k+1;
  s=-(_i>=p);
  _i-=p&s;
  k0=_k;
  _k=(_i+1)>>1;
  if(_k)_i-=2*_k-1;
  val=(k0-_k+s)^s;
  *_y++=val;
  yy=MAC16_16(yy,val,val);
  /*_n==1*/
  s=-(int)_i;
  val=(_k+s)^s;
  *_y=val;
  yy=MAC16_16(yy,val,val);
  return yy;
}
opus_val32 decode_pulses(int *_y,int _n,int _k,ec_dec *_dec){
  return cwrsi(_n,_k,ec_dec_uint(_dec,CELT_PVQ_V(_n,_k)),_y);
}
/* Copyright (c) 2001-2011 Timothy B. Terriberry
   Copyright (c) 2008-2009 Xiph.Org Foundation */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

- Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* This is a faster version of ec_tell_frac() that takes advantage
   of the low (1/8 bit) resolution to use just a linear function
   followed by a lookup to determine the exact transition thresholds. */
opus_uint32 ec_tell_frac(ec_ctx *_this){
  static const unsigned correction[8] =
    {35733, 38967, 42495, 46340,
     50535, 55109, 60097, 65535};
  opus_uint32 nbits;
  opus_uint32 r;
  int         l;
  unsigned    b;
  nbits=_this->nbits_total<<BITRES;
  l=EC_ILOG(_this->rng);
  r=_this->rng>>(l-16);
  b = (r>>12)-8;
  b += r>correction[b];
  l = (l<<3)+b;
  return nbits-l;
}
/* Copyright (c) 2001-2011 Timothy B. Terriberry
   Copyright (c) 2008-2009 Xiph.Org Foundation */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

- Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*Constants used by the entropy encoder/decoder.*/

/*The number of bits to output at a time.*/
#define EC_SYM_BITS (8)
/*The total number of bits in each of the state registers.*/
#define EC_CODE_BITS (32)
/*The maximum symbol value.*/
#define EC_SYM_MAX ((1U<<EC_SYM_BITS)-1)
/*Bits to shift by to move a symbol into the high-order position.*/
#define EC_CODE_SHIFT (EC_CODE_BITS-EC_SYM_BITS-1)
/*Carry bit of the high-order range symbol.*/
#define EC_CODE_TOP (((opus_uint32)1U)<<(EC_CODE_BITS-1))
/*Low-order bit of the high-order range symbol.*/
#define EC_CODE_BOT (EC_CODE_TOP>>EC_SYM_BITS)
/*The number of bits available for the last, partial symbol in the code field.*/
#define EC_CODE_EXTRA ((EC_CODE_BITS-2)%EC_SYM_BITS+1)
/*A range decoder.
  This is an entropy decoder based upon \cite{Mar79}, which is itself a
   rediscovery of the FIFO arithmetic code introduced by \cite{Pas76}.
  It is very similar to arithmetic encoding, except that encoding is done with
   digits in any base, instead of with bits, and so it is faster when using
   larger bases (i.e.: a byte).
  The author claims an average waste of $\frac{1}{2}\log_b(2b)$ bits, where $b$
   is the base, longer than the theoretical optimum, but to my knowledge there
   is no published justification for this claim.
  This only seems true when using near-infinite precision arithmetic so that
   the process is carried out with no rounding errors.

An excellent description of implementation details is available at
   http://www.arturocampos.com/ac_range.html
  A recent work \cite{MNW98} which proposes several changes to arithmetic
   encoding for efficiency actually re-discovers many of the principles
   behind range encoding, and presents a good theoretical analysis of them.

End of stream is handled by writing out the smallest number of bits that
   ensures that the stream will be correctly decoded regardless of the value of
   any subsequent bits.
  ec_tell() can be used to determine how many bits were needed to decode
   all the symbols thus far; other data can be packed in the remaining bits of
   the input buffer.
  @PHDTHESIS{Pas76,
    author="Richard Clark Pasco",
    title="Source coding algorithms for fast data compression",
    school="Dept. of Electrical Engineering, Stanford University",
    address="Stanford, CA",
    month=May,
    year=1976
  }
  @INPROCEEDINGS{Mar79,
   author="Martin, G.N.N.",
   title="Range encoding: an algorithm for removing redundancy from a digitised
    message",
   booktitle="Video & Data Recording Conference",
   year=1979,
   address="Southampton",
   month=Jul
  }
  @ARTICLE{MNW98,
   author="Alistair Moffat and Radford Neal and Ian H. Witten",
   title="Arithmetic Coding Revisited",
   journal="{ACM} Transactions on Information Systems",
   year=1998,
   volume=16,
   number=3,
   pages="256--294",
   month=Jul,
   URL="http://www.stanford.edu/class/ee398a/handouts/papers/Moffat98ArithmCoding.pdf"
  }*/
static int ec_read_byte(ec_dec *_this){
  return _this->offs<_this->storage?_this->buf[_this->offs++]:0;
}
static int ec_read_byte_from_end(ec_dec *_this){
  return _this->end_offs<_this->storage?
   _this->buf[_this->storage-++(_this->end_offs)]:0;
}
/*Normalizes the contents of val and rng so that rng lies entirely in the
   high-order symbol.*/
static void ec_dec_normalize(ec_dec *_this){
  /*If the range is too small, rescale it and input some bits.*/
  while(_this->rng<=EC_CODE_BOT){
    int sym;
    _this->nbits_total+=EC_SYM_BITS;
    _this->rng<<=EC_SYM_BITS;
    /*Use up the remaining bits from our last symbol.*/
    sym=_this->rem;
    /*Read the next value from the input.*/
    _this->rem=ec_read_byte(_this);
    /*Take the rest of the bits we need from this new symbol.*/
    sym=(sym<<EC_SYM_BITS|_this->rem)>>(EC_SYM_BITS-EC_CODE_EXTRA);
    /*And subtract them from val, capped to be less than EC_CODE_TOP.*/
    _this->val=((_this->val<<EC_SYM_BITS)+(EC_SYM_MAX&~sym))&(EC_CODE_TOP-1);
  }
}
unsigned ec_decode(ec_dec *_this,unsigned _ft){
  unsigned s;
  _this->ext=celt_udiv(_this->rng,_ft);
  s=(unsigned)(_this->val/_this->ext);
  return _ft-EC_MINI(s+1,_ft);
}
void ec_dec_update(ec_dec *_this,unsigned _fl,unsigned _fh,unsigned _ft){
  opus_uint32 s;
  s=IMUL32(_this->ext,_ft-_fh);
  _this->val-=s;
  _this->rng=_fl>0?IMUL32(_this->ext,_fh-_fl):_this->rng-s;
  ec_dec_normalize(_this);
}
/*The probability of having a "one" is 1/(1<<_logp).*/
int ec_dec_bit_logp(ec_dec *_this,unsigned _logp){
  opus_uint32 r;
  opus_uint32 d;
  opus_uint32 s;
  int         ret;
  r=_this->rng;
  d=_this->val;
  s=r>>_logp;
  ret=d<s;
  if(!ret)_this->val=d-s;
  _this->rng=ret?s:r-s;
  ec_dec_normalize(_this);
  return ret;
}
opus_uint32 ec_dec_uint(ec_dec *_this,opus_uint32 _ft){
  unsigned ft;
  unsigned s;
  int      ftb;
  /*In order to optimize EC_ILOG(), it is undefined for the value 0.*/
  celt_assert(_ft>1);
  _ft--;
  ftb=EC_ILOG(_ft);
  if(ftb>EC_UINT_BITS){
    opus_uint32 t;
    ftb-=EC_UINT_BITS;
    ft=(unsigned)(_ft>>ftb)+1;
    s=ec_decode(_this,ft);
    ec_dec_update(_this,s,s+1,ft);
    t=(opus_uint32)s<<ftb|ec_dec_bits(_this,ftb);
    if(t<=_ft)return t;
    _this->error=1;
    return _ft;
  }
  else{
    _ft++;
    s=ec_decode(_this,(unsigned)_ft);
    ec_dec_update(_this,s,s+1,(unsigned)_ft);
    return s;
  }
}
opus_uint32 ec_dec_bits(ec_dec *_this,unsigned _bits){
  ec_window   window;
  int         available;
  opus_uint32 ret;
  window=_this->end_window;
  available=_this->nend_bits;
  if((unsigned)available<_bits){
    do{
      window|=(ec_window)ec_read_byte_from_end(_this)<<available;
      available+=EC_SYM_BITS;
    }
    while(available<=EC_WINDOW_SIZE-EC_SYM_BITS);
  }
  ret=(opus_uint32)window&(((opus_uint32)1<<_bits)-1U);
  window>>=_bits;
  available-=_bits;
  _this->end_window=window;
  _this->nend_bits=available;
  _this->nbits_total+=_bits;
  return ret;
}
/*A range encoder.
  See entdec.c and the references for implementation details \cite{Mar79,MNW98}.

@INPROCEEDINGS{Mar79,
   author="Martin, G.N.N.",
   title="Range encoding: an algorithm for removing redundancy from a digitised
    message",
   booktitle="Video \& Data Recording Conference",
   year=1979,
   address="Southampton",
   month=Jul
  }
  @ARTICLE{MNW98,
   author="Alistair Moffat and Radford Neal and Ian H. Witten",
   title="Arithmetic Coding Revisited",
   journal="{ACM} Transactions on Information Systems",
   year=1998,
   volume=16,
   number=3,
   pages="256--294",
   month=Jul,
   URL="http://www.stanford.edu/class/ee398/handouts/papers/Moffat98ArithmCoding.pdf"
  }*/
static int ec_write_byte(ec_enc *_this,unsigned _value){
  if(_this->offs+_this->end_offs>=_this->storage)return -1;
  _this->buf[_this->offs++]=(unsigned char)_value;
  return 0;
}
static int ec_write_byte_at_end(ec_enc *_this,unsigned _value){
  if(_this->offs+_this->end_offs>=_this->storage)return -1;
  _this->buf[_this->storage-++(_this->end_offs)]=(unsigned char)_value;
  return 0;
}
/*Outputs a symbol, with a carry bit.
  If there is a potential to propagate a carry over several symbols, they are
   buffered until it can be determined whether or not an actual carry will
   occur.
  If the counter for the buffered symbols overflows, then the stream becomes
   undecodable.
  This gives a theoretical limit of a few billion symbols in a single packet on
   32-bit systems.
  The alternative is to truncate the range in order to force a carry, but
   requires similar carry tracking in the decoder, needlessly slowing it down.*/
static void ec_enc_carry_out(ec_enc *_this,int _c){
  if(_c!=EC_SYM_MAX){
    /*No further carry propagation possible, flush buffer.*/
    int carry;
    carry=_c>>EC_SYM_BITS;
    /*Don't output a byte on the first write.
      This compare should be taken care of by branch-prediction thereafter.*/
    if(_this->rem>=0)_this->error|=ec_write_byte(_this,_this->rem+carry);
    if(_this->ext>0){
      unsigned sym;
      sym=(EC_SYM_MAX+carry)&EC_SYM_MAX;
      do _this->error|=ec_write_byte(_this,sym);
      while(--(_this->ext)>0);
    }
    _this->rem=_c&EC_SYM_MAX;
  }
  else _this->ext++;
}
static OPUS_INLINE void ec_enc_normalize(ec_enc *_this){
  /*If the range is too small, output some bits and rescale it.*/
  while(_this->rng<=EC_CODE_BOT){
    ec_enc_carry_out(_this,(int)(_this->val>>EC_CODE_SHIFT));
    /*Move the next-to-high-order symbol into the high-order position.*/
    _this->val=(_this->val<<EC_SYM_BITS)&(EC_CODE_TOP-1);
    _this->rng<<=EC_SYM_BITS;
    _this->nbits_total+=EC_SYM_BITS;
  }
}
void ec_enc_init(ec_enc *_this,unsigned char *_buf,opus_uint32 _size){
  _this->buf=_buf;
  _this->end_offs=0;
  _this->end_window=0;
  _this->nend_bits=0;
  /*This is the offset from which ec_tell() will subtract partial bits.*/
  _this->nbits_total=EC_CODE_BITS+1;
  _this->offs=0;
  _this->rng=EC_CODE_TOP;
  _this->rem=-1;
  _this->val=0;
  _this->ext=0;
  _this->storage=_size;
  _this->error=0;
}
void ec_encode(ec_enc *_this,unsigned _fl,unsigned _fh,unsigned _ft){
  opus_uint32 r;
  r=celt_udiv(_this->rng,_ft);
  if(_fl>0){
    _this->val+=_this->rng-IMUL32(r,(_ft-_fl));
    _this->rng=IMUL32(r,(_fh-_fl));
  }
  else _this->rng-=IMUL32(r,(_ft-_fh));
  ec_enc_normalize(_this);
}
void ec_encode_bin(ec_enc *_this,unsigned _fl,unsigned _fh,unsigned _bits){
  opus_uint32 r;
  r=_this->rng>>_bits;
  if(_fl>0){
    _this->val+=_this->rng-IMUL32(r,((1U<<_bits)-_fl));
    _this->rng=IMUL32(r,(_fh-_fl));
  }
  else _this->rng-=IMUL32(r,((1U<<_bits)-_fh));
  ec_enc_normalize(_this);
}
/*The probability of having a "one" is 1/(1<<_logp).*/
void ec_enc_bit_logp(ec_enc *_this,int _val,unsigned _logp){
  opus_uint32 r;
  opus_uint32 s;
  opus_uint32 l;
  r=_this->rng;
  l=_this->val;
  s=r>>_logp;
  r-=s;
  if(_val)_this->val=l+r;
  _this->rng=_val?s:r;
  ec_enc_normalize(_this);
}
void ec_enc_icdf(ec_enc *_this,int _s,const unsigned char *_icdf,unsigned _ftb){
  opus_uint32 r;
  r=_this->rng>>_ftb;
  if(_s>0){
    _this->val+=_this->rng-IMUL32(r,_icdf[_s-1]);
    _this->rng=IMUL32(r,_icdf[_s-1]-_icdf[_s]);
  }
  else _this->rng-=IMUL32(r,_icdf[_s]);
  ec_enc_normalize(_this);
}
void ec_enc_uint(ec_enc *_this,opus_uint32 _fl,opus_uint32 _ft){
  unsigned  ft;
  unsigned  fl;
  int       ftb;
  /*In order to optimize EC_ILOG(), it is undefined for the value 0.*/
  celt_assert(_ft>1);
  _ft--;
  ftb=EC_ILOG(_ft);
  if(ftb>EC_UINT_BITS){
    ftb-=EC_UINT_BITS;
    ft=(_ft>>ftb)+1;
    fl=(unsigned)(_fl>>ftb);
    ec_encode(_this,fl,fl+1,ft);
    ec_enc_bits(_this,_fl&(((opus_uint32)1<<ftb)-1U),ftb);
  }
  else ec_encode(_this,_fl,_fl+1,_ft+1);
}
void ec_enc_bits(ec_enc *_this,opus_uint32 _fl,unsigned _bits){
  ec_window window;
  int       used;
  window=_this->end_window;
  used=_this->nend_bits;
  celt_assert(_bits>0);
  if(used+_bits>EC_WINDOW_SIZE){
    do{
      _this->error|=ec_write_byte_at_end(_this,(unsigned)window&EC_SYM_MAX);
      window>>=EC_SYM_BITS;
      used-=EC_SYM_BITS;
    }
    while(used>=EC_SYM_BITS);
  }
  window|=(ec_window)_fl<<used;
  used+=_bits;
  _this->end_window=window;
  _this->nend_bits=used;
  _this->nbits_total+=_bits;
}
void ec_enc_shrink(ec_enc *_this,opus_uint32 _size){
  celt_assert(_this->offs+_this->end_offs<=_size);
  OPUS_MOVE(_this->buf+_size-_this->end_offs,
   _this->buf+_this->storage-_this->end_offs,_this->end_offs);
  _this->storage=_size;
}
void ec_enc_done(ec_enc *_this){
  ec_window   window;
  int         used;
  opus_uint32 msk;
  opus_uint32 end;
  int         l;
  /*We output the minimum number of bits that ensures that the symbols encoded
     thus far will be decoded correctly regardless of the bits that follow.*/
  l=EC_CODE_BITS-EC_ILOG(_this->rng);
  msk=(EC_CODE_TOP-1)>>l;
  end=(_this->val+msk)&~msk;
  if((end|msk)>=_this->val+_this->rng){
    l++;
    msk>>=1;
    end=(_this->val+msk)&~msk;
  }
  while(l>0){
    ec_enc_carry_out(_this,(int)(end>>EC_CODE_SHIFT));
    end=(end<<EC_SYM_BITS)&(EC_CODE_TOP-1);
    l-=EC_SYM_BITS;
  }
  /*If we have a buffered byte flush it into the output buffer.*/
  if(_this->rem>=0||_this->ext>0)ec_enc_carry_out(_this,0);
  /*If we have buffered extra bits, flush them as well.*/
  window=_this->end_window;
  used=_this->nend_bits;
  while(used>=EC_SYM_BITS){
    _this->error|=ec_write_byte_at_end(_this,(unsigned)window&EC_SYM_MAX);
    window>>=EC_SYM_BITS;
    used-=EC_SYM_BITS;
  }
  /*Clear any excess space and add any remaining extra bits to the last byte.*/
  if(!_this->error){
    OPUS_CLEAR(_this->buf+_this->offs,
     _this->storage-_this->offs-_this->end_offs);
    if(used>0){
      /*If there's no range coder data at all, give up.*/
      if(_this->end_offs>=_this->storage)_this->error=-1;
      else{
        l=-l;
        /*If we've busted, don't add too many extra bits to the last byte; it
           would corrupt the range coder data, and that's more important.*/
        if(_this->offs+_this->end_offs>=_this->storage&&l<used){
          window&=(1<<l)-1;
          _this->error=-1;
        }
        _this->buf[_this->storage-_this->end_offs-1]|=(unsigned char)window;
      }
    }
  }
}
/* kiss_fft.h
   defines kiss_fft_scalar as either short or a float type
   and defines
   typedef struct { kiss_fft_scalar r; kiss_fft_scalar i; }kiss_fft_cpx; */

/*
  Explanation of macros dealing with complex math:

C_MUL(m,a,b)         : m = a*b
   C_FIXDIV( c , div )  : if a fixed point impl., c /= div. noop otherwise
   C_SUB( res, a,b)     : res = a - b
   C_SUBFROM( res , a)  : res -= a
   C_ADDTO( res , a)    : res += a
 * */
#define S_MUL(a,b) ( (a)*(b) )
#define C_MUL(m,a,b) do{ (m).r = (a).r*(b).r - (a).i*(b).i; (m).i = (a).r*(b).i + (a).i*(b).r; }while(0)
#define C_MULBYSCALAR(c,s) do{ (c).r *= (s); (c).i *= (s); }while(0)
#define CHECK_OVERFLOW_OP(a,op,b) 
#define C_ADD(res,a,b) do { CHECK_OVERFLOW_OP((a).r,+,(b).r) CHECK_OVERFLOW_OP((a).i,+,(b).i) (res).r=(a).r+(b).r; (res).i=(a).i+(b).i; }while(0)
#define C_SUB(res,a,b) do { CHECK_OVERFLOW_OP((a).r,-,(b).r) CHECK_OVERFLOW_OP((a).i,-,(b).i) (res).r=(a).r-(b).r; (res).i=(a).i-(b).i; }while(0)
#define C_ADDTO(res,a) do { CHECK_OVERFLOW_OP((res).r,+,(a).r) CHECK_OVERFLOW_OP((res).i,+,(a).i) (res).r += (a).r; (res).i += (a).i; }while(0)
#define HALF_OF(x) ((x)*.5f)
/* The guts header contains all the multiplication and addition macros that are defined for
   complex numbers.  It also delares the kf_ internal functions.
*/
static void kf_bfly2(
                     kiss_fft_cpx * Fout,
                     int m,
                     int N
                    )
{
   kiss_fft_cpx * Fout2;
   int i;
   (void)m;
   {
      opus_val16 tw;
      tw = QCONST16(0.7071067812f, 15);
      /* We know that m==4 here because the radix-2 is just after a radix-4 */
      celt_assert(m==4);
      for (i=0;i<N;i++)
      {
         kiss_fft_cpx t;
         Fout2 = Fout + 4;
         t = Fout2[0];
         C_SUB( Fout2[0] ,  Fout[0] , t );
         C_ADDTO( Fout[0] ,  t );

         t.r = S_MUL(ADD32_ovflw(Fout2[1].r, Fout2[1].i), tw);
         t.i = S_MUL(SUB32_ovflw(Fout2[1].i, Fout2[1].r), tw);
         C_SUB( Fout2[1] ,  Fout[1] , t );
         C_ADDTO( Fout[1] ,  t );

         t.r = Fout2[2].i;
         t.i = -Fout2[2].r;
         C_SUB( Fout2[2] ,  Fout[2] , t );
         C_ADDTO( Fout[2] ,  t );

         t.r = S_MUL(SUB32_ovflw(Fout2[3].i, Fout2[3].r), tw);
         t.i = S_MUL(NEG32_ovflw(ADD32_ovflw(Fout2[3].i, Fout2[3].r)), tw);
         C_SUB( Fout2[3] ,  Fout[3] , t );
         C_ADDTO( Fout[3] ,  t );
         Fout += 8;
      }
   }
}
static void kf_bfly4(
                     kiss_fft_cpx * Fout,
                     const size_t fstride,
                     const kiss_fft_state *st,
                     int m,
                     int N,
                     int mm
                    )
{
   int i;

   if (m==1)
   {
      /* Degenerate case where all the twiddles are 1. */
      for (i=0;i<N;i++)
      {
         kiss_fft_cpx scratch0, scratch1;

         C_SUB( scratch0 , *Fout, Fout[2] );
         C_ADDTO(*Fout, Fout[2]);
         C_ADD( scratch1 , Fout[1] , Fout[3] );
         C_SUB( Fout[2], *Fout, scratch1 );
         C_ADDTO( *Fout , scratch1 );
         C_SUB( scratch1 , Fout[1] , Fout[3] );

         Fout[1].r = ADD32_ovflw(scratch0.r, scratch1.i);
         Fout[1].i = SUB32_ovflw(scratch0.i, scratch1.r);
         Fout[3].r = SUB32_ovflw(scratch0.r, scratch1.i);
         Fout[3].i = ADD32_ovflw(scratch0.i, scratch1.r);
         Fout+=4;
      }
   } else {
      int j;
      kiss_fft_cpx scratch[6];
      const kiss_twiddle_cpx *tw1,*tw2,*tw3;
      const int m2=2*m;
      const int m3=3*m;
      kiss_fft_cpx * Fout_beg = Fout;
      for (i=0;i<N;i++)
      {
         Fout = Fout_beg + i*mm;
         tw3 = tw2 = tw1 = st->twiddles;
         /* m is guaranteed to be a multiple of 4. */
         for (j=0;j<m;j++)
         {
            C_MUL(scratch[0],Fout[m] , *tw1 );
            C_MUL(scratch[1],Fout[m2] , *tw2 );
            C_MUL(scratch[2],Fout[m3] , *tw3 );

            C_SUB( scratch[5] , *Fout, scratch[1] );
            C_ADDTO(*Fout, scratch[1]);
            C_ADD( scratch[3] , scratch[0] , scratch[2] );
            C_SUB( scratch[4] , scratch[0] , scratch[2] );
            C_SUB( Fout[m2], *Fout, scratch[3] );
            tw1 += fstride;
            tw2 += fstride*2;
            tw3 += fstride*3;
            C_ADDTO( *Fout , scratch[3] );

            Fout[m].r = ADD32_ovflw(scratch[5].r, scratch[4].i);
            Fout[m].i = SUB32_ovflw(scratch[5].i, scratch[4].r);
            Fout[m3].r = SUB32_ovflw(scratch[5].r, scratch[4].i);
            Fout[m3].i = ADD32_ovflw(scratch[5].i, scratch[4].r);
            ++Fout;
         }
      }
   }
}
static void kf_bfly3(
                     kiss_fft_cpx * Fout,
                     const size_t fstride,
                     const kiss_fft_state *st,
                     int m,
                     int N,
                     int mm
                    )
{
   int i;
   size_t k;
   const size_t m2 = 2*m;
   const kiss_twiddle_cpx *tw1,*tw2;
   kiss_fft_cpx scratch[5];
   kiss_twiddle_cpx epi3;

   kiss_fft_cpx * Fout_beg = Fout;
   epi3 = st->twiddles[fstride*m];
   for (i=0;i<N;i++)
   {
      Fout = Fout_beg + i*mm;
      tw1=tw2=st->twiddles;
      /* For non-custom modes, m is guaranteed to be a multiple of 4. */
      k=m;
      do {

         C_MUL(scratch[1],Fout[m] , *tw1);
         C_MUL(scratch[2],Fout[m2] , *tw2);

         C_ADD(scratch[3],scratch[1],scratch[2]);
         C_SUB(scratch[0],scratch[1],scratch[2]);
         tw1 += fstride;
         tw2 += fstride*2;

         Fout[m].r = SUB32_ovflw(Fout->r, HALF_OF(scratch[3].r));
         Fout[m].i = SUB32_ovflw(Fout->i, HALF_OF(scratch[3].i));

         C_MULBYSCALAR( scratch[0] , epi3.i );

         C_ADDTO(*Fout,scratch[3]);

         Fout[m2].r = ADD32_ovflw(Fout[m].r, scratch[0].i);
         Fout[m2].i = SUB32_ovflw(Fout[m].i, scratch[0].r);

         Fout[m].r = SUB32_ovflw(Fout[m].r, scratch[0].i);
         Fout[m].i = ADD32_ovflw(Fout[m].i, scratch[0].r);

         ++Fout;
      } while(--k);
   }
}
static void kf_bfly5(
                     kiss_fft_cpx * Fout,
                     const size_t fstride,
                     const kiss_fft_state *st,
                     int m,
                     int N,
                     int mm
                    )
{
   kiss_fft_cpx *Fout0,*Fout1,*Fout2,*Fout3,*Fout4;
   int i, u;
   kiss_fft_cpx scratch[13];
   const kiss_twiddle_cpx *tw;
   kiss_twiddle_cpx ya,yb;
   kiss_fft_cpx * Fout_beg = Fout;

   ya = st->twiddles[fstride*m];
   yb = st->twiddles[fstride*2*m];
   tw=st->twiddles;

   for (i=0;i<N;i++)
   {
      Fout = Fout_beg + i*mm;
      Fout0=Fout;
      Fout1=Fout0+m;
      Fout2=Fout0+2*m;
      Fout3=Fout0+3*m;
      Fout4=Fout0+4*m;

      /* For non-custom modes, m is guaranteed to be a multiple of 4. */
      for ( u=0; u<m; ++u ) {
         scratch[0] = *Fout0;

         C_MUL(scratch[1] ,*Fout1, tw[u*fstride]);
         C_MUL(scratch[2] ,*Fout2, tw[2*u*fstride]);
         C_MUL(scratch[3] ,*Fout3, tw[3*u*fstride]);
         C_MUL(scratch[4] ,*Fout4, tw[4*u*fstride]);

         C_ADD( scratch[7],scratch[1],scratch[4]);
         C_SUB( scratch[10],scratch[1],scratch[4]);
         C_ADD( scratch[8],scratch[2],scratch[3]);
         C_SUB( scratch[9],scratch[2],scratch[3]);

         Fout0->r = ADD32_ovflw(Fout0->r, ADD32_ovflw(scratch[7].r, scratch[8].r));
         Fout0->i = ADD32_ovflw(Fout0->i, ADD32_ovflw(scratch[7].i, scratch[8].i));

         scratch[5].r = ADD32_ovflw(scratch[0].r, ADD32_ovflw(S_MUL(scratch[7].r,ya.r), S_MUL(scratch[8].r,yb.r)));
         scratch[5].i = ADD32_ovflw(scratch[0].i, ADD32_ovflw(S_MUL(scratch[7].i,ya.r), S_MUL(scratch[8].i,yb.r)));

         scratch[6].r =  ADD32_ovflw(S_MUL(scratch[10].i,ya.i), S_MUL(scratch[9].i,yb.i));
         scratch[6].i = NEG32_ovflw(ADD32_ovflw(S_MUL(scratch[10].r,ya.i), S_MUL(scratch[9].r,yb.i)));

         C_SUB(*Fout1,scratch[5],scratch[6]);
         C_ADD(*Fout4,scratch[5],scratch[6]);

         scratch[11].r = ADD32_ovflw(scratch[0].r, ADD32_ovflw(S_MUL(scratch[7].r,yb.r), S_MUL(scratch[8].r,ya.r)));
         scratch[11].i = ADD32_ovflw(scratch[0].i, ADD32_ovflw(S_MUL(scratch[7].i,yb.r), S_MUL(scratch[8].i,ya.r)));
         scratch[12].r = SUB32_ovflw(S_MUL(scratch[9].i,ya.i), S_MUL(scratch[10].i,yb.i));
         scratch[12].i = SUB32_ovflw(S_MUL(scratch[10].r,yb.i), S_MUL(scratch[9].r,ya.i));

         C_ADD(*Fout2,scratch[11],scratch[12]);
         C_SUB(*Fout3,scratch[11],scratch[12]);

         ++Fout0;++Fout1;++Fout2;++Fout3;++Fout4;
      }
   }
}
void opus_fft_impl(const kiss_fft_state *st,kiss_fft_cpx *fout)
{
    int m2, m;
    int p;
    int L;
    int fstride[MAXFACTORS];
    int i;
    int shift;

    /* st->shift can be -1 */
    shift = st->shift>0 ? st->shift : 0;

    fstride[0] = 1;
    L=0;
    do {
       p = st->factors[2*L];
       m = st->factors[2*L+1];
       fstride[L+1] = fstride[L]*p;
       L++;
    } while(m!=1);
    m = st->factors[2*L-1];
    for (i=L-1;i>=0;i--)
    {
       if (i!=0)
          m2 = st->factors[2*i-1];
       else
          m2 = 1;
       switch (st->factors[2*i])
       {
       case 2:
          kf_bfly2(fout, m, fstride[i]);
          break;
       case 4:
          kf_bfly4(fout,fstride[i]<<shift,st,m, fstride[i], m2);
          break;
        case 3:
          kf_bfly3(fout,fstride[i]<<shift,st,m, fstride[i], m2);
          break;
       case 5:
          kf_bfly5(fout,fstride[i]<<shift,st,m, fstride[i], m2);
          break;
        }
       m = m2;
    }
}
void opus_fft_c(const kiss_fft_state *st,const kiss_fft_cpx *fin,kiss_fft_cpx *fout)
{
   int i;
   opus_val16 scale;
   scale = st->scale;

   celt_assert2 (fin != fout, "In-place FFT not supported");
   /* Bit-reverse the input */
   for (i=0;i<st->nfft;i++)
   {
      kiss_fft_cpx x = fin[i];
      fout[st->bitrev[i]].r = SHR32(MULT16_32_Q16(scale, x.r), scale_shift);
      fout[st->bitrev[i]].i = SHR32(MULT16_32_Q16(scale, x.i), scale_shift);
   }
   opus_fft_impl(st, fout);
}
void ec_laplace_encode(ec_enc *enc, int *value, unsigned fs, int decay);
/* The minimum probability of an energy delta (out of 32768). */
#define LAPLACE_LOG_MINP (0)
#define LAPLACE_MINP (1<<LAPLACE_LOG_MINP)
/* The minimum number of guaranteed representable energy deltas (in one
    direction). */
#define LAPLACE_NMIN (16)
/* When called, decay is positive and at most 11456. */
static unsigned ec_laplace_get_freq1(unsigned fs0, int decay)
{
   unsigned ft;
   ft = 32768 - LAPLACE_MINP*(2*LAPLACE_NMIN) - fs0;
   return ft*(opus_int32)(16384-decay)>>15;
}
void ec_laplace_encode(ec_enc *enc, int *value, unsigned fs, int decay)
{
   unsigned fl;
   int val = *value;
   fl = 0;
   if (val)
   {
      int s;
      int i;
      s = -(val<0);
      val = (val+s)^s;
      fl = fs;
      fs = ec_laplace_get_freq1(fs, decay);
      /* Search the decaying part of the PDF.*/
      for (i=1; fs > 0 && i < val; i++)
      {
         fs *= 2;
         fl += fs+2*LAPLACE_MINP;
         fs = (fs*(opus_int32)decay)>>15;
      }
      /* Everything beyond that has probability LAPLACE_MINP. */
      if (!fs)
      {
         int di;
         int ndi_max;
         ndi_max = (32768-fl+LAPLACE_MINP-1)>>LAPLACE_LOG_MINP;
         ndi_max = (ndi_max-s)>>1;
         di = IMIN(val - i, ndi_max - 1);
         fl += (2*di+1+s)*LAPLACE_MINP;
         fs = IMIN(LAPLACE_MINP, 32768-fl);
         *value = (i+di+s)^s;
      }
      else
      {
         fs += LAPLACE_MINP;
         fl += fs&~s;
      }
      celt_assert(fl+fs<=32768);
      celt_assert(fs>0);
   }
   ec_encode_bin(enc, fl, fl+fs, 15);
}
/*Compute floor(sqrt(_val)) with exact arithmetic.
  _val must be greater than 0.
  This has been tested on all possible 32-bit inputs greater than 0.*/
unsigned isqrt32(opus_uint32 _val){
  unsigned b;
  unsigned g;
  int      bshift;
  /*Uses the second method from
     http://www.azillionmonkeys.com/qed/sqroot.html
    The main idea is to search for the largest binary digit b such that
     (g+b)*(g+b) <= _val, and add it to the solution g.*/
  g=0;
  bshift=(EC_ILOG(_val)-1)>>1;
  b=1U<<bshift;
  do{
    opus_uint32 t;
    t=(((opus_uint32)g<<1)+b)<<bshift;
    if(t<=_val){
      g+=b;
      _val-=t;
    }
    b>>=1;
    bshift--;
  }
  while(bshift>=0);
  return g;
}
/* Forward MDCT trashes the input array */
void clt_mdct_forward_c(const mdct_lookup *l, kiss_fft_scalar *in, kiss_fft_scalar * OPUS_RESTRICT out,
      const opus_val16 *window, int overlap, int shift, int stride, int arch)
{
   int i;
   int N, N2, N4;
   VARDECL(kiss_fft_scalar, f);
   VARDECL(kiss_fft_cpx, f2);
   const kiss_fft_state *st = l->kfft[shift];
   const kiss_twiddle_scalar *trig;
   opus_val16 scale;
   SAVE_STACK;
   (void)arch;
   scale = st->scale;

   N = l->n;
   trig = l->trig;
   for (i=0;i<shift;i++)
   {
      N >>= 1;
      trig += N;
   }
   N2 = N>>1;
   N4 = N>>2;

   ALLOC(f, N2, kiss_fft_scalar);
   ALLOC(f2, N4, kiss_fft_cpx);

   /* Consider the input to be composed of four blocks: [a, b, c, d] */
   /* Window, shuffle, fold */
   {
      /* Temp pointers to make it really clear to the compiler what we're doing */
      const kiss_fft_scalar * OPUS_RESTRICT xp1 = in+(overlap>>1);
      const kiss_fft_scalar * OPUS_RESTRICT xp2 = in+N2-1+(overlap>>1);
      kiss_fft_scalar * OPUS_RESTRICT yp = f;
      const opus_val16 * OPUS_RESTRICT wp1 = window+(overlap>>1);
      const opus_val16 * OPUS_RESTRICT wp2 = window+(overlap>>1)-1;
      for(i=0;i<((overlap+3)>>2);i++)
      {
         /* Real part arranged as -d-cR, Imag part arranged as -b+aR*/
         *yp++ = MULT16_32_Q15(*wp2, xp1[N2]) + MULT16_32_Q15(*wp1,*xp2);
         *yp++ = MULT16_32_Q15(*wp1, *xp1)    - MULT16_32_Q15(*wp2, xp2[-N2]);
         xp1+=2;
         xp2-=2;
         wp1+=2;
         wp2-=2;
      }
      wp1 = window;
      wp2 = window+overlap-1;
      for(;i<N4-((overlap+3)>>2);i++)
      {
         /* Real part arranged as a-bR, Imag part arranged as -c-dR */
         *yp++ = *xp2;
         *yp++ = *xp1;
         xp1+=2;
         xp2-=2;
      }
      for(;i<N4;i++)
      {
         /* Real part arranged as a-bR, Imag part arranged as -c-dR */
         *yp++ =  -MULT16_32_Q15(*wp1, xp1[-N2]) + MULT16_32_Q15(*wp2, *xp2);
         *yp++ = MULT16_32_Q15(*wp2, *xp1)     + MULT16_32_Q15(*wp1, xp2[N2]);
         xp1+=2;
         xp2-=2;
         wp1+=2;
         wp2-=2;
      }
   }
   /* Pre-rotation */
   {
      kiss_fft_scalar * OPUS_RESTRICT yp = f;
      const kiss_twiddle_scalar *t = &trig[0];
      for(i=0;i<N4;i++)
      {
         kiss_fft_cpx yc;
         kiss_twiddle_scalar t0, t1;
         kiss_fft_scalar re, im, yr, yi;
         t0 = t[i];
         t1 = t[N4+i];
         re = *yp++;
         im = *yp++;
         yr = S_MUL(re,t0)  -  S_MUL(im,t1);
         yi = S_MUL(im,t0)  +  S_MUL(re,t1);
         yc.r = yr;
         yc.i = yi;
         yc.r = PSHR32(MULT16_32_Q16(scale, yc.r), scale_shift);
         yc.i = PSHR32(MULT16_32_Q16(scale, yc.i), scale_shift);
         f2[st->bitrev[i]] = yc;
      }
   }

   /* N/4 complex FFT, does not downscale anymore */
   opus_fft_impl(st, f2);

   /* Post-rotate */
   {
      /* Temp pointers to make it really clear to the compiler what we're doing */
      const kiss_fft_cpx * OPUS_RESTRICT fp = f2;
      kiss_fft_scalar * OPUS_RESTRICT yp1 = out;
      kiss_fft_scalar * OPUS_RESTRICT yp2 = out+stride*(N2-1);
      const kiss_twiddle_scalar *t = &trig[0];
      /* Temp pointers to make it really clear to the compiler what we're doing */
      for(i=0;i<N4;i++)
      {
         kiss_fft_scalar yr, yi;
         yr = S_MUL(fp->i,t[N4+i]) - S_MUL(fp->r,t[i]);
         yi = S_MUL(fp->r,t[N4+i]) + S_MUL(fp->i,t[i]);
         *yp1 = yr;
         *yp2 = yi;
         fp++;
         yp1 += 2*stride;
         yp2 -= 2*stride;
      }
   }
   RESTORE_STACK;
}
static const opus_int16 eband5ms[] = {
/*0  200 400 600 800  1k 1.2 1.4 1.6  2k 2.4 2.8 3.2  4k 4.8 5.6 6.8  8k 9.6 12k 15.6 */
  0,  1,  2,  3,  4,  5,  6,  7,  8, 10, 12, 14, 16, 20, 24, 28, 34, 40, 48, 60, 78, 100
};
/* Bit allocation table in units of 1/32 bit/sample (0.1875 dB SNR) */
static const unsigned char band_allocation[] = {
/*0  200 400 600 800  1k 1.2 1.4 1.6  2k 2.4 2.8 3.2  4k 4.8 5.6 6.8  8k 9.6 12k 15.6 */
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 90, 80, 75, 69, 63, 56, 49, 40, 34, 29, 20, 18, 10,  0,  0,  0,  0,  0,  0,  0,  0,
110,100, 90, 84, 78, 71, 65, 58, 51, 45, 39, 32, 26, 20, 12,  0,  0,  0,  0,  0,  0,
118,110,103, 93, 86, 80, 75, 70, 65, 59, 53, 47, 40, 31, 23, 15,  4,  0,  0,  0,  0,
126,119,112,104, 95, 89, 83, 78, 72, 66, 60, 54, 47, 39, 32, 25, 17, 12,  1,  0,  0,
134,127,120,114,103, 97, 91, 85, 78, 72, 66, 60, 54, 47, 41, 35, 29, 23, 16, 10,  1,
144,137,130,124,113,107,101, 95, 88, 82, 76, 70, 64, 57, 51, 45, 39, 33, 26, 15,  1,
152,145,138,132,123,117,111,105, 98, 92, 86, 80, 74, 67, 61, 55, 49, 43, 36, 20,  1,
162,155,148,142,133,127,121,115,108,102, 96, 90, 84, 77, 71, 65, 59, 53, 46, 30,  1,
172,165,158,152,143,137,131,125,118,112,106,100, 94, 87, 81, 75, 69, 63, 56, 45, 20,
200,200,200,200,200,200,200,200,198,193,188,183,178,173,168,163,158,153,148,129,104,
};
static const opus_val16 window120[120] = {
6.7286966e-05f, 0.00060551348f, 0.0016815970f, 0.0032947962f, 0.0054439943f,
0.0081276923f, 0.011344001f, 0.015090633f, 0.019364886f, 0.024163635f,
0.029483315f, 0.035319905f, 0.041668911f, 0.048525347f, 0.055883718f,
0.063737999f, 0.072081616f, 0.080907428f, 0.090207705f, 0.099974111f,
0.11019769f, 0.12086883f, 0.13197729f, 0.14351214f, 0.15546177f,
0.16781389f, 0.18055550f, 0.19367290f, 0.20715171f, 0.22097682f,
0.23513243f, 0.24960208f, 0.26436860f, 0.27941419f, 0.29472040f,
0.31026818f, 0.32603788f, 0.34200931f, 0.35816177f, 0.37447407f,
0.39092462f, 0.40749142f, 0.42415215f, 0.44088423f, 0.45766484f,
0.47447104f, 0.49127978f, 0.50806798f, 0.52481261f, 0.54149077f,
0.55807973f, 0.57455701f, 0.59090049f, 0.60708841f, 0.62309951f,
0.63891306f, 0.65450896f, 0.66986776f, 0.68497077f, 0.69980010f,
0.71433873f, 0.72857055f, 0.74248043f, 0.75605424f, 0.76927895f,
0.78214257f, 0.79463430f, 0.80674445f, 0.81846456f, 0.82978733f,
0.84070669f, 0.85121779f, 0.86131698f, 0.87100183f, 0.88027111f,
0.88912479f, 0.89756398f, 0.90559094f, 0.91320904f, 0.92042270f,
0.92723738f, 0.93365955f, 0.93969656f, 0.94535671f, 0.95064907f,
0.95558353f, 0.96017067f, 0.96442171f, 0.96834849f, 0.97196334f,
0.97527906f, 0.97830883f, 0.98106616f, 0.98356480f, 0.98581869f,
0.98784191f, 0.98964856f, 0.99125274f, 0.99266849f, 0.99390969f,
0.99499004f, 0.99592297f, 0.99672162f, 0.99739874f, 0.99796667f,
0.99843728f, 0.99882195f, 0.99913147f, 0.99937606f, 0.99956527f,
0.99970802f, 0.99981248f, 0.99988613f, 0.99993565f, 0.99996697f,
0.99998518f, 0.99999457f, 0.99999859f, 0.99999982f, 1.0000000f,
};
static const opus_int16 logN400[21] = {
0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 16, 16, 16, 21, 21, 24, 29, 34, 36, };
static const opus_int16 cache_index50[105] = {
-1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 41, 41, 41,
82, 82, 123, 164, 200, 222, 0, 0, 0, 0, 0, 0, 0, 0, 41,
41, 41, 41, 123, 123, 123, 164, 164, 240, 266, 283, 295, 41, 41, 41,
41, 41, 41, 41, 41, 123, 123, 123, 123, 240, 240, 240, 266, 266, 305,
318, 328, 336, 123, 123, 123, 123, 123, 123, 123, 123, 240, 240, 240, 240,
305, 305, 305, 318, 318, 343, 351, 358, 364, 240, 240, 240, 240, 240, 240,
240, 240, 305, 305, 305, 305, 343, 343, 343, 351, 351, 370, 376, 382, 387,
};
static const unsigned char cache_bits50[392] = {
40, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 40, 15, 23, 28,
31, 34, 36, 38, 39, 41, 42, 43, 44, 45, 46, 47, 47, 49, 50,
51, 52, 53, 54, 55, 55, 57, 58, 59, 60, 61, 62, 63, 63, 65,
66, 67, 68, 69, 70, 71, 71, 40, 20, 33, 41, 48, 53, 57, 61,
64, 66, 69, 71, 73, 75, 76, 78, 80, 82, 85, 87, 89, 91, 92,
94, 96, 98, 101, 103, 105, 107, 108, 110, 112, 114, 117, 119, 121, 123,
124, 126, 128, 40, 23, 39, 51, 60, 67, 73, 79, 83, 87, 91, 94,
97, 100, 102, 105, 107, 111, 115, 118, 121, 124, 126, 129, 131, 135, 139,
142, 145, 148, 150, 153, 155, 159, 163, 166, 169, 172, 174, 177, 179, 35,
28, 49, 65, 78, 89, 99, 107, 114, 120, 126, 132, 136, 141, 145, 149,
153, 159, 165, 171, 176, 180, 185, 189, 192, 199, 205, 211, 216, 220, 225,
229, 232, 239, 245, 251, 21, 33, 58, 79, 97, 112, 125, 137, 148, 157,
166, 174, 182, 189, 195, 201, 207, 217, 227, 235, 243, 251, 17, 35, 63,
86, 106, 123, 139, 152, 165, 177, 187, 197, 206, 214, 222, 230, 237, 250,
25, 31, 55, 75, 91, 105, 117, 128, 138, 146, 154, 161, 168, 174, 180,
185, 190, 200, 208, 215, 222, 229, 235, 240, 245, 255, 16, 36, 65, 89,
110, 128, 144, 159, 173, 185, 196, 207, 217, 226, 234, 242, 250, 11, 41,
74, 103, 128, 151, 172, 191, 209, 225, 241, 255, 9, 43, 79, 110, 138,
163, 186, 207, 227, 246, 12, 39, 71, 99, 123, 144, 164, 182, 198, 214,
228, 241, 253, 9, 44, 81, 113, 142, 168, 192, 214, 235, 255, 7, 49,
90, 127, 160, 191, 220, 247, 6, 51, 95, 134, 170, 203, 234, 7, 47,
87, 123, 155, 184, 212, 237, 6, 52, 97, 137, 174, 208, 240, 5, 57,
106, 151, 192, 231, 5, 59, 111, 158, 202, 243, 5, 55, 103, 147, 187,
224, 5, 60, 113, 161, 206, 248, 4, 65, 122, 175, 224, 4, 67, 127,
182, 234, };
static const unsigned char cache_caps50[168] = {
224, 224, 224, 224, 224, 224, 224, 224, 160, 160, 160, 160, 185, 185, 185,
178, 178, 168, 134, 61, 37, 224, 224, 224, 224, 224, 224, 224, 224, 240,
240, 240, 240, 207, 207, 207, 198, 198, 183, 144, 66, 40, 160, 160, 160,
160, 160, 160, 160, 160, 185, 185, 185, 185, 193, 193, 193, 183, 183, 172,
138, 64, 38, 240, 240, 240, 240, 240, 240, 240, 240, 207, 207, 207, 207,
204, 204, 204, 193, 193, 180, 143, 66, 40, 185, 185, 185, 185, 185, 185,
185, 185, 193, 193, 193, 193, 193, 193, 193, 183, 183, 172, 138, 65, 39,
207, 207, 207, 207, 207, 207, 207, 207, 204, 204, 204, 204, 201, 201, 201,
188, 188, 176, 141, 66, 40, 193, 193, 193, 193, 193, 193, 193, 193, 193,
193, 193, 193, 194, 194, 194, 184, 184, 173, 139, 65, 39, 204, 204, 204,
204, 204, 204, 204, 204, 201, 201, 201, 201, 198, 198, 198, 187, 187, 175,
140, 66, 40, };
static const kiss_twiddle_cpx fft_twiddles48000_960[480] = {
{1.0000000f, -0.0000000f}, {0.99991433f, -0.013089596f},
{0.99965732f, -0.026176948f}, {0.99922904f, -0.039259816f},
{0.99862953f, -0.052335956f}, {0.99785892f, -0.065403129f},
{0.99691733f, -0.078459096f}, {0.99580493f, -0.091501619f},
{0.99452190f, -0.10452846f}, {0.99306846f, -0.11753740f},
{0.99144486f, -0.13052619f}, {0.98965139f, -0.14349262f},
{0.98768834f, -0.15643447f}, {0.98555606f, -0.16934950f},
{0.98325491f, -0.18223553f}, {0.98078528f, -0.19509032f},
{0.97814760f, -0.20791169f}, {0.97534232f, -0.22069744f},
{0.97236992f, -0.23344536f}, {0.96923091f, -0.24615329f},
{0.96592583f, -0.25881905f}, {0.96245524f, -0.27144045f},
{0.95881973f, -0.28401534f}, {0.95501994f, -0.29654157f},
{0.95105652f, -0.30901699f}, {0.94693013f, -0.32143947f},
{0.94264149f, -0.33380686f}, {0.93819134f, -0.34611706f},
{0.93358043f, -0.35836795f}, {0.92880955f, -0.37055744f},
{0.92387953f, -0.38268343f}, {0.91879121f, -0.39474386f},
{0.91354546f, -0.40673664f}, {0.90814317f, -0.41865974f},
{0.90258528f, -0.43051110f}, {0.89687274f, -0.44228869f},
{0.89100652f, -0.45399050f}, {0.88498764f, -0.46561452f},
{0.87881711f, -0.47715876f}, {0.87249601f, -0.48862124f},
{0.86602540f, -0.50000000f}, {0.85940641f, -0.51129309f},
{0.85264016f, -0.52249856f}, {0.84572782f, -0.53361452f},
{0.83867057f, -0.54463904f}, {0.83146961f, -0.55557023f},
{0.82412619f, -0.56640624f}, {0.81664156f, -0.57714519f},
{0.80901699f, -0.58778525f}, {0.80125381f, -0.59832460f},
{0.79335334f, -0.60876143f}, {0.78531693f, -0.61909395f},
{0.77714596f, -0.62932039f}, {0.76884183f, -0.63943900f},
{0.76040597f, -0.64944805f}, {0.75183981f, -0.65934582f},
{0.74314483f, -0.66913061f}, {0.73432251f, -0.67880075f},
{0.72537437f, -0.68835458f}, {0.71630194f, -0.69779046f},
{0.70710678f, -0.70710678f}, {0.69779046f, -0.71630194f},
{0.68835458f, -0.72537437f}, {0.67880075f, -0.73432251f},
{0.66913061f, -0.74314483f}, {0.65934582f, -0.75183981f},
{0.64944805f, -0.76040597f}, {0.63943900f, -0.76884183f},
{0.62932039f, -0.77714596f}, {0.61909395f, -0.78531693f},
{0.60876143f, -0.79335334f}, {0.59832460f, -0.80125381f},
{0.58778525f, -0.80901699f}, {0.57714519f, -0.81664156f},
{0.56640624f, -0.82412619f}, {0.55557023f, -0.83146961f},
{0.54463904f, -0.83867057f}, {0.53361452f, -0.84572782f},
{0.52249856f, -0.85264016f}, {0.51129309f, -0.85940641f},
{0.50000000f, -0.86602540f}, {0.48862124f, -0.87249601f},
{0.47715876f, -0.87881711f}, {0.46561452f, -0.88498764f},
{0.45399050f, -0.89100652f}, {0.44228869f, -0.89687274f},
{0.43051110f, -0.90258528f}, {0.41865974f, -0.90814317f},
{0.40673664f, -0.91354546f}, {0.39474386f, -0.91879121f},
{0.38268343f, -0.92387953f}, {0.37055744f, -0.92880955f},
{0.35836795f, -0.93358043f}, {0.34611706f, -0.93819134f},
{0.33380686f, -0.94264149f}, {0.32143947f, -0.94693013f},
{0.30901699f, -0.95105652f}, {0.29654157f, -0.95501994f},
{0.28401534f, -0.95881973f}, {0.27144045f, -0.96245524f},
{0.25881905f, -0.96592583f}, {0.24615329f, -0.96923091f},
{0.23344536f, -0.97236992f}, {0.22069744f, -0.97534232f},
{0.20791169f, -0.97814760f}, {0.19509032f, -0.98078528f},
{0.18223553f, -0.98325491f}, {0.16934950f, -0.98555606f},
{0.15643447f, -0.98768834f}, {0.14349262f, -0.98965139f},
{0.13052619f, -0.99144486f}, {0.11753740f, -0.99306846f},
{0.10452846f, -0.99452190f}, {0.091501619f, -0.99580493f},
{0.078459096f, -0.99691733f}, {0.065403129f, -0.99785892f},
{0.052335956f, -0.99862953f}, {0.039259816f, -0.99922904f},
{0.026176948f, -0.99965732f}, {0.013089596f, -0.99991433f},
{6.1230318e-17f, -1.0000000f}, {-0.013089596f, -0.99991433f},
{-0.026176948f, -0.99965732f}, {-0.039259816f, -0.99922904f},
{-0.052335956f, -0.99862953f}, {-0.065403129f, -0.99785892f},
{-0.078459096f, -0.99691733f}, {-0.091501619f, -0.99580493f},
{-0.10452846f, -0.99452190f}, {-0.11753740f, -0.99306846f},
{-0.13052619f, -0.99144486f}, {-0.14349262f, -0.98965139f},
{-0.15643447f, -0.98768834f}, {-0.16934950f, -0.98555606f},
{-0.18223553f, -0.98325491f}, {-0.19509032f, -0.98078528f},
{-0.20791169f, -0.97814760f}, {-0.22069744f, -0.97534232f},
{-0.23344536f, -0.97236992f}, {-0.24615329f, -0.96923091f},
{-0.25881905f, -0.96592583f}, {-0.27144045f, -0.96245524f},
{-0.28401534f, -0.95881973f}, {-0.29654157f, -0.95501994f},
{-0.30901699f, -0.95105652f}, {-0.32143947f, -0.94693013f},
{-0.33380686f, -0.94264149f}, {-0.34611706f, -0.93819134f},
{-0.35836795f, -0.93358043f}, {-0.37055744f, -0.92880955f},
{-0.38268343f, -0.92387953f}, {-0.39474386f, -0.91879121f},
{-0.40673664f, -0.91354546f}, {-0.41865974f, -0.90814317f},
{-0.43051110f, -0.90258528f}, {-0.44228869f, -0.89687274f},
{-0.45399050f, -0.89100652f}, {-0.46561452f, -0.88498764f},
{-0.47715876f, -0.87881711f}, {-0.48862124f, -0.87249601f},
{-0.50000000f, -0.86602540f}, {-0.51129309f, -0.85940641f},
{-0.52249856f, -0.85264016f}, {-0.53361452f, -0.84572782f},
{-0.54463904f, -0.83867057f}, {-0.55557023f, -0.83146961f},
{-0.56640624f, -0.82412619f}, {-0.57714519f, -0.81664156f},
{-0.58778525f, -0.80901699f}, {-0.59832460f, -0.80125381f},
{-0.60876143f, -0.79335334f}, {-0.61909395f, -0.78531693f},
{-0.62932039f, -0.77714596f}, {-0.63943900f, -0.76884183f},
{-0.64944805f, -0.76040597f}, {-0.65934582f, -0.75183981f},
{-0.66913061f, -0.74314483f}, {-0.67880075f, -0.73432251f},
{-0.68835458f, -0.72537437f}, {-0.69779046f, -0.71630194f},
{-0.70710678f, -0.70710678f}, {-0.71630194f, -0.69779046f},
{-0.72537437f, -0.68835458f}, {-0.73432251f, -0.67880075f},
{-0.74314483f, -0.66913061f}, {-0.75183981f, -0.65934582f},
{-0.76040597f, -0.64944805f}, {-0.76884183f, -0.63943900f},
{-0.77714596f, -0.62932039f}, {-0.78531693f, -0.61909395f},
{-0.79335334f, -0.60876143f}, {-0.80125381f, -0.59832460f},
{-0.80901699f, -0.58778525f}, {-0.81664156f, -0.57714519f},
{-0.82412619f, -0.56640624f}, {-0.83146961f, -0.55557023f},
{-0.83867057f, -0.54463904f}, {-0.84572782f, -0.53361452f},
{-0.85264016f, -0.52249856f}, {-0.85940641f, -0.51129309f},
{-0.86602540f, -0.50000000f}, {-0.87249601f, -0.48862124f},
{-0.87881711f, -0.47715876f}, {-0.88498764f, -0.46561452f},
{-0.89100652f, -0.45399050f}, {-0.89687274f, -0.44228869f},
{-0.90258528f, -0.43051110f}, {-0.90814317f, -0.41865974f},
{-0.91354546f, -0.40673664f}, {-0.91879121f, -0.39474386f},
{-0.92387953f, -0.38268343f}, {-0.92880955f, -0.37055744f},
{-0.93358043f, -0.35836795f}, {-0.93819134f, -0.34611706f},
{-0.94264149f, -0.33380686f}, {-0.94693013f, -0.32143947f},
{-0.95105652f, -0.30901699f}, {-0.95501994f, -0.29654157f},
{-0.95881973f, -0.28401534f}, {-0.96245524f, -0.27144045f},
{-0.96592583f, -0.25881905f}, {-0.96923091f, -0.24615329f},
{-0.97236992f, -0.23344536f}, {-0.97534232f, -0.22069744f},
{-0.97814760f, -0.20791169f}, {-0.98078528f, -0.19509032f},
{-0.98325491f, -0.18223553f}, {-0.98555606f, -0.16934950f},
{-0.98768834f, -0.15643447f}, {-0.98965139f, -0.14349262f},
{-0.99144486f, -0.13052619f}, {-0.99306846f, -0.11753740f},
{-0.99452190f, -0.10452846f}, {-0.99580493f, -0.091501619f},
{-0.99691733f, -0.078459096f}, {-0.99785892f, -0.065403129f},
{-0.99862953f, -0.052335956f}, {-0.99922904f, -0.039259816f},
{-0.99965732f, -0.026176948f}, {-0.99991433f, -0.013089596f},
{-1.0000000f, -1.2246064e-16f}, {-0.99991433f, 0.013089596f},
{-0.99965732f, 0.026176948f}, {-0.99922904f, 0.039259816f},
{-0.99862953f, 0.052335956f}, {-0.99785892f, 0.065403129f},
{-0.99691733f, 0.078459096f}, {-0.99580493f, 0.091501619f},
{-0.99452190f, 0.10452846f}, {-0.99306846f, 0.11753740f},
{-0.99144486f, 0.13052619f}, {-0.98965139f, 0.14349262f},
{-0.98768834f, 0.15643447f}, {-0.98555606f, 0.16934950f},
{-0.98325491f, 0.18223553f}, {-0.98078528f, 0.19509032f},
{-0.97814760f, 0.20791169f}, {-0.97534232f, 0.22069744f},
{-0.97236992f, 0.23344536f}, {-0.96923091f, 0.24615329f},
{-0.96592583f, 0.25881905f}, {-0.96245524f, 0.27144045f},
{-0.95881973f, 0.28401534f}, {-0.95501994f, 0.29654157f},
{-0.95105652f, 0.30901699f}, {-0.94693013f, 0.32143947f},
{-0.94264149f, 0.33380686f}, {-0.93819134f, 0.34611706f},
{-0.93358043f, 0.35836795f}, {-0.92880955f, 0.37055744f},
{-0.92387953f, 0.38268343f}, {-0.91879121f, 0.39474386f},
{-0.91354546f, 0.40673664f}, {-0.90814317f, 0.41865974f},
{-0.90258528f, 0.43051110f}, {-0.89687274f, 0.44228869f},
{-0.89100652f, 0.45399050f}, {-0.88498764f, 0.46561452f},
{-0.87881711f, 0.47715876f}, {-0.87249601f, 0.48862124f},
{-0.86602540f, 0.50000000f}, {-0.85940641f, 0.51129309f},
{-0.85264016f, 0.52249856f}, {-0.84572782f, 0.53361452f},
{-0.83867057f, 0.54463904f}, {-0.83146961f, 0.55557023f},
{-0.82412619f, 0.56640624f}, {-0.81664156f, 0.57714519f},
{-0.80901699f, 0.58778525f}, {-0.80125381f, 0.59832460f},
{-0.79335334f, 0.60876143f}, {-0.78531693f, 0.61909395f},
{-0.77714596f, 0.62932039f}, {-0.76884183f, 0.63943900f},
{-0.76040597f, 0.64944805f}, {-0.75183981f, 0.65934582f},
{-0.74314483f, 0.66913061f}, {-0.73432251f, 0.67880075f},
{-0.72537437f, 0.68835458f}, {-0.71630194f, 0.69779046f},
{-0.70710678f, 0.70710678f}, {-0.69779046f, 0.71630194f},
{-0.68835458f, 0.72537437f}, {-0.67880075f, 0.73432251f},
{-0.66913061f, 0.74314483f}, {-0.65934582f, 0.75183981f},
{-0.64944805f, 0.76040597f}, {-0.63943900f, 0.76884183f},
{-0.62932039f, 0.77714596f}, {-0.61909395f, 0.78531693f},
{-0.60876143f, 0.79335334f}, {-0.59832460f, 0.80125381f},
{-0.58778525f, 0.80901699f}, {-0.57714519f, 0.81664156f},
{-0.56640624f, 0.82412619f}, {-0.55557023f, 0.83146961f},
{-0.54463904f, 0.83867057f}, {-0.53361452f, 0.84572782f},
{-0.52249856f, 0.85264016f}, {-0.51129309f, 0.85940641f},
{-0.50000000f, 0.86602540f}, {-0.48862124f, 0.87249601f},
{-0.47715876f, 0.87881711f}, {-0.46561452f, 0.88498764f},
{-0.45399050f, 0.89100652f}, {-0.44228869f, 0.89687274f},
{-0.43051110f, 0.90258528f}, {-0.41865974f, 0.90814317f},
{-0.40673664f, 0.91354546f}, {-0.39474386f, 0.91879121f},
{-0.38268343f, 0.92387953f}, {-0.37055744f, 0.92880955f},
{-0.35836795f, 0.93358043f}, {-0.34611706f, 0.93819134f},
{-0.33380686f, 0.94264149f}, {-0.32143947f, 0.94693013f},
{-0.30901699f, 0.95105652f}, {-0.29654157f, 0.95501994f},
{-0.28401534f, 0.95881973f}, {-0.27144045f, 0.96245524f},
{-0.25881905f, 0.96592583f}, {-0.24615329f, 0.96923091f},
{-0.23344536f, 0.97236992f}, {-0.22069744f, 0.97534232f},
{-0.20791169f, 0.97814760f}, {-0.19509032f, 0.98078528f},
{-0.18223553f, 0.98325491f}, {-0.16934950f, 0.98555606f},
{-0.15643447f, 0.98768834f}, {-0.14349262f, 0.98965139f},
{-0.13052619f, 0.99144486f}, {-0.11753740f, 0.99306846f},
{-0.10452846f, 0.99452190f}, {-0.091501619f, 0.99580493f},
{-0.078459096f, 0.99691733f}, {-0.065403129f, 0.99785892f},
{-0.052335956f, 0.99862953f}, {-0.039259816f, 0.99922904f},
{-0.026176948f, 0.99965732f}, {-0.013089596f, 0.99991433f},
{-1.8369095e-16f, 1.0000000f}, {0.013089596f, 0.99991433f},
{0.026176948f, 0.99965732f}, {0.039259816f, 0.99922904f},
{0.052335956f, 0.99862953f}, {0.065403129f, 0.99785892f},
{0.078459096f, 0.99691733f}, {0.091501619f, 0.99580493f},
{0.10452846f, 0.99452190f}, {0.11753740f, 0.99306846f},
{0.13052619f, 0.99144486f}, {0.14349262f, 0.98965139f},
{0.15643447f, 0.98768834f}, {0.16934950f, 0.98555606f},
{0.18223553f, 0.98325491f}, {0.19509032f, 0.98078528f},
{0.20791169f, 0.97814760f}, {0.22069744f, 0.97534232f},
{0.23344536f, 0.97236992f}, {0.24615329f, 0.96923091f},
{0.25881905f, 0.96592583f}, {0.27144045f, 0.96245524f},
{0.28401534f, 0.95881973f}, {0.29654157f, 0.95501994f},
{0.30901699f, 0.95105652f}, {0.32143947f, 0.94693013f},
{0.33380686f, 0.94264149f}, {0.34611706f, 0.93819134f},
{0.35836795f, 0.93358043f}, {0.37055744f, 0.92880955f},
{0.38268343f, 0.92387953f}, {0.39474386f, 0.91879121f},
{0.40673664f, 0.91354546f}, {0.41865974f, 0.90814317f},
{0.43051110f, 0.90258528f}, {0.44228869f, 0.89687274f},
{0.45399050f, 0.89100652f}, {0.46561452f, 0.88498764f},
{0.47715876f, 0.87881711f}, {0.48862124f, 0.87249601f},
{0.50000000f, 0.86602540f}, {0.51129309f, 0.85940641f},
{0.52249856f, 0.85264016f}, {0.53361452f, 0.84572782f},
{0.54463904f, 0.83867057f}, {0.55557023f, 0.83146961f},
{0.56640624f, 0.82412619f}, {0.57714519f, 0.81664156f},
{0.58778525f, 0.80901699f}, {0.59832460f, 0.80125381f},
{0.60876143f, 0.79335334f}, {0.61909395f, 0.78531693f},
{0.62932039f, 0.77714596f}, {0.63943900f, 0.76884183f},
{0.64944805f, 0.76040597f}, {0.65934582f, 0.75183981f},
{0.66913061f, 0.74314483f}, {0.67880075f, 0.73432251f},
{0.68835458f, 0.72537437f}, {0.69779046f, 0.71630194f},
{0.70710678f, 0.70710678f}, {0.71630194f, 0.69779046f},
{0.72537437f, 0.68835458f}, {0.73432251f, 0.67880075f},
{0.74314483f, 0.66913061f}, {0.75183981f, 0.65934582f},
{0.76040597f, 0.64944805f}, {0.76884183f, 0.63943900f},
{0.77714596f, 0.62932039f}, {0.78531693f, 0.61909395f},
{0.79335334f, 0.60876143f}, {0.80125381f, 0.59832460f},
{0.80901699f, 0.58778525f}, {0.81664156f, 0.57714519f},
{0.82412619f, 0.56640624f}, {0.83146961f, 0.55557023f},
{0.83867057f, 0.54463904f}, {0.84572782f, 0.53361452f},
{0.85264016f, 0.52249856f}, {0.85940641f, 0.51129309f},
{0.86602540f, 0.50000000f}, {0.87249601f, 0.48862124f},
{0.87881711f, 0.47715876f}, {0.88498764f, 0.46561452f},
{0.89100652f, 0.45399050f}, {0.89687274f, 0.44228869f},
{0.90258528f, 0.43051110f}, {0.90814317f, 0.41865974f},
{0.91354546f, 0.40673664f}, {0.91879121f, 0.39474386f},
{0.92387953f, 0.38268343f}, {0.92880955f, 0.37055744f},
{0.93358043f, 0.35836795f}, {0.93819134f, 0.34611706f},
{0.94264149f, 0.33380686f}, {0.94693013f, 0.32143947f},
{0.95105652f, 0.30901699f}, {0.95501994f, 0.29654157f},
{0.95881973f, 0.28401534f}, {0.96245524f, 0.27144045f},
{0.96592583f, 0.25881905f}, {0.96923091f, 0.24615329f},
{0.97236992f, 0.23344536f}, {0.97534232f, 0.22069744f},
{0.97814760f, 0.20791169f}, {0.98078528f, 0.19509032f},
{0.98325491f, 0.18223553f}, {0.98555606f, 0.16934950f},
{0.98768834f, 0.15643447f}, {0.98965139f, 0.14349262f},
{0.99144486f, 0.13052619f}, {0.99306846f, 0.11753740f},
{0.99452190f, 0.10452846f}, {0.99580493f, 0.091501619f},
{0.99691733f, 0.078459096f}, {0.99785892f, 0.065403129f},
{0.99862953f, 0.052335956f}, {0.99922904f, 0.039259816f},
{0.99965732f, 0.026176948f}, {0.99991433f, 0.013089596f},
};
static const opus_int16 fft_bitrev480[480] = {
0, 96, 192, 288, 384, 32, 128, 224, 320, 416, 64, 160, 256, 352, 448,
8, 104, 200, 296, 392, 40, 136, 232, 328, 424, 72, 168, 264, 360, 456,
16, 112, 208, 304, 400, 48, 144, 240, 336, 432, 80, 176, 272, 368, 464,
24, 120, 216, 312, 408, 56, 152, 248, 344, 440, 88, 184, 280, 376, 472,
4, 100, 196, 292, 388, 36, 132, 228, 324, 420, 68, 164, 260, 356, 452,
12, 108, 204, 300, 396, 44, 140, 236, 332, 428, 76, 172, 268, 364, 460,
20, 116, 212, 308, 404, 52, 148, 244, 340, 436, 84, 180, 276, 372, 468,
28, 124, 220, 316, 412, 60, 156, 252, 348, 444, 92, 188, 284, 380, 476,
1, 97, 193, 289, 385, 33, 129, 225, 321, 417, 65, 161, 257, 353, 449,
9, 105, 201, 297, 393, 41, 137, 233, 329, 425, 73, 169, 265, 361, 457,
17, 113, 209, 305, 401, 49, 145, 241, 337, 433, 81, 177, 273, 369, 465,
25, 121, 217, 313, 409, 57, 153, 249, 345, 441, 89, 185, 281, 377, 473,
5, 101, 197, 293, 389, 37, 133, 229, 325, 421, 69, 165, 261, 357, 453,
13, 109, 205, 301, 397, 45, 141, 237, 333, 429, 77, 173, 269, 365, 461,
21, 117, 213, 309, 405, 53, 149, 245, 341, 437, 85, 181, 277, 373, 469,
29, 125, 221, 317, 413, 61, 157, 253, 349, 445, 93, 189, 285, 381, 477,
2, 98, 194, 290, 386, 34, 130, 226, 322, 418, 66, 162, 258, 354, 450,
10, 106, 202, 298, 394, 42, 138, 234, 330, 426, 74, 170, 266, 362, 458,
18, 114, 210, 306, 402, 50, 146, 242, 338, 434, 82, 178, 274, 370, 466,
26, 122, 218, 314, 410, 58, 154, 250, 346, 442, 90, 186, 282, 378, 474,
6, 102, 198, 294, 390, 38, 134, 230, 326, 422, 70, 166, 262, 358, 454,
14, 110, 206, 302, 398, 46, 142, 238, 334, 430, 78, 174, 270, 366, 462,
22, 118, 214, 310, 406, 54, 150, 246, 342, 438, 86, 182, 278, 374, 470,
30, 126, 222, 318, 414, 62, 158, 254, 350, 446, 94, 190, 286, 382, 478,
3, 99, 195, 291, 387, 35, 131, 227, 323, 419, 67, 163, 259, 355, 451,
11, 107, 203, 299, 395, 43, 139, 235, 331, 427, 75, 171, 267, 363, 459,
19, 115, 211, 307, 403, 51, 147, 243, 339, 435, 83, 179, 275, 371, 467,
27, 123, 219, 315, 411, 59, 155, 251, 347, 443, 91, 187, 283, 379, 475,
7, 103, 199, 295, 391, 39, 135, 231, 327, 423, 71, 167, 263, 359, 455,
15, 111, 207, 303, 399, 47, 143, 239, 335, 431, 79, 175, 271, 367, 463,
23, 119, 215, 311, 407, 55, 151, 247, 343, 439, 87, 183, 279, 375, 471,
31, 127, 223, 319, 415, 63, 159, 255, 351, 447, 95, 191, 287, 383, 479,
};
static const opus_int16 fft_bitrev240[240] = {
0, 48, 96, 144, 192, 16, 64, 112, 160, 208, 32, 80, 128, 176, 224,
4, 52, 100, 148, 196, 20, 68, 116, 164, 212, 36, 84, 132, 180, 228,
8, 56, 104, 152, 200, 24, 72, 120, 168, 216, 40, 88, 136, 184, 232,
12, 60, 108, 156, 204, 28, 76, 124, 172, 220, 44, 92, 140, 188, 236,
1, 49, 97, 145, 193, 17, 65, 113, 161, 209, 33, 81, 129, 177, 225,
5, 53, 101, 149, 197, 21, 69, 117, 165, 213, 37, 85, 133, 181, 229,
9, 57, 105, 153, 201, 25, 73, 121, 169, 217, 41, 89, 137, 185, 233,
13, 61, 109, 157, 205, 29, 77, 125, 173, 221, 45, 93, 141, 189, 237,
2, 50, 98, 146, 194, 18, 66, 114, 162, 210, 34, 82, 130, 178, 226,
6, 54, 102, 150, 198, 22, 70, 118, 166, 214, 38, 86, 134, 182, 230,
10, 58, 106, 154, 202, 26, 74, 122, 170, 218, 42, 90, 138, 186, 234,
14, 62, 110, 158, 206, 30, 78, 126, 174, 222, 46, 94, 142, 190, 238,
3, 51, 99, 147, 195, 19, 67, 115, 163, 211, 35, 83, 131, 179, 227,
7, 55, 103, 151, 199, 23, 71, 119, 167, 215, 39, 87, 135, 183, 231,
11, 59, 107, 155, 203, 27, 75, 123, 171, 219, 43, 91, 139, 187, 235,
15, 63, 111, 159, 207, 31, 79, 127, 175, 223, 47, 95, 143, 191, 239,
};
static const opus_int16 fft_bitrev120[120] = {
0, 24, 48, 72, 96, 8, 32, 56, 80, 104, 16, 40, 64, 88, 112,
4, 28, 52, 76, 100, 12, 36, 60, 84, 108, 20, 44, 68, 92, 116,
1, 25, 49, 73, 97, 9, 33, 57, 81, 105, 17, 41, 65, 89, 113,
5, 29, 53, 77, 101, 13, 37, 61, 85, 109, 21, 45, 69, 93, 117,
2, 26, 50, 74, 98, 10, 34, 58, 82, 106, 18, 42, 66, 90, 114,
6, 30, 54, 78, 102, 14, 38, 62, 86, 110, 22, 46, 70, 94, 118,
3, 27, 51, 75, 99, 11, 35, 59, 83, 107, 19, 43, 67, 91, 115,
7, 31, 55, 79, 103, 15, 39, 63, 87, 111, 23, 47, 71, 95, 119,
};
static const opus_int16 fft_bitrev60[60] = {
0, 12, 24, 36, 48, 4, 16, 28, 40, 52, 8, 20, 32, 44, 56,
1, 13, 25, 37, 49, 5, 17, 29, 41, 53, 9, 21, 33, 45, 57,
2, 14, 26, 38, 50, 6, 18, 30, 42, 54, 10, 22, 34, 46, 58,
3, 15, 27, 39, 51, 7, 19, 31, 43, 55, 11, 23, 35, 47, 59,
};
static const kiss_fft_state fft_state48000_960_0 = {
480,    /* nfft */
0.002083333f,   /* scale */
-1,     /* shift */
{5, 96, 3, 32, 4, 8, 2, 4, 4, 1, 0, 0, 0, 0, 0, 0, },   /* factors */
fft_bitrev480,  /* bitrev */
fft_twiddles48000_960,  /* bitrev */
NULL,
};
static const kiss_fft_state fft_state48000_960_1 = {
240,    /* nfft */
0.004166667f,   /* scale */
1,      /* shift */
{5, 48, 3, 16, 4, 4, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, },    /* factors */
fft_bitrev240,  /* bitrev */
fft_twiddles48000_960,  /* bitrev */
NULL,
};
static const kiss_fft_state fft_state48000_960_2 = {
120,    /* nfft */
0.008333333f,   /* scale */
2,      /* shift */
{5, 24, 3, 8, 2, 4, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, },    /* factors */
fft_bitrev120,  /* bitrev */
fft_twiddles48000_960,  /* bitrev */
NULL,
};
static const kiss_fft_state fft_state48000_960_3 = {
60,     /* nfft */
0.016666667f,   /* scale */
3,      /* shift */
{5, 12, 3, 4, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },    /* factors */
fft_bitrev60,   /* bitrev */
fft_twiddles48000_960,  /* bitrev */
NULL,
};
static const opus_val16 mdct_twiddles960[1800] = {
0.99999994f, 0.99999321f, 0.99997580f, 0.99994773f, 0.99990886f,
0.99985933f, 0.99979913f, 0.99972820f, 0.99964654f, 0.99955416f,
0.99945110f, 0.99933738f, 0.99921292f, 0.99907774f, 0.99893188f,
0.99877530f, 0.99860805f, 0.99843007f, 0.99824142f, 0.99804211f,
0.99783206f, 0.99761140f, 0.99737996f, 0.99713790f, 0.99688518f,
0.99662173f, 0.99634761f, 0.99606287f, 0.99576741f, 0.99546129f,
0.99514455f, 0.99481714f, 0.99447906f, 0.99413031f, 0.99377096f,
0.99340093f, 0.99302030f, 0.99262899f, 0.99222708f, 0.99181455f,
0.99139136f, 0.99095762f, 0.99051321f, 0.99005818f, 0.98959261f,
0.98911643f, 0.98862964f, 0.98813224f, 0.98762429f, 0.98710573f,
0.98657662f, 0.98603696f, 0.98548669f, 0.98492593f, 0.98435456f,
0.98377270f, 0.98318028f, 0.98257732f, 0.98196387f, 0.98133987f,
0.98070538f, 0.98006040f, 0.97940493f, 0.97873890f, 0.97806245f,
0.97737551f, 0.97667813f, 0.97597027f, 0.97525197f, 0.97452319f,
0.97378403f, 0.97303438f, 0.97227436f, 0.97150391f, 0.97072303f,
0.96993178f, 0.96913016f, 0.96831810f, 0.96749574f, 0.96666300f,
0.96581990f, 0.96496642f, 0.96410263f, 0.96322852f, 0.96234411f,
0.96144938f, 0.96054435f, 0.95962906f, 0.95870346f, 0.95776761f,
0.95682150f, 0.95586514f, 0.95489854f, 0.95392174f, 0.95293468f,
0.95193744f, 0.95093000f, 0.94991243f, 0.94888461f, 0.94784665f,
0.94679856f, 0.94574034f, 0.94467193f, 0.94359344f, 0.94250488f,
0.94140619f, 0.94029742f, 0.93917859f, 0.93804967f, 0.93691075f,
0.93576175f, 0.93460274f, 0.93343377f, 0.93225473f, 0.93106574f,
0.92986679f, 0.92865789f, 0.92743903f, 0.92621022f, 0.92497152f,
0.92372292f, 0.92246443f, 0.92119598f, 0.91991776f, 0.91862965f,
0.91733170f, 0.91602397f, 0.91470635f, 0.91337901f, 0.91204184f,
0.91069490f, 0.90933824f, 0.90797186f, 0.90659571f, 0.90520984f,
0.90381432f, 0.90240908f, 0.90099424f, 0.89956969f, 0.89813554f,
0.89669174f, 0.89523834f, 0.89377540f, 0.89230281f, 0.89082074f,
0.88932908f, 0.88782793f, 0.88631725f, 0.88479710f, 0.88326746f,
0.88172835f, 0.88017982f, 0.87862182f, 0.87705445f, 0.87547767f,
0.87389153f, 0.87229604f, 0.87069118f, 0.86907703f, 0.86745358f,
0.86582077f, 0.86417878f, 0.86252749f, 0.86086690f, 0.85919720f,
0.85751826f, 0.85583007f, 0.85413277f, 0.85242635f, 0.85071075f,
0.84898609f, 0.84725231f, 0.84550947f, 0.84375757f, 0.84199661f,
0.84022665f, 0.83844769f, 0.83665979f, 0.83486289f, 0.83305705f,
0.83124226f, 0.82941860f, 0.82758605f, 0.82574469f, 0.82389444f,
0.82203537f, 0.82016748f, 0.81829083f, 0.81640542f, 0.81451124f,
0.81260836f, 0.81069672f, 0.80877650f, 0.80684757f, 0.80490994f,
0.80296379f, 0.80100900f, 0.79904562f, 0.79707366f, 0.79509324f,
0.79310423f, 0.79110676f, 0.78910083f, 0.78708643f, 0.78506362f,
0.78303236f, 0.78099275f, 0.77894479f, 0.77688843f, 0.77482378f,
0.77275085f, 0.77066964f, 0.76858020f, 0.76648247f, 0.76437658f,
0.76226246f, 0.76014024f, 0.75800985f, 0.75587130f, 0.75372469f,
0.75157005f, 0.74940729f, 0.74723655f, 0.74505776f, 0.74287105f,
0.74067634f, 0.73847371f, 0.73626316f, 0.73404479f, 0.73181850f,
0.72958434f, 0.72734243f, 0.72509271f, 0.72283524f, 0.72057003f,
0.71829706f, 0.71601641f, 0.71372813f, 0.71143216f, 0.70912862f,
0.70681745f, 0.70449871f, 0.70217246f, 0.69983864f, 0.69749737f,
0.69514859f, 0.69279242f, 0.69042879f, 0.68805778f, 0.68567938f,
0.68329364f, 0.68090063f, 0.67850029f, 0.67609268f, 0.67367786f,
0.67125577f, 0.66882652f, 0.66639012f, 0.66394657f, 0.66149592f,
0.65903819f, 0.65657341f, 0.65410155f, 0.65162271f, 0.64913690f,
0.64664418f, 0.64414448f, 0.64163786f, 0.63912445f, 0.63660413f,
0.63407701f, 0.63154310f, 0.62900239f, 0.62645501f, 0.62390089f,
0.62134010f, 0.61877263f, 0.61619854f, 0.61361790f, 0.61103064f,
0.60843682f, 0.60583651f, 0.60322970f, 0.60061646f, 0.59799677f,
0.59537065f, 0.59273821f, 0.59009939f, 0.58745426f, 0.58480281f,
0.58214509f, 0.57948118f, 0.57681108f, 0.57413477f, 0.57145232f,
0.56876373f, 0.56606907f, 0.56336832f, 0.56066155f, 0.55794877f,
0.55523002f, 0.55250537f, 0.54977477f, 0.54703826f, 0.54429591f,
0.54154772f, 0.53879374f, 0.53603399f, 0.53326851f, 0.53049731f,
0.52772039f, 0.52493787f, 0.52214974f, 0.51935595f, 0.51655668f,
0.51375180f, 0.51094145f, 0.50812566f, 0.50530440f, 0.50247771f,
0.49964568f, 0.49680826f, 0.49396557f, 0.49111754f, 0.48826426f,
0.48540577f, 0.48254207f, 0.47967321f, 0.47679919f, 0.47392011f,
0.47103590f, 0.46814668f, 0.46525243f, 0.46235323f, 0.45944905f,
0.45653993f, 0.45362595f, 0.45070711f, 0.44778344f, 0.44485497f,
0.44192174f, 0.43898380f, 0.43604112f, 0.43309379f, 0.43014181f,
0.42718524f, 0.42422408f, 0.42125839f, 0.41828820f, 0.41531351f,
0.41233435f, 0.40935081f, 0.40636289f, 0.40337059f, 0.40037400f,
0.39737311f, 0.39436796f, 0.39135858f, 0.38834500f, 0.38532731f,
0.38230544f, 0.37927949f, 0.37624949f, 0.37321547f, 0.37017745f,
0.36713544f, 0.36408952f, 0.36103970f, 0.35798600f, 0.35492846f,
0.35186714f, 0.34880206f, 0.34573323f, 0.34266070f, 0.33958447f,
0.33650464f, 0.33342120f, 0.33033419f, 0.32724363f, 0.32414958f,
0.32105204f, 0.31795108f, 0.31484672f, 0.31173897f, 0.30862790f,
0.30551350f, 0.30239585f, 0.29927495f, 0.29615086f, 0.29302359f,
0.28989318f, 0.28675964f, 0.28362307f, 0.28048345f, 0.27734083f,
0.27419522f, 0.27104670f, 0.26789525f, 0.26474094f, 0.26158381f,
0.25842386f, 0.25526115f, 0.25209570f, 0.24892756f, 0.24575676f,
0.24258332f, 0.23940729f, 0.23622867f, 0.23304754f, 0.22986393f,
0.22667783f, 0.22348931f, 0.22029841f, 0.21710514f, 0.21390954f,
0.21071166f, 0.20751151f, 0.20430915f, 0.20110460f, 0.19789790f,
0.19468907f, 0.19147816f, 0.18826519f, 0.18505022f, 0.18183327f,
0.17861435f, 0.17539354f, 0.17217083f, 0.16894630f, 0.16571994f,
0.16249183f, 0.15926196f, 0.15603039f, 0.15279715f, 0.14956227f,
0.14632578f, 0.14308774f, 0.13984816f, 0.13660708f, 0.13336454f,
0.13012058f, 0.12687522f, 0.12362850f, 0.12038045f, 0.11713112f,
0.11388054f, 0.11062872f, 0.10737573f, 0.10412160f, 0.10086634f,
0.097609997f, 0.094352618f, 0.091094226f, 0.087834857f, 0.084574550f,
0.081313334f, 0.078051247f, 0.074788325f, 0.071524605f, 0.068260118f,
0.064994894f, 0.061728980f, 0.058462404f, 0.055195201f, 0.051927410f,
0.048659060f, 0.045390189f, 0.042120833f, 0.038851023f, 0.035580799f,
0.032310195f, 0.029039243f, 0.025767982f, 0.022496443f, 0.019224664f,
0.015952680f, 0.012680525f, 0.0094082337f, 0.0061358409f, 0.0028633832f,
-0.00040910527f, -0.0036815894f, -0.0069540343f, -0.010226404f, -0.013498665f,
-0.016770782f, -0.020042717f, -0.023314439f, -0.026585912f, -0.029857099f,
-0.033127967f, -0.036398482f, -0.039668605f, -0.042938303f, -0.046207540f,
-0.049476285f, -0.052744497f, -0.056012146f, -0.059279196f, -0.062545612f,
-0.065811358f, -0.069076397f, -0.072340697f, -0.075604223f, -0.078866936f,
-0.082128808f, -0.085389800f, -0.088649876f, -0.091909006f, -0.095167145f,
-0.098424271f, -0.10168034f, -0.10493532f, -0.10818918f, -0.11144188f,
-0.11469338f, -0.11794366f, -0.12119267f, -0.12444039f, -0.12768677f,
-0.13093179f, -0.13417540f, -0.13741758f, -0.14065829f, -0.14389749f,
-0.14713514f, -0.15037122f, -0.15360570f, -0.15683852f, -0.16006967f,
-0.16329910f, -0.16652679f, -0.16975269f, -0.17297678f, -0.17619900f,
-0.17941935f, -0.18263777f, -0.18585424f, -0.18906870f, -0.19228116f,
-0.19549155f, -0.19869985f, -0.20190603f, -0.20511003f, -0.20831184f,
-0.21151142f, -0.21470875f, -0.21790376f, -0.22109644f, -0.22428675f,
-0.22747467f, -0.23066014f, -0.23384315f, -0.23702365f, -0.24020162f,
-0.24337701f, -0.24654980f, -0.24971995f, -0.25288740f, -0.25605217f,
-0.25921419f, -0.26237345f, -0.26552987f, -0.26868346f, -0.27183419f,
-0.27498198f, -0.27812684f, -0.28126872f, -0.28440759f, -0.28754342f,
-0.29067615f, -0.29380578f, -0.29693225f, -0.30005556f, -0.30317566f,
-0.30629250f, -0.30940607f, -0.31251630f, -0.31562322f, -0.31872672f,
-0.32182685f, -0.32492352f, -0.32801670f, -0.33110636f, -0.33419248f,
-0.33727503f, -0.34035397f, -0.34342924f, -0.34650084f, -0.34956875f,
-0.35263291f, -0.35569328f, -0.35874987f, -0.36180258f, -0.36485144f,
-0.36789638f, -0.37093741f, -0.37397444f, -0.37700745f, -0.38003644f,
-0.38306138f, -0.38608220f, -0.38909888f, -0.39211139f, -0.39511973f,
-0.39812380f, -0.40112361f, -0.40411916f, -0.40711036f, -0.41009718f,
-0.41307965f, -0.41605768f, -0.41903123f, -0.42200032f, -0.42496487f,
-0.42792490f, -0.43088034f, -0.43383113f, -0.43677729f, -0.43971881f,
-0.44265559f, -0.44558764f, -0.44851488f, -0.45143735f, -0.45435500f,
-0.45726776f, -0.46017563f, -0.46307856f, -0.46597654f, -0.46886954f,
-0.47175750f, -0.47464043f, -0.47751826f, -0.48039100f, -0.48325855f,
-0.48612097f, -0.48897815f, -0.49183011f, -0.49467680f, -0.49751821f,
-0.50035429f, -0.50318497f, -0.50601029f, -0.50883019f, -0.51164466f,
-0.51445359f, -0.51725709f, -0.52005500f, -0.52284735f, -0.52563411f,
-0.52841520f, -0.53119069f, -0.53396046f, -0.53672451f, -0.53948283f,
-0.54223537f, -0.54498214f, -0.54772300f, -0.55045801f, -0.55318713f,
-0.55591035f, -0.55862761f, -0.56133890f, -0.56404412f, -0.56674337f,
-0.56943649f, -0.57212353f, -0.57480448f, -0.57747924f, -0.58014780f,
-0.58281022f, -0.58546633f, -0.58811617f, -0.59075975f, -0.59339696f,
-0.59602785f, -0.59865236f, -0.60127044f, -0.60388207f, -0.60648727f,
-0.60908598f, -0.61167812f, -0.61426371f, -0.61684275f, -0.61941516f,
-0.62198097f, -0.62454009f, -0.62709254f, -0.62963831f, -0.63217729f,
-0.63470948f, -0.63723493f, -0.63975352f, -0.64226526f, -0.64477009f,
-0.64726806f, -0.64975911f, -0.65224314f, -0.65472025f, -0.65719032f,
-0.65965337f, -0.66210932f, -0.66455823f, -0.66700000f, -0.66943461f,
-0.67186207f, -0.67428231f, -0.67669535f, -0.67910111f, -0.68149966f,
-0.68389088f, -0.68627477f, -0.68865126f, -0.69102043f, -0.69338220f,
-0.69573659f, -0.69808346f, -0.70042288f, -0.70275480f, -0.70507920f,
-0.70739603f, -0.70970529f, -0.71200693f, -0.71430099f, -0.71658736f,
-0.71886611f, -0.72113711f, -0.72340041f, -0.72565591f, -0.72790372f,
-0.73014367f, -0.73237586f, -0.73460019f, -0.73681659f, -0.73902518f,
-0.74122584f, -0.74341851f, -0.74560326f, -0.74778003f, -0.74994880f,
-0.75210953f, -0.75426215f, -0.75640678f, -0.75854325f, -0.76067162f,
-0.76279181f, -0.76490390f, -0.76700771f, -0.76910341f, -0.77119076f,
-0.77326995f, -0.77534080f, -0.77740335f, -0.77945763f, -0.78150350f,
-0.78354102f, -0.78557014f, -0.78759086f, -0.78960317f, -0.79160696f,
-0.79360235f, -0.79558921f, -0.79756755f, -0.79953730f, -0.80149853f,
-0.80345118f, -0.80539525f, -0.80733067f, -0.80925739f, -0.81117553f,
-0.81308490f, -0.81498563f, -0.81687760f, -0.81876087f, -0.82063532f,
-0.82250100f, -0.82435787f, -0.82620591f, -0.82804507f, -0.82987541f,
-0.83169687f, -0.83350939f, -0.83531296f, -0.83710766f, -0.83889335f,
-0.84067005f, -0.84243774f, -0.84419644f, -0.84594607f, -0.84768665f,
-0.84941816f, -0.85114056f, -0.85285389f, -0.85455805f, -0.85625303f,
-0.85793889f, -0.85961550f, -0.86128294f, -0.86294121f, -0.86459017f,
-0.86622989f, -0.86786032f, -0.86948150f, -0.87109333f, -0.87269586f,
-0.87428904f, -0.87587279f, -0.87744725f, -0.87901229f, -0.88056785f,
-0.88211405f, -0.88365078f, -0.88517809f, -0.88669586f, -0.88820416f,
-0.88970292f, -0.89119220f, -0.89267188f, -0.89414203f, -0.89560264f,
-0.89705360f, -0.89849502f, -0.89992678f, -0.90134889f, -0.90276134f,
-0.90416414f, -0.90555727f, -0.90694070f, -0.90831441f, -0.90967834f,
-0.91103262f, -0.91237706f, -0.91371179f, -0.91503674f, -0.91635185f,
-0.91765714f, -0.91895264f, -0.92023826f, -0.92151409f, -0.92277998f,
-0.92403603f, -0.92528218f, -0.92651838f, -0.92774469f, -0.92896110f,
-0.93016750f, -0.93136400f, -0.93255049f, -0.93372697f, -0.93489349f,
-0.93604994f, -0.93719643f, -0.93833286f, -0.93945926f, -0.94057560f,
-0.94168180f, -0.94277799f, -0.94386405f, -0.94494003f, -0.94600588f,
-0.94706154f, -0.94810712f, -0.94914252f, -0.95016778f, -0.95118284f,
-0.95218778f, -0.95318246f, -0.95416695f, -0.95514119f, -0.95610523f,
-0.95705903f, -0.95800257f, -0.95893586f, -0.95985889f, -0.96077162f,
-0.96167403f, -0.96256620f, -0.96344805f, -0.96431959f, -0.96518075f,
-0.96603161f, -0.96687216f, -0.96770233f, -0.96852213f, -0.96933156f,
-0.97013056f, -0.97091925f, -0.97169751f, -0.97246534f, -0.97322279f,
-0.97396982f, -0.97470641f, -0.97543252f, -0.97614825f, -0.97685349f,
-0.97754824f, -0.97823256f, -0.97890645f, -0.97956979f, -0.98022264f,
-0.98086500f, -0.98149687f, -0.98211825f, -0.98272908f, -0.98332942f,
-0.98391914f, -0.98449844f, -0.98506713f, -0.98562527f, -0.98617285f,
-0.98670989f, -0.98723638f, -0.98775226f, -0.98825759f, -0.98875231f,
-0.98923647f, -0.98971003f, -0.99017298f, -0.99062532f, -0.99106705f,
-0.99149817f, -0.99191868f, -0.99232858f, -0.99272782f, -0.99311644f,
-0.99349445f, -0.99386179f, -0.99421853f, -0.99456459f, -0.99489999f,
-0.99522477f, -0.99553883f, -0.99584228f, -0.99613506f, -0.99641716f,
-0.99668860f, -0.99694937f, -0.99719942f, -0.99743885f, -0.99766755f,
-0.99788558f, -0.99809295f, -0.99828959f, -0.99847561f, -0.99865085f,
-0.99881548f, -0.99896932f, -0.99911255f, -0.99924499f, -0.99936682f,
-0.99947786f, -0.99957830f, -0.99966794f, -0.99974692f, -0.99981517f,
-0.99987274f, -0.99991959f, -0.99995571f, -0.99998116f, -0.99999589f,
0.99999964f, 0.99997288f, 0.99990326f, 0.99979085f, 0.99963558f,
0.99943751f, 0.99919659f, 0.99891287f, 0.99858636f, 0.99821711f,
0.99780506f, 0.99735034f, 0.99685282f, 0.99631262f, 0.99572974f,
0.99510419f, 0.99443603f, 0.99372530f, 0.99297196f, 0.99217612f,
0.99133772f, 0.99045694f, 0.98953366f, 0.98856801f, 0.98756003f,
0.98650974f, 0.98541719f, 0.98428243f, 0.98310548f, 0.98188645f,
0.98062533f, 0.97932225f, 0.97797716f, 0.97659022f, 0.97516143f,
0.97369087f, 0.97217858f, 0.97062469f, 0.96902919f, 0.96739221f,
0.96571374f, 0.96399397f, 0.96223283f, 0.96043050f, 0.95858705f,
0.95670253f, 0.95477700f, 0.95281059f, 0.95080340f, 0.94875544f,
0.94666684f, 0.94453770f, 0.94236809f, 0.94015813f, 0.93790787f,
0.93561745f, 0.93328691f, 0.93091643f, 0.92850608f, 0.92605597f,
0.92356616f, 0.92103678f, 0.91846794f, 0.91585976f, 0.91321236f,
0.91052586f, 0.90780038f, 0.90503591f, 0.90223277f, 0.89939094f,
0.89651060f, 0.89359182f, 0.89063478f, 0.88763964f, 0.88460642f,
0.88153529f, 0.87842643f, 0.87527996f, 0.87209594f, 0.86887461f,
0.86561602f, 0.86232042f, 0.85898781f, 0.85561842f, 0.85221243f,
0.84876984f, 0.84529096f, 0.84177583f, 0.83822471f, 0.83463764f,
0.83101481f, 0.82735640f, 0.82366252f, 0.81993335f, 0.81616908f,
0.81236988f, 0.80853581f, 0.80466717f, 0.80076402f, 0.79682660f,
0.79285502f, 0.78884947f, 0.78481019f, 0.78073722f, 0.77663082f,
0.77249116f, 0.76831841f, 0.76411277f, 0.75987434f, 0.75560343f,
0.75130010f, 0.74696463f, 0.74259710f, 0.73819780f, 0.73376691f,
0.72930455f, 0.72481096f, 0.72028631f, 0.71573079f, 0.71114463f,
0.70652801f, 0.70188117f, 0.69720417f, 0.69249737f, 0.68776089f,
0.68299496f, 0.67819971f, 0.67337549f, 0.66852236f, 0.66364062f,
0.65873051f, 0.65379208f, 0.64882571f, 0.64383155f, 0.63880974f,
0.63376063f, 0.62868434f, 0.62358117f, 0.61845124f, 0.61329484f,
0.60811216f, 0.60290343f, 0.59766883f, 0.59240872f, 0.58712316f,
0.58181250f, 0.57647687f, 0.57111657f, 0.56573176f, 0.56032276f,
0.55488980f, 0.54943299f, 0.54395270f, 0.53844911f, 0.53292239f,
0.52737290f, 0.52180082f, 0.51620632f, 0.51058978f, 0.50495136f,
0.49929130f, 0.49360985f, 0.48790723f, 0.48218375f, 0.47643960f,
0.47067502f, 0.46489030f, 0.45908567f, 0.45326138f, 0.44741765f,
0.44155475f, 0.43567297f, 0.42977250f, 0.42385364f, 0.41791660f,
0.41196167f, 0.40598908f, 0.39999911f, 0.39399201f, 0.38796803f,
0.38192743f, 0.37587047f, 0.36979741f, 0.36370850f, 0.35760403f,
0.35148421f, 0.34534934f, 0.33919969f, 0.33303553f, 0.32685706f,
0.32066461f, 0.31445843f, 0.30823877f, 0.30200592f, 0.29576012f,
0.28950164f, 0.28323078f, 0.27694780f, 0.27065292f, 0.26434645f,
0.25802869f, 0.25169984f, 0.24536023f, 0.23901010f, 0.23264973f,
0.22627939f, 0.21989937f, 0.21350993f, 0.20711134f, 0.20070387f,
0.19428782f, 0.18786344f, 0.18143101f, 0.17499080f, 0.16854310f,
0.16208819f, 0.15562633f, 0.14915779f, 0.14268288f, 0.13620184f,
0.12971498f, 0.12322257f, 0.11672486f, 0.11022217f, 0.10371475f,
0.097202882f, 0.090686858f, 0.084166944f, 0.077643424f, 0.071116582f,
0.064586692f, 0.058054037f, 0.051518895f, 0.044981543f, 0.038442269f,
0.031901345f, 0.025359053f, 0.018815678f, 0.012271495f, 0.0057267868f,
-0.00081816671f, -0.0073630852f, -0.013907688f, -0.020451695f, -0.026994826f,
-0.033536803f, -0.040077340f, -0.046616159f, -0.053152986f, -0.059687532f,
-0.066219524f, -0.072748676f, -0.079274714f, -0.085797355f, -0.092316322f,
-0.098831341f, -0.10534211f, -0.11184838f, -0.11834986f, -0.12484626f,
-0.13133731f, -0.13782275f, -0.14430228f, -0.15077563f, -0.15724251f,
-0.16370267f, -0.17015581f, -0.17660165f, -0.18303993f, -0.18947038f,
-0.19589271f, -0.20230664f, -0.20871192f, -0.21510825f, -0.22149536f,
-0.22787298f, -0.23424086f, -0.24059868f, -0.24694622f, -0.25328314f,
-0.25960925f, -0.26592422f, -0.27222782f, -0.27851975f, -0.28479972f,
-0.29106751f, -0.29732284f, -0.30356544f, -0.30979502f, -0.31601134f,
-0.32221413f, -0.32840309f, -0.33457801f, -0.34073856f, -0.34688455f,
-0.35301566f, -0.35913166f, -0.36523229f, -0.37131724f, -0.37738630f,
-0.38343921f, -0.38947567f, -0.39549544f, -0.40149832f, -0.40748394f,
-0.41345215f, -0.41940263f, -0.42533514f, -0.43124944f, -0.43714526f,
-0.44302234f, -0.44888046f, -0.45471936f, -0.46053877f, -0.46633846f,
-0.47211814f, -0.47787762f, -0.48361665f, -0.48933494f, -0.49503228f,
-0.50070840f, -0.50636309f, -0.51199609f, -0.51760709f, -0.52319598f,
-0.52876246f, -0.53430629f, -0.53982723f, -0.54532504f, -0.55079949f,
-0.55625033f, -0.56167740f, -0.56708032f, -0.57245898f, -0.57781315f,
-0.58314258f, -0.58844697f, -0.59372622f, -0.59897995f, -0.60420811f,
-0.60941035f, -0.61458647f, -0.61973625f, -0.62485951f, -0.62995601f,
-0.63502556f, -0.64006782f, -0.64508271f, -0.65007001f, -0.65502942f,
-0.65996075f, -0.66486382f, -0.66973841f, -0.67458433f, -0.67940134f,
-0.68418926f, -0.68894786f, -0.69367695f, -0.69837630f, -0.70304573f,
-0.70768511f, -0.71229410f, -0.71687263f, -0.72142041f, -0.72593731f,
-0.73042315f, -0.73487765f, -0.73930067f, -0.74369204f, -0.74805158f,
-0.75237900f, -0.75667429f, -0.76093709f, -0.76516730f, -0.76936477f,
-0.77352923f, -0.77766061f, -0.78175867f, -0.78582323f, -0.78985411f,
-0.79385114f, -0.79781419f, -0.80174309f, -0.80563760f, -0.80949765f,
-0.81332302f, -0.81711352f, -0.82086903f, -0.82458937f, -0.82827437f,
-0.83192390f, -0.83553779f, -0.83911592f, -0.84265804f, -0.84616417f,
-0.84963393f, -0.85306740f, -0.85646427f, -0.85982448f, -0.86314780f,
-0.86643422f, -0.86968350f, -0.87289548f, -0.87607014f, -0.87920725f,
-0.88230664f, -0.88536829f, -0.88839203f, -0.89137769f, -0.89432514f,
-0.89723432f, -0.90010506f, -0.90293723f, -0.90573072f, -0.90848541f,
-0.91120118f, -0.91387796f, -0.91651553f, -0.91911387f, -0.92167282f,
-0.92419231f, -0.92667222f, -0.92911243f, -0.93151283f, -0.93387336f,
-0.93619382f, -0.93847424f, -0.94071442f, -0.94291431f, -0.94507378f,
-0.94719279f, -0.94927126f, -0.95130903f, -0.95330608f, -0.95526224f,
-0.95717752f, -0.95905179f, -0.96088499f, -0.96267700f, -0.96442777f,
-0.96613729f, -0.96780539f, -0.96943200f, -0.97101706f, -0.97256058f,
-0.97406244f, -0.97552258f, -0.97694093f, -0.97831738f, -0.97965199f,
-0.98094457f, -0.98219514f, -0.98340368f, -0.98457009f, -0.98569429f,
-0.98677629f, -0.98781598f, -0.98881340f, -0.98976845f, -0.99068111f,
-0.99155134f, -0.99237907f, -0.99316430f, -0.99390697f, -0.99460709f,
-0.99526459f, -0.99587947f, -0.99645168f, -0.99698120f, -0.99746799f,
-0.99791211f, -0.99831343f, -0.99867201f, -0.99898779f, -0.99926084f,
-0.99949104f, -0.99967843f, -0.99982297f, -0.99992472f, -0.99998361f,
0.99999869f, 0.99989158f, 0.99961317f, 0.99916345f, 0.99854255f,
0.99775058f, 0.99678761f, 0.99565387f, 0.99434954f, 0.99287480f,
0.99122995f, 0.98941529f, 0.98743105f, 0.98527765f, 0.98295540f,
0.98046476f, 0.97780609f, 0.97497988f, 0.97198665f, 0.96882683f,
0.96550101f, 0.96200979f, 0.95835376f, 0.95453346f, 0.95054960f,
0.94640291f, 0.94209403f, 0.93762374f, 0.93299282f, 0.92820197f,
0.92325211f, 0.91814411f, 0.91287869f, 0.90745693f, 0.90187967f,
0.89614785f, 0.89026248f, 0.88422459f, 0.87803519f, 0.87169534f,
0.86520612f, 0.85856867f, 0.85178405f, 0.84485358f, 0.83777827f,
0.83055943f, 0.82319832f, 0.81569612f, 0.80805415f, 0.80027372f,
0.79235619f, 0.78430289f, 0.77611518f, 0.76779449f, 0.75934225f,
0.75075996f, 0.74204898f, 0.73321080f, 0.72424710f, 0.71515924f,
0.70594883f, 0.69661748f, 0.68716675f, 0.67759830f, 0.66791373f,
0.65811473f, 0.64820296f, 0.63818014f, 0.62804794f, 0.61780810f,
0.60746247f, 0.59701276f, 0.58646071f, 0.57580817f, 0.56505698f,
0.55420899f, 0.54326600f, 0.53222996f, 0.52110273f, 0.50988621f,
0.49858227f, 0.48719296f, 0.47572014f, 0.46416581f, 0.45253196f,
0.44082057f, 0.42903364f, 0.41717321f, 0.40524128f, 0.39323992f,
0.38117120f, 0.36903715f, 0.35683987f, 0.34458145f, 0.33226398f,
0.31988961f, 0.30746040f, 0.29497850f, 0.28244606f, 0.26986524f,
0.25723818f, 0.24456702f, 0.23185398f, 0.21910121f, 0.20631088f,
0.19348522f, 0.18062639f, 0.16773662f, 0.15481812f, 0.14187308f,
0.12890373f, 0.11591230f, 0.10290100f, 0.089872077f, 0.076827750f,
0.063770257f, 0.050701842f, 0.037624735f, 0.024541186f, 0.011453429f,
-0.0016362892f, -0.014725727f, -0.027812643f, -0.040894791f, -0.053969935f,
-0.067035832f, -0.080090240f, -0.093130924f, -0.10615565f, -0.11916219f,
-0.13214831f, -0.14511178f, -0.15805040f, -0.17096193f, -0.18384418f,
-0.19669491f, -0.20951195f, -0.22229309f, -0.23503613f, -0.24773891f,
-0.26039925f, -0.27301496f, -0.28558388f, -0.29810387f, -0.31057280f,
-0.32298848f, -0.33534884f, -0.34765175f, -0.35989508f, -0.37207675f,
-0.38419467f, -0.39624676f, -0.40823093f, -0.42014518f, -0.43198743f,
-0.44375566f, -0.45544785f, -0.46706200f, -0.47859612f, -0.49004826f,
-0.50141639f, -0.51269865f, -0.52389306f, -0.53499764f, -0.54601061f,
-0.55693001f, -0.56775403f, -0.57848072f, -0.58910829f, -0.59963489f,
-0.61005878f, -0.62037814f, -0.63059121f, -0.64069623f, -0.65069145f,
-0.66057515f, -0.67034572f, -0.68000144f, -0.68954057f, -0.69896162f,
-0.70826286f, -0.71744281f, -0.72649974f, -0.73543227f, -0.74423873f,
-0.75291771f, -0.76146764f, -0.76988715f, -0.77817470f, -0.78632891f,
-0.79434842f, -0.80223179f, -0.80997771f, -0.81758487f, -0.82505190f,
-0.83237761f, -0.83956063f, -0.84659988f, -0.85349399f, -0.86024189f,
-0.86684239f, -0.87329435f, -0.87959671f, -0.88574833f, -0.89174819f,
-0.89759529f, -0.90328854f, -0.90882701f, -0.91420978f, -0.91943592f,
-0.92450452f, -0.92941469f, -0.93416560f, -0.93875647f, -0.94318646f,
-0.94745487f, -0.95156091f, -0.95550388f, -0.95928317f, -0.96289814f,
-0.96634805f, -0.96963239f, -0.97275060f, -0.97570217f, -0.97848648f,
-0.98110318f, -0.98355180f, -0.98583186f, -0.98794299f, -0.98988485f,
-0.99165714f, -0.99325943f, -0.99469161f, -0.99595332f, -0.99704438f,
-0.99796462f, -0.99871385f, -0.99929196f, -0.99969882f, -0.99993443f,
0.99999464f, 0.99956632f, 0.99845290f, 0.99665523f, 0.99417448f,
0.99101239f, 0.98717111f, 0.98265326f, 0.97746199f, 0.97160077f,
0.96507365f, 0.95788515f, 0.95004016f, 0.94154406f, 0.93240267f,
0.92262226f, 0.91220951f, 0.90117162f, 0.88951606f, 0.87725091f,
0.86438453f, 0.85092574f, 0.83688372f, 0.82226819f, 0.80708915f,
0.79135692f, 0.77508235f, 0.75827658f, 0.74095112f, 0.72311783f,
0.70478898f, 0.68597710f, 0.66669506f, 0.64695615f, 0.62677377f,
0.60616189f, 0.58513457f, 0.56370622f, 0.54189157f, 0.51970547f,
0.49716324f, 0.47428027f, 0.45107225f, 0.42755505f, 0.40374488f,
0.37965798f, 0.35531086f, 0.33072025f, 0.30590299f, 0.28087607f,
0.25565663f, 0.23026201f, 0.20470956f, 0.17901683f, 0.15320139f,
0.12728097f, 0.10127331f, 0.075196236f, 0.049067631f, 0.022905400f,
-0.0032725304f, -0.029448219f, -0.055603724f, -0.081721120f, -0.10778251f,
-0.13377003f, -0.15966587f, -0.18545228f, -0.21111161f, -0.23662624f,
-0.26197869f, -0.28715160f, -0.31212771f, -0.33688989f, -0.36142120f,
-0.38570482f, -0.40972409f, -0.43346253f, -0.45690393f, -0.48003218f,
-0.50283146f, -0.52528608f, -0.54738069f, -0.56910020f, -0.59042966f,
-0.61135447f, -0.63186026f, -0.65193301f, -0.67155898f, -0.69072473f,
-0.70941705f, -0.72762316f, -0.74533063f, -0.76252723f, -0.77920127f,
-0.79534131f, -0.81093621f, -0.82597536f, -0.84044844f, -0.85434550f,
-0.86765707f, -0.88037395f, -0.89248747f, -0.90398932f, -0.91487163f,
-0.92512697f, -0.93474823f, -0.94372886f, -0.95206273f, -0.95974404f,
-0.96676767f, -0.97312868f, -0.97882277f, -0.98384601f, -0.98819500f,
-0.99186671f, -0.99485862f, -0.99716878f, -0.99879545f, -0.99973762f,
};
static const CELTMode mode48000_960_120 = {
48000,  /* Fs */
120,    /* overlap */
21,     /* nbEBands */
21,     /* effEBands */
{0.85000610f, 0.0000000f, 1.0000000f, 1.0000000f, },    /* preemph */
eband5ms,       /* eBands */
3,      /* maxLM */
8,      /* nbShortMdcts */
120,    /* shortMdctSize */
11,     /* nbAllocVectors */
band_allocation,        /* allocVectors */
logN400,        /* logN */
window120,      /* window */
{1920, 3, {&fft_state48000_960_0, &fft_state48000_960_1, &fft_state48000_960_2, &fft_state48000_960_3, }, mdct_twiddles960},    /* mdct */
{392, cache_index50, cache_bits50, cache_caps50},       /* cache */
};
/* List of all the available modes */
#define TOTAL_MODES 1
static const CELTMode * const static_mode_list[TOTAL_MODES] = {
&mode48000_960_120,
};
CELTMode *opus_custom_mode_create(opus_int32 Fs, int frame_size, int *error)
{
   int i;

   for (i=0;i<TOTAL_MODES;i++)
   {
      int j;
      for (j=0;j<4;j++)
      {
         if (Fs == static_mode_list[i]->Fs &&
               (frame_size<<j) == static_mode_list[i]->shortMdctSize*static_mode_list[i]->nbShortMdcts)
         {
            if (error)
               *error = OPUS_OK;
            return (CELTMode*)static_mode_list[i];
         }
      }
   }

   if (error)
      *error = OPUS_BAD_ARG;
   return NULL;
}
static void find_best_pitch(opus_val32 *xcorr, opus_val16 *y, int len,
                            int max_pitch, int *best_pitch
                            )
{
   int i, j;
   opus_val32 Syy=1;
   opus_val16 best_num[2];
   opus_val32 best_den[2];

   best_num[0] = -1;
   best_num[1] = -1;
   best_den[0] = 0;
   best_den[1] = 0;
   best_pitch[0] = 0;
   best_pitch[1] = 1;
   for (j=0;j<len;j++)
      Syy = ADD32(Syy, SHR32(MULT16_16(y[j],y[j]), yshift));
   for (i=0;i<max_pitch;i++)
   {
      if (xcorr[i]>0)
      {
         opus_val16 num;
         opus_val32 xcorr16;
         xcorr16 = EXTRACT16(VSHR32(xcorr[i], xshift));
         /* Considering the range of xcorr16, this should avoid both underflows
            and overflows (inf) when squaring xcorr16 */
         xcorr16 *= 1e-12f;
         num = MULT16_16_Q15(xcorr16,xcorr16);
         if (MULT16_32_Q15(num,best_den[1]) > MULT16_32_Q15(best_num[1],Syy))
         {
            if (MULT16_32_Q15(num,best_den[0]) > MULT16_32_Q15(best_num[0],Syy))
            {
               best_num[1] = best_num[0];
               best_den[1] = best_den[0];
               best_pitch[1] = best_pitch[0];
               best_num[0] = num;
               best_den[0] = Syy;
               best_pitch[0] = i;
            } else {
               best_num[1] = num;
               best_den[1] = Syy;
               best_pitch[1] = i;
            }
         }
      }
      Syy += SHR32(MULT16_16(y[i+len],y[i+len]),yshift) - SHR32(MULT16_16(y[i],y[i]),yshift);
      Syy = MAX32(1, Syy);
   }
}
static void celt_fir5(opus_val16 *x,
         const opus_val16 *num,
         int N)
{
   int i;
   opus_val16 num0, num1, num2, num3, num4;
   opus_val32 mem0, mem1, mem2, mem3, mem4;
   num0=num[0];
   num1=num[1];
   num2=num[2];
   num3=num[3];
   num4=num[4];
   mem0=0;
   mem1=0;
   mem2=0;
   mem3=0;
   mem4=0;
   for (i=0;i<N;i++)
   {
      opus_val32 sum = SHL32(EXTEND32(x[i]), SIG_SHIFT);
      sum = MAC16_16(sum,num0,mem0);
      sum = MAC16_16(sum,num1,mem1);
      sum = MAC16_16(sum,num2,mem2);
      sum = MAC16_16(sum,num3,mem3);
      sum = MAC16_16(sum,num4,mem4);
      mem4 = mem3;
      mem3 = mem2;
      mem2 = mem1;
      mem1 = mem0;
      mem0 = x[i];
      x[i] = ROUND16(sum, SIG_SHIFT);
   }
}
void pitch_downsample(celt_sig * OPUS_RESTRICT x[], opus_val16 * OPUS_RESTRICT x_lp,
      int len, int C, int arch)
{
   int i;
   opus_val32 ac[5];
   opus_val16 tmp=Q15ONE;
   opus_val16 lpc[4];
   opus_val16 lpc2[5];
   opus_val16 c1 = QCONST16(.8f,15);
   for (i=1;i<len>>1;i++)
      x_lp[i] = .25f*x[0][(2*i-1)] + .25f*x[0][(2*i+1)] + .5f*x[0][2*i];
   x_lp[0] = .25f*x[0][1] + .5f*x[0][0];
   if (C==2)
   {
      for (i=1;i<len>>1;i++)
         x_lp[i] += .25f*x[1][(2*i-1)] + .25f*x[1][(2*i+1)] + .5f*x[1][2*i];
      x_lp[0] += .25f*x[1][1] + .5f*x[1][0];
   }
   _celt_autocorr(x_lp, ac, NULL, 0,
                  4, len>>1, arch);

   /* Noise floor -40 dB */
   ac[0] *= 1.0001f;
   /* Lag windowing */
   for (i=1;i<=4;i++)
   {
      /*ac[i] *= exp(-.5*(2*M_PI*.002*i)*(2*M_PI*.002*i));*/
      ac[i] -= ac[i]*(.008f*i)*(.008f*i);
   }

   _celt_lpc(lpc, ac, 4);
   for (i=0;i<4;i++)
   {
      tmp = MULT16_16_Q15(QCONST16(.9f,15), tmp);
      lpc[i] = MULT16_16_Q15(lpc[i], tmp);
   }
   /* Add a zero */
   lpc2[0] = lpc[0] + QCONST16(.8f,SIG_SHIFT);
   lpc2[1] = lpc[1] + MULT16_16_Q15(c1,lpc[0]);
   lpc2[2] = lpc[2] + MULT16_16_Q15(c1,lpc[1]);
   lpc2[3] = lpc[3] + MULT16_16_Q15(c1,lpc[2]);
   lpc2[4] = MULT16_16_Q15(c1,lpc[3]);
   celt_fir5(x_lp, lpc2, len>>1);
}
/* Pure C implementation. */
void
celt_pitch_xcorr_c(const opus_val16 *_x, const opus_val16 *_y,
      opus_val32 *xcorr, int len, int max_pitch, int arch)
{

   int i;
   /*The EDSP version requires that max_pitch is at least 1, and that _x is
      32-bit aligned.
     Since it's hard to put asserts in assembly, put them here.*/
   celt_assert(max_pitch>0);
   celt_sig_assert(((size_t)_x&3)==0);
   for (i=0;i<max_pitch-3;i+=4)
   {
      opus_val32 sum[4]={0,0,0,0};
         xcorr_kernel(_x, _y+i, sum, len, arch);
      xcorr[i]=sum[0];
      xcorr[i+1]=sum[1];
      xcorr[i+2]=sum[2];
      xcorr[i+3]=sum[3];
   }
   /* In case max_pitch isn't a multiple of 4, do non-unrolled version. */
   for (;i<max_pitch;i++)
   {
      opus_val32 sum;
      sum = celt_inner_prod(_x, _y+i, len, arch);
      xcorr[i] = sum;
   }
}
void pitch_search(const opus_val16 * OPUS_RESTRICT x_lp, opus_val16 * OPUS_RESTRICT y,
                  int len, int max_pitch, int *pitch, int arch)
{
   int i, j;
   int lag;
   int best_pitch[2]={0,0};
   VARDECL(opus_val16, x_lp4);
   VARDECL(opus_val16, y_lp4);
   VARDECL(opus_val32, xcorr);
   int offset;

   SAVE_STACK;

   celt_assert(len>0);
   celt_assert(max_pitch>0);
   lag = len+max_pitch;

   ALLOC(x_lp4, len>>2, opus_val16);
   ALLOC(y_lp4, lag>>2, opus_val16);
   ALLOC(xcorr, max_pitch>>1, opus_val32);

   /* Downsample by 2 again */
   for (j=0;j<len>>2;j++)
      x_lp4[j] = x_lp[2*j];
   for (j=0;j<lag>>2;j++)
      y_lp4[j] = y[2*j];

   /* Coarse search with 4x decimation */

   celt_pitch_xcorr(x_lp4, y_lp4, xcorr, len>>2, max_pitch>>2, arch);

   find_best_pitch(xcorr, y_lp4, len>>2, max_pitch>>2, best_pitch
                   );

   /* Finer search with 2x decimation */
   for (i=0;i<max_pitch>>1;i++)
   {
      opus_val32 sum;
      xcorr[i] = 0;
      if (abs(i-2*best_pitch[0])>2 && abs(i-2*best_pitch[1])>2)
         continue;
      sum = celt_inner_prod(x_lp, y+i, len>>1, arch);
      xcorr[i] = MAX32(-1, sum);
   }
   find_best_pitch(xcorr, y, len>>1, max_pitch>>1, best_pitch
                   );

   /* Refine by pseudo-interpolation */
   if (best_pitch[0]>0 && best_pitch[0]<(max_pitch>>1)-1)
   {
      opus_val32 a, b, c;
      a = xcorr[best_pitch[0]-1];
      b = xcorr[best_pitch[0]];
      c = xcorr[best_pitch[0]+1];
      if ((c-a) > MULT16_32_Q15(QCONST16(.7f,15),b-a))
         offset = 1;
      else if ((a-c) > MULT16_32_Q15(QCONST16(.7f,15),b-c))
         offset = -1;
      else
         offset = 0;
   } else {
      offset = 0;
   }
   *pitch = 2*best_pitch[0]-offset;

   RESTORE_STACK;
}
static opus_val16 compute_pitch_gain(opus_val32 xy, opus_val32 xx, opus_val32 yy)
{
   return xy/celt_sqrt(1+xx*yy);
}
static const int second_check[16] = {0, 0, 3, 2, 3, 2, 5, 2, 3, 2, 3, 2, 5, 2, 3, 2};
opus_val16 remove_doubling(opus_val16 *x, int maxperiod, int minperiod,
      int N, int *T0_, int prev_period, opus_val16 prev_gain, int arch)
{
   int k, i, T, T0;
   opus_val16 g, g0;
   opus_val16 pg;
   opus_val32 xy,xx,yy,xy2;
   opus_val32 xcorr[3];
   opus_val32 best_xy, best_yy;
   int offset;
   int minperiod0;
   VARDECL(opus_val32, yy_lookup);
   SAVE_STACK;

   minperiod0 = minperiod;
   maxperiod /= 2;
   minperiod /= 2;
   *T0_ /= 2;
   prev_period /= 2;
   N /= 2;
   x += maxperiod;
   if (*T0_>=maxperiod)
      *T0_=maxperiod-1;

   T = T0 = *T0_;
   ALLOC(yy_lookup, maxperiod+1, opus_val32);
   dual_inner_prod(x, x, x-T0, N, &xx, &xy, arch);
   yy_lookup[0] = xx;
   yy=xx;
   for (i=1;i<=maxperiod;i++)
   {
      yy = yy+MULT16_16(x[-i],x[-i])-MULT16_16(x[N-i],x[N-i]);
      yy_lookup[i] = MAX32(0, yy);
   }
   yy = yy_lookup[T0];
   best_xy = xy;
   best_yy = yy;
   g = g0 = compute_pitch_gain(xy, xx, yy);
   /* Look for any pitch at T/k */
   for (k=2;k<=15;k++)
   {
      int T1, T1b;
      opus_val16 g1;
      opus_val16 cont=0;
      opus_val16 thresh;
      T1 = celt_udiv(2*T0+k, 2*k);
      if (T1 < minperiod)
         break;
      /* Look for another strong correlation at T1b */
      if (k==2)
      {
         if (T1+T0>maxperiod)
            T1b = T0;
         else
            T1b = T0+T1;
      } else
      {
         T1b = celt_udiv(2*second_check[k]*T0+k, 2*k);
      }
      dual_inner_prod(x, &x[-T1], &x[-T1b], N, &xy, &xy2, arch);
      xy = HALF32(xy + xy2);
      yy = HALF32(yy_lookup[T1] + yy_lookup[T1b]);
      g1 = compute_pitch_gain(xy, xx, yy);
      if (abs(T1-prev_period)<=1)
         cont = prev_gain;
      else if (abs(T1-prev_period)<=2 && 5*k*k < T0)
         cont = HALF16(prev_gain);
      else
         cont = 0;
      thresh = MAX16(QCONST16(.3f,15), MULT16_16_Q15(QCONST16(.7f,15),g0)-cont);
      /* Bias against very high pitch (very short period) to avoid false-positives
         due to short-term correlation */
      if (T1<3*minperiod)
         thresh = MAX16(QCONST16(.4f,15), MULT16_16_Q15(QCONST16(.85f,15),g0)-cont);
      else if (T1<2*minperiod)
         thresh = MAX16(QCONST16(.5f,15), MULT16_16_Q15(QCONST16(.9f,15),g0)-cont);
      if (g1 > thresh)
      {
         best_xy = xy;
         best_yy = yy;
         T = T1;
         g = g1;
      }
   }
   best_xy = MAX32(0, best_xy);
   if (best_yy <= best_xy)
      pg = Q15ONE;
   else
      pg = SHR32(frac_div32(best_xy,best_yy+1),16);

   for (k=0;k<3;k++)
      xcorr[k] = celt_inner_prod(x, x-(T+k-1), N, arch);
   if ((xcorr[2]-xcorr[0]) > MULT16_32_Q15(QCONST16(.7f,15),xcorr[1]-xcorr[0]))
      offset = 1;
   else if ((xcorr[0]-xcorr[2]) > MULT16_32_Q15(QCONST16(.7f,15),xcorr[1]-xcorr[2]))
      offset = -1;
   else
      offset = 0;
   if (pg > g)
      pg = g;
   *T0_ = 2*T+offset;

   if (*T0_<minperiod0)
      *T0_=minperiod0;
   RESTORE_STACK;
   return pg;
}
/* Mean energy in each band quantized in Q4 and converted back to float */
const opus_val16 eMeans[25] = {
      6.437500f, 6.250000f, 5.750000f, 5.312500f, 5.062500f,
      4.812500f, 4.500000f, 4.375000f, 4.875000f, 4.687500f,
      4.562500f, 4.437500f, 4.875000f, 4.625000f, 4.312500f,
      4.500000f, 4.375000f, 4.625000f, 4.750000f, 4.437500f,
      3.750000f, 3.750000f, 3.750000f, 3.750000f, 3.750000f
};
/* prediction coefficients: 0.9, 0.8, 0.65, 0.5 */
static const opus_val16 pred_coef[4] = {29440/32768., 26112/32768., 21248/32768., 16384/32768.};
static const opus_val16 beta_coef[4] = {30147/32768., 22282/32768., 12124/32768., 6554/32768.};
static const opus_val16 beta_intra = 4915/32768.;
/*Parameters of the Laplace-like probability models used for the coarse energy.
  There is one pair of parameters for each frame size, prediction type
   (inter/intra), and band number.
  The first number of each pair is the probability of 0, and the second is the
   decay rate, both in Q8 precision.*/
static const unsigned char e_prob_model[4][2][42] = {
   /*120 sample frames.*/
   {
      /*Inter*/
      {
          72, 127,  65, 129,  66, 128,  65, 128,  64, 128,  62, 128,  64, 128,
          64, 128,  92,  78,  92,  79,  92,  78,  90,  79, 116,  41, 115,  40,
         114,  40, 132,  26, 132,  26, 145,  17, 161,  12, 176,  10, 177,  11
      },
      /*Intra*/
      {
          24, 179,  48, 138,  54, 135,  54, 132,  53, 134,  56, 133,  55, 132,
          55, 132,  61, 114,  70,  96,  74,  88,  75,  88,  87,  74,  89,  66,
          91,  67, 100,  59, 108,  50, 120,  40, 122,  37,  97,  43,  78,  50
      }
   },
   /*240 sample frames.*/
   {
      /*Inter*/
      {
          83,  78,  84,  81,  88,  75,  86,  74,  87,  71,  90,  73,  93,  74,
          93,  74, 109,  40, 114,  36, 117,  34, 117,  34, 143,  17, 145,  18,
         146,  19, 162,  12, 165,  10, 178,   7, 189,   6, 190,   8, 177,   9
      },
      /*Intra*/
      {
          23, 178,  54, 115,  63, 102,  66,  98,  69,  99,  74,  89,  71,  91,
          73,  91,  78,  89,  86,  80,  92,  66,  93,  64, 102,  59, 103,  60,
         104,  60, 117,  52, 123,  44, 138,  35, 133,  31,  97,  38,  77,  45
      }
   },
   /*480 sample frames.*/
   {
      /*Inter*/
      {
          61,  90,  93,  60, 105,  42, 107,  41, 110,  45, 116,  38, 113,  38,
         112,  38, 124,  26, 132,  27, 136,  19, 140,  20, 155,  14, 159,  16,
         158,  18, 170,  13, 177,  10, 187,   8, 192,   6, 175,   9, 159,  10
      },
      /*Intra*/
      {
          21, 178,  59, 110,  71,  86,  75,  85,  84,  83,  91,  66,  88,  73,
          87,  72,  92,  75,  98,  72, 105,  58, 107,  54, 115,  52, 114,  55,
         112,  56, 129,  51, 132,  40, 150,  33, 140,  29,  98,  35,  77,  42
      }
   },
   /*960 sample frames.*/
   {
      /*Inter*/
      {
          42, 121,  96,  66, 108,  43, 111,  40, 117,  44, 123,  32, 120,  36,
         119,  33, 127,  33, 134,  34, 139,  21, 147,  23, 152,  20, 158,  25,
         154,  26, 166,  21, 173,  16, 184,  13, 184,  10, 150,  13, 139,  15
      },
      /*Intra*/
      {
          22, 178,  63, 114,  74,  82,  84,  83,  92,  82, 103,  62,  96,  72,
          96,  67, 101,  73, 107,  72, 113,  55, 118,  52, 125,  52, 118,  52,
         117,  55, 135,  49, 137,  39, 157,  32, 145,  29,  97,  33,  77,  40
      }
   }
};
static const unsigned char small_energy_icdf[3]={2,1,0};
static opus_val32 loss_distortion(const opus_val16 *eBands, opus_val16 *oldEBands, int start, int end, int len, int C)
{
   int c, i;
   opus_val32 dist = 0;
   c=0; do {
      for (i=start;i<end;i++)
      {
         opus_val16 d = SUB16(SHR16(eBands[i+c*len], 3), SHR16(oldEBands[i+c*len], 3));
         dist = MAC16_16(dist, d,d);
      }
   } while (++c<C);
   return MIN32(200,SHR32(dist,2*DB_SHIFT-6));
}
static int quant_coarse_energy_impl(const CELTMode *m, int start, int end,
      const opus_val16 *eBands, opus_val16 *oldEBands,
      opus_int32 budget, opus_int32 tell,
      const unsigned char *prob_model, opus_val16 *error, ec_enc *enc,
      int C, int LM, int intra, opus_val16 max_decay, int lfe)
{
   int i, c;
   int badness = 0;
   opus_val32 prev[2] = {0,0};
   opus_val16 coef;
   opus_val16 beta;

   if (tell+3 <= budget)
      ec_enc_bit_logp(enc, intra, 3);
   if (intra)
   {
      coef = 0;
      beta = beta_intra;
   } else {
      beta = beta_coef[LM];
      coef = pred_coef[LM];
   }

   /* Encode at a fixed coarse resolution */
   for (i=start;i<end;i++)
   {
      c=0;
      do {
         int bits_left;
         int qi, qi0;
         opus_val32 q;
         opus_val16 x;
         opus_val32 f, tmp;
         opus_val16 oldE;
         opus_val16 decay_bound;
         x = eBands[i+c*m->nbEBands];
         oldE = MAX16(-QCONST16(9.f,DB_SHIFT), oldEBands[i+c*m->nbEBands]);
         f = x-coef*oldE-prev[c];
         /* Rounding to nearest integer here is really important! */
         qi = (int)floor(.5f+f);
         decay_bound = MAX16(-QCONST16(28.f,DB_SHIFT), oldEBands[i+c*m->nbEBands]) - max_decay;
         /* Prevent the energy from going down too quickly (e.g. for bands
            that have just one bin) */
         if (qi < 0 && x < decay_bound)
         {
            qi += (int)SHR16(SUB16(decay_bound,x), DB_SHIFT);
            if (qi > 0)
               qi = 0;
         }
         qi0 = qi;
         /* If we don't have enough bits to encode all the energy, just assume
             something safe. */
         tell = ec_tell(enc);
         bits_left = budget-tell-3*C*(end-i);
         if (i!=start && bits_left < 30)
         {
            if (bits_left < 24)
               qi = IMIN(1, qi);
            if (bits_left < 16)
               qi = IMAX(-1, qi);
         }
         if (lfe && i>=2)
            qi = IMIN(qi, 0);
         if (budget-tell >= 15)
         {
            int pi;
            pi = 2*IMIN(i,20);
            ec_laplace_encode(enc, &qi,
                  prob_model[pi]<<7, prob_model[pi+1]<<6);
         }
         else if(budget-tell >= 2)
         {
            qi = IMAX(-1, IMIN(qi, 1));
            ec_enc_icdf(enc, 2*qi^-(qi<0), small_energy_icdf, 2);
         }
         else if(budget-tell >= 1)
         {
            qi = IMIN(0, qi);
            ec_enc_bit_logp(enc, -qi, 1);
         }
         else
            qi = -1;
         error[i+c*m->nbEBands] = PSHR32(f,7) - SHL16(qi,DB_SHIFT);
         badness += abs(qi0-qi);
         q = (opus_val32)SHL32(EXTEND32(qi),DB_SHIFT);

         tmp = PSHR32(MULT16_16(coef,oldE),8) + prev[c] + SHL32(q,7);
         oldEBands[i+c*m->nbEBands] = PSHR32(tmp, 7);
         prev[c] = prev[c] + SHL32(q,7) - MULT16_16(beta,PSHR32(q,8));
      } while (++c < C);
   }
   return lfe ? 0 : badness;
}
void quant_coarse_energy(const CELTMode *m, int start, int end, int effEnd,
      const opus_val16 *eBands, opus_val16 *oldEBands, opus_uint32 budget,
      opus_val16 *error, ec_enc *enc, int C, int LM, int nbAvailableBytes,
      int force_intra, opus_val32 *delayedIntra, int two_pass, int loss_rate, int lfe)
{
   int intra;
   opus_val16 max_decay;
   VARDECL(opus_val16, oldEBands_intra);
   VARDECL(opus_val16, error_intra);
   ec_enc enc_start_state;
   opus_uint32 tell;
   int badness1=0;
   opus_int32 intra_bias;
   opus_val32 new_distortion;
   SAVE_STACK;

   intra = force_intra || (!two_pass && *delayedIntra>2*C*(end-start) && nbAvailableBytes > (end-start)*C);
   intra_bias = (opus_int32)((budget**delayedIntra*loss_rate)/(C*512));
   new_distortion = loss_distortion(eBands, oldEBands, start, effEnd, m->nbEBands, C);

   tell = ec_tell(enc);
   if (tell+3 > budget)
      two_pass = intra = 0;

   max_decay = QCONST16(16.f,DB_SHIFT);
   if (end-start>10)
   {
      max_decay = MIN32(max_decay, .125f*nbAvailableBytes);
   }
   if (lfe)
      max_decay = QCONST16(3.f,DB_SHIFT);
   enc_start_state = *enc;

   ALLOC(oldEBands_intra, C*m->nbEBands, opus_val16);
   ALLOC(error_intra, C*m->nbEBands, opus_val16);
   OPUS_COPY(oldEBands_intra, oldEBands, C*m->nbEBands);

   if (two_pass || intra)
   {
      badness1 = quant_coarse_energy_impl(m, start, end, eBands, oldEBands_intra, budget,
            tell, e_prob_model[LM][1], error_intra, enc, C, LM, 1, max_decay, lfe);
   }

   if (!intra)
   {
      unsigned char *intra_buf;
      ec_enc enc_intra_state;
      opus_int32 tell_intra;
      opus_uint32 nstart_bytes;
      opus_uint32 nintra_bytes;
      opus_uint32 save_bytes;
      int badness2;
      VARDECL(unsigned char, intra_bits);

      tell_intra = ec_tell_frac(enc);

      enc_intra_state = *enc;

      nstart_bytes = ec_range_bytes(&enc_start_state);
      nintra_bytes = ec_range_bytes(&enc_intra_state);
      intra_buf = ec_get_buffer(&enc_intra_state) + nstart_bytes;
      save_bytes = nintra_bytes-nstart_bytes;
      if (save_bytes == 0)
         save_bytes = ALLOC_NONE;
      ALLOC(intra_bits, save_bytes, unsigned char);
      /* Copy bits from intra bit-stream */
      OPUS_COPY(intra_bits, intra_buf, nintra_bytes - nstart_bytes);

      *enc = enc_start_state;

      badness2 = quant_coarse_energy_impl(m, start, end, eBands, oldEBands, budget,
            tell, e_prob_model[LM][intra], error, enc, C, LM, 0, max_decay, lfe);

      if (two_pass && (badness1 < badness2 || (badness1 == badness2 && ((opus_int32)ec_tell_frac(enc))+intra_bias > tell_intra)))
      {
         *enc = enc_intra_state;
         /* Copy intra bits to bit-stream */
         OPUS_COPY(intra_buf, intra_bits, nintra_bytes - nstart_bytes);
         OPUS_COPY(oldEBands, oldEBands_intra, C*m->nbEBands);
         OPUS_COPY(error, error_intra, C*m->nbEBands);
         intra = 1;
      }
   } else {
      OPUS_COPY(oldEBands, oldEBands_intra, C*m->nbEBands);
      OPUS_COPY(error, error_intra, C*m->nbEBands);
   }

   if (intra)
      *delayedIntra = new_distortion;
   else
      *delayedIntra = ADD32(MULT16_32_Q15(MULT16_16_Q15(pred_coef[LM], pred_coef[LM]),*delayedIntra),
            new_distortion);

   RESTORE_STACK;
}
void quant_fine_energy(const CELTMode *m, int start, int end, opus_val16 *oldEBands, opus_val16 *error, int *fine_quant, ec_enc *enc, int C)
{
   int i, c;

   /* Encode finer resolution */
   for (i=start;i<end;i++)
   {
      opus_int16 frac = 1<<fine_quant[i];
      if (fine_quant[i] <= 0)
         continue;
      c=0;
      do {
         int q2;
         opus_val16 offset;
         q2 = (int)floor((error[i+c*m->nbEBands]+.5f)*frac);
         if (q2 > frac-1)
            q2 = frac-1;
         if (q2<0)
            q2 = 0;
         ec_enc_bits(enc, q2, fine_quant[i]);
         offset = (q2+.5f)*(1<<(14-fine_quant[i]))*(1.f/16384) - .5f;
         oldEBands[i+c*m->nbEBands] += offset;
         error[i+c*m->nbEBands] -= offset;
         /*printf ("%f ", error[i] - offset);*/
      } while (++c < C);
   }
}
void quant_energy_finalise(const CELTMode *m, int start, int end, opus_val16 *oldEBands, opus_val16 *error, int *fine_quant, int *fine_priority, int bits_left, ec_enc *enc, int C)
{
   int i, prio, c;

   /* Use up the remaining bits */
   for (prio=0;prio<2;prio++)
   {
      for (i=start;i<end && bits_left>=C ;i++)
      {
         if (fine_quant[i] >= MAX_FINE_BITS || fine_priority[i]!=prio)
            continue;
         c=0;
         do {
            int q2;
            opus_val16 offset;
            q2 = error[i+c*m->nbEBands]<0 ? 0 : 1;
            ec_enc_bits(enc, q2, 1);
            offset = (q2-.5f)*(1<<(14-fine_quant[i]-1))*(1.f/16384);
            oldEBands[i+c*m->nbEBands] += offset;
            error[i+c*m->nbEBands] -= offset;
            bits_left--;
         } while (++c < C);
      }
   }
}
void amp2Log2(const CELTMode *m, int effEnd, int end,
      celt_ener *bandE, opus_val16 *bandLogE, int C)
{
   int c, i;
   c=0;
   do {
      for (i=0;i<effEnd;i++)
      {
         bandLogE[i+c*m->nbEBands] =
               celt_log2(bandE[i+c*m->nbEBands])
               - SHL16((opus_val16)eMeans[i],6);
      }
      for (i=effEnd;i<end;i++)
         bandLogE[c*m->nbEBands+i] = -QCONST16(14.f,DB_SHIFT);
   } while (++c < C);
}
/* Copyright (c) 2001-2011 Timothy B. Terriberry
   Copyright (c) 2008-2009 Xiph.Org Foundation */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

- Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
static const unsigned char LOG2_FRAC_TABLE[24]={
   0,
   8,13,
  16,19,21,23,
  24,26,27,28,29,30,31,32,
  32,33,34,34,35,36,36,37,37
};
#define ALLOC_STEPS 6
static OPUS_INLINE int interp_bits2pulses(const CELTMode *m, int start, int end, int skip_start,
      const int *bits1, const int *bits2, const int *thresh, const int *cap, opus_int32 total, opus_int32 *_balance,
      int skip_rsv, int *intensity, int intensity_rsv, int *dual_stereo, int dual_stereo_rsv, int *bits,
      int *ebits, int *fine_priority, int C, int LM, ec_ctx *ec, int encode, int prev, int signalBandwidth)
{
   opus_int32 psum;
   int lo, hi;
   int i, j;
   int logM;
   int stereo;
   int codedBands=-1;
   int alloc_floor;
   opus_int32 left, percoeff;
   int done;
   opus_int32 balance;
   SAVE_STACK;

   alloc_floor = C<<BITRES;
   stereo = C>1;

   logM = LM<<BITRES;
   lo = 0;
   hi = 1<<ALLOC_STEPS;
   for (i=0;i<ALLOC_STEPS;i++)
   {
      int mid = (lo+hi)>>1;
      psum = 0;
      done = 0;
      for (j=end;j-->start;)
      {
         int tmp = bits1[j] + (mid*(opus_int32)bits2[j]>>ALLOC_STEPS);
         if (tmp >= thresh[j] || done)
         {
            done = 1;
            /* Don't allocate more than we can actually use */
            psum += IMIN(tmp, cap[j]);
         } else {
            if (tmp >= alloc_floor)
               psum += alloc_floor;
         }
      }
      if (psum > total)
         hi = mid;
      else
         lo = mid;
   }
   psum = 0;
   /*printf ("interp bisection gave %d\n", lo);*/
   done = 0;
   for (j=end;j-->start;)
   {
      int tmp = bits1[j] + ((opus_int32)lo*bits2[j]>>ALLOC_STEPS);
      if (tmp < thresh[j] && !done)
      {
         if (tmp >= alloc_floor)
            tmp = alloc_floor;
         else
            tmp = 0;
      } else
         done = 1;
      /* Don't allocate more than we can actually use */
      tmp = IMIN(tmp, cap[j]);
      bits[j] = tmp;
      psum += tmp;
   }

   /* Decide which bands to skip, working backwards from the end. */
   for (codedBands=end;;codedBands--)
   {
      int band_width;
      int band_bits;
      int rem;
      j = codedBands-1;
      /* Never skip the first band, nor a band that has been boosted by
          dynalloc.
         In the first case, we'd be coding a bit to signal we're going to waste
          all the other bits.
         In the second case, we'd be coding a bit to redistribute all the bits
          we just signaled should be cocentrated in this band. */
      if (j<=skip_start)
      {
         /* Give the bit we reserved to end skipping back. */
         total += skip_rsv;
         break;
      }
      /*Figure out how many left-over bits we would be adding to this band.
        This can include bits we've stolen back from higher, skipped bands.*/
      left = total-psum;
      percoeff = celt_udiv(left, m->eBands[codedBands]-m->eBands[start]);
      left -= (m->eBands[codedBands]-m->eBands[start])*percoeff;
      rem = IMAX(left-(m->eBands[j]-m->eBands[start]),0);
      band_width = m->eBands[codedBands]-m->eBands[j];
      band_bits = (int)(bits[j] + percoeff*band_width + rem);
      /*Only code a skip decision if we're above the threshold for this band.
        Otherwise it is force-skipped.
        This ensures that we have enough bits to code the skip flag.*/
      if (band_bits >= IMAX(thresh[j], alloc_floor+(1<<BITRES)))
      {
         if (encode)
         {
            /*This if() block is the only part of the allocation function that
               is not a mandatory part of the bitstream: any bands we choose to
               skip here must be explicitly signaled.*/
            int depth_threshold;
            /*We choose a threshold with some hysteresis to keep bands from
               fluctuating in and out, but we try not to fold below a certain point. */
            if (codedBands > 17)
               depth_threshold = j<prev ? 7 : 9;
            else
               depth_threshold = 0;
            if (codedBands<=start+2 || (band_bits > (depth_threshold*band_width<<LM<<BITRES)>>4 && j<=signalBandwidth))
            {
               ec_enc_bit_logp(ec, 1, 1);
               break;
            }
            ec_enc_bit_logp(ec, 0, 1);
         } else if (ec_dec_bit_logp(ec, 1)) {
            break;
         }
         /*We used a bit to skip this band.*/
         psum += 1<<BITRES;
         band_bits -= 1<<BITRES;
      }
      /*Reclaim the bits originally allocated to this band.*/
      psum -= bits[j]+intensity_rsv;
      if (intensity_rsv > 0)
         intensity_rsv = LOG2_FRAC_TABLE[j-start];
      psum += intensity_rsv;
      if (band_bits >= alloc_floor)
      {
         /*If we have enough for a fine energy bit per channel, use it.*/
         psum += alloc_floor;
         bits[j] = alloc_floor;
      } else {
         /*Otherwise this band gets nothing at all.*/
         bits[j] = 0;
      }
   }

   celt_assert(codedBands > start);
   /* Code the intensity and dual stereo parameters. */
   if (intensity_rsv > 0)
   {
      if (encode)
      {
         *intensity = IMIN(*intensity, codedBands);
         ec_enc_uint(ec, *intensity-start, codedBands+1-start);
      }
      else
         *intensity = start+ec_dec_uint(ec, codedBands+1-start);
   }
   else
      *intensity = 0;
   if (*intensity <= start)
   {
      total += dual_stereo_rsv;
      dual_stereo_rsv = 0;
   }
   if (dual_stereo_rsv > 0)
   {
      if (encode)
         ec_enc_bit_logp(ec, *dual_stereo, 1);
      else
         *dual_stereo = ec_dec_bit_logp(ec, 1);
   }
   else
      *dual_stereo = 0;

   /* Allocate the remaining bits */
   left = total-psum;
   percoeff = celt_udiv(left, m->eBands[codedBands]-m->eBands[start]);
   left -= (m->eBands[codedBands]-m->eBands[start])*percoeff;
   for (j=start;j<codedBands;j++)
      bits[j] += ((int)percoeff*(m->eBands[j+1]-m->eBands[j]));
   for (j=start;j<codedBands;j++)
   {
      int tmp = (int)IMIN(left, m->eBands[j+1]-m->eBands[j]);
      bits[j] += tmp;
      left -= tmp;
   }
   /*for (j=0;j<end;j++)printf("%d ", bits[j]);printf("\n");*/

   balance = 0;
   for (j=start;j<codedBands;j++)
   {
      int N0, N, den;
      int offset;
      int NClogN;
      opus_int32 excess, bit;

      celt_assert(bits[j] >= 0);
      N0 = m->eBands[j+1]-m->eBands[j];
      N=N0<<LM;
      bit = (opus_int32)bits[j]+balance;

      if (N>1)
      {
         excess = MAX32(bit-cap[j],0);
         bits[j] = bit-excess;

         /* Compensate for the extra DoF in stereo */
         den=(C*N+ ((C==2 && N>2 && !*dual_stereo && j<*intensity) ? 1 : 0));

         NClogN = den*(m->logN[j] + logM);

         /* Offset for the number of fine bits by log2(N)/2 + FINE_OFFSET
            compared to their "fair share" of total/N */
         offset = (NClogN>>1)-den*FINE_OFFSET;

         /* N=2 is the only point that doesn't match the curve */
         if (N==2)
            offset += den<<BITRES>>2;

         /* Changing the offset for allocating the second and third
             fine energy bit */
         if (bits[j] + offset < den*2<<BITRES)
            offset += NClogN>>2;
         else if (bits[j] + offset < den*3<<BITRES)
            offset += NClogN>>3;

         /* Divide with rounding */
         ebits[j] = IMAX(0, (bits[j] + offset + (den<<(BITRES-1))));
         ebits[j] = celt_udiv(ebits[j], den)>>BITRES;

         /* Make sure not to bust */
         if (C*ebits[j] > (bits[j]>>BITRES))
            ebits[j] = bits[j] >> stereo >> BITRES;

         /* More than that is useless because that's about as far as PVQ can go */
         ebits[j] = IMIN(ebits[j], MAX_FINE_BITS);

         /* If we rounded down or capped this band, make it a candidate for the
             final fine energy pass */
         fine_priority[j] = ebits[j]*(den<<BITRES) >= bits[j]+offset;

         /* Remove the allocated fine bits; the rest are assigned to PVQ */
         bits[j] -= C*ebits[j]<<BITRES;

      } else {
         /* For N=1, all bits go to fine energy except for a single sign bit */
         excess = MAX32(0,bit-(C<<BITRES));
         bits[j] = bit-excess;
         ebits[j] = 0;
         fine_priority[j] = 1;
      }

      /* Fine energy can't take advantage of the re-balancing in
          quant_all_bands().
         Instead, do the re-balancing here.*/
      if(excess > 0)
      {
         int extra_fine;
         int extra_bits;
         extra_fine = IMIN(excess>>(stereo+BITRES),MAX_FINE_BITS-ebits[j]);
         ebits[j] += extra_fine;
         extra_bits = extra_fine*C<<BITRES;
         fine_priority[j] = extra_bits >= excess-balance;
         excess -= extra_bits;
      }
      balance = excess;

      celt_assert(bits[j] >= 0);
      celt_assert(ebits[j] >= 0);
   }
   /* Save any remaining bits over the cap for the rebalancing in
       quant_all_bands(). */
   *_balance = balance;

   /* The skipped bands use all their bits for fine energy. */
   for (;j<end;j++)
   {
      ebits[j] = bits[j] >> stereo >> BITRES;
      celt_assert(C*ebits[j]<<BITRES == bits[j]);
      bits[j] = 0;
      fine_priority[j] = ebits[j]<1;
   }
   RESTORE_STACK;
   return codedBands;
}
int clt_compute_allocation(const CELTMode *m, int start, int end, const int *offsets, const int *cap, int alloc_trim, int *intensity, int *dual_stereo,
      opus_int32 total, opus_int32 *balance, int *pulses, int *ebits, int *fine_priority, int C, int LM, ec_ctx *ec, int encode, int prev, int signalBandwidth)
{
   int lo, hi, len, j;
   int codedBands;
   int skip_start;
   int skip_rsv;
   int intensity_rsv;
   int dual_stereo_rsv;
   VARDECL(int, bits1);
   VARDECL(int, bits2);
   VARDECL(int, thresh);
   VARDECL(int, trim_offset);
   SAVE_STACK;

   total = IMAX(total, 0);
   len = m->nbEBands;
   skip_start = start;
   /* Reserve a bit to signal the end of manually skipped bands. */
   skip_rsv = total >= 1<<BITRES ? 1<<BITRES : 0;
   total -= skip_rsv;
   /* Reserve bits for the intensity and dual stereo parameters. */
   intensity_rsv = dual_stereo_rsv = 0;
   if (C==2)
   {
      intensity_rsv = LOG2_FRAC_TABLE[end-start];
      if (intensity_rsv>total)
         intensity_rsv = 0;
      else
      {
         total -= intensity_rsv;
         dual_stereo_rsv = total>=1<<BITRES ? 1<<BITRES : 0;
         total -= dual_stereo_rsv;
      }
   }
   ALLOC(bits1, len, int);
   ALLOC(bits2, len, int);
   ALLOC(thresh, len, int);
   ALLOC(trim_offset, len, int);

   for (j=start;j<end;j++)
   {
      /* Below this threshold, we're sure not to allocate any PVQ bits */
      thresh[j] = IMAX((C)<<BITRES, (3*(m->eBands[j+1]-m->eBands[j])<<LM<<BITRES)>>4);
      /* Tilt of the allocation curve */
      trim_offset[j] = C*(m->eBands[j+1]-m->eBands[j])*(alloc_trim-5-LM)*(end-j-1)
            *(1<<(LM+BITRES))>>6;
      /* Giving less resolution to single-coefficient bands because they get
         more benefit from having one coarse value per coefficient*/
      if ((m->eBands[j+1]-m->eBands[j])<<LM==1)
         trim_offset[j] -= C<<BITRES;
   }
   lo = 1;
   hi = m->nbAllocVectors - 1;
   do
   {
      int done = 0;
      int psum = 0;
      int mid = (lo+hi) >> 1;
      for (j=end;j-->start;)
      {
         int bitsj;
         int N = m->eBands[j+1]-m->eBands[j];
         bitsj = C*N*m->allocVectors[mid*len+j]<<LM>>2;
         if (bitsj > 0)
            bitsj = IMAX(0, bitsj + trim_offset[j]);
         bitsj += offsets[j];
         if (bitsj >= thresh[j] || done)
         {
            done = 1;
            /* Don't allocate more than we can actually use */
            psum += IMIN(bitsj, cap[j]);
         } else {
            if (bitsj >= C<<BITRES)
               psum += C<<BITRES;
         }
      }
      if (psum > total)
         hi = mid - 1;
      else
         lo = mid + 1;
      /*printf ("lo = %d, hi = %d\n", lo, hi);*/
   }
   while (lo <= hi);
   hi = lo--;
   /*printf ("interp between %d and %d\n", lo, hi);*/
   for (j=start;j<end;j++)
   {
      int bits1j, bits2j;
      int N = m->eBands[j+1]-m->eBands[j];
      bits1j = C*N*m->allocVectors[lo*len+j]<<LM>>2;
      bits2j = hi>=m->nbAllocVectors ?
            cap[j] : C*N*m->allocVectors[hi*len+j]<<LM>>2;
      if (bits1j > 0)
         bits1j = IMAX(0, bits1j + trim_offset[j]);
      if (bits2j > 0)
         bits2j = IMAX(0, bits2j + trim_offset[j]);
      if (lo > 0)
         bits1j += offsets[j];
      bits2j += offsets[j];
      if (offsets[j]>0)
         skip_start = j;
      bits2j = IMAX(0,bits2j-bits1j);
      bits1[j] = bits1j;
      bits2[j] = bits2j;
   }
   codedBands = interp_bits2pulses(m, start, end, skip_start, bits1, bits2, thresh, cap,
         total, balance, skip_rsv, intensity, intensity_rsv, dual_stereo, dual_stereo_rsv,
         pulses, ebits, fine_priority, C, LM, ec, encode, prev, signalBandwidth);
   RESTORE_STACK;
   return codedBands;
}
static void exp_rotation1(celt_norm *X, int len, int stride, opus_val16 c, opus_val16 s)
{
   int i;
   opus_val16 ms;
   celt_norm *Xptr;
   Xptr = X;
   ms = NEG16(s);
   for (i=0;i<len-stride;i++)
   {
      celt_norm x1, x2;
      x1 = Xptr[0];
      x2 = Xptr[stride];
      Xptr[stride] = EXTRACT16(PSHR32(MAC16_16(MULT16_16(c, x2),  s, x1), 15));
      *Xptr++      = EXTRACT16(PSHR32(MAC16_16(MULT16_16(c, x1), ms, x2), 15));
   }
   Xptr = &X[len-2*stride-1];
   for (i=len-2*stride-1;i>=0;i--)
   {
      celt_norm x1, x2;
      x1 = Xptr[0];
      x2 = Xptr[stride];
      Xptr[stride] = EXTRACT16(PSHR32(MAC16_16(MULT16_16(c, x2),  s, x1), 15));
      *Xptr--      = EXTRACT16(PSHR32(MAC16_16(MULT16_16(c, x1), ms, x2), 15));
   }
}
void exp_rotation(celt_norm *X, int len, int dir, int stride, int K, int spread)
{
   static const int SPREAD_FACTOR[3]={15,10,5};
   int i;
   opus_val16 c, s;
   opus_val16 gain, theta;
   int stride2=0;
   int factor;

   if (2*K>=len || spread==SPREAD_NONE)
      return;
   factor = SPREAD_FACTOR[spread-1];

   gain = celt_div((opus_val32)MULT16_16(Q15_ONE,len),(opus_val32)(len+factor*K));
   theta = HALF16(MULT16_16_Q15(gain,gain));

   c = celt_cos_norm(EXTEND32(theta));
   s = celt_cos_norm(EXTEND32(SUB16(Q15ONE,theta))); /*  sin(theta) */

   if (len>=8*stride)
   {
      stride2 = 1;
      /* This is just a simple (equivalent) way of computing sqrt(len/stride) with rounding.
         It's basically incrementing long as (stride2+0.5)^2 < len/stride. */
      while ((stride2*stride2+stride2)*stride + (stride>>2) < len)
         stride2++;
   }
   /*NOTE: As a minor optimization, we could be passing around log2(B), not B, for both this and for
      extract_collapse_mask().*/
   len = celt_udiv(len, stride);
   for (i=0;i<stride;i++)
   {
      if (dir < 0)
      {
         if (stride2)
            exp_rotation1(X+i*len, len, stride2, s, c);
         exp_rotation1(X+i*len, len, 1, c, s);
      } else {
         exp_rotation1(X+i*len, len, 1, c, -s);
         if (stride2)
            exp_rotation1(X+i*len, len, stride2, s, -c);
      }
   }
}
static void normalise_residual(int * OPUS_RESTRICT iy, celt_norm * OPUS_RESTRICT X,
      int N, opus_val32 Ryy, opus_val16 gain)
{
   int i;
   opus_val32 t;
   opus_val16 g;

   t = VSHR32(Ryy, 2*(k-7));
   g = MULT16_16_P15(celt_rsqrt_norm(t),gain);

   i=0;
   do
      X[i] = EXTRACT16(PSHR32(MULT16_16(g, iy[i]), k+1));
   while (++i < N);
}
static unsigned extract_collapse_mask(int *iy, int N, int B)
{
   unsigned collapse_mask;
   int N0;
   int i;
   if (B<=1)
      return 1;
   /*NOTE: As a minor optimization, we could be passing around log2(B), not B, for both this and for
      exp_rotation().*/
   N0 = celt_udiv(N, B);
   collapse_mask = 0;
   i=0; do {
      int j;
      unsigned tmp=0;
      j=0; do {
         tmp |= iy[i*N0+j];
      } while (++j<N0);
      collapse_mask |= (tmp!=0)<<i;
   } while (++i<B);
   return collapse_mask;
}
opus_val16 op_pvq_search_c(celt_norm *X, int *iy, int K, int N, int arch)
{
   VARDECL(celt_norm, y);
   VARDECL(int, signx);
   int i, j;
   int pulsesLeft;
   opus_val32 sum;
   opus_val32 xy;
   opus_val16 yy;
   SAVE_STACK;

   (void)arch;
   ALLOC(y, N, celt_norm);
   ALLOC(signx, N, int);

   /* Get rid of the sign */
   sum = 0;
   j=0; do {
      signx[j] = X[j]<0;
      /* OPT: Make sure the compiler doesn't use a branch on ABS16(). */
      X[j] = ABS16(X[j]);
      iy[j] = 0;
      y[j] = 0;
   } while (++j<N);

   xy = yy = 0;

   pulsesLeft = K;

   /* Do a pre-search by projecting on the pyramid */
   if (K > (N>>1))
   {
      opus_val16 rcp;
      j=0; do {
         sum += X[j];
      }  while (++j<N);

      /* If X is too small, just replace it with a pulse at 0 */
      /* Prevents infinities and NaNs from causing too many pulses
         to be allocated. 64 is an approximation of infinity here. */
      if (!(sum > EPSILON && sum < 64))
      {
         X[0] = QCONST16(1.f,14);
         j=1; do
            X[j]=0;
         while (++j<N);
         sum = QCONST16(1.f,14);
      }
      /* Using K+e with e < 1 guarantees we cannot get more than K pulses. */
      rcp = EXTRACT16(MULT16_32_Q16(K+0.8f, celt_rcp(sum)));
      j=0; do {
         iy[j] = (int)floor(rcp*X[j]);
         y[j] = (celt_norm)iy[j];
         yy = MAC16_16(yy, y[j],y[j]);
         xy = MAC16_16(xy, X[j],y[j]);
         y[j] *= 2;
         pulsesLeft -= iy[j];
      }  while (++j<N);
   }
   celt_sig_assert(pulsesLeft>=0);

   /* This should never happen, but just in case it does (e.g. on silence)
      we fill the first bin with pulses. */
   if (pulsesLeft > N+3)
   {
      opus_val16 tmp = (opus_val16)pulsesLeft;
      yy = MAC16_16(yy, tmp, tmp);
      yy = MAC16_16(yy, tmp, y[0]);
      iy[0] += pulsesLeft;
      pulsesLeft=0;
   }

   for (i=0;i<pulsesLeft;i++)
   {
      opus_val16 Rxy, Ryy;
      int best_id;
      opus_val32 best_num;
      opus_val16 best_den;
      best_id = 0;
      /* The squared magnitude term gets added anyway, so we might as well
         add it outside the loop */
      yy = ADD16(yy, 1);

      /* Calculations for position 0 are out of the loop, in part to reduce
         mispredicted branches (since the if condition is usually false)
         in the loop. */
      /* Temporary sums of the new pulse(s) */
      Rxy = EXTRACT16(SHR32(ADD32(xy, EXTEND32(X[0])),rshift));
      /* We're multiplying y[j] by two so we don't have to do it here */
      Ryy = ADD16(yy, y[0]);

      /* Approximate score: we maximise Rxy/sqrt(Ryy) (we're guaranteed that
         Rxy is positive because the sign is pre-computed) */
      Rxy = MULT16_16_Q15(Rxy,Rxy);
      best_den = Ryy;
      best_num = Rxy;
      j=1;
      do {
         /* Temporary sums of the new pulse(s) */
         Rxy = EXTRACT16(SHR32(ADD32(xy, EXTEND32(X[j])),rshift));
         /* We're multiplying y[j] by two so we don't have to do it here */
         Ryy = ADD16(yy, y[j]);

         /* Approximate score: we maximise Rxy/sqrt(Ryy) (we're guaranteed that
            Rxy is positive because the sign is pre-computed) */
         Rxy = MULT16_16_Q15(Rxy,Rxy);
         /* The idea is to check for num/den >= best_num/best_den, but that way
            we can do it without any division */
         /* OPT: It's not clear whether a cmov is faster than a branch here
            since the condition is more often false than true and using
            a cmov introduces data dependencies across iterations. The optimal
            choice may be architecture-dependent. */
         if (opus_unlikely(MULT16_16(best_den, Rxy) > MULT16_16(Ryy, best_num)))
         {
            best_den = Ryy;
            best_num = Rxy;
            best_id = j;
         }
      } while (++j<N);

      /* Updating the sums of the new pulse(s) */
      xy = ADD32(xy, EXTEND32(X[best_id]));
      /* We're multiplying y[j] by two so we don't have to do it here */
      yy = ADD16(yy, y[best_id]);

      /* Only now that we've made the final choice, update y/iy */
      /* Multiplying y[j] by 2 so we don't have to do it everywhere else */
      y[best_id] += 2;
      iy[best_id]++;
   }

   /* Put the original sign back */
   j=0;
   do {
      /*iy[j] = signx[j] ? -iy[j] : iy[j];*/
      /* OPT: The is more likely to be compiled without a branch than the code above
         but has the same performance otherwise. */
      iy[j] = (iy[j]^-signx[j]) + signx[j];
   } while (++j<N);
   RESTORE_STACK;
   return yy;
}
unsigned alg_quant(celt_norm *X, int N, int K, int spread, int B, ec_enc *enc,
      opus_val16 gain, int resynth, int arch)
{
   VARDECL(int, iy);
   opus_val16 yy;
   unsigned collapse_mask;
   SAVE_STACK;

   celt_assert2(K>0, "alg_quant() needs at least one pulse");
   celt_assert2(N>1, "alg_quant() needs at least two dimensions");

   /* Covers vectorization by up to 4. */
   ALLOC(iy, N+3, int);

   exp_rotation(X, N, 1, B, K, spread);

   yy = op_pvq_search(X, iy, K, N, arch);

   encode_pulses(iy, N, K, enc);

   if (resynth)
   {
      normalise_residual(iy, X, N, yy, gain);
      exp_rotation(X, N, -1, B, K, spread);
   }

   collapse_mask = extract_collapse_mask(iy, N, B);
   RESTORE_STACK;
   return collapse_mask;
}
unsigned alg_unquant(celt_norm *X, int N, int K, int spread, int B,
      ec_dec *dec, opus_val16 gain)
{
   opus_val32 Ryy;
   unsigned collapse_mask;
   VARDECL(int, iy);
   SAVE_STACK;

   celt_assert2(K>0, "alg_unquant() needs at least one pulse");
   celt_assert2(N>1, "alg_unquant() needs at least two dimensions");
   ALLOC(iy, N, int);
   Ryy = decode_pulses(iy, N, K, dec);
   normalise_residual(iy, X, N, Ryy, gain);
   exp_rotation(X, N, -1, B, K, spread);
   collapse_mask = extract_collapse_mask(iy, N, B);
   RESTORE_STACK;
   return collapse_mask;
}
void renormalise_vector(celt_norm *X, int N, opus_val16 gain, int arch)
{
   int i;
   opus_val32 E;
   opus_val16 g;
   opus_val32 t;
   celt_norm *xptr;
   E = EPSILON + celt_inner_prod(X, X, N, arch);
   t = VSHR32(E, 2*(k-7));
   g = MULT16_16_P15(celt_rsqrt_norm(t),gain);

   xptr = X;
   for (i=0;i<N;i++)
   {
      *xptr = EXTRACT16(PSHR32(MULT16_16(g, *xptr), k+1));
      xptr++;
   }
   /*return celt_sqrt(E);*/
}
int stereo_itheta(const celt_norm *X, const celt_norm *Y, int stereo, int N, int arch)
{
   int i;
   int itheta;
   opus_val16 mid, side;
   opus_val32 Emid, Eside;

   Emid = Eside = EPSILON;
   if (stereo)
   {
      for (i=0;i<N;i++)
      {
         celt_norm m, s;
         m = ADD16(SHR16(X[i],1),SHR16(Y[i],1));
         s = SUB16(SHR16(X[i],1),SHR16(Y[i],1));
         Emid = MAC16_16(Emid, m, m);
         Eside = MAC16_16(Eside, s, s);
      }
   } else {
      Emid += celt_inner_prod(X, X, N, arch);
      Eside += celt_inner_prod(Y, Y, N, arch);
   }
   mid = celt_sqrt(Emid);
   side = celt_sqrt(Eside);
   itheta = (int)floor(.5f+16384*0.63662f*fast_atan2f(side,mid));

   return itheta;
}

typedef struct OpusEncoder OpusEncoder;
OPUS_EXPORT OPUS_WARN_UNUSED_RESULT int opus_encoder_get_size(int channels);

OPUS_EXPORT OPUS_WARN_UNUSED_RESULT OpusEncoder *opus_encoder_create(
    opus_int32 Fs,
    int channels,
    int application,
    int *error
);
OPUS_EXPORT int opus_encoder_init(
    OpusEncoder *st,
    opus_int32 Fs,
    int channels,
    int application
) OPUS_ARG_NONNULL(1);
OPUS_EXPORT OPUS_WARN_UNUSED_RESULT opus_int32 opus_encode(
    OpusEncoder *st,
    const opus_int16 *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes
) OPUS_ARG_NONNULL(1) OPUS_ARG_NONNULL(2) OPUS_ARG_NONNULL(4);
OPUS_EXPORT void opus_encoder_destroy(OpusEncoder *st);
OPUS_EXPORT int opus_encoder_ctl(OpusEncoder *st, int request, ...) OPUS_ARG_NONNULL(1);
OPUS_EXPORT OPUS_WARN_UNUSED_RESULT int opus_packet_get_samples_per_frame(const unsigned char *data, opus_int32 Fs) OPUS_ARG_NONNULL(1);
OPUS_EXPORT OPUS_WARN_UNUSED_RESULT int opus_packet_get_nb_frames(const unsigned char packet[], opus_int32 len) OPUS_ARG_NONNULL(1);

typedef struct OpusRepacketizer OpusRepacketizer;
OPUS_EXPORT OpusRepacketizer *opus_repacketizer_init(OpusRepacketizer *rp) OPUS_ARG_NONNULL(1);
OPUS_EXPORT int opus_repacketizer_cat(OpusRepacketizer *rp, const unsigned char *data, opus_int32 len) OPUS_ARG_NONNULL(1) OPUS_ARG_NONNULL(2);
OPUS_EXPORT int opus_packet_pad(unsigned char *data, opus_int32 len, opus_int32 new_len);
struct OpusRepacketizer {
   unsigned char toc;
   int nb_frames;
   const unsigned char *frames[48];
   opus_int16 len[48];
   int framesize;
   const unsigned char *paddings[48];
   opus_int32 padding_len[48];
};
typedef struct {
   int id;
   int frame;
   const unsigned char *data;
   opus_int32 len;
} opus_extension_data;
#define MODE_SILK_ONLY 1000
#define MODE_HYBRID 1001
#define MODE_CELT_ONLY 1002
#define OPUS_SET_VOICE_RATIO_REQUEST 11018
#define OPUS_GET_VOICE_RATIO_REQUEST 11019
#define OPUS_SET_VOICE_RATIO(x) OPUS_SET_VOICE_RATIO_REQUEST, __opus_check_int(x)
#define OPUS_GET_VOICE_RATIO(x) OPUS_GET_VOICE_RATIO_REQUEST, __opus_check_int_ptr(x)
#define OPUS_SET_FORCE_MODE_REQUEST 11002
#define OPUS_SET_FORCE_MODE(x) OPUS_SET_FORCE_MODE_REQUEST, __opus_check_int(x)
typedef void (*downmix_func)(const void *, opus_val32 *, int, int, int, int, int);
void downmix_int(const void *_x, opus_val32 *sub, int subframe, int offset, int c1, int c2, int C);
int is_digital_silence(const opus_val16* pcm, int frame_size, int channels, int lsb_depth);
int encode_size(int size, unsigned char *data);
opus_int32 frame_size_select(opus_int32 frame_size, int variable_duration, opus_int32 Fs);
opus_int32 opus_encode_native(OpusEncoder *st, const opus_val16 *pcm, int frame_size,
      unsigned char *data, opus_int32 out_data_bytes, int lsb_depth,
      const void *analysis_pcm, opus_int32 analysis_size, int c1, int c2,
      int analysis_channels, downmix_func downmix, int float_api);
/* Make sure everything is properly aligned. */
static OPUS_INLINE int align(int i)
{
    struct foo {char c; union { void* p; opus_int32 i; opus_val32 v; } u;};

    unsigned int alignment = offsetof(struct foo, u);

    /* Optimizing compilers should optimize div and multiply into and
       for all sensible alignment values. */
    return ((i + alignment - 1) / alignment) * alignment;
}
/* More than that is ridiculous for now (3 * max frames per packet)*/
opus_int32 skip_extension(const unsigned char **data, opus_int32 len, opus_int32 *header_size);
int opus_packet_parse_impl(const unsigned char *data, opus_int32 len,
      int self_delimited, unsigned char *out_toc,
      const unsigned char *frames[48], opus_int16 size[48],
      int *payload_offset, opus_int32 *packet_offset,
      const unsigned char **padding, opus_int32 *padding_len);
opus_int32 opus_repacketizer_out_range_impl(OpusRepacketizer *rp, int begin, int end,
      unsigned char *data, opus_int32 maxlen, int self_delimited, int pad,
      const opus_extension_data *extensions, int nb_extensions);
opus_int32 opus_packet_extensions_parse(const unsigned char *data, opus_int32 len, opus_extension_data *extensions, opus_int32 *nb_extensions);
opus_int32 opus_packet_extensions_generate(unsigned char *data, opus_int32 len, const opus_extension_data  *extensions, opus_int32 nb_extensions, int pad);
opus_int32 opus_packet_extensions_count(const unsigned char *data, opus_int32 len);
opus_int32 opus_packet_pad_impl(unsigned char *data, opus_int32 len, opus_int32 new_len, int pad, const opus_extension_data  *extensions, int nb_extensions);
#define WEIGHTS_SCALE (1.f/128)
#define MAX_NEURONS 32
typedef struct {
  const opus_int8 *bias;
  const opus_int8 *input_weights;
  int nb_inputs;
  int nb_neurons;
  int sigmoid;
} AnalysisDenseLayer;
typedef struct {
  const opus_int8 *bias;
  const opus_int8 *input_weights;
  const opus_int8 *recurrent_weights;
  int nb_inputs;
  int nb_neurons;
} AnalysisGRULayer;
extern const AnalysisDenseLayer layer0;
extern const AnalysisGRULayer layer1;
extern const AnalysisDenseLayer layer2;
void analysis_compute_dense(const AnalysisDenseLayer *layer, float *output, const float *input);
void analysis_compute_gru(const AnalysisGRULayer *gru, float *state, const float *input);
#define NB_FRAMES 8
#define NB_TBANDS 18
#define ANALYSIS_BUF_SIZE 720
/* At that point we can stop counting frames because it no longer matters. */
#define ANALYSIS_COUNT_MAX 10000
#define DETECT_SIZE 100
/* Uncomment this to print the MLP features on stdout. */
/*#define MLP_TRAINING*/
typedef struct {
   int arch;
   int application;
   opus_int32 Fs;
#define TONALITY_ANALYSIS_RESET_START angle
   float angle[240];
   float d_angle[240];
   float d2_angle[240];
   opus_val32 inmem[ANALYSIS_BUF_SIZE];
   int   mem_fill;                      /* number of usable samples in the buffer */
   float prev_band_tonality[NB_TBANDS];
   float prev_tonality;
   int prev_bandwidth;
   float E[NB_FRAMES][NB_TBANDS];
   float logE[NB_FRAMES][NB_TBANDS];
   float lowE[NB_TBANDS];
   float highE[NB_TBANDS];
   float meanE[NB_TBANDS+1];
   float mem[32];
   float cmean[8];
   float std[9];
   float Etracker;
   float lowECount;
   int E_count;
   int count;
   int analysis_offset;
   int write_pos;
   int read_pos;
   int read_subframe;
   float hp_ener_accum;
   int initialized;
   float rnn_state[MAX_NEURONS];
   opus_val32 downmix_state[3];
   AnalysisInfo info[DETECT_SIZE];
} TonalityAnalysisState;
void tonality_analysis_init(TonalityAnalysisState *analysis, opus_int32 Fs);
void tonality_analysis_reset(TonalityAnalysisState *analysis);
void tonality_get_info(TonalityAnalysisState *tonal, AnalysisInfo *info_out, int len);
void run_analysis(TonalityAnalysisState *analysis, const CELTMode *celt_mode, const void *analysis_pcm,
                 int analysis_frame_size, int frame_size, int c1, int c2, int C, opus_int32 Fs,
                 int lsb_depth, downmix_func downmix, AnalysisInfo *analysis_info);
/* Copyright (C) 2002-2003 Jean-Marc Valin
   Copyright (C) 2007-2009 Xiph.Org Foundation */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

- Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* Copyright (C) 2001 Erik de Castro Lopo <erikd AT mega-nerd DOT com> */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

- Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* Version 1.1 */
#define TRANSITION_PENALTY 10
static const float dct_table[128] = {
        0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f,
        0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f,
        0.351851f, 0.338330f, 0.311806f, 0.273300f, 0.224292f, 0.166664f, 0.102631f, 0.034654f,
       -0.034654f,-0.102631f,-0.166664f,-0.224292f,-0.273300f,-0.311806f,-0.338330f,-0.351851f,
        0.346760f, 0.293969f, 0.196424f, 0.068975f,-0.068975f,-0.196424f,-0.293969f,-0.346760f,
       -0.346760f,-0.293969f,-0.196424f,-0.068975f, 0.068975f, 0.196424f, 0.293969f, 0.346760f,
        0.338330f, 0.224292f, 0.034654f,-0.166664f,-0.311806f,-0.351851f,-0.273300f,-0.102631f,
        0.102631f, 0.273300f, 0.351851f, 0.311806f, 0.166664f,-0.034654f,-0.224292f,-0.338330f,
        0.326641f, 0.135299f,-0.135299f,-0.326641f,-0.326641f,-0.135299f, 0.135299f, 0.326641f,
        0.326641f, 0.135299f,-0.135299f,-0.326641f,-0.326641f,-0.135299f, 0.135299f, 0.326641f,
        0.311806f, 0.034654f,-0.273300f,-0.338330f,-0.102631f, 0.224292f, 0.351851f, 0.166664f,
       -0.166664f,-0.351851f,-0.224292f, 0.102631f, 0.338330f, 0.273300f,-0.034654f,-0.311806f,
        0.293969f,-0.068975f,-0.346760f,-0.196424f, 0.196424f, 0.346760f, 0.068975f,-0.293969f,
       -0.293969f, 0.068975f, 0.346760f, 0.196424f,-0.196424f,-0.346760f,-0.068975f, 0.293969f,
        0.273300f,-0.166664f,-0.338330f, 0.034654f, 0.351851f, 0.102631f,-0.311806f,-0.224292f,
        0.224292f, 0.311806f,-0.102631f,-0.351851f,-0.034654f, 0.338330f, 0.166664f,-0.273300f,
};
static const float analysis_window[240] = {
      0.000043f, 0.000171f, 0.000385f, 0.000685f, 0.001071f, 0.001541f, 0.002098f, 0.002739f,
      0.003466f, 0.004278f, 0.005174f, 0.006156f, 0.007222f, 0.008373f, 0.009607f, 0.010926f,
      0.012329f, 0.013815f, 0.015385f, 0.017037f, 0.018772f, 0.020590f, 0.022490f, 0.024472f,
      0.026535f, 0.028679f, 0.030904f, 0.033210f, 0.035595f, 0.038060f, 0.040604f, 0.043227f,
      0.045928f, 0.048707f, 0.051564f, 0.054497f, 0.057506f, 0.060591f, 0.063752f, 0.066987f,
      0.070297f, 0.073680f, 0.077136f, 0.080665f, 0.084265f, 0.087937f, 0.091679f, 0.095492f,
      0.099373f, 0.103323f, 0.107342f, 0.111427f, 0.115579f, 0.119797f, 0.124080f, 0.128428f,
      0.132839f, 0.137313f, 0.141849f, 0.146447f, 0.151105f, 0.155823f, 0.160600f, 0.165435f,
      0.170327f, 0.175276f, 0.180280f, 0.185340f, 0.190453f, 0.195619f, 0.200838f, 0.206107f,
      0.211427f, 0.216797f, 0.222215f, 0.227680f, 0.233193f, 0.238751f, 0.244353f, 0.250000f,
      0.255689f, 0.261421f, 0.267193f, 0.273005f, 0.278856f, 0.284744f, 0.290670f, 0.296632f,
      0.302628f, 0.308658f, 0.314721f, 0.320816f, 0.326941f, 0.333097f, 0.339280f, 0.345492f,
      0.351729f, 0.357992f, 0.364280f, 0.370590f, 0.376923f, 0.383277f, 0.389651f, 0.396044f,
      0.402455f, 0.408882f, 0.415325f, 0.421783f, 0.428254f, 0.434737f, 0.441231f, 0.447736f,
      0.454249f, 0.460770f, 0.467298f, 0.473832f, 0.480370f, 0.486912f, 0.493455f, 0.500000f,
      0.506545f, 0.513088f, 0.519630f, 0.526168f, 0.532702f, 0.539230f, 0.545751f, 0.552264f,
      0.558769f, 0.565263f, 0.571746f, 0.578217f, 0.584675f, 0.591118f, 0.597545f, 0.603956f,
      0.610349f, 0.616723f, 0.623077f, 0.629410f, 0.635720f, 0.642008f, 0.648271f, 0.654508f,
      0.660720f, 0.666903f, 0.673059f, 0.679184f, 0.685279f, 0.691342f, 0.697372f, 0.703368f,
      0.709330f, 0.715256f, 0.721144f, 0.726995f, 0.732807f, 0.738579f, 0.744311f, 0.750000f,
      0.755647f, 0.761249f, 0.766807f, 0.772320f, 0.777785f, 0.783203f, 0.788573f, 0.793893f,
      0.799162f, 0.804381f, 0.809547f, 0.814660f, 0.819720f, 0.824724f, 0.829673f, 0.834565f,
      0.839400f, 0.844177f, 0.848895f, 0.853553f, 0.858151f, 0.862687f, 0.867161f, 0.871572f,
      0.875920f, 0.880203f, 0.884421f, 0.888573f, 0.892658f, 0.896677f, 0.900627f, 0.904508f,
      0.908321f, 0.912063f, 0.915735f, 0.919335f, 0.922864f, 0.926320f, 0.929703f, 0.933013f,
      0.936248f, 0.939409f, 0.942494f, 0.945503f, 0.948436f, 0.951293f, 0.954072f, 0.956773f,
      0.959396f, 0.961940f, 0.964405f, 0.966790f, 0.969096f, 0.971321f, 0.973465f, 0.975528f,
      0.977510f, 0.979410f, 0.981228f, 0.982963f, 0.984615f, 0.986185f, 0.987671f, 0.989074f,
      0.990393f, 0.991627f, 0.992778f, 0.993844f, 0.994826f, 0.995722f, 0.996534f, 0.997261f,
      0.997902f, 0.998459f, 0.998929f, 0.999315f, 0.999615f, 0.999829f, 0.999957f, 1.000000f,
};
static const int tbands[NB_TBANDS+1] = {
      4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 136, 160, 192, 240
};
#define NB_TONAL_SKIP_BANDS 9
static opus_val32 silk_resampler_down2_hp(
    opus_val32                  *S,                 /* I/O  State vector [ 2 ]                                          */
    opus_val32                  *out,               /* O    Output signal [ floor(len/2) ]                              */
    const opus_val32            *in,                /* I    Input signal [ len ]                                        */
    int                         inLen               /* I    Number of input samples                                     */
)
{
    int k, len2 = inLen/2;
    opus_val32 in32, out32, out32_hp, Y, X;
    opus_val64 hp_ener = 0;
    /* Internal variables and state are in Q10 format */
    for( k = 0; k < len2; k++ ) {
        /* Convert to Q10 */
        in32 = in[ 2 * k ];

        /* All-pass section for even input sample */
        Y      = SUB32( in32, S[ 0 ] );
        X      = MULT16_32_Q15(QCONST16(0.6074371f, 15), Y);
        out32  = ADD32( S[ 0 ], X );
        S[ 0 ] = ADD32( in32, X );
        out32_hp = out32;
        /* Convert to Q10 */
        in32 = in[ 2 * k + 1 ];

        /* All-pass section for odd input sample, and add to output of previous section */
        Y      = SUB32( in32, S[ 1 ] );
        X      = MULT16_32_Q15(QCONST16(0.15063f, 15), Y);
        out32  = ADD32( out32, S[ 1 ] );
        out32  = ADD32( out32, X );
        S[ 1 ] = ADD32( in32, X );

        Y      = SUB32( -in32, S[ 2 ] );
        X      = MULT16_32_Q15(QCONST16(0.15063f, 15), Y);
        out32_hp  = ADD32( out32_hp, S[ 2 ] );
        out32_hp  = ADD32( out32_hp, X );
        S[ 2 ] = ADD32( -in32, X );

        hp_ener += out32_hp*(opus_val64)out32_hp;
        /* Add, convert back to int16 and store to output */
        out[ k ] = HALF32(out32);
    }
    return (opus_val32)hp_ener;
}
static opus_val32 downmix_and_resample(downmix_func downmix, const void *_x, opus_val32 *y, opus_val32 S[3], int subframe, int offset, int c1, int c2, int C, int Fs)
{
   VARDECL(opus_val32, tmp);
   opus_val32 scale;
   int j;
   opus_val32 ret = 0;
   SAVE_STACK;

   if (subframe==0) return 0;
   if (Fs == 48000)
   {
      subframe *= 2;
      offset *= 2;
   } else if (Fs == 16000) {
      subframe = subframe*2/3;
      offset = offset*2/3;
   }
   ALLOC(tmp, subframe, opus_val32);

   downmix(_x, tmp, subframe, offset, c1, c2, C);
   scale = 1.f/32768;
   if (c2==-2)
      scale /= C;
   else if (c2>-1)
      scale /= 2;
   for (j=0;j<subframe;j++)
      tmp[j] *= scale;
   if (Fs == 48000)
   {
      ret = silk_resampler_down2_hp(S, y, tmp, subframe);
   } else if (Fs == 24000) {
      OPUS_COPY(y, tmp, subframe);
   } else if (Fs == 16000) {
      VARDECL(opus_val32, tmp3x);
      ALLOC(tmp3x, 3*subframe, opus_val32);
      /* Don't do this at home! This resampler is horrible and it's only (barely)
         usable for the purpose of the analysis because we don't care about all
         the aliasing between 8 kHz and 12 kHz. */
      for (j=0;j<subframe;j++)
      {
         tmp3x[3*j] = tmp[j];
         tmp3x[3*j+1] = tmp[j];
         tmp3x[3*j+2] = tmp[j];
      }
      silk_resampler_down2_hp(S, y, tmp3x, 3*subframe);
   }
   RESTORE_STACK;
   return ret;
}
void tonality_analysis_init(TonalityAnalysisState *tonal, opus_int32 Fs)
{
  /* Initialize reusable fields. */
  tonal->arch = opus_select_arch();
  tonal->Fs = Fs;
  /* Clear remaining fields. */
  tonality_analysis_reset(tonal);
}
void tonality_analysis_reset(TonalityAnalysisState *tonal)
{
  /* Clear non-reusable fields. */
  char *start = (char*)&tonal->TONALITY_ANALYSIS_RESET_START;
  OPUS_CLEAR(start, sizeof(TonalityAnalysisState) - (start - (char*)tonal));
}
void tonality_get_info(TonalityAnalysisState *tonal, AnalysisInfo *info_out, int len)
{
   int pos;
   int curr_lookahead;
   float tonality_max;
   float tonality_avg;
   int tonality_count;
   int i;
   int pos0;
   float prob_avg;
   float prob_count;
   float prob_min, prob_max;
   float vad_prob;
   int mpos, vpos;
   int bandwidth_span;

   pos = tonal->read_pos;
   curr_lookahead = tonal->write_pos-tonal->read_pos;
   if (curr_lookahead<0)
      curr_lookahead += DETECT_SIZE;

   tonal->read_subframe += len/(tonal->Fs/400);
   while (tonal->read_subframe>=8)
   {
      tonal->read_subframe -= 8;
      tonal->read_pos++;
   }
   if (tonal->read_pos>=DETECT_SIZE)
      tonal->read_pos-=DETECT_SIZE;

   /* On long frames, look at the second analysis window rather than the first. */
   if (len > tonal->Fs/50 && pos != tonal->write_pos)
   {
      pos++;
      if (pos==DETECT_SIZE)
         pos=0;
   }
   if (pos == tonal->write_pos)
      pos--;
   if (pos<0)
      pos = DETECT_SIZE-1;
   pos0 = pos;
   OPUS_COPY(info_out, &tonal->info[pos], 1);
   if (!info_out->valid)
      return;
   tonality_max = tonality_avg = info_out->tonality;
   tonality_count = 1;
   /* Look at the neighbouring frames and pick largest bandwidth found (to be safe). */
   bandwidth_span = 6;
   /* If possible, look ahead for a tone to compensate for the delay in the tone detector. */
   for (i=0;i<3;i++)
   {
      pos++;
      if (pos==DETECT_SIZE)
         pos = 0;
      if (pos == tonal->write_pos)
         break;
      tonality_max = MAX32(tonality_max, tonal->info[pos].tonality);
      tonality_avg += tonal->info[pos].tonality;
      tonality_count++;
      info_out->bandwidth = IMAX(info_out->bandwidth, tonal->info[pos].bandwidth);
      bandwidth_span--;
   }
   pos = pos0;
   /* Look back in time to see if any has a wider bandwidth than the current frame. */
   for (i=0;i<bandwidth_span;i++)
   {
      pos--;
      if (pos < 0)
         pos = DETECT_SIZE-1;
      if (pos == tonal->write_pos)
         break;
      info_out->bandwidth = IMAX(info_out->bandwidth, tonal->info[pos].bandwidth);
   }
   info_out->tonality = MAX32(tonality_avg/tonality_count, tonality_max-.2f);

   mpos = vpos = pos0;
   /* If we have enough look-ahead, compensate for the ~5-frame delay in the music prob and
      ~1 frame delay in the VAD prob. */
   if (curr_lookahead > 15)
   {
      mpos += 5;
      if (mpos>=DETECT_SIZE)
         mpos -= DETECT_SIZE;
      vpos += 1;
      if (vpos>=DETECT_SIZE)
         vpos -= DETECT_SIZE;
   }

   /* The following calculations attempt to minimize a "badness function"
      for the transition. When switching from speech to music, the badness
      of switching at frame k is
      b_k = S*v_k + \sum_{i=0}^{k-1} v_i*(p_i - T)
      where
      v_i is the activity probability (VAD) at frame i,
      p_i is the music probability at frame i
      T is the probability threshold for switching
      S is the penalty for switching during active audio rather than silence
      the current frame has index i=0

      Rather than apply badness to directly decide when to switch, what we compute
      instead is the threshold for which the optimal switching point is now. When
      considering whether to switch now (frame 0) or at frame k, we have:
      S*v_0 = S*v_k + \sum_{i=0}^{k-1} v_i*(p_i - T)
      which gives us:
      T = ( \sum_{i=0}^{k-1} v_i*p_i + S*(v_k-v_0) ) / ( \sum_{i=0}^{k-1} v_i )
      We take the min threshold across all positive values of k (up to the maximum
      amount of lookahead we have) to give us the threshold for which the current
      frame is the optimal switch point.

      The last step is that we need to consider whether we want to switch at all.
      For that we use the average of the music probability over the entire window.
      If the threshold is higher than that average we're not going to
      switch, so we compute a min with the average as well. The result of all these
      min operations is music_prob_min, which gives the threshold for switching to music
      if we're currently encoding for speech.

      We do the exact opposite to compute music_prob_max which is used for switching
      from music to speech.
    */
   prob_min = 1.f;
   prob_max = 0.f;
   vad_prob = tonal->info[vpos].activity_probability;
   prob_count = MAX16(.1f, vad_prob);
   prob_avg = MAX16(.1f, vad_prob)*tonal->info[mpos].music_prob;
   while (1)
   {
      float pos_vad;
      mpos++;
      if (mpos==DETECT_SIZE)
         mpos = 0;
      if (mpos == tonal->write_pos)
         break;
      vpos++;
      if (vpos==DETECT_SIZE)
         vpos = 0;
      if (vpos == tonal->write_pos)
         break;
      pos_vad = tonal->info[vpos].activity_probability;
      prob_min = MIN16((prob_avg - TRANSITION_PENALTY*(vad_prob - pos_vad))/prob_count, prob_min);
      prob_max = MAX16((prob_avg + TRANSITION_PENALTY*(vad_prob - pos_vad))/prob_count, prob_max);
      prob_count += MAX16(.1f, pos_vad);
      prob_avg += MAX16(.1f, pos_vad)*tonal->info[mpos].music_prob;
   }
   info_out->music_prob = prob_avg/prob_count;
   prob_min = MIN16(prob_avg/prob_count, prob_min);
   prob_max = MAX16(prob_avg/prob_count, prob_max);
   prob_min = MAX16(prob_min, 0.f);
   prob_max = MIN16(prob_max, 1.f);

   /* If we don't have enough look-ahead, do our best to make a decent decision. */
   if (curr_lookahead < 10)
   {
      float pmin, pmax;
      pmin = prob_min;
      pmax = prob_max;
      pos = pos0;
      /* Look for min/max in the past. */
      for (i=0;i<IMIN(tonal->count-1, 15);i++)
      {
         pos--;
         if (pos < 0)
            pos = DETECT_SIZE-1;
         pmin = MIN16(pmin, tonal->info[pos].music_prob);
         pmax = MAX16(pmax, tonal->info[pos].music_prob);
      }
      /* Bias against switching on active audio. */
      pmin = MAX16(0.f, pmin - .1f*vad_prob);
      pmax = MIN16(1.f, pmax + .1f*vad_prob);
      prob_min += (1.f-.1f*curr_lookahead)*(pmin - prob_min);
      prob_max += (1.f-.1f*curr_lookahead)*(pmax - prob_max);
   }
   info_out->music_prob_min = prob_min;
   info_out->music_prob_max = prob_max;

   /* printf("%f %f %f %f %f\n", prob_min, prob_max, prob_avg/prob_count, vad_prob, info_out->music_prob); */
}
static const float std_feature_bias[9] = {
      5.684947f, 3.475288f, 1.770634f, 1.599784f, 3.773215f,
      2.163313f, 1.260756f, 1.116868f, 1.918795f
};
#define LEAKAGE_OFFSET 2.5f
#define LEAKAGE_SLOPE 2.f
#define SCALE_ENER(e) (e)
#define is_digital_silence32(pcm,frame_size,channels,lsb_depth) is_digital_silence(pcm, frame_size, channels, lsb_depth)
static void tonality_analysis(TonalityAnalysisState *tonal, const CELTMode *celt_mode, const void *x, int len, int offset, int c1, int c2, int C, int lsb_depth, downmix_func downmix)
{
    int i, b;
    const kiss_fft_state *kfft;
    VARDECL(kiss_fft_cpx, in);
    VARDECL(kiss_fft_cpx, out);
    int N = 480, N2=240;
    float * OPUS_RESTRICT A = tonal->angle;
    float * OPUS_RESTRICT dA = tonal->d_angle;
    float * OPUS_RESTRICT d2A = tonal->d2_angle;
    VARDECL(float, tonality);
    VARDECL(float, noisiness);
    float band_tonality[NB_TBANDS];
    float logE[NB_TBANDS];
    float BFCC[8];
    float features[25];
    float frame_tonality;
    float max_frame_tonality;
    /*float tw_sum=0;*/
    float frame_noisiness;
    const float pi4 = (float)(M_PI*M_PI*M_PI*M_PI);
    float slope=0;
    float frame_stationarity;
    float relativeE;
    float frame_probs[2];
    float alpha, alphaE, alphaE2;
    float frame_loudness;
    float bandwidth_mask;
    int is_masked[NB_TBANDS+1];
    int bandwidth=0;
    float maxE = 0;
    float noise_floor;
    int remaining;
    AnalysisInfo *info;
    float hp_ener;
    float tonality2[240];
    float midE[8];
    float spec_variability=0;
    float band_log2[NB_TBANDS+1];
    float leakage_from[NB_TBANDS+1];
    float leakage_to[NB_TBANDS+1];
    float layer_out[MAX_NEURONS];
    float below_max_pitch;
    float above_max_pitch;
    int is_silence;
    SAVE_STACK;

    if (!tonal->initialized)
    {
       tonal->mem_fill = 240;
       tonal->initialized = 1;
    }
    alpha = 1.f/IMIN(10, 1+tonal->count);
    alphaE = 1.f/IMIN(25, 1+tonal->count);
    /* Noise floor related decay for bandwidth detection: -2.2 dB/second */
    alphaE2 = 1.f/IMIN(100, 1+tonal->count);
    if (tonal->count <= 1) alphaE2 = 1;

    if (tonal->Fs == 48000)
    {
       /* len and offset are now at 24 kHz. */
       len/= 2;
       offset /= 2;
    } else if (tonal->Fs == 16000) {
       len = 3*len/2;
       offset = 3*offset/2;
    }

    kfft = celt_mode->mdct.kfft[0];
    tonal->hp_ener_accum += (float)downmix_and_resample(downmix, x,
          &tonal->inmem[tonal->mem_fill], tonal->downmix_state,
          IMIN(len, ANALYSIS_BUF_SIZE-tonal->mem_fill), offset, c1, c2, C, tonal->Fs);
    if (tonal->mem_fill+len < ANALYSIS_BUF_SIZE)
    {
       tonal->mem_fill += len;
       /* Don't have enough to update the analysis */
       RESTORE_STACK;
       return;
    }
    hp_ener = tonal->hp_ener_accum;
    info = &tonal->info[tonal->write_pos++];
    if (tonal->write_pos>=DETECT_SIZE)
       tonal->write_pos-=DETECT_SIZE;

    is_silence = is_digital_silence32(tonal->inmem, ANALYSIS_BUF_SIZE, 1, lsb_depth);

    ALLOC(in, 480, kiss_fft_cpx);
    ALLOC(out, 480, kiss_fft_cpx);
    ALLOC(tonality, 240, float);
    ALLOC(noisiness, 240, float);
    for (i=0;i<N2;i++)
    {
       float w = analysis_window[i];
       in[i].r = (kiss_fft_scalar)(w*tonal->inmem[i]);
       in[i].i = (kiss_fft_scalar)(w*tonal->inmem[N2+i]);
       in[N-i-1].r = (kiss_fft_scalar)(w*tonal->inmem[N-i-1]);
       in[N-i-1].i = (kiss_fft_scalar)(w*tonal->inmem[N+N2-i-1]);
    }
    OPUS_MOVE(tonal->inmem, tonal->inmem+ANALYSIS_BUF_SIZE-240, 240);
    remaining = len - (ANALYSIS_BUF_SIZE-tonal->mem_fill);
    tonal->hp_ener_accum = (float)downmix_and_resample(downmix, x,
          &tonal->inmem[240], tonal->downmix_state, remaining,
          offset+ANALYSIS_BUF_SIZE-tonal->mem_fill, c1, c2, C, tonal->Fs);
    tonal->mem_fill = 240 + remaining;
    if (is_silence)
    {
       /* On silence, copy the previous analysis. */
       int prev_pos = tonal->write_pos-2;
       if (prev_pos < 0)
          prev_pos += DETECT_SIZE;
       OPUS_COPY(info, &tonal->info[prev_pos], 1);
       RESTORE_STACK;
       return;
    }
    opus_fft(kfft, in, out, tonal->arch);
    /* If there's any NaN on the input, the entire output will be NaN, so we only need to check one value. */
    if (celt_isnan(out[0].r))
    {
       info->valid = 0;
       RESTORE_STACK;
       return;
    }

    for (i=1;i<N2;i++)
    {
       float X1r, X2r, X1i, X2i;
       float angle, d_angle, d2_angle;
       float angle2, d_angle2, d2_angle2;
       float mod1, mod2, avg_mod;
       X1r = (float)out[i].r+out[N-i].r;
       X1i = (float)out[i].i-out[N-i].i;
       X2r = (float)out[i].i+out[N-i].i;
       X2i = (float)out[N-i].r-out[i].r;

       angle = (float)(.5f/M_PI)*fast_atan2f(X1i, X1r);
       d_angle = angle - A[i];
       d2_angle = d_angle - dA[i];

       angle2 = (float)(.5f/M_PI)*fast_atan2f(X2i, X2r);
       d_angle2 = angle2 - angle;
       d2_angle2 = d_angle2 - d_angle;

       mod1 = d2_angle - (float)float2int(d2_angle);
       noisiness[i] = ABS16(mod1);
       mod1 *= mod1;
       mod1 *= mod1;

       mod2 = d2_angle2 - (float)float2int(d2_angle2);
       noisiness[i] += ABS16(mod2);
       mod2 *= mod2;
       mod2 *= mod2;

       avg_mod = .25f*(d2A[i]+mod1+2*mod2);
       /* This introduces an extra delay of 2 frames in the detection. */
       tonality[i] = 1.f/(1.f+40.f*16.f*pi4*avg_mod)-.015f;
       /* No delay on this detection, but it's less reliable. */
       tonality2[i] = 1.f/(1.f+40.f*16.f*pi4*mod2)-.015f;

       A[i] = angle2;
       dA[i] = d_angle2;
       d2A[i] = mod2;
    }
    for (i=2;i<N2-1;i++)
    {
       float tt = MIN32(tonality2[i], MAX32(tonality2[i-1], tonality2[i+1]));
       tonality[i] = .9f*MAX32(tonality[i], tt-.1f);
    }
    frame_tonality = 0;
    max_frame_tonality = 0;
    /*tw_sum = 0;*/
    info->activity = 0;
    frame_noisiness = 0;
    frame_stationarity = 0;
    if (!tonal->count)
    {
       for (b=0;b<NB_TBANDS;b++)
       {
          tonal->lowE[b] = 1e10;
          tonal->highE[b] = -1e10;
       }
    }
    relativeE = 0;
    frame_loudness = 0;
    /* The energy of the very first band is special because of DC. */
    {
       float E = 0;
       float X1r, X2r;
       X1r = 2*(float)out[0].r;
       X2r = 2*(float)out[0].i;
       E = X1r*X1r + X2r*X2r;
       for (i=1;i<4;i++)
       {
          float binE = out[i].r*(float)out[i].r + out[N-i].r*(float)out[N-i].r
                     + out[i].i*(float)out[i].i + out[N-i].i*(float)out[N-i].i;
          E += binE;
       }
       E = SCALE_ENER(E);
       band_log2[0] = .5f*1.442695f*(float)log(E+1e-10f);
    }
    for (b=0;b<NB_TBANDS;b++)
    {
       float E=0, tE=0, nE=0;
       float L1, L2;
       float stationarity;
       for (i=tbands[b];i<tbands[b+1];i++)
       {
          float binE = out[i].r*(float)out[i].r + out[N-i].r*(float)out[N-i].r
                     + out[i].i*(float)out[i].i + out[N-i].i*(float)out[N-i].i;
          binE = SCALE_ENER(binE);
          E += binE;
          tE += binE*MAX32(0, tonality[i]);
          nE += binE*2.f*(.5f-noisiness[i]);
       }
       /* Check for extreme band energies that could cause NaNs later. */
       if (!(E<1e9f) || celt_isnan(E))
       {
          info->valid = 0;
          RESTORE_STACK;
          return;
       }

       tonal->E[tonal->E_count][b] = E;
       frame_noisiness += nE/(1e-15f+E);

       frame_loudness += (float)sqrt(E+1e-10f);
       logE[b] = (float)log(E+1e-10f);
       band_log2[b+1] = .5f*1.442695f*(float)log(E+1e-10f);
       tonal->logE[tonal->E_count][b] = logE[b];
       if (tonal->count==0)
          tonal->highE[b] = tonal->lowE[b] = logE[b];
       if (tonal->highE[b] > tonal->lowE[b] + 7.5)
       {
          if (tonal->highE[b] - logE[b] > logE[b] - tonal->lowE[b])
             tonal->highE[b] -= .01f;
          else
             tonal->lowE[b] += .01f;
       }
       if (logE[b] > tonal->highE[b])
       {
          tonal->highE[b] = logE[b];
          tonal->lowE[b] = MAX32(tonal->highE[b]-15, tonal->lowE[b]);
       } else if (logE[b] < tonal->lowE[b])
       {
          tonal->lowE[b] = logE[b];
          tonal->highE[b] = MIN32(tonal->lowE[b]+15, tonal->highE[b]);
       }
       relativeE += (logE[b]-tonal->lowE[b])/(1e-5f + (tonal->highE[b]-tonal->lowE[b]));

       L1=L2=0;
       for (i=0;i<NB_FRAMES;i++)
       {
          L1 += (float)sqrt(tonal->E[i][b]);
          L2 += tonal->E[i][b];
       }

       stationarity = MIN16(0.99f,L1/(float)sqrt(1e-15+NB_FRAMES*L2));
       stationarity *= stationarity;
       stationarity *= stationarity;
       frame_stationarity += stationarity;
       /*band_tonality[b] = tE/(1e-15+E)*/;
       band_tonality[b] = MAX16(tE/(1e-15f+E), stationarity*tonal->prev_band_tonality[b]);
       frame_tonality += band_tonality[b];
       if (b>=NB_TBANDS-NB_TONAL_SKIP_BANDS)
          frame_tonality -= band_tonality[b-NB_TBANDS+NB_TONAL_SKIP_BANDS];
       max_frame_tonality = MAX16(max_frame_tonality, (1.f+.03f*(b-NB_TBANDS))*frame_tonality);
       slope += band_tonality[b]*(b-8);
       /*printf("%f %f ", band_tonality[b], stationarity);*/
       tonal->prev_band_tonality[b] = band_tonality[b];
    }

    leakage_from[0] = band_log2[0];
    leakage_to[0] = band_log2[0] - LEAKAGE_OFFSET;
    for (b=1;b<NB_TBANDS+1;b++)
    {
       float leak_slope = LEAKAGE_SLOPE*(tbands[b]-tbands[b-1])/4;
       leakage_from[b] = MIN16(leakage_from[b-1]+leak_slope, band_log2[b]);
       leakage_to[b] = MAX16(leakage_to[b-1]-leak_slope, band_log2[b]-LEAKAGE_OFFSET);
    }
    for (b=NB_TBANDS-2;b>=0;b--)
    {
       float leak_slope = LEAKAGE_SLOPE*(tbands[b+1]-tbands[b])/4;
       leakage_from[b] = MIN16(leakage_from[b+1]+leak_slope, leakage_from[b]);
       leakage_to[b] = MAX16(leakage_to[b+1]-leak_slope, leakage_to[b]);
    }
    celt_assert(NB_TBANDS+1 <= LEAK_BANDS);
    for (b=0;b<NB_TBANDS+1;b++)
    {
       /* leak_boost[] is made up of two terms. The first, based on leakage_to[],
          represents the boost needed to overcome the amount of analysis leakage
          cause in a weaker band b by louder neighbouring bands.
          The second, based on leakage_from[], applies to a loud band b for
          which the quantization noise causes synthesis leakage to the weaker
          neighbouring bands. */
       float boost = MAX16(0, leakage_to[b] - band_log2[b]) +
             MAX16(0, band_log2[b] - (leakage_from[b]+LEAKAGE_OFFSET));
       info->leak_boost[b] = IMIN(255, (int)floor(.5 + 64.f*boost));
    }
    for (;b<LEAK_BANDS;b++) info->leak_boost[b] = 0;

    for (i=0;i<NB_FRAMES;i++)
    {
       int j;
       float mindist = 1e15f;
       for (j=0;j<NB_FRAMES;j++)
       {
          int k;
          float dist=0;
          for (k=0;k<NB_TBANDS;k++)
          {
             float tmp;
             tmp = tonal->logE[i][k] - tonal->logE[j][k];
             dist += tmp*tmp;
          }
          if (j!=i)
             mindist = MIN32(mindist, dist);
       }
       spec_variability += mindist;
    }
    spec_variability = (float)sqrt(spec_variability/NB_FRAMES/NB_TBANDS);
    bandwidth_mask = 0;
    bandwidth = 0;
    maxE = 0;
    noise_floor = 5.7e-4f/(1<<(IMAX(0,lsb_depth-8)));
    noise_floor *= noise_floor;
    below_max_pitch=0;
    above_max_pitch=0;
    for (b=0;b<NB_TBANDS;b++)
    {
       float E=0;
       float Em;
       int band_start, band_end;
       /* Keep a margin of 300 Hz for aliasing */
       band_start = tbands[b];
       band_end = tbands[b+1];
       for (i=band_start;i<band_end;i++)
       {
          float binE = out[i].r*(float)out[i].r + out[N-i].r*(float)out[N-i].r
                     + out[i].i*(float)out[i].i + out[N-i].i*(float)out[N-i].i;
          E += binE;
       }
       E = SCALE_ENER(E);
       maxE = MAX32(maxE, E);
       if (band_start < 64)
       {
          below_max_pitch += E;
       } else {
          above_max_pitch += E;
       }
       tonal->meanE[b] = MAX32((1-alphaE2)*tonal->meanE[b], E);
       Em = MAX32(E, tonal->meanE[b]);
       /* Consider the band "active" only if all these conditions are met:
          1) less than 90 dB below the peak band (maximal masking possible considering
             both the ATH and the loudness-dependent slope of the spreading function)
          2) above the PCM quantization noise floor
          We use b+1 because the first CELT band isn't included in tbands[]
       */
       if (E*1e9f > maxE && (Em > 3*noise_floor*(band_end-band_start) || E > noise_floor*(band_end-band_start)))
          bandwidth = b+1;
       /* Check if the band is masked (see below). */
       is_masked[b] = E < (tonal->prev_bandwidth >= b+1  ? .01f : .05f)*bandwidth_mask;
       /* Use a simple follower with 13 dB/Bark slope for spreading function. */
       bandwidth_mask = MAX32(.05f*bandwidth_mask, E);
    }
    /* Special case for the last two bands, for which we don't have spectrum but only
       the energy above 12 kHz. The difficulty here is that the high-pass we use
       leaks some LF energy, so we need to increase the threshold without accidentally cutting
       off the band. */
    if (tonal->Fs == 48000) {
       float noise_ratio;
       float Em;
       float E = hp_ener*(1.f/(60*60));
       noise_ratio = tonal->prev_bandwidth==20 ? 10.f : 30.f;

       above_max_pitch += E;
       tonal->meanE[b] = MAX32((1-alphaE2)*tonal->meanE[b], E);
       Em = MAX32(E, tonal->meanE[b]);
       if (Em > 3*noise_ratio*noise_floor*160 || E > noise_ratio*noise_floor*160)
          bandwidth = 20;
       /* Check if the band is masked (see below). */
       is_masked[b] = E < (tonal->prev_bandwidth == 20  ? .01f : .05f)*bandwidth_mask;
    }
    if (above_max_pitch > below_max_pitch)
       info->max_pitch_ratio = below_max_pitch/above_max_pitch;
    else
       info->max_pitch_ratio = 1;
    /* In some cases, resampling aliasing can create a small amount of energy in the first band
       being cut. So if the last band is masked, we don't include it.  */
    if (bandwidth == 20 && is_masked[NB_TBANDS])
       bandwidth-=2;
    else if (bandwidth > 0 && bandwidth <= NB_TBANDS && is_masked[bandwidth-1])
       bandwidth--;
    if (tonal->count<=2)
       bandwidth = 20;
    frame_loudness = 20*(float)log10(frame_loudness);
    tonal->Etracker = MAX32(tonal->Etracker-.003f, frame_loudness);
    tonal->lowECount *= (1-alphaE);
    if (frame_loudness < tonal->Etracker-30)
       tonal->lowECount += alphaE;

    for (i=0;i<8;i++)
    {
       float sum=0;
       for (b=0;b<16;b++)
          sum += dct_table[i*16+b]*logE[b];
       BFCC[i] = sum;
    }
    for (i=0;i<8;i++)
    {
       float sum=0;
       for (b=0;b<16;b++)
          sum += dct_table[i*16+b]*.5f*(tonal->highE[b]+tonal->lowE[b]);
       midE[i] = sum;
    }

    frame_stationarity /= NB_TBANDS;
    relativeE /= NB_TBANDS;
    if (tonal->count<10)
       relativeE = .5f;
    frame_noisiness /= NB_TBANDS;
    info->activity = frame_noisiness + (1-frame_noisiness)*relativeE;
    frame_tonality = (max_frame_tonality/(NB_TBANDS-NB_TONAL_SKIP_BANDS));
    frame_tonality = MAX16(frame_tonality, tonal->prev_tonality*.8f);
    tonal->prev_tonality = frame_tonality;

    slope /= 8*8;
    info->tonality_slope = slope;

    tonal->E_count = (tonal->E_count+1)%NB_FRAMES;
    tonal->count = IMIN(tonal->count+1, ANALYSIS_COUNT_MAX);
    info->tonality = frame_tonality;

    for (i=0;i<4;i++)
       features[i] = -0.12299f*(BFCC[i]+tonal->mem[i+24]) + 0.49195f*(tonal->mem[i]+tonal->mem[i+16]) + 0.69693f*tonal->mem[i+8] - 1.4349f*tonal->cmean[i];

    for (i=0;i<4;i++)
       tonal->cmean[i] = (1-alpha)*tonal->cmean[i] + alpha*BFCC[i];

    for (i=0;i<4;i++)
        features[4+i] = 0.63246f*(BFCC[i]-tonal->mem[i+24]) + 0.31623f*(tonal->mem[i]-tonal->mem[i+16]);
    for (i=0;i<3;i++)
        features[8+i] = 0.53452f*(BFCC[i]+tonal->mem[i+24]) - 0.26726f*(tonal->mem[i]+tonal->mem[i+16]) -0.53452f*tonal->mem[i+8];

    if (tonal->count > 5)
    {
       for (i=0;i<9;i++)
          tonal->std[i] = (1-alpha)*tonal->std[i] + alpha*features[i]*features[i];
    }
    for (i=0;i<4;i++)
       features[i] = BFCC[i]-midE[i];

    for (i=0;i<8;i++)
    {
       tonal->mem[i+24] = tonal->mem[i+16];
       tonal->mem[i+16] = tonal->mem[i+8];
       tonal->mem[i+8] = tonal->mem[i];
       tonal->mem[i] = BFCC[i];
    }
    for (i=0;i<9;i++)
       features[11+i] = (float)sqrt(tonal->std[i]) - std_feature_bias[i];
    features[18] = spec_variability - 0.78f;
    features[20] = info->tonality - 0.154723f;
    features[21] = info->activity - 0.724643f;
    features[22] = frame_stationarity - 0.743717f;
    features[23] = info->tonality_slope + 0.069216f;
    features[24] = tonal->lowECount - 0.067930f;

    analysis_compute_dense(&layer0, layer_out, features);
    analysis_compute_gru(&layer1, tonal->rnn_state, layer_out);
    analysis_compute_dense(&layer2, frame_probs, tonal->rnn_state);

    /* Probability of speech or music vs noise */
    info->activity_probability = frame_probs[1];
    info->music_prob = frame_probs[0];

    /*printf("%f %f %f\n", frame_probs[0], frame_probs[1], info->music_prob);*/

    info->bandwidth = bandwidth;
    tonal->prev_bandwidth = bandwidth;
    /*printf("%d %d\n", info->bandwidth, info->opus_bandwidth);*/
    info->noisiness = frame_noisiness;
    info->valid = 1;
    RESTORE_STACK;
}
void run_analysis(TonalityAnalysisState *analysis, const CELTMode *celt_mode, const void *analysis_pcm,
                 int analysis_frame_size, int frame_size, int c1, int c2, int C, opus_int32 Fs,
                 int lsb_depth, downmix_func downmix, AnalysisInfo *analysis_info)
{
   int offset;
   int pcm_len;

   analysis_frame_size -= analysis_frame_size&1;
   if (analysis_pcm != NULL)
   {
      /* Avoid overflow/wrap-around of the analysis buffer */
      analysis_frame_size = IMIN((DETECT_SIZE-5)*Fs/50, analysis_frame_size);

      pcm_len = analysis_frame_size - analysis->analysis_offset;
      offset = analysis->analysis_offset;
      while (pcm_len>0) {
         tonality_analysis(analysis, celt_mode, analysis_pcm, IMIN(Fs/50, pcm_len), offset, c1, c2, C, lsb_depth, downmix);
         offset += Fs/50;
         pcm_len -= Fs/50;
      }
      analysis->analysis_offset = analysis_frame_size;

      analysis->analysis_offset -= frame_size;
   }

   tonality_get_info(analysis, analysis_info, frame_size);
}
#define fmadd(a,b,c) ((a)*(b)+(c))
static OPUS_INLINE float tansig_approx(float x)
{
    const float N0 = 952.52801514f;
    const float N1 = 96.39235687f;
    const float N2 = 0.60863042f;
    const float D0 = 952.72399902f;
    const float D1 = 413.36801147f;
    const float D2 = 11.88600922f;
    float X2, num, den;
    X2 = x*x;
    num = fmadd(fmadd(N2, X2, N1), X2, N0);
    den = fmadd(fmadd(D2, X2, D1), X2, D0);
    num = num*x/den;
    return MAX32(-1.f, MIN32(1.f, num));
}
static OPUS_INLINE float sigmoid_approx(float x)
{
   return .5f + .5f*tansig_approx(.5f*x);
}
static void gemm_accum(float *out, const opus_int8 *weights, int rows, int cols, int col_stride, const float *x)
{
   int i, j;
   for (i=0;i<rows;i++)
   {
      for (j=0;j<cols;j++)
         out[i] += weights[j*col_stride + i]*x[j];
   }
}
void analysis_compute_dense(const AnalysisDenseLayer *layer, float *output, const float *input)
{
   int i;
   int N, M;
   int stride;
   M = layer->nb_inputs;
   N = layer->nb_neurons;
   stride = N;
   for (i=0;i<N;i++)
      output[i] = layer->bias[i];
   gemm_accum(output, layer->input_weights, N, M, stride, input);
   for (i=0;i<N;i++)
      output[i] *= WEIGHTS_SCALE;
   if (layer->sigmoid) {
      for (i=0;i<N;i++)
         output[i] = sigmoid_approx(output[i]);
   } else {
      for (i=0;i<N;i++)
         output[i] = tansig_approx(output[i]);
   }
}
void analysis_compute_gru(const AnalysisGRULayer *gru, float *state, const float *input)
{
   int i;
   int N, M;
   int stride;
   float tmp[MAX_NEURONS];
   float z[MAX_NEURONS];
   float r[MAX_NEURONS];
   float h[MAX_NEURONS];
   M = gru->nb_inputs;
   N = gru->nb_neurons;
   stride = 3*N;
   /* Compute update gate. */
   for (i=0;i<N;i++)
      z[i] = gru->bias[i];
   gemm_accum(z, gru->input_weights, N, M, stride, input);
   gemm_accum(z, gru->recurrent_weights, N, N, stride, state);
   for (i=0;i<N;i++)
      z[i] = sigmoid_approx(WEIGHTS_SCALE*z[i]);

   /* Compute reset gate. */
   for (i=0;i<N;i++)
      r[i] = gru->bias[N + i];
   gemm_accum(r, &gru->input_weights[N], N, M, stride, input);
   gemm_accum(r, &gru->recurrent_weights[N], N, N, stride, state);
   for (i=0;i<N;i++)
      r[i] = sigmoid_approx(WEIGHTS_SCALE*r[i]);

   /* Compute output. */
   for (i=0;i<N;i++)
      h[i] = gru->bias[2*N + i];
   for (i=0;i<N;i++)
      tmp[i] = state[i] * r[i];
   gemm_accum(h, &gru->input_weights[2*N], N, M, stride, input);
   gemm_accum(h, &gru->recurrent_weights[2*N], N, N, stride, tmp);
   for (i=0;i<N;i++)
      h[i] = z[i]*state[i] + (1-z[i])*tansig_approx(WEIGHTS_SCALE*h[i]);
   for (i=0;i<N;i++)
      state[i] = h[i];
}
static const opus_int8 layer0_weights[800] = {
   -30, -9, 2, -12, 5, -1, 8, 9,
   9, 8, -13, 18, -17, -34, -5, 17,
   -11, 0, -4, 10, 2, 10, 15, -8,
   2, -1, 0, 5, 13, -3, -16, 1,
   -5, 3, 7, -28, -13, 6, 36, -3,
   19, -60, -17, -28, 7, -11, -30, -7,
   2, -42, -21, -3, 6, -22, 33, -9,
   7, -30, 21, -14, 24, -11, -20, -18,
   -5, -12, 12, -49, -50, -49, 16, 9,
   -37, -1, 9, 34, -13, -31, -31, 12,
   16, 44, -42, 2, -9, 8, -18, -6,
   9, 36, 19, 11, 13, 12, -21, 3,
   -28, -12, 3, 33, 25, -14, 11, 1,
   -94, -39, 18, -12, -11, -15, -7, 49,
   52, 10, -43, 9, 57, 8, 21, -6,
   14, -15, 44, -8, 7, -30, -13, -2,
   -9, 25, -2, -127, 18, -11, -52, 26,
   -27, 27, 10, -10, 7, 43, 6, -24,
   41, 10, -18, -27, 10, 17, 9, 10,
   -17, -10, 20, -6, 22, 55, 35, -80,
   36, 25, -24, -36, 15, 9, -19, 88,
   19, 64, -51, -35, 17, 0, -7, 41,
   -16, 27, 4, 15, -1, 18, -16, 47,
   -39, -54, -8, 13, -25, -20, 102, -18,
   -5, 44, 11, -28, 71, 2, -51, -5,
   5, 2, -83, -9, -29, 8, 21, -53,
   58, -37, -7, 13, 38, 9, 34, -1,
   -41, 21, 4, -24, -36, -33, -21, 32,
   75, -2, 1, -68, -1, 47, -29, 32,
   20, 12, -65, -87, 5, 16, -12, 24,
   40, 15, 7, 19, -26, -17, 17, 6,
   -2, -37, -30, -9, 32, -127, -39, 0,
   -31, -27, 4, -22, 23, -6, -77, 35,
   -61, 32, -37, -24, 13, -11, -1, -40,
   -3, 17, -7, 13, 11, 59, -19, 10,
   6, -18, 0, 13, 3, -6, -23, 19,
   11, -17, 13, -1, -80, 40, -53, 69,
   -29, -54, 0, -4, 33, -25, -2, 38,
   35, 36, -15, 46, 2, -13, -16, -8,
   -8, 12, -24, -9, -55, -5, -9, 32,
   11, 7, 12, -18, -10, -86, -38, 54,
   37, -25, 18, -43, 7, -27, -27, -54,
   13, 9, 22, 70, 6, 35, -7, 23,
   -15, -44, -6, 7, -66, -85, 32, 40,
   -19, -9, -7, 12, -15, 7, 2, 6,
   -35, 11, 28, 0, 26, 14, 1, 1,
   4, 12, 18, 35, 22, -18, -3, 14,
   -1, 7, 14, -8, -14, -3, 4, -3,
   -19, -7, -1, -25, -27, 25, -26, -2,
   33, -22, -27, -25, 4, -9, 7, 21,
   26, -30, 10, -9, -20, 11, 27, 10,
   5, -18, 14, -4, 2, -17, -5, -7,
   -9, -13, 15, 29, 1, -10, -16, -10,
   35, 36, -7, -22, -44, 17, 30, 22,
   21, -1, 22, -11, 32, -8, -7, 5,
   -10, 5, 30, -20, 29, -20, -34, 12,
   -4, -6, 6, -13, 10, -5, -68, -1,
   24, 9, 19, -24, -64, 31, 19, 27,
   -26, 75, -45, 41, 39, -42, 8, 6,
   23, -30, 16, -25, 30, 34, 8, -38,
   -3, 18, 16, -31, 22, -4, -9, 1,
   20, 9, 38, -32, 0, -45, 0, -6,
   -13, 11, -25, -32, -22, 31, -24, -11,
   -11, -4, -4, 20, -34, 22, 20, 9,
   -25, 27, -5, 28, -29, 29, 6, 21,
   -6, -18, 54, 4, -46, 23, 21, -14,
   -31, 36, -41, -24, 4, 22, 10, 11,
   7, 36, -32, -13, -52, -17, 24, 28,
   -37, -36, -1, 24, 9, -38, 35, 48,
   18, 2, -1, 45, 10, 39, 24, -38,
   13, 8, -16, 8, 25, 11, 7, -29,
   -11, 7, 20, -30, -38, -45, 14, -18,
   -28, -9, 65, 61, 22, -53, -38, -16,
   36, 46, 20, -39, 32, -61, -6, -6,
   -36, -33, -18, -28, 56, 101, 45, 11,
   -28, -23, -29, -61, 20, -47, 2, 48,
   27, -17, 1, 40, 1, 3, -51, 15,
   35, 28, 22, 35, 53, -61, -29, 12,
   -6, -21, 10, 3, -20, 2, -25, 1,
   -6, 31, 11, -3, 1, -10, -52, 6,
   126, -105, 122, 127, -128, 127, 127, -128,
   127, 108, 12, 127, 48, -128, -36, -128,
   127, 127, -128, -128, 127, 89, -128, 127,
   -128, -128, -128, 127, 127, -128, -128, -93,
   -82, 20, 125, 65, -82, 127, 38, -74,
   81, 88, -88, 79, 51, -47, -111, -26,
   14, 83, -88, -112, 24, 35, -101, 98,
   -99, -48, -45, 46, 83, -60, -79, 45,
   -20, -41, 9, 4, 52, 54, 93, -10,
   4, 13, 3, 123, 6, 94, -111, -69,
   -14, -31, 10, 12, 53, -79, -11, -21,
   -2, -44, -72, 92, 65, -57, 56, -38,
   127, -56, -128, 127, 127, -128, 86, 117,
   -75, -128, 127, -19, -99, -112, 127, -128,
   127, -48, 114, 118, -128, -128, 117, -17,
   -6, 121, -128, 127, -128, 82, 54, -106,
   127, 127, -33, 100, -39, -23, 18, -78,
   -34, -29, -1, -30, 127, -26, 127, -128,
   126, -128, 27, -23, -79, -120, -127, 127,
   72, 66, 29, 7, -66, -56, -117, -128
};
static const opus_int8 layer0_bias[32] = {
   51, -16, 1, 13, -5, -6, -16, -7,
   11, -6, 106, 26, 28, -14, 21, -29,
   7, 18, -18, -17, 21, -17, -9, 20,
   -25, -3, -34, 48, 11, -13, -31, -20
};
static const opus_int8 layer1_weights[2304] = {
   22, -1, -7, 7, 29, -27, -31, -17,
   -13, 33, 44, -8, 11, 33, 24, 78,
   15, 19, 30, -2, -24, 5, 49, 5,
   36, 29, -14, -11, -48, -33, 21, -42,
   -38, -12, 55, -37, 54, -8, 1, 36,
   17, 0, 51, 31, 59, 7, -12, 53,
   4, 32, -14, 48, 5, -10, -16, -8,
   1, -16, -56, -24, -6, 18, -2, 23,
   6, 46, -6, -10, 20, 35, -44, -15,
   -49, 36, 16, 5, -7, -79, -67, 12,
   70, -3, -79, -54, -85, -24, 47, -22,
   33, 21, 69, -1, 11, 22, 14, -16,
   -16, -22, -28, -11, 11, -41, 31, -26,
   -33, -19, -4, 27, 32, -50, 5, -10,
   -38, -22, -8, 35, -31, 1, -41, -15,
   -11, 44, 28, -17, -41, -23, 17, 2,
   -23, -26, -13, -13, -17, 6, 14, -31,
   -25, 9, -19, 39, -8, 4, 31, -1,
   -45, -11, -28, -92, -46, -15, 21, 118,
   -22, 45, -51, 11, -20, -20, -15, 13,
   -21, -97, -29, -32, -23, -42, 94, 1,
   23, -8, 63, -3, -46, 19, -26, 32,
   -40, -74, -26, 26, -4, -13, 30, -20,
   -30, -25, -14, -31, -45, -43, 4, -60,
   -48, -12, -34, 2, 2, 3, 13, 15,
   11, 16, 5, 46, -9, -55, -16, -57,
   29, 14, 38, -50, -2, -44, -11, -8,
   52, -27, -38, -7, 20, 47, 17, -59,
   0, 47, 46, -63, 35, -17, 19, 33,
   68, -19, 2, 15, -16, 28, -16, -103,
   26, -35, 47, -39, -60, 30, 31, -23,
   -52, -13, 116, 47, -25, 30, 40, 30,
   -22, 2, 12, -27, -18, 31, -10, 27,
   -8, -66, 12, 14, 4, -26, -28, -13,
   3, 13, -26, -51, 37, 5, 2, -21,
   47, 3, 13, 25, -41, -27, -8, -4,
   5, -76, -33, 28, 10, 9, -46, -74,
   19, 28, 25, 31, 54, -55, 68, 38,
   -24, -32, 2, 4, 68, 11, -1, 99,
   5, 16, -2, -74, 40, 26, -26, 33,
   31, -1, -68, 14, -6, 25, 9, 29,
   60, 61, 7, -7, 0, -24, 7, 77,
   4, -1, 16, -7, 13, -15, -19, 28,
   -31, -24, -16, 37, 24, 13, 30, 10,
   -30, 11, 11, -10, 22, 60, 28, 45,
   -3, -40, -62, -5, -102, 9, -32, -27,
   -54, 21, 15, -5, 37, -43, -11, 37,
   -19, 47, -64, -128, -27, -114, 21, -66,
   59, 46, -3, -12, -87, -9, 4, 19,
   -113, -36, 78, 57, -26, -38, -77, -10,
   6, 6, -75, 25, -97, -11, 33, -46,
   1, 13, -21, -33, -20, 16, -6, -3,
   -11, -4, -27, 38, 8, -41, -2, -33,
   18, 19, -26, 1, -29, -22, -4, -14,
   -55, -11, -80, -3, 11, 34, 90, 51,
   11, 17, 43, 36, 127, -32, 29, 103,
   9, 27, 13, 64, 56, 70, -14, 3,
   -12, 10, 37, 3, 12, -22, -10, 46,
   28, 10, 20, 26, -24, 18, 9, 7,
   14, 34, -5, -7, 31, -14, -56, 11,
   -18, -8, -17, -7, -10, -40, 10, -33,
   -32, -43, 5, 9, 11, -4, 10, 50,
   -12, -5, 46, 9, 7, 1, 11, 15,
   91, -17, 7, -50, 23, 6, -30, -99,
   0, -17, 14, 8, -10, -25, -30, -69,
   -62, 31, 127, 114, -23, 101, -5, -54,
   -6, -22, 7, -56, 39, 18, -29, 0,
   46, 8, -79, 4, -21, 18, -32, 62,
   -12, -8, -12, -58, 31, -32, 17, 6,
   -24, 25, 24, 9, -4, -19, 45, 6,
   17, -14, 5, -27, 16, -4, -41, 25,
   -36, 5, 15, 12, 50, 27, 25, 23,
   -44, -69, -9, -19, -48, -8, 4, 12,
   -6, 13, -19, -30, -36, 26, 37, -1,
   -3, -30, -42, -14, -10, -20, 26, -54,
   -27, -44, 4, 73, -26, 90, 32, -69,
   -29, -16, 3, 103, 15, -17, 37, 24,
   -23, -31, 33, -37, -64, 25, 13, -81,
   -28, -32, 27, 5, -35, -23, 15, -22,
   19, -7, 9, 30, 19, -23, 27, -13,
   43, 29, -29, -6, 9, -40, -33, -33,
   -32, 9, 11, -48, -8, -23, -52, 46,
   17, -22, -42, 35, -15, -41, 16, 34,
   31, -42, -19, -11, 55, 7, -39, 89,
   -11, -33, 20, -14, 22, 32, 3, -17,
   -6, 14, 34, 1, 55, -21, -90, -8,
   18, 27, 13, -29, 21, 15, -33, -51,
   -9, -11, 4, -16, -18, 23, -4, -4,
   48, 1, 7, 29, -14, -12, -16, 17,
   35, 8, 0, -7, -2, 9, 8, 17,
   -6, 53, -32, -21, -50, 5, 99, -60,
   -5, -53, 10, -31, 12, -5, 7, 80,
   36, 18, -31, 9, 98, 36, -63, -35,
   4, -13, -28, -24, 28, -13, 18, 16,
   -1, -18, -34, 10, 20, 7, 4, 29,
   11, 25, -7, 36, 14, 45, 24, 1,
   -16, 30, 6, 35, -6, -11, -24, 13,
   -1, 27, 39, 20, 48, -11, -4, -13,
   28, 11, -31, -18, 31, -29, 22, -2,
   -20, -16, 5, 30, -12, -28, -3, 93,
   -16, 23, 18, -29, 6, -54, -37, 28,
   -3, -3, -47, -3, -36, -55, -3, 41,
   -10, 47, -2, 23, 42, -7, -71, -27,
   83, -64, 7, -24, 8, 26, -17, 15,
   12, 31, -30, -38, -13, -33, -56, 4,
   -17, 20, 18, 1, -30, -5, -6, -31,
   -14, -37, 0, 22, 10, -30, 37, -17,
   18, 6, 5, 23, -36, -32, 14, 18,
   -13, -61, -52, -69, 44, -30, 16, 18,
   -4, -25, 14, 81, 26, -8, -23, -59,
   52, -104, 17, 119, -32, 26, 17, 1,
   23, 45, 29, -64, -57, -14, 73, 21,
   -13, -13, 9, -68, -7, -52, 3, 24,
   -39, 44, -15, 27, 14, 19, -9, -28,
   -11, 5, 3, -34, -2, 2, 22, -6,
   -23, 4, 3, 13, -22, -13, -10, -18,
   29, 6, 44, -13, -24, -8, 2, 30,
   14, 43, 6, 17, -73, -6, -7, 20,
   -80, -7, -7, -28, 15, -69, -38, -5,
   -100, -35, 15, -79, 23, 29, -18, -27,
   21, -66, -37, 8, -22, -39, 48, 4,
   -13, 1, -9, 11, -29, 22, 6, -49,
   32, -14, 47, -18, -4, 44, -52, -74,
   43, 30, 23, -14, 5, 0, -27, 4,
   -7, 10, -4, 10, 1, -16, 11, -18,
   -2, -5, 2, -11, 0, -20, -4, 38,
   74, 59, 39, 64, -10, 26, -3, -40,
   -68, 3, -30, -51, 8, -19, -27, -46,
   51, 52, 54, 36, 90, 92, 14, 13,
   -5, 0, 16, -62, 16, 11, -47, -37,
   -6, -5, 21, 54, -57, 32, 42, -6,
   62, -9, 16, 21, 24, 9, -10, -4,
   33, 50, 13, -15, 1, -35, -48, 18,
   -11, -17, -67, -13, 21, 38, -44, 36,
   -16, 29, 17, 5, -10, 18, 17, -32,
   2, 8, 22, -56, -15, -32, 40, 43,
   19, 46, -7, -100, -96, 19, 53, 24,
   21, -26, -48, -101, -82, 61, 38, -85,
   -28, -34, -1, 63, -5, -5, 39, 39,
   -38, 32, -12, -28, 20, 40, -8, 2,
   31, 12, -35, -13, 20, -25, 30, 8,
   3, -13, -9, -20, 2, -13, 24, 37,
   -10, 33, 6, 20, -16, -24, -6, -6,
   -19, -5, 22, 21, 10, 11, -4, -39,
   -1, 6, 49, 41, -15, -57, 21, -62,
   77, -69, -13, 0, -74, 1, -7, -38,
   -8, 6, 63, 28, 4, 26, -52, 82,
   63, 13, 45, -33, 44, -52, -65, -21,
   -46, -49, 64, -17, 32, 24, 68, -39,
   -16, -5, -26, 28, 5, -61, -28, 2,
   24, 11, -12, -33, 9, -37, -3, -28,
   22, -37, -12, 19, 0, -18, -2, 14,
   1, 4, 8, -9, -2, 43, -17, -2,
   -66, -31, 56, -40, -87, -36, -2, -4,
   -42, -45, -1, 31, -43, -15, 27, 63,
   -11, 32, -10, -33, 27, -19, 4, 15,
   -26, -34, 29, -4, -39, -65, 14, -20,
   -21, -17, -36, 13, 59, 47, -38, -33,
   13, -37, -8, -37, -7, -6, -76, -31,
   -12, -46, 7, 24, -21, -30, -14, 9,
   15, -12, -13, 47, -27, -25, -1, -39,
   0, 20, -9, 6, 7, 4, 3, 7,
   39, 50, 22, -7, 14, -20, 1, 70,
   -28, 29, -41, 10, -16, -5, -28, -2,
   -37, 32, -18, 17, 62, -11, -20, -50,
   36, 21, -62, -12, -56, 52, 50, 17,
   3, 48, 44, -41, -25, 3, 16, -3,
   0, 33, -6, 15, 27, 34, -25, 22,
   9, 17, -11, 36, 16, -2, 12, 21,
   -52, 45, -2, -10, 46, 21, -18, 67,
   -28, -13, 30, 37, 42, 16, -9, 11,
   75, 7, -64, -40, -10, 29, 57, -23,
   5, 53, -77, 3, -17, -5, 47, -55,
   -35, -36, -13, 52, -53, -71, 52, -111,
   -23, -26, -28, 29, -43, 55, -19, 43,
   -19, 54, -12, -33, -44, -39, -19, -10,
   -31, -10, 21, 38, -57, -20, 2, -25,
   8, -6, 50, 12, 15, 25, -25, 15,
   -30, -6, 9, 25, 37, 19, -4, 31,
   -22, 2, 4, 2, 36, 7, 3, -34,
   -80, 36, -10, -2, -5, 31, -36, 49,
   -70, 20, -36, 21, 24, 25, -46, -51,
   36, -58, -48, -40, -10, 55, 71, 47,
   10, -1, 1, 2, -46, -68, 16, 13,
   0, -74, -29, 73, -52, -18, -11, 7,
   -44, -82, -32, -70, -28, -1, -39, -68,
   -6, -41, 12, -22, -16, 40, -11, -25,
   51, -9, 21, 4, 4, -34, 7, -78,
   16, 6, -38, -30, -2, -44, 32, 0,
   22, 64, 5, -72, -2, -14, -10, -16,
   -8, -25, 12, 102, -58, 37, -10, -23,
   15, 49, 7, -7, 2, -20, -32, 45,
   -6, 48, 28, 30, 33, -1, 22, -6,
   30, 65, -17, 29, 74, 37, -26, -10,
   15, -24, 19, -66, 22, -10, -31, -1,
   -18, -9, 11, 37, -4, 45, 5, 41,
   17, 1, 1, 24, -58, 41, 5, -51,
   14, 8, 43, 16, -10, -1, 45, 32,
   -64, 3, -33, -25, -3, -27, -68, 12,
   23, -11, -13, -37, -40, 4, -21, -12,
   32, -23, -19, 76, 41, -23, -24, -44,
   -65, -1, -15, 1, 71, 63, 5, 20,
   -3, 21, -23, 31, -32, 18, -2, 27,
   31, 46, -5, -39, -5, -35, 18, -18,
   -40, -10, 3, 12, 2, -2, -22, 40,
   5, -6, 60, 36, 3, 29, -27, 10,
   25, -54, 5, 26, 39, 35, -24, -37,
   30, -91, 28, -4, -21, -27, -39, -6,
   5, 12, -128, 38, -16, 29, -95, -29,
   82, -2, 35, 2, 12, 8, -22, 10,
   80, -47, 2, -25, -73, -79, 16, -30,
   -32, -66, 48, 21, -45, -11, -47, 14,
   -27, -17, -7, 15, -44, -14, -44, -26,
   -32, 26, -23, 17, -7, -28, 26, -6,
   28, 6, -26, 2, 13, -14, -23, -14,
   19, 46, 16, 2, -33, -21, 28, -17,
   -42, 44, -37, 1, -39, 28, 84, -46,
   15, 10, 13, -44, 72, -26, 26, 32,
   -28, -12, -83, 2, 10, -30, -44, -10,
   -28, 53, 45, 65, 0, -25, 57, 36,
   -33, 6, 29, 44, -53, 11, 19, -2,
   -27, 35, 32, 49, 4, 23, 38, 36,
   24, 10, 51, -39, 4, -7, 26, 37,
   -35, 11, -47, -18, 28, 16, -35, 42,
   17, -21, -41, 28, 14, -12, 11, -45,
   7, -43, -15, 18, -5, 38, -40, -50,
   -30, -21, 9, -98, 13, 12, 23, 75,
   -56, -7, -3, -4, -1, -34, 12, -49,
   11, 26, -18, -28, -17, 33, 13, -14,
   40, 24, -72, -37, 10, 17, -6, 22,
   16, 16, -6, -12, -30, -14, 10, 40,
   -23, 12, 15, -3, -15, 13, -56, -4,
   -30, 1, -3, -17, 27, 50, -5, 64,
   -36, -19, 7, 29, 22, 25, 9, -16,
   -58, -69, -40, -61, -71, -14, 42, 93,
   26, 11, -6, -58, -11, 70, -52, 19,
   9, -30, -33, 11, -37, -47, -21, -22,
   -40, 10, 47, 4, -23, 17, 48, 41,
   -48, 14, 10, 15, 34, -23, -2, -47,
   23, -32, -13, -10, -26, -26, -4, 16,
   38, -14, 0, -12, -7, -7, 20, 44,
   -1, -32, -27, -16, 4, -6, -18, 14,
   5, 4, -29, 28, 7, -7, 15, -11,
   -20, -45, -36, 16, 84, 34, -59, -30,
   22, 126, 8, 68, 79, -17, 21, -68,
   37, 5, 15, 63, 49, 127, -90, 85,
   43, 7, 16, 9, 6, -45, -57, -43,
   57, 11, -23, -11, -29, 60, -26, 0,
   7, 42, -24, 10, 23, -25, 8, -7,
   -40, 19, -17, 35, 4, 27, -39, -91,
   27, -36, 34, 2, 16, -24, 25, 7,
   -21, 5, 17, 10, -22, -30, 9, -17,
   -61, -26, 33, 21, 58, -51, -14, 69,
   -38, 20, 7, 80, -4, -65, -6, -27,
   53, -12, 47, -1, -15, 1, 60, 102,
   -79, -4, 12, 9, 22, 37, -8, -4,
   37, 2, -3, -15, -16, -11, -5, 19,
   -6, -43, 20, -25, -18, 10, -27, 0,
   -28, -27, -11, 10, -18, -2, -4, -16,
   26, 14, -6, 7, -6, 1, 53, -2,
   -29, 23, 9, -30, -6, -4, -6, 56,
   70, 0, -33, -20, -17, -9, -24, 46,
   -5, -105, 47, -46, -51, 20, 20, -53,
   -81, -1, -7, 75, -5, -21, -65, 12,
   -52, 22, -50, -12, 49, 54, 76, -81,
   10, 45, -41, -59, 18, -19, 25, 14,
   -31, -53, -5, 12, 31, 84, -23, 2,
   7, 2, 10, -32, 39, -2, -12, 1,
   -9, 0, -10, -11, 9, 15, -8, -2,
   2, -1, 10, 14, -5, -40, 19, -7,
   -7, 26, -4, 2, 1, -27, 35, 32,
   21, -31, 26, 43, -9, 4, -32, 40,
   -62, -52, 36, 22, 38, 22, 36, -96,
   6, -10, -23, -49, 15, -33, -18, -3,
   0, 41, 21, -19, 21, 23, -39, -23,
   -6, 6, 47, 56, 4, 74, 0, -98,
   29, -47, -14, -36, 21, -22, 22, 16,
   13, 12, 16, -5, 13, 17, -13, -15,
   1, -34, -26, 26, 12, 32, 27, 13,
   -67, 27, 2, 8, 10, 18, 16, 20,
   -17, -17, 57, -64, 5, 14, 19, 31,
   -18, -44, -46, -16, 4, -25, 17, -126,
   -24, 39, 4, 8, 55, -25, -34, 39,
   -16, 3, 9, 71, 72, -31, -55, 6,
   10, -25, 32, -85, -21, 18, -8, 15,
   12, -27, -7, 1, -21, -2, -5, 48,
   -16, 18, 1, -22, -26, 16, 14, -31,
   27, -6, -15, -21, 4, -14, 18, -36
};
static const opus_int8 layer1_recur_weights[1728] = {
   20, 67, -99, 12, 41, -25, 49, -44,
   35, 81, 110, 47, 34, -66, -14, 14,
   -60, 34, 29, -73, 10, 41, 35, 89,
   7, -35, 22, 7, 27, -20, -6, 56,
   26, 66, 6, 33, -55, 53, 1, -21,
   14, 17, 68, 55, 59, 0, 18, -9,
   5, -41, 6, -5, -114, -12, 29, 42,
   -23, 10, 81, -27, 20, -53, -30, -62,
   40, 95, 25, -4, 3, 18, -8, -15,
   -29, -82, 2, -57, -3, -61, -29, -29,
   49, 2, -55, 5, -69, -99, -49, -51,
   6, -25, 12, 89, 44, -33, 5, 41,
   1, 23, -37, -37, -28, -48, 3, 4,
   -41, -30, -57, -35, -39, -1, -13, -56,
   -5, 50, 49, 41, -4, -4, 33, -22,
   -1, 33, 34, 18, 40, -42, 12, 1,
   -6, -2, 18, 17, 39, 44, 11, 65,
   -60, -45, 10, 91, 21, 9, -62, -11,
   8, 69, 37, 24, -30, 21, 26, -27,
   1, -28, 24, 66, -8, 6, -71, 34,
   24, 44, 58, -78, -19, 57, 17, -60,
   1, 12, -3, -1, -40, 22, 11, -5,
   25, 12, 1, 72, 79, 7, -50, 23,
   18, 13, 21, -11, -20, 5, 77, -94,
   24, 15, 57, -51, 3, 36, 53, -1,
   4, 14, 30, -31, 22, 40, 32, -11,
   -34, -36, -59, 58, 25, 21, -54, -23,
   40, 46, 18, 0, 12, 54, -96, -99,
   -59, 5, 119, -38, 50, 55, 12, -16,
   67, 0, 34, 35, 39, 35, -1, 69,
   24, 27, -30, -35, -4, -70, 2, -44,
   -7, -6, 19, -9, 60, 44, -21, -10,
   37, 43, -16, -3, 30, -15, -65, 31,
   -55, 18, -98, 76, 64, 25, 24, -18,
   -7, -68, -10, 38, 27, -60, 36, 33,
   16, 30, 34, -39, -37, 31, 12, 53,
   -54, 14, -26, -49, -128, -13, -5, -22,
   -11, -85, 55, -8, -51, -11, -33, -10,
   -31, -76, -41, 23, 44, -40, -54, -127,
   -101, 19, -23, -15, 15, 27, 58, -60,
   8, 14, -33, 1, 48, -9, -11, -123,
   3, 53, 23, 4, -28, 22, 2, -29,
   -67, 36, 12, 7, 55, -21, 88, 20,
   -1, -21, -17, 3, 41, 32, -10, -14,
   -5, -57, 67, 57, 21, 23, -2, -27,
   -73, -24, 120, 21, 18, -35, 42, -7,
   3, -45, -25, 76, -34, 50, 11, -54,
   -91, 3, -113, -20, -5, 47, 15, -47,
   17, 27, -3, -26, -7, 10, 7, 74,
   -40, 64, -7, -5, -24, -49, -24, -3,
   -10, 27, -17, -8, -3, 14, -27, 33,
   13, 39, 28, -7, -38, 29, 16, 44,
   19, 55, -3, 9, -13, -57, 43, 43,
   31, 0, -93, -17, 19, -56, 4, -12,
   -25, 37, -85, -13, -118, 33, -17, 56,
   71, -80, -4, 6, -11, -18, 47, -52,
   25, 9, 48, -107, 1, 21, 20, -3,
   10, -16, -4, 24, 17, 31, -61, -18,
   -50, 24, -10, 12, 71, 26, 11, -3,
   4, 1, 0, -7, -40, 18, 38, -34,
   38, 17, 8, -34, 2, 21, 123, -32,
   -26, 43, 14, -34, -1, -9, 37, -16,
   6, -17, -62, 68, 22, 17, 11, -75,
   33, -80, 62, -9, -75, 76, 36, -41,
   -8, -40, -11, -71, 40, -39, 62, -49,
   -81, 16, -9, -52, 52, 61, 17, -103,
   -27, -10, -8, -54, -57, 21, 23, -16,
   -52, 36, 18, 10, -5, 8, 15, -29,
   5, -19, -37, 8, -53, 6, 19, -37,
   38, -17, 48, 10, 0, 81, 46, 70,
   -29, 101, 11, 44, -44, -3, 24, 11,
   3, 14, -9, 11, 14, -45, 13, 46,
   -3, -57, 68, 44, 63, 98, 25, -28,
   -23, 15, 32, -10, 53, -6, -2, -9,
   -6, 16, -107, -11, -11, -28, 59, 57,
   -22, 38, 42, 83, 27, 5, 29, -30,
   12, -21, -13, 31, 38, -21, 58, -10,
   -10, -15, -2, -5, 11, 12, -73, -28,
   -38, 22, 2, -25, 73, -52, -12, -55,
   32, -63, 21, 51, 33, 52, -26, 55,
   -26, -26, 57, -32, -4, -52, -61, 21,
   -33, -91, -51, 69, -90, -53, -38, -44,
   12, -76, -20, 77, -45, -7, 86, 43,
   -109, -33, -105, -40, -121, -10, 0, -72,
   45, -51, -75, -49, -38, -1, -62, 18,
   -1, 30, -44, -14, -10, -67, 40, -10,
   -34, 46, -64, -32, 29, -13, 33, 3,
   -32, -5, 28, -27, -25, 93, 24, 68,
   -40, 57, 23, -3, -21, -58, 17, -39,
   -17, -22, -89, 11, 18, -46, 27, 24,
   46, 127, 61, 87, 31, 127, -36, 47,
   -23, 47, 127, -24, 110, 122, 30, 100,
   0, 96, -12, 6, 50, 44, -13, 73,
   4, 55, -11, -15, 49, 42, -6, 20,
   -35, 58, 18, 38, 42, 72, 19, -21,
   11, 9, -37, 7, 29, 31, 16, -17,
   13, -50, 19, 5, -23, 51, -16, -5,
   4, -24, 76, 10, -53, -28, -7, -65,
   74, 40, -16, -29, 32, -16, -49, -35,
   -3, 59, -96, -50, -43, -43, -61, -15,
   -8, -36, -34, -33, -14, 11, -3, -39,
   4, -114, -123, -11, -49, -21, 14, -56,
   1, 43, -63, 26, 40, 18, -10, -26,
   -14, -15, -35, -35, -11, 32, -44, -67,
   2, 22, 7, 3, -9, -30, -51, -28,
   28, 6, -22, 16, 34, -25, -52, -54,
   -8, -6, 5, 8, 20, -16, -17, -44,
   27, 3, 31, -5, -48, -1, -3, 116,
   11, 71, -31, -47, 109, 50, -22, -12,
   -57, 32, 66, 8, -25, -93, -54, -10,
   19, -76, -34, 97, 48, -36, -18, -30,
   -39, -26, -12, 28, 14, 12, -12, -31,
   38, 2, 10, 4, -40, 20, 16, -61,
   2, 64, 39, 5, 15, 33, 40, -61,
   -49, 93, -10, 33, 28, -11, -27, -18,
   39, -62, -6, -6, 62, 11, -8, 38,
   -67, 12, 27, 39, -27, 123, -18, -6,
   -65, 83, -64, 20, 19, -11, 33, 24,
   17, 56, 78, 7, -15, 54, -101, -9,
   115, -96, 50, 51, 35, 34, 27, 37,
   -40, -11, 8, -36, 42, -45, 2, -23,
   0, 67, -8, -9, -13, 50, -14, -27,
   4, 0, -8, -14, 30, -9, 29, 15,
   9, -38, 37, -8, 50, -46, 54, 41,
   -11, -8, -11, -26, 39, 45, 14, -26,
   -17, -27, 69, 38, 39, 98, 66, 0,
   42, 123, -101, -19, -83, 117, -32, 56,
   10, 12, -88, 79, -53, 56, 63, 95,
   -62, 9, 36, -13, -79, -16, 37, -46,
   35, -34, 14, 17, -54, 5, 21, -7,
   7, 63, 56, 15, 27, -76, -25, 4,
   -26, -63, 28, -67, -52, 43, -47, -70,
   40, -12, 40, -66, -37, 0, 35, 37,
   -53, 4, -17, -51, 11, 21, 14, -34,
   -4, 24, -42, 29, 22, 7, 28, 12,
   37, 39, -39, -19, 65, -60, -50, -2,
   1, 82, 39, 19, -23, -43, -22, -67,
   -35, -34, 32, 102, 81, 127, 36, 67,
   -45, 1, -67, -52, -4, 35, 20, 28,
   71, 86, -35, -9, -83, -34, 12, 9,
   -23, 2, 14, 28, -23, 7, -25, 45,
   7, 17, -37, 0, -19, 31, 26, 40,
   -27, -16, 17, 5, -21, 23, 24, 96,
   -55, 52, -19, -14, -6, 1, 50, -34,
   86, -53, 38, 2, -52, -36, -13, 60,
   -85, -120, 32, 7, -12, 22, 70, -7,
   -94, 38, -76, -31, -20, 15, -28, 7,
   6, 40, 53, 88, 3, 38, 18, -8,
   -22, -23, 51, 37, -9, 13, -32, 25,
   -21, 27, 31, 20, 18, -9, -13, 1,
   21, -24, -13, 39, 15, -11, -29, -36,
   18, 15, 8, 27, 21, -94, -1, -22,
   49, 66, -1, 6, -3, -40, -18, 6,
   28, 12, 33, -59, 62, 60, -48, 90,
   -1, 108, 9, 18, -2, 27, 77, -65,
   82, -48, -38, -19, -11, 127, 50, 66,
   18, -13, -22, 60, -38, 40, -14, -26,
   -13, 38, 67, 57, 30, 33, 26, 36,
   38, -17, 27, -28, 20, 12, -64, 18,
   5, -33, -27, 13, -26, 32, 35, -5,
   -48, -14, 92, 43, -47, -14, 40, 11,
   51, 66, 22, -63, -16, -61, 4, -28,
   27, 20, -33, -30, -21, -29, -53, 31,
   -40, 24, 43, -4, -19, 21, 67, 20,
   100, -16, -93, 78, -6, -18, -52, -37,
   -9, 66, -31, -8, 26, 18, 4, 24,
   -22, 17, -2, -13, 27, 0, 8, -18,
   -25, 5, -21, -24, -7, 18, -93, 21,
   7, 2, -75, 69, 50, -5, -15, -17,
   60, -42, 55, 1, -4, 3, 10, 46,
   16, -13, 45, -7, -10, -44, -108, 49,
   2, -15, -64, -12, -72, 32, -38, -45,
   10, -54, 13, -13, -27, -36, -64, 58,
   -62, -101, 88, -86, -71, -39, -9, -128,
   32, 15, -4, 54, -16, -39, -26, -36,
   46, 48, -64, -10, 19, 30, -13, 34,
   -8, 50, 60, -22, -6, -11, -30, 5,
   50, 32, 56, 0, 25, 6, 68, 11,
   -29, 45, -9, -12, 4, 1, 18, -49,
   0, -38, -19, 90, 29, 35, 51, 8,
   -48, 96, -1, -12, -9, -32, -63, -65,
   -7, 38, 89, 28, -85, -28, -23, -25,
   -128, 56, 79, -36, 99, -6, -37, 7,
   -13, -69, -46, -29, 25, 64, -21, 17,
   1, 42, -66, 1, 80, 26, -32, 21,
   15, 15, 6, 6, -10, 15, 127, 5,
   38, 27, 87, -57, -25, 11, 72, -21,
   -5, 11, -13, -66, 78, 36, -3, 41,
   -21, 8, -33, 23, 73, 28, 57, -25,
   -5, 4, -22, -47, 15, 4, -57, -72,
   33, 1, 18, 2, 53, -71, -99, -21,
   -3, -111, 108, 71, -14, 82, 25, 61,
   -48, 5, 9, -51, -20, -25, -3, 14,
   -33, 14, -3, -34, 22, 12, -19, -38,
   -16, 2, 21, 16, 26, -31, 75, 44,
   -31, 16, 26, 66, 17, -9, -22, -22,
   22, -44, 22, 27, 2, 58, -14, 10,
   -73, -42, 55, -25, -61, 72, -1, 30,
   -58, -25, 63, 26, -48, -40, 26, -30,
   60, 8, -17, -1, -18, -20, 43, -20,
   -4, -28, 127, -106, 29, 70, 64, -27,
   39, -33, -5, -88, -40, -52, 26, 44,
   -17, 23, 2, -49, 22, -9, -8, 86,
   49, -43, -60, 1, 10, 45, 36, -53,
   -4, 33, 38, 48, -72, 1, 19, 21,
   -65, 4, -5, -62, 27, -25, 17, -6,
   6, -45, -39, -46, 4, 26, 127, -9,
   18, -33, -18, -3, 33, 2, -5, 15,
   -26, -22, -117, -63, -17, -59, 61, -74,
   7, -47, -58, -128, -67, 15, -16, -128,
   12, 2, 20, 9, -48, -40, 43, 3,
   -40, -16, -38, -6, -22, -28, -16, -59,
   -22, 6, -5, 11, -12, -66, -40, 27,
   -62, -44, -19, 38, -3, 39, -8, 40,
   -24, 13, 21, 50, -60, -22, 53, -29,
   -6, 1, 22, -59, 0, 17, -39, 115
};
static const opus_int8 layer1_bias[72] = {
   -42, 20, 16, 0, 105, 60, 1, -97,
   24, 60, 18, 13, 62, 25, 127, 34,
   79, 55, 118, 127, 95, 31, -4, 87,
   21, 12, 2, -14, 18, 23, 8, 17,
   -1, -8, 5, 4, 24, 37, 21, 13,
   36, 13, 17, 18, 37, 30, 33, 1,
   8, -16, -11, -5, -31, -3, -5, 0,
   6, 3, 58, -7, -1, -16, 5, -13,
   16, 10, -2, -14, 11, -4, 3, -11
};
static const opus_int8 layer2_weights[48] = {
   -113, -88, 31, -128, -126, -61, 85, -35,
   118, -128, -61, 127, -128, -17, -128, 127,
   104, -9, -128, 33, 45, 127, 5, 83,
   84, -128, -85, -128, -45, 48, -53, -128,
   46, 127, -17, 125, 117, -41, -117, -91,
   -127, -68, -1, -89, -80, 32, 106, 7
};
static const opus_int8 layer2_bias[2] = {
   14, 117
};
const AnalysisDenseLayer layer0 = {
   layer0_bias,
   layer0_weights,
   25, 32, 0
};
const AnalysisGRULayer layer1 = {
   layer1_bias,
   layer1_weights,
   layer1_recur_weights,
   32, 24
};
const AnalysisDenseLayer layer2 = {
   layer2_bias,
   layer2_weights,
   24, 2, 1
};
int encode_size(int size, unsigned char *data)
{
   if (size < 252)
   {
      data[0] = size;
      return 1;
   } else {
      data[0] = 252+(size&0x3);
      data[1] = (size-(int)data[0])>>2;
      return 2;
   }
}
static int parse_size(const unsigned char *data, opus_int32 len, opus_int16 *size)
{
   if (len<1)
   {
      *size = -1;
      return -1;
   } else if (data[0]<252)
   {
      *size = data[0];
      return 1;
   } else if (len<2)
   {
      *size = -1;
      return -1;
   } else {
      *size = 4*data[1] + data[0];
      return 2;
   }
}
int opus_packet_get_samples_per_frame(const unsigned char *data,
      opus_int32 Fs)
{
   int audiosize;
   if (data[0]&0x80)
   {
      audiosize = ((data[0]>>3)&0x3);
      audiosize = (Fs<<audiosize)/400;
   } else if ((data[0]&0x60) == 0x60)
   {
      audiosize = (data[0]&0x08) ? Fs/50 : Fs/100;
   } else {
      audiosize = ((data[0]>>3)&0x3);
      if (audiosize == 3)
         audiosize = Fs*60/1000;
      else
         audiosize = (Fs<<audiosize)/100;
   }
   return audiosize;
}
int opus_packet_parse_impl(const unsigned char *data, opus_int32 len,
      int self_delimited, unsigned char *out_toc,
      const unsigned char *frames[48], opus_int16 size[48],
      int *payload_offset, opus_int32 *packet_offset,
      const unsigned char **padding, opus_int32 *padding_len)
{
   int i, bytes;
   int count;
   int cbr;
   unsigned char ch, toc;
   int framesize;
   opus_int32 last_size;
   opus_int32 pad = 0;
   const unsigned char *data0 = data;

   if (size==NULL || len<0)
      return OPUS_BAD_ARG;
   if (len==0)
      return OPUS_INVALID_PACKET;

   framesize = opus_packet_get_samples_per_frame(data, 48000);

   cbr = 0;
   toc = *data++;
   len--;
   last_size = len;
   switch (toc&0x3)
   {
   /* One frame */
   case 0:
      count=1;
      break;
   /* Two CBR frames */
   case 1:
      count=2;
      cbr = 1;
      if (!self_delimited)
      {
         if (len&0x1)
            return OPUS_INVALID_PACKET;
         last_size = len/2;
         /* If last_size doesn't fit in size[0], we'll catch it later */
         size[0] = (opus_int16)last_size;
      }
      break;
   /* Two VBR frames */
   case 2:
      count = 2;
      bytes = parse_size(data, len, size);
      len -= bytes;
      if (size[0]<0 || size[0] > len)
         return OPUS_INVALID_PACKET;
      data += bytes;
      last_size = len-size[0];
      break;
   /* Multiple CBR/VBR frames (from 0 to 120 ms) */
   default: /*case 3:*/
      if (len<1)
         return OPUS_INVALID_PACKET;
      /* Number of frames encoded in bits 0 to 5 */
      ch = *data++;
      count = ch&0x3F;
      if (count <= 0 || framesize*(opus_int32)count > 5760)
         return OPUS_INVALID_PACKET;
      len--;
      /* Padding flag is bit 6 */
      if (ch&0x40)
      {
         int p;
         do {
            int tmp;
            if (len<=0)
               return OPUS_INVALID_PACKET;
            p = *data++;
            len--;
            tmp = p==255 ? 254: p;
            len -= tmp;
            pad += tmp;
         } while (p==255);
      }
      if (len<0)
         return OPUS_INVALID_PACKET;
      /* VBR flag is bit 7 */
      cbr = !(ch&0x80);
      if (!cbr)
      {
         /* VBR case */
         last_size = len;
         for (i=0;i<count-1;i++)
         {
            bytes = parse_size(data, len, size+i);
            len -= bytes;
            if (size[i]<0 || size[i] > len)
               return OPUS_INVALID_PACKET;
            data += bytes;
            last_size -= bytes+size[i];
         }
         if (last_size<0)
            return OPUS_INVALID_PACKET;
      } else if (!self_delimited)
      {
         /* CBR case */
         last_size = len/count;
         if (last_size*count!=len)
            return OPUS_INVALID_PACKET;
         for (i=0;i<count-1;i++)
            size[i] = (opus_int16)last_size;
      }
      break;
   }
   /* Self-delimited framing has an extra size for the last frame. */
   if (self_delimited)
   {
      bytes = parse_size(data, len, size+count-1);
      len -= bytes;
      if (size[count-1]<0 || size[count-1] > len)
         return OPUS_INVALID_PACKET;
      data += bytes;
      /* For CBR packets, apply the size to all the frames. */
      if (cbr)
      {
         if (size[count-1]*count > len)
            return OPUS_INVALID_PACKET;
         for (i=0;i<count-1;i++)
            size[i] = size[count-1];
      } else if (bytes+size[count-1] > last_size)
         return OPUS_INVALID_PACKET;
   } else
   {
      /* Because it's not encoded explicitly, it's possible the size of the
         last packet (or all the packets, for the CBR case) is larger than
         1275. Reject them here.*/
      if (last_size > 1275)
         return OPUS_INVALID_PACKET;
      size[count-1] = (opus_int16)last_size;
   }

   if (payload_offset)
      *payload_offset = (int)(data-data0);

   for (i=0;i<count;i++)
   {
      if (frames)
         frames[i] = data;
      data += size[i];
   }

   if (padding != NULL)
   {
      *padding = data;
      *padding_len = pad;
   }
   if (packet_offset)
      *packet_offset = pad+(opus_int32)(data-data0);

   if (out_toc)
      *out_toc = toc;

   return count;
}
#define silk_int32_MAX 0x7FFFFFFF
/* assertions */
#define silk_assert(COND) 
/* Structure for controlling encoder operation */
typedef struct {
    /* I:   Number of channels; 1/2                                                         */
    opus_int32 nChannelsAPI;

    /* I:   Number of channels; 1/2                                                         */
    opus_int32 nChannelsInternal;

    /* I:   Input signal sampling rate in Hertz; 8000/12000/16000/24000/32000/44100/48000   */
    opus_int32 API_sampleRate;

    /* I:   Maximum internal sampling rate in Hertz; 8000/12000/16000                       */
    opus_int32 maxInternalSampleRate;

    /* I:   Minimum internal sampling rate in Hertz; 8000/12000/16000                       */
    opus_int32 minInternalSampleRate;

    /* I:   Soft request for internal sampling rate in Hertz; 8000/12000/16000              */
    opus_int32 desiredInternalSampleRate;

    /* I:   Number of samples per packet in milliseconds; 10/20/40/60                       */
    opus_int payloadSize_ms;

    /* I:   Bitrate during active speech in bits/second; internally limited                 */
    opus_int32 bitRate;

    /* I:   Uplink packet loss in percent (0-100)                                           */
    opus_int packetLossPercentage;

    /* I:   Complexity mode; 0 is lowest, 10 is highest complexity                          */
    opus_int complexity;

    /* I:   Flag to enable in-band Forward Error Correction (FEC); 0/1                      */
    opus_int useInBandFEC;

    /* I:   Flag to enable in-band Deep REDundancy (DRED); 0/1                              */
    opus_int useDRED;

    /* I:   Flag to actually code in-band Forward Error Correction (FEC) in the current packet; 0/1 */
    opus_int LBRR_coded;

    /* I:   Flag to enable discontinuous transmission (DTX); 0/1                            */
    opus_int useDTX;

    /* I:   Flag to use constant bitrate                                                    */
    opus_int useCBR;

    /* I:   Maximum number of bits allowed for the frame                                    */
    opus_int maxBits;

    /* I:   Causes a smooth downmix to mono                                                 */
    opus_int toMono;

    /* I:   Opus encoder is allowing us to switch bandwidth                                 */
    opus_int opusCanSwitch;

    /* I: Make frames as independent as possible (but still use LPC)                        */
    opus_int reducedDependency;

    /* O:   Internal sampling rate used, in Hertz; 8000/12000/16000                         */
    opus_int32 internalSampleRate;

    /* O: Flag that bandwidth switching is allowed (because low voice activity)             */
    opus_int allowBandwidthSwitch;

    /* O:   Flag that SILK runs in WB mode without variable LP filter (use for switching between WB/SWB/FB) */
    opus_int inWBmodeWithoutVariableLP;

    /* O:   Stereo width */
    opus_int stereoWidth_Q14;

    /* O:   Tells the Opus encoder we're ready to switch                                    */
    opus_int switchReady;

    /* O: SILK Signal type */
    opus_int signalType;

    /* O: SILK offset (dithering) */
    opus_int offset;
} silk_EncControlStruct;
/* Encoder functions                    */

/* Get size in bytes of the Silk encoder state */
opus_int silk_Get_Encoder_Size(                         /* O    Returns error code                              */
    opus_int                        *encSizeBytes       /* O    Number of bytes in SILK encoder state           */
);
/* Init or reset encoder */
opus_int silk_InitEncoder(                              /* O    Returns error code                              */
    void                            *encState,          /* I/O  State                                           */
    int                              arch,              /* I    Run-time architecture                           */
    silk_EncControlStruct           *encStatus          /* O    Encoder Status                                  */
);
/* Encode frame with Silk */
/* Note: if prefillFlag is set, the input must contain 10 ms of audio, irrespective of what                     */
/* encControl->payloadSize_ms is set to                                                                         */
opus_int silk_Encode(                                   /* O    Returns error code                              */
    void                            *encState,          /* I/O  State                                           */
    silk_EncControlStruct           *encControl,        /* I    Control status                                  */
    const opus_int16                *samplesIn,         /* I    Speech sample input vector                      */
    opus_int                        nSamplesIn,         /* I    Number of samples in input vector               */
    ec_enc                          *psRangeEnc,        /* I/O  Compressor data structure                       */
    opus_int32                      *nBytesOut,         /* I/O  Number of bytes in payload (input: Max bytes)   */
    const opus_int                  prefillFlag,        /* I    Flag to indicate prefilling buffers no coding   */
    int                             activity            /* I    Decision of Opus voice activity detector        */
);
/* This is an OPUS_INLINE header file for general platform. */

/* (a32 * (opus_int32)((opus_int16)(b32))) >> 16 output have to be 32bit int */
#define silk_SMULWB(a32,b32) ((opus_int32)(((a32) * (opus_int64)((opus_int16)(b32))) >> 16))
/* a32 + (b32 * (opus_int32)((opus_int16)(c32))) >> 16 output have to be 32bit int */
#define silk_SMLAWB(a32,b32,c32) ((opus_int32)((a32) + (((b32) * (opus_int64)((opus_int16)(c32))) >> 16)))
/* (opus_int32)((opus_int16)(a3))) * (opus_int32)((opus_int16)(b32)) output have to be 32bit int */
#define silk_SMULBB(a32,b32) ((opus_int32)((opus_int16)(a32)) * (opus_int32)((opus_int16)(b32)))
/* (a32 * b32) >> 16 */
#define silk_SMULWW(a32,b32) ((opus_int32)(((opus_int64)(a32) * (b32)) >> 16))
static OPUS_INLINE opus_int32 silk_CLZ32(opus_int32 in32)
{
    return in32 ? 32 - EC_ILOG(in32) : 32;
}
/*                        SCALAR FUNCTIONS                          */

/* Approximation of 128 * log2() (exact inverse of approx 2^() below) */
/* Convert input to a log scale    */
opus_int32 silk_lin2log(
    const opus_int32            inLin               /* I  input in linear scale                                         */
);
/* Approximation of 2^() (exact inverse of approx log2() above) */
/* Convert input to a linear scale */
opus_int32 silk_log2lin(
    const opus_int32            inLog_Q7            /* I  input on log scale                                            */
);
/*                                MACROS                            */

/* Rotate a32 right by 'rot' bits. Negative rot values result in rotating
   left. Output is 32bit int.
   Note: contemporary compilers recognize the C expression below and
   compile it into a 'ror' instruction if available. No need for OPUS_INLINE ASM! */
static OPUS_INLINE opus_int32 silk_ROR32( opus_int32 a32, opus_int rot )
{
    opus_uint32 x = (opus_uint32) a32;
    opus_uint32 r = (opus_uint32) rot;
    opus_uint32 m = (opus_uint32) -rot;
    if( rot == 0 ) {
        return a32;
    } else if( rot < 0 ) {
        return (opus_int32) ((x << m) | (x >> (32 - m)));
    } else {
        return (opus_int32) ((x << (32 - r)) | (x >> r));
    }
}
/* Fixed point macros */

/* (a32 * b32) output have to be 32bit int */
#define silk_MUL(a32,b32) ((a32) * (b32))
/* a32 + (b32 * c32) output have to be 32bit int */
#define silk_MLA(a32,b32,c32) silk_ADD32((a32),((b32) * (c32)))
#define silk_DIV32_16(a32,b16) ((opus_int32)((a32) / (b16)))
#define silk_ADD32(a,b) ((a) + (b))
#define silk_LSHIFT32(a,shift) ((opus_int32)((opus_uint32)(a)<<(shift)))
#define silk_LSHIFT(a,shift) silk_LSHIFT32(a, shift)
#define silk_RSHIFT32(a,shift) ((a)>>(shift))
#define silk_RSHIFT(a,shift) silk_RSHIFT32(a, shift)
#define silk_ADD_LSHIFT32(a,b,shift) silk_ADD32((a), silk_LSHIFT32((b), (shift)))
#define silk_ADD_RSHIFT32(a,b,shift) silk_ADD32((a), silk_RSHIFT32((b), (shift)))
#define silk_min(a,b) (((a) < (b)) ? (a) : (b))
/* Macro to convert floating-point constants to fixed-point */
#define SILK_FIX_CONST(C,Q) ((opus_int32)((C) * ((opus_int64)1 << (Q)) + 0.5))
/* get number of leading zeros and fractional part (the bits right after the leading one */
static OPUS_INLINE void silk_CLZ_FRAC(
    opus_int32 in,            /* I  input                               */
    opus_int32 *lz,           /* O  number of leading zeros             */
    opus_int32 *frac_Q7       /* O  the 7 bits right after the leading one */
)
{
    opus_int32 lzeros = silk_CLZ32(in);

    * lz = lzeros;
    * frac_Q7 = silk_ROR32(in, 24 - lzeros) & 0x7f;
}
/* DTX settings */
#define NB_SPEECH_FRAMES_BEFORE_DTX 10
#define MAX_CONSECUTIVE_DTX 20
#define DTX_ACTIVITY_THRESHOLD 0.1f
/* VAD decision */
#define VAD_NO_DECISION -1
int opus_packet_get_nb_frames(const unsigned char packet[], opus_int32 len)
{
   int count;
   if (len<1)
      return OPUS_BAD_ARG;
   count = packet[0]&0x3;
   if (count==0)
      return 1;
   else if (count!=3)
      return 2;
   else if (len<2)
      return OPUS_INVALID_PACKET;
   else
      return packet[1]&0x3F;
}
/* Given an extension payload, advance data to the next extension and return the
   length of the remaining extensions. */
opus_int32 skip_extension(const unsigned char **data, opus_int32 len, opus_int32 *header_size)
{
   int id, L;
   if (len==0)
      return 0;
   id = **data>>1;
   L = **data&1;
   if (id == 0 && L == 1)
   {
      *header_size = 1;
      if (len < 1)
         return -1;
      (*data)++;
      len--;
      return len;
   } else if (id > 0 && id < 32)
   {
      if (len < 1+L)
         return -1;
      *data += 1+L;
      len -= 1+L;
      *header_size = 1;
      return len;
   } else {
      if (L==0)
      {
         *data += len;
         *header_size = 1;
         return 0;
      } else {
         opus_int32 bytes=0;
         *header_size = 1;
         do {
            (*data)++;
            len--;
            if (len == 0)
               return -1;
            bytes += **data;
            (*header_size)++;
         } while (**data == 255);
         (*data)++;
         len--;
         if (bytes <= len)
         {
            len -= bytes;
            *data += bytes;
         } else {
            return -1;
         }
         return len;
      }
   }
}
/* Count the number of extensions, excluding real padding and separators. */
opus_int32 opus_packet_extensions_count(const unsigned char *data, opus_int32 len)
{
   opus_int32 curr_len;
   opus_int32 count=0;
   const unsigned char *curr_data = data;

   celt_assert(len >= 0);
   celt_assert(data != NULL || len == 0);

   curr_len = len;
   while (curr_len > 0)
   {
      int id;
      opus_int32 header_size;
      id = *curr_data>>1;
      curr_len = skip_extension(&curr_data, curr_len, &header_size);
      if (curr_len < 0)
         return OPUS_INVALID_PACKET;
      if (id > 1)
         count++;
   }
   return count;
}
/* Extract extensions from Opus padding (excluding real padding and separators) */
opus_int32 opus_packet_extensions_parse(const unsigned char *data, opus_int32 len, opus_extension_data *extensions, opus_int32 *nb_extensions)
{
   const unsigned char *curr_data;
   opus_int32 curr_len;
   int curr_frame=0;
   opus_int32 count=0;

   celt_assert(len >= 0);
   celt_assert(data != NULL || len == 0);
   celt_assert(nb_extensions != NULL);
   celt_assert(extensions != NULL || *nb_extensions == 0);

   curr_data = data;
   curr_len = len;
   while (curr_len > 0)
   {
      int id;
      opus_int32 header_size;
      opus_extension_data curr_ext;
      id = *curr_data>>1;
      if (id > 1)
      {
         curr_ext.id = id;
         curr_ext.frame = curr_frame;
         curr_ext.data = curr_data;
      } else if (id == 1)
      {
         int L = *curr_data&1;
         if (L==0)
            curr_frame++;
         else {
            if (curr_len >= 2)
               curr_frame += curr_data[1];
            /* Else we're at the end and it doesn't matter. */
         }
         if (curr_frame >= 48)
         {
            *nb_extensions = count;
            return OPUS_INVALID_PACKET;
         }
      }
      curr_len = skip_extension(&curr_data, curr_len, &header_size);
      /* printf("curr_len = %d, header_size = %d\n", curr_len, header_size); */
      if (curr_len < 0)
      {
         *nb_extensions = count;
         return OPUS_INVALID_PACKET;
      }
      celt_assert(curr_data - data == len - curr_len);
      if (id > 1)
      {
         if (count == *nb_extensions)
         {
             return OPUS_BUFFER_TOO_SMALL;
         }
         curr_ext.len = curr_data - curr_ext.data - header_size;
         curr_ext.data += header_size;
         extensions[count++] = curr_ext;
      }
   }
   celt_assert(curr_len == 0);
   *nb_extensions = count;
   return OPUS_OK;
}
opus_int32 opus_packet_extensions_generate(unsigned char *data, opus_int32 len, const opus_extension_data  *extensions, opus_int32 nb_extensions, int pad)
{
   int max_frame=0;
   opus_int32 i;
   int frame;
   int curr_frame = 0;
   opus_int32 pos = 0;
   opus_int32 written = 0;

   celt_assert(len >= 0);

   for (i=0;i<nb_extensions;i++)
   {
      max_frame = IMAX(max_frame, extensions[i].frame);
      if (extensions[i].id < 2 || extensions[i].id > 127)
         return OPUS_BAD_ARG;
   }
   if (max_frame >= 48) return OPUS_BAD_ARG;
   for (frame=0;frame<=max_frame;frame++)
   {
      for (i=0;i<nb_extensions;i++)
      {
         if (extensions[i].frame == frame)
         {
            /* Insert separator when needed. */
            if (frame != curr_frame) {
               int diff = frame - curr_frame;
               if (len-pos < 2)
                  return OPUS_BUFFER_TOO_SMALL;
               if (diff == 1) {
                  if (data) data[pos] = 0x02;
                  pos++;
               } else {
                  if (data) data[pos] = 0x03;
                  pos++;
                  if (data) data[pos] = diff;
                  pos++;
               }
               curr_frame = frame;
            }
            if (extensions[i].id < 32)
            {
               if (extensions[i].len < 0 || extensions[i].len > 1)
                  return OPUS_BAD_ARG;
               if (len-pos < extensions[i].len+1)
                  return OPUS_BUFFER_TOO_SMALL;
               if (data) data[pos] = (extensions[i].id<<1) + extensions[i].len;
               pos++;
               if (extensions[i].len > 0) {
                  if (data) data[pos] = extensions[i].data[0];
                  pos++;
               }
            } else {
               int last;
               opus_int32 length_bytes;
               if (extensions[i].len < 0)
                  return OPUS_BAD_ARG;
               last = (written == nb_extensions - 1);
               length_bytes = 1 + extensions[i].len/255;
               if (last)
                  length_bytes = 0;
               if (len-pos < 1 + length_bytes + extensions[i].len)
                  return OPUS_BUFFER_TOO_SMALL;
               if (data) data[pos] = (extensions[i].id<<1) + !last;
               pos++;
               if (!last)
               {
                  opus_int32 j;
                  for (j=0;j<extensions[i].len/255;j++) {
                     if (data) data[pos] = 255;
                     pos++;
                  }
                  if (data) data[pos] = extensions[i].len % 255;
                  pos++;
               }
               if (data) OPUS_COPY(&data[pos], extensions[i].data, extensions[i].len);
               pos += extensions[i].len;
            }
            written++;
         }
      }
   }
   /* If we need to pad, just prepend 0x01 bytes. Even better would be to fill the
      end with zeros, but that requires checking that turning the last extesion into
      an L=1 case still fits. */
   if (pad && pos < len)
   {
      opus_int32 padding = len - pos;
      if (data) {
         OPUS_MOVE(data+padding, data, pos);
         for (i=0;i<padding;i++)
            data[i] = 0x01;
      }
      pos += padding;
   }
   return pos;
}
OpusRepacketizer *opus_repacketizer_init(OpusRepacketizer *rp)
{
   rp->nb_frames = 0;
   return rp;
}
static int opus_repacketizer_cat_impl(OpusRepacketizer *rp, const unsigned char *data, opus_int32 len, int self_delimited)
{
   unsigned char tmp_toc;
   int curr_nb_frames,ret;
   /* Set of check ToC */
   if (len<1) return OPUS_INVALID_PACKET;
   if (rp->nb_frames == 0)
   {
      rp->toc = data[0];
      rp->framesize = opus_packet_get_samples_per_frame(data, 8000);
   } else if ((rp->toc&0xFC) != (data[0]&0xFC))
   {
      /*fprintf(stderr, "toc mismatch: 0x%x vs 0x%x\n", rp->toc, data[0]);*/
      return OPUS_INVALID_PACKET;
   }
   curr_nb_frames = opus_packet_get_nb_frames(data, len);
   if(curr_nb_frames<1) return OPUS_INVALID_PACKET;

   /* Check the 120 ms maximum packet size */
   if ((curr_nb_frames+rp->nb_frames)*rp->framesize > 960)
   {
      return OPUS_INVALID_PACKET;
   }

   ret=opus_packet_parse_impl(data, len, self_delimited, &tmp_toc, &rp->frames[rp->nb_frames], &rp->len[rp->nb_frames],
       NULL, NULL, &rp->paddings[rp->nb_frames], &rp->padding_len[rp->nb_frames]);
   if(ret<1)return ret;

   /* set padding length to zero for all but the first frame */
   while (curr_nb_frames > 1)
   {
      rp->nb_frames++;
      rp->padding_len[rp->nb_frames] = 0;
      rp->paddings[rp->nb_frames] = NULL;
      curr_nb_frames--;
   }
   rp->nb_frames++;
   return OPUS_OK;
}
int opus_repacketizer_cat(OpusRepacketizer *rp, const unsigned char *data, opus_int32 len)
{
   return opus_repacketizer_cat_impl(rp, data, len, 0);
}
opus_int32 opus_repacketizer_out_range_impl(OpusRepacketizer *rp, int begin, int end,
      unsigned char *data, opus_int32 maxlen, int self_delimited, int pad, const opus_extension_data *extensions, int nb_extensions)
{
   int i, count;
   opus_int32 tot_size;
   opus_int16 *len;
   const unsigned char **frames;
   unsigned char * ptr;
   int ones_begin=0, ones_end=0;
   int ext_begin=0, ext_len=0;
   int ext_count, total_ext_count;
   VARDECL(opus_extension_data, all_extensions);
   SAVE_STACK;

   if (begin<0 || begin>=end || end>rp->nb_frames)
   {
      /*fprintf(stderr, "%d %d %d\n", begin, end, rp->nb_frames);*/
      RESTORE_STACK;
      return OPUS_BAD_ARG;
   }
   count = end-begin;

   len = rp->len+begin;
   frames = rp->frames+begin;
   if (self_delimited)
      tot_size = 1 + (len[count-1]>=252);
   else
      tot_size = 0;

   /* figure out total number of extensions */
   total_ext_count = nb_extensions;
   for (i=begin;i<end;i++)
   {
      int n = opus_packet_extensions_count(rp->paddings[i], rp->padding_len[i]);
      if (n > 0) total_ext_count += n;
   }
   ALLOC(all_extensions, total_ext_count ? total_ext_count : ALLOC_NONE, opus_extension_data);
   /* copy over any extensions that were passed in */
   for (ext_count=0;ext_count<nb_extensions;ext_count++)
   {
      all_extensions[ext_count] = extensions[ext_count];
   }

   /* incorporate any extensions from the repacketizer padding */
   for (i=begin;i<end;i++)
   {
      int j;
      opus_int32 frame_ext_count;
      frame_ext_count = total_ext_count - ext_count;
      int ret = opus_packet_extensions_parse(rp->paddings[i], rp->padding_len[i],
         &all_extensions[ext_count], &frame_ext_count);
      if (ret<0)
      {
         RESTORE_STACK;
         return OPUS_INTERNAL_ERROR;
      }
      /* renumber the extension frame numbers */
      for (j=0;j<frame_ext_count;j++)
      {
         all_extensions[ext_count+j].frame += i-begin;
      }
      ext_count += frame_ext_count;
   }

   ptr = data;
   if (count==1)
   {
      /* Code 0 */
      tot_size += len[0]+1;
      if (tot_size > maxlen)
      {
         RESTORE_STACK;
         return OPUS_BUFFER_TOO_SMALL;
      }
      *ptr++ = rp->toc&0xFC;
   } else if (count==2)
   {
      if (len[1] == len[0])
      {
         /* Code 1 */
         tot_size += 2*len[0]+1;
         if (tot_size > maxlen)
         {
            RESTORE_STACK;
            return OPUS_BUFFER_TOO_SMALL;
         }
         *ptr++ = (rp->toc&0xFC) | 0x1;
      } else {
         /* Code 2 */
         tot_size += len[0]+len[1]+2+(len[0]>=252);
         if (tot_size > maxlen)
         {
            RESTORE_STACK;
            return OPUS_BUFFER_TOO_SMALL;
         }
         *ptr++ = (rp->toc&0xFC) | 0x2;
         ptr += encode_size(len[0], ptr);
      }
   }
   if (count > 2 || (pad && tot_size < maxlen) || ext_count > 0)
   {
      /* Code 3 */
      int vbr;
      int pad_amount=0;

      /* Restart the process for the padding case */
      ptr = data;
      if (self_delimited)
         tot_size = 1 + (len[count-1]>=252);
      else
         tot_size = 0;
      vbr = 0;
      for (i=1;i<count;i++)
      {
         if (len[i] != len[0])
         {
            vbr=1;
            break;
         }
      }
      if (vbr)
      {
         tot_size += 2;
         for (i=0;i<count-1;i++)
            tot_size += 1 + (len[i]>=252) + len[i];
         tot_size += len[count-1];

         if (tot_size > maxlen)
         {
            RESTORE_STACK;
            return OPUS_BUFFER_TOO_SMALL;
         }
         *ptr++ = (rp->toc&0xFC) | 0x3;
         *ptr++ = count | 0x80;
      } else {
         tot_size += count*len[0]+2;
         if (tot_size > maxlen)
         {
            RESTORE_STACK;
            return OPUS_BUFFER_TOO_SMALL;
         }
         *ptr++ = (rp->toc&0xFC) | 0x3;
         *ptr++ = count;
      }
      pad_amount = pad ? (maxlen-tot_size) : 0;
      if (ext_count>0)
      {
         /* figure out how much space we need for the extensions */
         ext_len = opus_packet_extensions_generate(NULL, maxlen-tot_size, all_extensions, ext_count, 0);
         if (ext_len < 0) return ext_len;
         if (!pad)
            pad_amount = ext_len + ext_len/254 + 1;
      }
      if (pad_amount != 0)
      {
         int nb_255s;
         data[1] |= 0x40;
         nb_255s = (pad_amount-1)/255;
         if (tot_size + ext_len + nb_255s + 1 > maxlen)
         {
            RESTORE_STACK;
            return OPUS_BUFFER_TOO_SMALL;
         }
         ext_begin = tot_size+pad_amount-ext_len;
         /* Prepend 0x01 padding */
         ones_begin = tot_size+nb_255s+1;
         ones_end = tot_size+pad_amount-ext_len;
         for (i=0;i<nb_255s;i++)
            *ptr++ = 255;
         *ptr++ = pad_amount-255*nb_255s-1;
         tot_size += pad_amount;
      }
      if (vbr)
      {
         for (i=0;i<count-1;i++)
            ptr += encode_size(len[i], ptr);
      }
   }
   if (self_delimited) {
      int sdlen = encode_size(len[count-1], ptr);
      ptr += sdlen;
   }
   /* Copy the actual data */
   for (i=0;i<count;i++)
   {
      /* Using OPUS_MOVE() instead of OPUS_COPY() in case we're doing in-place
         padding from opus_packet_pad or opus_packet_unpad(). */
      /* assert disabled because it's not valid in C. */
      /* celt_assert(frames[i] + len[i] <= data || ptr <= frames[i]); */
      OPUS_MOVE(ptr, frames[i], len[i]);
      ptr += len[i];
   }
   if (ext_len > 0) {
      int ret = opus_packet_extensions_generate(&data[ext_begin], ext_len, all_extensions, ext_count, 0);
      celt_assert(ret == ext_len);
   }
   for (i=ones_begin;i<ones_end;i++)
      data[i] = 0x01;
   if (pad && ext_count==0)
   {
      /* Fill padding with zeros. */
      while (ptr<data+maxlen)
         *ptr++=0;
   }
   RESTORE_STACK;
   return tot_size;
}
opus_int32 opus_packet_pad_impl(unsigned char *data, opus_int32 len, opus_int32 new_len, int pad, const opus_extension_data  *extensions, int nb_extensions)
{
   OpusRepacketizer rp;
   opus_int32 ret;
   VARDECL(unsigned char, copy);
   SAVE_STACK;
   if (len < 1)
      return OPUS_BAD_ARG;
   if (len==new_len)
      return OPUS_OK;
   else if (len > new_len)
      return OPUS_BAD_ARG;
   ALLOC(copy, len, unsigned char);
   opus_repacketizer_init(&rp);
   /* Moving payload to the end of the packet so we can do in-place padding */
   OPUS_COPY(copy, data, len);
   ret = opus_repacketizer_cat(&rp, copy, len);
   if (ret != OPUS_OK)
      return ret;
   ret = opus_repacketizer_out_range_impl(&rp, 0, rp.nb_frames, data, new_len, 0, pad, extensions, nb_extensions);
   RESTORE_STACK;
   return ret;
}
int opus_packet_pad(unsigned char *data, opus_int32 len, opus_int32 new_len)
{
   opus_int32 ret;
   ALLOC_STACK;
   ret = opus_packet_pad_impl(data, len, new_len, 1, NULL, 0);
   RESTORE_STACK;
   if (ret > 0)
      return OPUS_OK;
   else
      return ret;
}
#define VARIABLE_HP_SMTH_COEF2 0.015f
/* Min and max cut-off frequency values (-3 dB points) */
#define VARIABLE_HP_MIN_CUTOFF_HZ 60
#define MAX_ENCODER_BUFFER 480
#define PSEUDO_SNR_THRESHOLD 316.23f
typedef struct {
   opus_val32 XX, XY, YY;
   opus_val16 smoothed_width;
   opus_val16 max_follower;
} StereoWidthState;
struct OpusEncoder {
    int          celt_enc_offset;
    int          silk_enc_offset;
    silk_EncControlStruct silk_mode;
    int          application;
    int          channels;
    int          delay_compensation;
    int          force_channels;
    int          signal_type;
    int          user_bandwidth;
    int          max_bandwidth;
    int          user_forced_mode;
    int          voice_ratio;
    opus_int32   Fs;
    int          use_vbr;
    int          vbr_constraint;
    int          variable_duration;
    opus_int32   bitrate_bps;
    opus_int32   user_bitrate_bps;
    int          lsb_depth;
    int          encoder_buffer;
    int          lfe;
    int          arch;
    int          use_dtx;                 /* general DTX for both SILK and CELT */
    int          fec_config;
    TonalityAnalysisState analysis;

#define OPUS_ENCODER_RESET_START stream_channels
    int          stream_channels;
    opus_int16   hybrid_stereo_width_Q14;
    opus_int32   variable_HP_smth2_Q15;
    opus_val16   prev_HB_gain;
    opus_val32   hp_mem[4];
    int          mode;
    int          prev_mode;
    int          prev_channels;
    int          prev_framesize;
    int          bandwidth;
    /* Bandwidth determined automatically from the rate (before any other adjustment) */
    int          auto_bandwidth;
    int          silk_bw_switch;
    /* Sampling rate (at the API level) */
    int          first;
    opus_val16 * energy_masking;
    StereoWidthState width_mem;
    opus_val16   delay_buffer[MAX_ENCODER_BUFFER*2];
    int          detected_bandwidth;
    int          nb_no_activity_ms_Q1;
    opus_val32   peak_signal_energy;
    int          nonfinal_frame; /* current frame is not the final in a packet */
    opus_uint32  rangeFinal;
};
/* Transition tables for the voice and music. First column is the
   middle (memoriless) threshold. The second column is the hysteresis
   (difference with the middle) */
static const opus_int32 mono_voice_bandwidth_thresholds[8] = {
         9000,  700, /* NB<->MB */
         9000,  700, /* MB<->WB */
        13500, 1000, /* WB<->SWB */
        14000, 2000, /* SWB<->FB */
};
static const opus_int32 mono_music_bandwidth_thresholds[8] = {
         9000,  700, /* NB<->MB */
         9000,  700, /* MB<->WB */
        11000, 1000, /* WB<->SWB */
        12000, 2000, /* SWB<->FB */
};
static const opus_int32 stereo_voice_bandwidth_thresholds[8] = {
         9000,  700, /* NB<->MB */
         9000,  700, /* MB<->WB */
        13500, 1000, /* WB<->SWB */
        14000, 2000, /* SWB<->FB */
};
static const opus_int32 stereo_music_bandwidth_thresholds[8] = {
         9000,  700, /* NB<->MB */
         9000,  700, /* MB<->WB */
        11000, 1000, /* WB<->SWB */
        12000, 2000, /* SWB<->FB */
};
/* Threshold bit-rates for switching between mono and stereo */
static const opus_int32 stereo_voice_threshold = 19000;
static const opus_int32 stereo_music_threshold = 17000;
/* Threshold bit-rate for switching between SILK/hybrid and CELT-only */
static const opus_int32 mode_thresholds[2][2] = {
      /* voice */ /* music */
      {  64000,      10000}, /* mono */
      {  44000,      10000}, /* stereo */
};
static const opus_int32 fec_thresholds[] = {
        12000, 1000, /* NB */
        14000, 1000, /* MB */
        16000, 1000, /* WB */
        20000, 1000, /* SWB */
        22000, 1000, /* FB */
};
int opus_encoder_get_size(int channels)
{
    int silkEncSizeBytes, celtEncSizeBytes;
    int ret;
    if (channels<1 || channels > 2)
        return 0;
    ret = silk_Get_Encoder_Size( &silkEncSizeBytes );
    if (ret)
        return 0;
    silkEncSizeBytes = align(silkEncSizeBytes);
    celtEncSizeBytes = celt_encoder_get_size(channels);
    return align(sizeof(OpusEncoder))+silkEncSizeBytes+celtEncSizeBytes;
}
int opus_encoder_init(OpusEncoder* st, opus_int32 Fs, int channels, int application)
{
    void *silk_enc;
    CELTEncoder *celt_enc;
    int err;
    int ret, silkEncSizeBytes;

   if((Fs!=48000&&Fs!=24000&&Fs!=16000&&Fs!=12000&&Fs!=8000)||(channels!=1&&channels!=2)||
        (application != OPUS_APPLICATION_VOIP && application != OPUS_APPLICATION_AUDIO
        && application != OPUS_APPLICATION_RESTRICTED_LOWDELAY))
        return OPUS_BAD_ARG;

    OPUS_CLEAR((char*)st, opus_encoder_get_size(channels));
    /* Create SILK encoder */
    ret = silk_Get_Encoder_Size( &silkEncSizeBytes );
    if (ret)
        return OPUS_BAD_ARG;
    silkEncSizeBytes = align(silkEncSizeBytes);
    st->silk_enc_offset = align(sizeof(OpusEncoder));
    st->celt_enc_offset = st->silk_enc_offset+silkEncSizeBytes;
    silk_enc = (char*)st+st->silk_enc_offset;
    celt_enc = (CELTEncoder*)((char*)st+st->celt_enc_offset);

    st->stream_channels = st->channels = channels;

    st->Fs = Fs;

    st->arch = opus_select_arch();

    ret = silk_InitEncoder( silk_enc, st->arch, &st->silk_mode );
    if(ret)return OPUS_INTERNAL_ERROR;

    /* default SILK parameters */
    st->silk_mode.nChannelsAPI              = channels;
    st->silk_mode.nChannelsInternal         = channels;
    st->silk_mode.API_sampleRate            = st->Fs;
    st->silk_mode.maxInternalSampleRate     = 16000;
    st->silk_mode.minInternalSampleRate     = 8000;
    st->silk_mode.desiredInternalSampleRate = 16000;
    st->silk_mode.payloadSize_ms            = 20;
    st->silk_mode.bitRate                   = 25000;
    st->silk_mode.packetLossPercentage      = 0;
    st->silk_mode.complexity                = 9;
    st->silk_mode.useInBandFEC              = 0;
    st->silk_mode.useDRED                   = 0;
    st->silk_mode.useDTX                    = 0;
    st->silk_mode.useCBR                    = 0;
    st->silk_mode.reducedDependency         = 0;

    /* Create CELT encoder */
    /* Initialize CELT encoder */
    err = celt_encoder_init(celt_enc, Fs, channels, st->arch);
    if(err!=OPUS_OK)return OPUS_INTERNAL_ERROR;

    celt_encoder_ctl(celt_enc, CELT_SET_SIGNALLING(0));
    celt_encoder_ctl(celt_enc, OPUS_SET_COMPLEXITY(st->silk_mode.complexity));

    st->use_vbr = 1;
    /* Makes constrained VBR the default (safer for real-time use) */
    st->vbr_constraint = 1;
    st->user_bitrate_bps = OPUS_AUTO;
    st->bitrate_bps = 3000+Fs*channels;
    st->application = application;
    st->signal_type = OPUS_AUTO;
    st->user_bandwidth = OPUS_AUTO;
    st->max_bandwidth = OPUS_BANDWIDTH_FULLBAND;
    st->force_channels = OPUS_AUTO;
    st->user_forced_mode = OPUS_AUTO;
    st->voice_ratio = -1;
    st->encoder_buffer = st->Fs/100;
    st->lsb_depth = 24;
    st->variable_duration = OPUS_FRAMESIZE_ARG;

    /* Delay compensation of 4 ms (2.5 ms for SILK's extra look-ahead
       + 1.5 ms for SILK resamplers and stereo prediction) */
    st->delay_compensation = st->Fs/250;

    st->hybrid_stereo_width_Q14 = 1 << 14;
    st->prev_HB_gain = Q15ONE;
    st->variable_HP_smth2_Q15 = silk_LSHIFT( silk_lin2log( VARIABLE_HP_MIN_CUTOFF_HZ ), 8 );
    st->first = 1;
    st->mode = MODE_HYBRID;
    st->bandwidth = OPUS_BANDWIDTH_FULLBAND;

    tonality_analysis_init(&st->analysis, st->Fs);
    st->analysis.application = st->application;

    return OPUS_OK;
}
static unsigned char gen_toc(int mode, int framerate, int bandwidth, int channels)
{
   int period;
   unsigned char toc;
   period = 0;
   while (framerate < 400)
   {
       framerate <<= 1;
       period++;
   }
   if (mode == MODE_SILK_ONLY)
   {
       toc = (bandwidth-OPUS_BANDWIDTH_NARROWBAND)<<5;
       toc |= (period-2)<<3;
   } else if (mode == MODE_CELT_ONLY)
   {
       int tmp = bandwidth-OPUS_BANDWIDTH_MEDIUMBAND;
       if (tmp < 0)
           tmp = 0;
       toc = 0x80;
       toc |= tmp << 5;
       toc |= period<<3;
   } else /* Hybrid */
   {
       toc = 0x60;
       toc |= (bandwidth-OPUS_BANDWIDTH_SUPERWIDEBAND)<<4;
       toc |= (period-2)<<3;
   }
   toc |= (channels==2)<<2;
   return toc;
}
static void silk_biquad_float(
    const opus_val16      *in,            /* I:    Input signal                   */
    const opus_int32      *B_Q28,         /* I:    MA coefficients [3]            */
    const opus_int32      *A_Q28,         /* I:    AR coefficients [2]            */
    opus_val32            *S,             /* I/O:  State vector [2]               */
    opus_val16            *out,           /* O:    Output signal                  */
    const opus_int32      len,            /* I:    Signal length (must be even)   */
    int stride
)
{
    /* DIRECT FORM II TRANSPOSED (uses 2 element state vector) */
    opus_int   k;
    opus_val32 vout;
    opus_val32 inval;
    opus_val32 A[2], B[3];

    A[0] = (opus_val32)(A_Q28[0] * (1.f/((opus_int32)1<<28)));
    A[1] = (opus_val32)(A_Q28[1] * (1.f/((opus_int32)1<<28)));
    B[0] = (opus_val32)(B_Q28[0] * (1.f/((opus_int32)1<<28)));
    B[1] = (opus_val32)(B_Q28[1] * (1.f/((opus_int32)1<<28)));
    B[2] = (opus_val32)(B_Q28[2] * (1.f/((opus_int32)1<<28)));

    /* Negate A_Q28 values and split in two parts */

    for( k = 0; k < len; k++ ) {
        /* S[ 0 ], S[ 1 ]: Q12 */
        inval = in[ k*stride ];
        vout = S[ 0 ] + B[0]*inval;

        S[ 0 ] = S[1] - vout*A[0] + B[1]*inval;

        S[ 1 ] = - vout*A[1] + B[2]*inval + VERY_SMALL;

        /* Scale back to Q0 and saturate */
        out[ k*stride ] = vout;
    }
}
static void hp_cutoff(const opus_val16 *in, opus_int32 cutoff_Hz, opus_val16 *out, opus_val32 *hp_mem, int len, int channels, opus_int32 Fs, int arch)
{
   opus_int32 B_Q28[ 3 ], A_Q28[ 2 ];
   opus_int32 Fc_Q19, r_Q28, r_Q22;
   (void)arch;

   silk_assert( cutoff_Hz <= silk_int32_MAX / SILK_FIX_CONST( 1.5 * 3.14159 / 1000, 19 ) );
   Fc_Q19 = silk_DIV32_16( silk_SMULBB( SILK_FIX_CONST( 1.5 * 3.14159 / 1000, 19 ), cutoff_Hz ), Fs/1000 );
   silk_assert( Fc_Q19 > 0 && Fc_Q19 < 32768 );

   r_Q28 = SILK_FIX_CONST( 1.0, 28 ) - silk_MUL( SILK_FIX_CONST( 0.92, 9 ), Fc_Q19 );

   /* b = r * [ 1; -2; 1 ]; */
   /* a = [ 1; -2 * r * ( 1 - 0.5 * Fc^2 ); r^2 ]; */
   B_Q28[ 0 ] = r_Q28;
   B_Q28[ 1 ] = silk_LSHIFT( -r_Q28, 1 );
   B_Q28[ 2 ] = r_Q28;

   /* -r * ( 2 - Fc * Fc ); */
   r_Q22  = silk_RSHIFT( r_Q28, 6 );
   A_Q28[ 0 ] = silk_SMULWW( r_Q22, silk_SMULWW( Fc_Q19, Fc_Q19 ) - SILK_FIX_CONST( 2.0,  22 ) );
   A_Q28[ 1 ] = silk_SMULWW( r_Q22, r_Q22 );

   silk_biquad_float( in, B_Q28, A_Q28, hp_mem, out, len, channels );
   if( channels == 2 ) {
       silk_biquad_float( in+1, B_Q28, A_Q28, hp_mem+2, out+1, len, channels );
   }
}
static void dc_reject(const opus_val16 *in, opus_int32 cutoff_Hz, opus_val16 *out, opus_val32 *hp_mem, int len, int channels, opus_int32 Fs)
{
   int i;
   float coef, coef2;
   coef = 6.3f*cutoff_Hz/Fs;
   coef2 = 1-coef;
   if (channels==2)
   {
      float m0, m2;
      m0 = hp_mem[0];
      m2 = hp_mem[2];
      for (i=0;i<len;i++)
      {
         opus_val32 x0, x1, out0, out1;
         x0 = in[2*i+0];
         x1 = in[2*i+1];
         out0 = x0-m0;
         out1 = x1-m2;
         m0 = coef*x0 + VERY_SMALL + coef2*m0;
         m2 = coef*x1 + VERY_SMALL + coef2*m2;
         out[2*i+0] = out0;
         out[2*i+1] = out1;
      }
      hp_mem[0] = m0;
      hp_mem[2] = m2;
   } else {
      float m0;
      m0 = hp_mem[0];
      for (i=0;i<len;i++)
      {
         opus_val32 x, y;
         x = in[i];
         y = x-m0;
         m0 = coef*x + VERY_SMALL + coef2*m0;
         out[i] = y;
      }
      hp_mem[0] = m0;
   }
}
static void stereo_fade(const opus_val16 *in, opus_val16 *out, opus_val16 g1, opus_val16 g2,
        int overlap48, int frame_size, int channels, const opus_val16 *window, opus_int32 Fs)
{
    int i;
    int overlap;
    int inc;
    inc = 48000/Fs;
    overlap=overlap48/inc;
    g1 = Q15ONE-g1;
    g2 = Q15ONE-g2;
    for (i=0;i<overlap;i++)
    {
       opus_val32 diff;
       opus_val16 g, w;
       w = MULT16_16_Q15(window[i*inc], window[i*inc]);
       g = SHR32(MAC16_16(MULT16_16(w,g2),
             Q15ONE-w, g1), 15);
       diff = EXTRACT16(HALF32((opus_val32)in[i*channels] - (opus_val32)in[i*channels+1]));
       diff = MULT16_16_Q15(g, diff);
       out[i*channels] = out[i*channels] - diff;
       out[i*channels+1] = out[i*channels+1] + diff;
    }
    for (;i<frame_size;i++)
    {
       opus_val32 diff;
       diff = EXTRACT16(HALF32((opus_val32)in[i*channels] - (opus_val32)in[i*channels+1]));
       diff = MULT16_16_Q15(g2, diff);
       out[i*channels] = out[i*channels] - diff;
       out[i*channels+1] = out[i*channels+1] + diff;
    }
}
static void gain_fade(const opus_val16 *in, opus_val16 *out, opus_val16 g1, opus_val16 g2,
        int overlap48, int frame_size, int channels, const opus_val16 *window, opus_int32 Fs)
{
    int i;
    int inc;
    int overlap;
    int c;
    inc = 48000/Fs;
    overlap=overlap48/inc;
    if (channels==1)
    {
       for (i=0;i<overlap;i++)
       {
          opus_val16 g, w;
          w = MULT16_16_Q15(window[i*inc], window[i*inc]);
          g = SHR32(MAC16_16(MULT16_16(w,g2),
                Q15ONE-w, g1), 15);
          out[i] = MULT16_16_Q15(g, in[i]);
       }
    } else {
       for (i=0;i<overlap;i++)
       {
          opus_val16 g, w;
          w = MULT16_16_Q15(window[i*inc], window[i*inc]);
          g = SHR32(MAC16_16(MULT16_16(w,g2),
                Q15ONE-w, g1), 15);
          out[i*2] = MULT16_16_Q15(g, in[i*2]);
          out[i*2+1] = MULT16_16_Q15(g, in[i*2+1]);
       }
    }
    c=0;do {
       for (i=overlap;i<frame_size;i++)
       {
          out[i*channels+c] = MULT16_16_Q15(g2, in[i*channels+c]);
       }
    }
    while (++c<channels);
}
OpusEncoder *opus_encoder_create(opus_int32 Fs, int channels, int application, int *error)
{
   int ret;
   OpusEncoder *st;
   if((Fs!=48000&&Fs!=24000&&Fs!=16000&&Fs!=12000&&Fs!=8000)||(channels!=1&&channels!=2)||
       (application != OPUS_APPLICATION_VOIP && application != OPUS_APPLICATION_AUDIO
       && application != OPUS_APPLICATION_RESTRICTED_LOWDELAY))
   {
      if (error)
         *error = OPUS_BAD_ARG;
      return NULL;
   }
   st = (OpusEncoder *)opus_alloc(opus_encoder_get_size(channels));
   if (st == NULL)
   {
      if (error)
         *error = OPUS_ALLOC_FAIL;
      return NULL;
   }
   ret = opus_encoder_init(st, Fs, channels, application);
   if (error)
      *error = ret;
   if (ret != OPUS_OK)
   {
      opus_free(st);
      st = NULL;
   }
   return st;
}
static opus_int32 user_bitrate_to_bitrate(OpusEncoder *st, int frame_size, int max_data_bytes)
{
  if(!frame_size)frame_size=st->Fs/400;
  if (st->user_bitrate_bps==OPUS_AUTO)
    return 60*st->Fs/frame_size + st->Fs*st->channels;
  else if (st->user_bitrate_bps==OPUS_BITRATE_MAX)
    return max_data_bytes*8*st->Fs/frame_size;
  else
    return st->user_bitrate_bps;
}
void downmix_int(const void *_x, opus_val32 *y, int subframe, int offset, int c1, int c2, int C)
{
   const opus_int16 *x;
   int j;

   x = (const opus_int16 *)_x;
   for (j=0;j<subframe;j++)
      y[j] = x[(j+offset)*C+c1];
   if (c2>-1)
   {
      for (j=0;j<subframe;j++)
         y[j] += x[(j+offset)*C+c2];
   } else if (c2==-2)
   {
      int c;
      for (c=1;c<C;c++)
      {
         for (j=0;j<subframe;j++)
            y[j] += x[(j+offset)*C+c];
      }
   }
}
opus_int32 frame_size_select(opus_int32 frame_size, int variable_duration, opus_int32 Fs)
{
   int new_size;
   if (frame_size<Fs/400)
      return -1;
   if (variable_duration == OPUS_FRAMESIZE_ARG)
      new_size = frame_size;
   else if (variable_duration >= OPUS_FRAMESIZE_2_5_MS && variable_duration <= OPUS_FRAMESIZE_120_MS)
   {
      if (variable_duration <= OPUS_FRAMESIZE_40_MS)
         new_size = (Fs/400)<<(variable_duration-OPUS_FRAMESIZE_2_5_MS);
      else
         new_size = (variable_duration-OPUS_FRAMESIZE_2_5_MS-2)*Fs/50;
   }
   else
      return -1;
   if (new_size>frame_size)
      return -1;
   if (400*new_size!=Fs   && 200*new_size!=Fs   && 100*new_size!=Fs   &&
        50*new_size!=Fs   &&  25*new_size!=Fs   &&  50*new_size!=3*Fs &&
        50*new_size!=4*Fs &&  50*new_size!=5*Fs &&  50*new_size!=6*Fs)
      return -1;
   return new_size;
}
opus_val16 compute_stereo_width(const opus_val16 *pcm, int frame_size, opus_int32 Fs, StereoWidthState *mem)
{
   opus_val32 xx, xy, yy;
   opus_val16 sqrt_xx, sqrt_yy;
   opus_val16 qrrt_xx, qrrt_yy;
   int frame_rate;
   int i;
   opus_val16 short_alpha;

   frame_rate = Fs/frame_size;
   short_alpha = Q15ONE - MULT16_16(25, Q15ONE)/IMAX(50,frame_rate);
   xx=xy=yy=0;
   /* Unroll by 4. The frame size is always a multiple of 4 *except* for
      2.5 ms frames at 12 kHz. Since this setting is very rare (and very
      stupid), we just discard the last two samples. */
   for (i=0;i<frame_size-3;i+=4)
   {
      opus_val32 pxx=0;
      opus_val32 pxy=0;
      opus_val32 pyy=0;
      opus_val16 x, y;
      x = pcm[2*i];
      y = pcm[2*i+1];
      pxx = SHR32(MULT16_16(x,x),2);
      pxy = SHR32(MULT16_16(x,y),2);
      pyy = SHR32(MULT16_16(y,y),2);
      x = pcm[2*i+2];
      y = pcm[2*i+3];
      pxx += SHR32(MULT16_16(x,x),2);
      pxy += SHR32(MULT16_16(x,y),2);
      pyy += SHR32(MULT16_16(y,y),2);
      x = pcm[2*i+4];
      y = pcm[2*i+5];
      pxx += SHR32(MULT16_16(x,x),2);
      pxy += SHR32(MULT16_16(x,y),2);
      pyy += SHR32(MULT16_16(y,y),2);
      x = pcm[2*i+6];
      y = pcm[2*i+7];
      pxx += SHR32(MULT16_16(x,x),2);
      pxy += SHR32(MULT16_16(x,y),2);
      pyy += SHR32(MULT16_16(y,y),2);

      xx += SHR32(pxx, 10);
      xy += SHR32(pxy, 10);
      yy += SHR32(pyy, 10);
   }
   if (!(xx < 1e9f) || celt_isnan(xx) || !(yy < 1e9f) || celt_isnan(yy))
   {
      xy = xx = yy = 0;
   }
   mem->XX += MULT16_32_Q15(short_alpha, xx-mem->XX);
   mem->XY += MULT16_32_Q15(short_alpha, xy-mem->XY);
   mem->YY += MULT16_32_Q15(short_alpha, yy-mem->YY);
   mem->XX = MAX32(0, mem->XX);
   mem->XY = MAX32(0, mem->XY);
   mem->YY = MAX32(0, mem->YY);
   if (MAX32(mem->XX, mem->YY)>QCONST16(8e-4f, 18))
   {
      opus_val16 corr;
      opus_val16 ldiff;
      opus_val16 width;
      sqrt_xx = celt_sqrt(mem->XX);
      sqrt_yy = celt_sqrt(mem->YY);
      qrrt_xx = celt_sqrt(sqrt_xx);
      qrrt_yy = celt_sqrt(sqrt_yy);
      /* Inter-channel correlation */
      mem->XY = MIN32(mem->XY, sqrt_xx*sqrt_yy);
      corr = SHR32(frac_div32(mem->XY,EPSILON+MULT16_16(sqrt_xx,sqrt_yy)),16);
      /* Approximate loudness difference */
      ldiff = MULT16_16(Q15ONE, ABS16(qrrt_xx-qrrt_yy))/(EPSILON+qrrt_xx+qrrt_yy);
      width = MULT16_16_Q15(celt_sqrt(QCONST32(1.f,30)-MULT16_16(corr,corr)), ldiff);
      /* Smoothing over one second */
      mem->smoothed_width += (width-mem->smoothed_width)/frame_rate;
      /* Peak follower */
      mem->max_follower = MAX16(mem->max_follower-QCONST16(.02f,15)/frame_rate, mem->smoothed_width);
   }
   /*printf("%f %f %f %f %f ", corr/(float)Q15ONE, ldiff/(float)Q15ONE, width/(float)Q15ONE, mem->smoothed_width/(float)Q15ONE, mem->max_follower/(float)Q15ONE);*/
   return EXTRACT16(MIN32(Q15ONE, MULT16_16(20, mem->max_follower)));
}
static int decide_fec(int useInBandFEC, int PacketLoss_perc, int last_fec, int mode, int *bandwidth, opus_int32 rate)
{
   int orig_bandwidth;
   if (!useInBandFEC || PacketLoss_perc == 0 || mode == MODE_CELT_ONLY)
      return 0;
   orig_bandwidth = *bandwidth;
   for (;;)
   {
      opus_int32 hysteresis;
      opus_int32 LBRR_rate_thres_bps;
      /* Compute threshold for using FEC at the current bandwidth setting */
      LBRR_rate_thres_bps = fec_thresholds[2*(*bandwidth - OPUS_BANDWIDTH_NARROWBAND)];
      hysteresis = fec_thresholds[2*(*bandwidth - OPUS_BANDWIDTH_NARROWBAND) + 1];
      if (last_fec == 1) LBRR_rate_thres_bps -= hysteresis;
      if (last_fec == 0) LBRR_rate_thres_bps += hysteresis;
      LBRR_rate_thres_bps = silk_SMULWB( silk_MUL( LBRR_rate_thres_bps,
            125 - silk_min( PacketLoss_perc, 25 ) ), SILK_FIX_CONST( 0.01, 16 ) );
      /* If loss <= 5%, we look at whether we have enough rate to enable FEC.
         If loss > 5%, we decrease the bandwidth until we can enable FEC. */
      if (rate > LBRR_rate_thres_bps)
         return 1;
      else if (PacketLoss_perc <= 5)
         return 0;
      else if (*bandwidth > OPUS_BANDWIDTH_NARROWBAND)
         (*bandwidth)--;
      else
         break;
   }
   /* Couldn't find any bandwidth to enable FEC, keep original bandwidth. */
   *bandwidth = orig_bandwidth;
   return 0;
}
static int compute_silk_rate_for_hybrid(int rate, int bandwidth, int frame20ms, int vbr, int fec, int channels) {
   int entry;
   int i;
   int N;
   int silk_rate;
   static int rate_table[][5] = {
  /*  |total| |-------- SILK------------|
              |-- No FEC -| |--- FEC ---|
               10ms   20ms   10ms   20ms */
      {    0,     0,     0,     0,     0},
      {12000, 10000, 10000, 11000, 11000},
      {16000, 13500, 13500, 15000, 15000},
      {20000, 16000, 16000, 18000, 18000},
      {24000, 18000, 18000, 21000, 21000},
      {32000, 22000, 22000, 28000, 28000},
      {64000, 38000, 38000, 50000, 50000}
   };
   /* Do the allocation per-channel. */
   rate /= channels;
   entry = 1 + frame20ms + 2*fec;
   N = sizeof(rate_table)/sizeof(rate_table[0]);
   for (i=1;i<N;i++)
   {
      if (rate_table[i][0] > rate) break;
   }
   if (i == N)
   {
      silk_rate = rate_table[i-1][entry];
      /* For now, just give 50% of the extra bits to SILK. */
      silk_rate += (rate-rate_table[i-1][0])/2;
   } else {
      opus_int32 lo, hi, x0, x1;
      lo = rate_table[i-1][entry];
      hi = rate_table[i][entry];
      x0 = rate_table[i-1][0];
      x1 = rate_table[i][0];
      silk_rate = (lo*(x1-rate) + hi*(rate-x0))/(x1-x0);
   }
   if (!vbr)
   {
      /* Tiny boost to SILK for CBR. We should probably tune this better. */
      silk_rate += 100;
   }
   if (bandwidth==OPUS_BANDWIDTH_SUPERWIDEBAND)
      silk_rate += 300;
   silk_rate *= channels;
   /* Small adjustment for stereo (calibrated for 32 kb/s, haven't tried other bitrates). */
   if (channels == 2 && rate >= 12000)
      silk_rate -= 1000;
   return silk_rate;
}
/* Returns the equivalent bitrate corresponding to 20 ms frames,
   complexity 10 VBR operation. */
static opus_int32 compute_equiv_rate(opus_int32 bitrate, int channels,
      int frame_rate, int vbr, int mode, int complexity, int loss)
{
   opus_int32 equiv;
   equiv = bitrate;
   /* Take into account overhead from smaller frames. */
   if (frame_rate > 50)
      equiv -= (40*channels+20)*(frame_rate - 50);
   /* CBR is about a 8% penalty for both SILK and CELT. */
   if (!vbr)
      equiv -= equiv/12;
   /* Complexity makes about 10% difference (from 0 to 10) in general. */
   equiv = equiv * (90+complexity)/100;
   if (mode == MODE_SILK_ONLY || mode == MODE_HYBRID)
   {
      /* SILK complexity 0-1 uses the non-delayed-decision NSQ, which
         costs about 20%. */
      if (complexity<2)
         equiv = equiv*4/5;
      equiv -= equiv*loss/(6*loss + 10);
   } else if (mode == MODE_CELT_ONLY) {
      /* CELT complexity 0-4 doesn't have the pitch filter, which costs
         about 10%. */
      if (complexity<5)
         equiv = equiv*9/10;
   } else {
      /* Mode not known yet */
      /* Half the SILK loss*/
      equiv -= equiv*loss/(12*loss + 20);
   }
   return equiv;
}
int is_digital_silence(const opus_val16* pcm, int frame_size, int channels, int lsb_depth)
{
   int silence = 0;
   opus_val32 sample_max = 0;
   sample_max = celt_maxabs16(pcm, frame_size*channels);

   silence = (sample_max <= (opus_val16) 1 / (1 << lsb_depth));

   return silence;
}
static opus_val32 compute_frame_energy(const opus_val16 *pcm, int frame_size, int channels, int arch)
{
   int len = frame_size*channels;
   return celt_inner_prod(pcm, pcm, len, arch)/len;
}
/* Decides if DTX should be turned on (=1) or off (=0) */
static int decide_dtx_mode(opus_int activity,            /* indicates if this frame contains speech/music */
                           int *nb_no_activity_ms_Q1,    /* number of consecutive milliseconds with no activity, in Q1 */
                           int frame_size_ms_Q1          /* number of miliseconds in this update, in Q1 */
                           )

{
   if (!activity)
   {
      /* The number of consecutive DTX frames should be within the allowed bounds.
         Note that the allowed bound is defined in the SILK headers and assumes 20 ms
         frames. As this function can be called with any frame length, a conversion to
         milliseconds is done before the comparisons. */
      (*nb_no_activity_ms_Q1) += frame_size_ms_Q1;
      if (*nb_no_activity_ms_Q1 > NB_SPEECH_FRAMES_BEFORE_DTX*20*2)
      {
         if (*nb_no_activity_ms_Q1 <= (NB_SPEECH_FRAMES_BEFORE_DTX + MAX_CONSECUTIVE_DTX)*20*2)
            /* Valid frame for DTX! */
            return 1;
         else
            (*nb_no_activity_ms_Q1) = NB_SPEECH_FRAMES_BEFORE_DTX*20*2;
      }
   } else
      (*nb_no_activity_ms_Q1) = 0;

   return 0;
}
static int compute_redundancy_bytes(opus_int32 max_data_bytes, opus_int32 bitrate_bps, int frame_rate, int channels)
{
   int redundancy_bytes_cap;
   int redundancy_bytes;
   opus_int32 redundancy_rate;
   int base_bits;
   opus_int32 available_bits;
   base_bits = (40*channels+20);

   /* Equivalent rate for 5 ms frames. */
   redundancy_rate = bitrate_bps + base_bits*(200 - frame_rate);
   /* For VBR, further increase the bitrate if we can afford it. It's pretty short
      and we'll avoid artefacts. */
   redundancy_rate = 3*redundancy_rate/2;
   redundancy_bytes = redundancy_rate/1600;

   /* Compute the max rate we can use given CBR or VBR with cap. */
   available_bits = max_data_bytes*8 - 2*base_bits;
   redundancy_bytes_cap = (available_bits*240/(240+48000/frame_rate) + base_bits)/8;
   redundancy_bytes = IMIN(redundancy_bytes, redundancy_bytes_cap);
   /* It we can't get enough bits for redundancy to be worth it, rely on the decoder PLC. */
   if (redundancy_bytes > 4 + 8*channels)
      redundancy_bytes = IMIN(257, redundancy_bytes);
   else
      redundancy_bytes = 0;
   return redundancy_bytes;
}
static opus_int32 opus_encode_frame_native(OpusEncoder *st, const opus_val16 *pcm, int frame_size,
                unsigned char *data, opus_int32 max_data_bytes,
                int float_api, int first_frame,
                AnalysisInfo *analysis_info, int is_silence,
                int redundancy, int celt_to_silk, int prefill,
                opus_int32 equiv_rate, int to_celt);
opus_int32 opus_encode_native(OpusEncoder *st, const opus_val16 *pcm, int frame_size,
                unsigned char *data, opus_int32 out_data_bytes, int lsb_depth,
                const void *analysis_pcm, opus_int32 analysis_size, int c1, int c2,
                int analysis_channels, downmix_func downmix, int float_api)
{
    void *silk_enc;
    CELTEncoder *celt_enc;
    int i;
    int ret=0;
    int prefill=0;
    int redundancy = 0;
    int celt_to_silk = 0;
    int to_celt = 0;
    int voice_est; /* Probability of voice in Q7 */
    opus_int32 equiv_rate;
    int frame_rate;
    opus_int32 max_rate; /* Max bitrate we're allowed to use */
    int curr_bandwidth;
    opus_int32 max_data_bytes; /* Max number of bytes we're allowed to use */
    opus_int32 cbr_bytes=-1;
    opus_val16 stereo_width;
    const CELTMode *celt_mode;
    AnalysisInfo analysis_info;
    int analysis_read_pos_bak=-1;
    int analysis_read_subframe_bak=-1;
    int is_silence = 0;
    ALLOC_STACK;

    max_data_bytes = IMIN(1276, out_data_bytes);

    st->rangeFinal = 0;
    if (frame_size <= 0 || max_data_bytes <= 0)
    {
       RESTORE_STACK;
       return OPUS_BAD_ARG;
    }

    /* Cannot encode 100 ms in 1 byte */
    if (max_data_bytes==1 && st->Fs==(frame_size*10))
    {
      RESTORE_STACK;
      return OPUS_BUFFER_TOO_SMALL;
    }

    silk_enc = (char*)st+st->silk_enc_offset;
    celt_enc = (CELTEncoder*)((char*)st+st->celt_enc_offset);

    lsb_depth = IMIN(lsb_depth, st->lsb_depth);

    celt_encoder_ctl(celt_enc, CELT_GET_MODE(&celt_mode));
    analysis_info.valid = 0;
    if (st->silk_mode.complexity >= 7 && st->Fs>=16000)
    {
       is_silence = is_digital_silence(pcm, frame_size, st->channels, lsb_depth);
       analysis_read_pos_bak = st->analysis.read_pos;
       analysis_read_subframe_bak = st->analysis.read_subframe;
       run_analysis(&st->analysis, celt_mode, analysis_pcm, analysis_size, frame_size,
             c1, c2, analysis_channels, st->Fs,
             lsb_depth, downmix, &analysis_info);

       /* Track the peak signal energy */
       if (!is_silence && analysis_info.activity_probability > DTX_ACTIVITY_THRESHOLD)
          st->peak_signal_energy = MAX32(MULT16_32_Q15(QCONST16(0.999f, 15), st->peak_signal_energy),
                compute_frame_energy(pcm, frame_size, st->channels, st->arch));
    } else if (st->analysis.initialized) {
       tonality_analysis_reset(&st->analysis);
    }

    /* Reset voice_ratio if this frame is not silent or if analysis is disabled.
     * Otherwise, preserve voice_ratio from the last non-silent frame */
    if (!is_silence)
      st->voice_ratio = -1;

    st->detected_bandwidth = 0;
    if (analysis_info.valid)
    {
       int analysis_bandwidth;
       if (st->signal_type == OPUS_AUTO)
       {
          float prob;
          if (st->prev_mode == 0)
             prob = analysis_info.music_prob;
          else if (st->prev_mode == MODE_CELT_ONLY)
             prob = analysis_info.music_prob_max;
          else
             prob = analysis_info.music_prob_min;
          st->voice_ratio = (int)floor(.5+100*(1-prob));
       }

       analysis_bandwidth = analysis_info.bandwidth;
       if (analysis_bandwidth<=12)
          st->detected_bandwidth = OPUS_BANDWIDTH_NARROWBAND;
       else if (analysis_bandwidth<=14)
          st->detected_bandwidth = OPUS_BANDWIDTH_MEDIUMBAND;
       else if (analysis_bandwidth<=16)
          st->detected_bandwidth = OPUS_BANDWIDTH_WIDEBAND;
       else if (analysis_bandwidth<=18)
          st->detected_bandwidth = OPUS_BANDWIDTH_SUPERWIDEBAND;
       else
          st->detected_bandwidth = OPUS_BANDWIDTH_FULLBAND;
    }

    if (st->channels==2 && st->force_channels!=1)
       stereo_width = compute_stereo_width(pcm, frame_size, st->Fs, &st->width_mem);
    else
       stereo_width = 0;
    st->bitrate_bps = user_bitrate_to_bitrate(st, frame_size, max_data_bytes);

    frame_rate = st->Fs/frame_size;
    if (!st->use_vbr)
    {
       /* Multiply by 12 to make sure the division is exact. */
       int frame_rate12 = 12*st->Fs/frame_size;
       /* We need to make sure that "int" values always fit in 16 bits. */
       cbr_bytes = IMIN( (12*st->bitrate_bps/8 + frame_rate12/2)/frame_rate12, max_data_bytes);
       st->bitrate_bps = cbr_bytes*(opus_int32)frame_rate12*8/12;
       /* Make sure we provide at least one byte to avoid failing. */
       max_data_bytes = IMAX(1, cbr_bytes);
    }
    if (max_data_bytes<3 || st->bitrate_bps < 3*frame_rate*8
       || (frame_rate<50 && (max_data_bytes*frame_rate<300 || st->bitrate_bps < 2400)))
    {
       /*If the space is too low to do something useful, emit 'PLC' frames.*/
       int tocmode = st->mode;
       int bw = st->bandwidth == 0 ? OPUS_BANDWIDTH_NARROWBAND : st->bandwidth;
       int packet_code = 0;
       int num_multiframes = 0;

       if (tocmode==0)
          tocmode = MODE_SILK_ONLY;
       if (frame_rate>100)
          tocmode = MODE_CELT_ONLY;
       /* 40 ms -> 2 x 20 ms if in CELT_ONLY or HYBRID mode */
       if (frame_rate==25 && tocmode!=MODE_SILK_ONLY)
       {
          frame_rate = 50;
          packet_code = 1;
       }

       /* >= 60 ms frames */
       if (frame_rate<=16)
       {
          /* 1 x 60 ms, 2 x 40 ms, 2 x 60 ms */
          if (out_data_bytes==1 || (tocmode==MODE_SILK_ONLY && frame_rate!=10))
          {
             tocmode = MODE_SILK_ONLY;

             packet_code = frame_rate <= 12;
             frame_rate = frame_rate == 12 ? 25 : 16;
          }
          else
          {
             num_multiframes = 50/frame_rate;
             frame_rate = 50;
             packet_code = 3;
          }
       }

       if(tocmode==MODE_SILK_ONLY&&bw>OPUS_BANDWIDTH_WIDEBAND)
          bw=OPUS_BANDWIDTH_WIDEBAND;
       else if (tocmode==MODE_CELT_ONLY&&bw==OPUS_BANDWIDTH_MEDIUMBAND)
          bw=OPUS_BANDWIDTH_NARROWBAND;
       else if (tocmode==MODE_HYBRID&&bw<=OPUS_BANDWIDTH_SUPERWIDEBAND)
          bw=OPUS_BANDWIDTH_SUPERWIDEBAND;

       data[0] = gen_toc(tocmode, frame_rate, bw, st->stream_channels);
       data[0] |= packet_code;

       ret = packet_code <= 1 ? 1 : 2;

       max_data_bytes = IMAX(max_data_bytes, ret);

       if (packet_code==3)
          data[1] = num_multiframes;

       if (!st->use_vbr)
       {
          ret = opus_packet_pad(data, ret, max_data_bytes);
          if (ret == OPUS_OK)
             ret = max_data_bytes;
          else
             ret = OPUS_INTERNAL_ERROR;
       }
       RESTORE_STACK;
       return ret;
    }
    max_rate = frame_rate*max_data_bytes*8;

    /* Equivalent 20-ms rate for mode/channel/bandwidth decisions */
    equiv_rate = compute_equiv_rate(st->bitrate_bps, st->channels, st->Fs/frame_size,
          st->use_vbr, 0, st->silk_mode.complexity, st->silk_mode.packetLossPercentage);

    if (st->signal_type == OPUS_SIGNAL_VOICE)
       voice_est = 127;
    else if (st->signal_type == OPUS_SIGNAL_MUSIC)
       voice_est = 0;
    else if (st->voice_ratio >= 0)
    {
       voice_est = st->voice_ratio*327>>8;
       /* For AUDIO, never be more than 90% confident of having speech */
       if (st->application == OPUS_APPLICATION_AUDIO)
          voice_est = IMIN(voice_est, 115);
    } else if (st->application == OPUS_APPLICATION_VOIP)
       voice_est = 115;
    else
       voice_est = 48;

    if (st->force_channels!=OPUS_AUTO && st->channels == 2)
    {
        st->stream_channels = st->force_channels;
    } else {
       /* Rate-dependent mono-stereo decision */
       if (st->channels == 2)
       {
          opus_int32 stereo_threshold;
          stereo_threshold = stereo_music_threshold + ((voice_est*voice_est*(stereo_voice_threshold-stereo_music_threshold))>>14);
          if (st->stream_channels == 2)
             stereo_threshold -= 1000;
          else
             stereo_threshold += 1000;
          st->stream_channels = (equiv_rate > stereo_threshold) ? 2 : 1;
       } else {
          st->stream_channels = st->channels;
       }
    }
    /* Update equivalent rate for channels decision. */
    equiv_rate = compute_equiv_rate(st->bitrate_bps, st->stream_channels, st->Fs/frame_size,
          st->use_vbr, 0, st->silk_mode.complexity, st->silk_mode.packetLossPercentage);

    /* Allow SILK DTX if DTX is enabled but the generalized DTX cannot be used,
       e.g. because of the complexity setting or sample rate. */
    st->silk_mode.useDTX = st->use_dtx && !(analysis_info.valid || is_silence);

    /* Mode selection depending on application and signal type */
    if (st->application == OPUS_APPLICATION_RESTRICTED_LOWDELAY)
    {
       st->mode = MODE_CELT_ONLY;
    } else if (st->user_forced_mode == OPUS_AUTO)
    {
       opus_int32 mode_voice, mode_music;
       opus_int32 threshold;

       /* Interpolate based on stereo width */
       mode_voice = (opus_int32)(MULT16_32_Q15(Q15ONE-stereo_width,mode_thresholds[0][0])
             + MULT16_32_Q15(stereo_width,mode_thresholds[1][0]));
       mode_music = (opus_int32)(MULT16_32_Q15(Q15ONE-stereo_width,mode_thresholds[1][1])
             + MULT16_32_Q15(stereo_width,mode_thresholds[1][1]));
       /* Interpolate based on speech/music probability */
       threshold = mode_music + ((voice_est*voice_est*(mode_voice-mode_music))>>14);
       /* Bias towards SILK for VoIP because of some useful features */
       if (st->application == OPUS_APPLICATION_VOIP)
          threshold += 8000;

       /*printf("%f %d\n", stereo_width/(float)Q15ONE, threshold);*/
       /* Hysteresis */
       if (st->prev_mode == MODE_CELT_ONLY)
           threshold -= 4000;
       else if (st->prev_mode>0)
           threshold += 4000;

       st->mode = (equiv_rate >= threshold) ? MODE_CELT_ONLY: MODE_SILK_ONLY;

       /* When FEC is enabled and there's enough packet loss, use SILK.
          Unless the FEC is set to 2, in which case we don't switch to SILK if we're confident we have music. */
       if (st->silk_mode.useInBandFEC && st->silk_mode.packetLossPercentage > (128-voice_est)>>4 && (st->fec_config != 2 || voice_est > 25))
          st->mode = MODE_SILK_ONLY;
       /* When encoding voice and DTX is enabled but the generalized DTX cannot be used,
          use SILK in order to make use of its DTX. */
       if (st->silk_mode.useDTX && voice_est > 100)
          st->mode = MODE_SILK_ONLY;

       /* If max_data_bytes represents less than 6 kb/s, switch to CELT-only mode */
       if (max_data_bytes < (frame_rate > 50 ? 9000 : 6000)*frame_size / (st->Fs * 8))
          st->mode = MODE_CELT_ONLY;
    } else {
       st->mode = st->user_forced_mode;
    }

    /* Override the chosen mode to make sure we meet the requested frame size */
    if (st->mode != MODE_CELT_ONLY && frame_size < st->Fs/100)
       st->mode = MODE_CELT_ONLY;
    if (st->lfe)
       st->mode = MODE_CELT_ONLY;

    if (st->prev_mode > 0 &&
        ((st->mode != MODE_CELT_ONLY && st->prev_mode == MODE_CELT_ONLY) ||
    (st->mode == MODE_CELT_ONLY && st->prev_mode != MODE_CELT_ONLY)))
    {
        redundancy = 1;
        celt_to_silk = (st->mode != MODE_CELT_ONLY);
        if (!celt_to_silk)
        {
            /* Switch to SILK/hybrid if frame size is 10 ms or more*/
            if (frame_size >= st->Fs/100)
            {
                st->mode = st->prev_mode;
                to_celt = 1;
            } else {
                redundancy=0;
            }
        }
    }

    /* When encoding multiframes, we can ask for a switch to CELT only in the last frame. This switch
     * is processed above as the requested mode shouldn't interrupt stereo->mono transition. */
    if (st->stream_channels == 1 && st->prev_channels ==2 && st->silk_mode.toMono==0
          && st->mode != MODE_CELT_ONLY && st->prev_mode != MODE_CELT_ONLY)
    {
       /* Delay stereo->mono transition by two frames so that SILK can do a smooth downmix */
       st->silk_mode.toMono = 1;
       st->stream_channels = 2;
    } else {
       st->silk_mode.toMono = 0;
    }

    /* Update equivalent rate with mode decision. */
    equiv_rate = compute_equiv_rate(st->bitrate_bps, st->stream_channels, st->Fs/frame_size,
          st->use_vbr, st->mode, st->silk_mode.complexity, st->silk_mode.packetLossPercentage);

    if (st->mode != MODE_CELT_ONLY && st->prev_mode == MODE_CELT_ONLY)
    {
        silk_EncControlStruct dummy;
        silk_InitEncoder( silk_enc, st->arch, &dummy);
        prefill=1;
    }

    /* Automatic (rate-dependent) bandwidth selection */
    if (st->mode == MODE_CELT_ONLY || st->first || st->silk_mode.allowBandwidthSwitch)
    {
        const opus_int32 *voice_bandwidth_thresholds, *music_bandwidth_thresholds;
        opus_int32 bandwidth_thresholds[8];
        int bandwidth = OPUS_BANDWIDTH_FULLBAND;

        if (st->channels==2 && st->force_channels!=1)
        {
           voice_bandwidth_thresholds = stereo_voice_bandwidth_thresholds;
           music_bandwidth_thresholds = stereo_music_bandwidth_thresholds;
        } else {
           voice_bandwidth_thresholds = mono_voice_bandwidth_thresholds;
           music_bandwidth_thresholds = mono_music_bandwidth_thresholds;
        }
        /* Interpolate bandwidth thresholds depending on voice estimation */
        for (i=0;i<8;i++)
        {
           bandwidth_thresholds[i] = music_bandwidth_thresholds[i]
                    + ((voice_est*voice_est*(voice_bandwidth_thresholds[i]-music_bandwidth_thresholds[i]))>>14);
        }
        do {
            int threshold, hysteresis;
            threshold = bandwidth_thresholds[2*(bandwidth-OPUS_BANDWIDTH_MEDIUMBAND)];
            hysteresis = bandwidth_thresholds[2*(bandwidth-OPUS_BANDWIDTH_MEDIUMBAND)+1];
            if (!st->first)
            {
                if (st->auto_bandwidth >= bandwidth)
                    threshold -= hysteresis;
                else
                    threshold += hysteresis;
            }
            if (equiv_rate >= threshold)
                break;
        } while (--bandwidth>OPUS_BANDWIDTH_NARROWBAND);
        /* We don't use mediumband anymore, except when explicitly requested or during
           mode transitions. */
        if (bandwidth == OPUS_BANDWIDTH_MEDIUMBAND)
           bandwidth = OPUS_BANDWIDTH_WIDEBAND;
        st->bandwidth = st->auto_bandwidth = bandwidth;
        /* Prevents any transition to SWB/FB until the SILK layer has fully
           switched to WB mode and turned the variable LP filter off */
        if (!st->first && st->mode != MODE_CELT_ONLY && !st->silk_mode.inWBmodeWithoutVariableLP && st->bandwidth > OPUS_BANDWIDTH_WIDEBAND)
            st->bandwidth = OPUS_BANDWIDTH_WIDEBAND;
    }

    if (st->bandwidth>st->max_bandwidth)
       st->bandwidth = st->max_bandwidth;

    if (st->user_bandwidth != OPUS_AUTO)
        st->bandwidth = st->user_bandwidth;

    /* This prevents us from using hybrid at unsafe CBR/max rates */
    if (st->mode != MODE_CELT_ONLY && max_rate < 15000)
    {
       st->bandwidth = IMIN(st->bandwidth, OPUS_BANDWIDTH_WIDEBAND);
    }

    /* Prevents Opus from wasting bits on frequencies that are above
       the Nyquist rate of the input signal */
    if (st->Fs <= 24000 && st->bandwidth > OPUS_BANDWIDTH_SUPERWIDEBAND)
        st->bandwidth = OPUS_BANDWIDTH_SUPERWIDEBAND;
    if (st->Fs <= 16000 && st->bandwidth > OPUS_BANDWIDTH_WIDEBAND)
        st->bandwidth = OPUS_BANDWIDTH_WIDEBAND;
    if (st->Fs <= 12000 && st->bandwidth > OPUS_BANDWIDTH_MEDIUMBAND)
        st->bandwidth = OPUS_BANDWIDTH_MEDIUMBAND;
    if (st->Fs <= 8000 && st->bandwidth > OPUS_BANDWIDTH_NARROWBAND)
        st->bandwidth = OPUS_BANDWIDTH_NARROWBAND;
    /* Use detected bandwidth to reduce the encoded bandwidth. */
    if (st->detected_bandwidth && st->user_bandwidth == OPUS_AUTO)
    {
       int min_detected_bandwidth;
       /* Makes bandwidth detection more conservative just in case the detector
          gets it wrong when we could have coded a high bandwidth transparently.
          When operating in SILK/hybrid mode, we don't go below wideband to avoid
          more complicated switches that require redundancy. */
       if (equiv_rate <= 18000*st->stream_channels && st->mode == MODE_CELT_ONLY)
          min_detected_bandwidth = OPUS_BANDWIDTH_NARROWBAND;
       else if (equiv_rate <= 24000*st->stream_channels && st->mode == MODE_CELT_ONLY)
          min_detected_bandwidth = OPUS_BANDWIDTH_MEDIUMBAND;
       else if (equiv_rate <= 30000*st->stream_channels)
          min_detected_bandwidth = OPUS_BANDWIDTH_WIDEBAND;
       else if (equiv_rate <= 44000*st->stream_channels)
          min_detected_bandwidth = OPUS_BANDWIDTH_SUPERWIDEBAND;
       else
          min_detected_bandwidth = OPUS_BANDWIDTH_FULLBAND;

       st->detected_bandwidth = IMAX(st->detected_bandwidth, min_detected_bandwidth);
       st->bandwidth = IMIN(st->bandwidth, st->detected_bandwidth);
    }
    st->silk_mode.LBRR_coded = decide_fec(st->silk_mode.useInBandFEC, st->silk_mode.packetLossPercentage,
          st->silk_mode.LBRR_coded, st->mode, &st->bandwidth, equiv_rate);
    celt_encoder_ctl(celt_enc, OPUS_SET_LSB_DEPTH(lsb_depth));

    /* CELT mode doesn't support mediumband, use wideband instead */
    if (st->mode == MODE_CELT_ONLY && st->bandwidth == OPUS_BANDWIDTH_MEDIUMBAND)
        st->bandwidth = OPUS_BANDWIDTH_WIDEBAND;
    if (st->lfe)
       st->bandwidth = OPUS_BANDWIDTH_NARROWBAND;

    curr_bandwidth = st->bandwidth;

    /* Chooses the appropriate mode for speech
       *NEVER* switch to/from CELT-only mode here as this will invalidate some assumptions */
    if (st->mode == MODE_SILK_ONLY && curr_bandwidth > OPUS_BANDWIDTH_WIDEBAND)
        st->mode = MODE_HYBRID;
    if (st->mode == MODE_HYBRID && curr_bandwidth <= OPUS_BANDWIDTH_WIDEBAND)
        st->mode = MODE_SILK_ONLY;

    /* Can't support higher than >60 ms frames, and >20 ms when in Hybrid or CELT-only modes */
    if ((frame_size > st->Fs/50 && (st->mode != MODE_SILK_ONLY)) || frame_size > 3*st->Fs/50)
    {
       int enc_frame_size;
       int nb_frames;
       VARDECL(unsigned char, tmp_data);
       VARDECL(OpusRepacketizer, rp);
       int max_header_bytes;
       opus_int32 repacketize_len;
       opus_int32 max_len_sum;
       opus_int32 tot_size=0;
       unsigned char *curr_data;
       int tmp_len;
       int dtx_count = 0;

       if (st->mode == MODE_SILK_ONLY)
       {
         if (frame_size == 2*st->Fs/25)  /* 80 ms -> 2x 40 ms */
           enc_frame_size = st->Fs/25;
         else if (frame_size == 3*st->Fs/25)  /* 120 ms -> 2x 60 ms */
           enc_frame_size = 3*st->Fs/50;
         else                            /* 100 ms -> 5x 20 ms */
           enc_frame_size = st->Fs/50;
       }
       else
         enc_frame_size = st->Fs/50;

       nb_frames = frame_size/enc_frame_size;

       if (analysis_read_pos_bak!= -1)
       {
          /* Reset analysis position to the beginning of the first frame so we
             can use it one frame at a time. */
          st->analysis.read_pos = analysis_read_pos_bak;
          st->analysis.read_subframe = analysis_read_subframe_bak;
       }

       /* Worst cases:
        * 2 frames: Code 2 with different compressed sizes
        * >2 frames: Code 3 VBR */
       max_header_bytes = nb_frames == 2 ? 3 : (2+(nb_frames-1)*2);

       if (st->use_vbr || st->user_bitrate_bps==OPUS_BITRATE_MAX)
          repacketize_len = out_data_bytes;
       else {
          celt_assert(cbr_bytes>=0);
          repacketize_len = IMIN(cbr_bytes, out_data_bytes);
       }
       max_len_sum = nb_frames + repacketize_len - max_header_bytes;

       ALLOC(tmp_data, max_len_sum, unsigned char);
       curr_data = tmp_data;
       ALLOC(rp, 1, OpusRepacketizer);
       opus_repacketizer_init(rp);

       int bak_to_mono = st->silk_mode.toMono;
       if (bak_to_mono)
          st->force_channels = 1;
       else
          st->prev_channels = st->stream_channels;

       for (i=0;i<nb_frames;i++)
       {
          int first_frame;
          int frame_to_celt;
          int frame_redundancy;
          opus_int32 curr_max;
          /* Attempt DRED encoding until we have a non-DTX frame. In case of DTX refresh,
             that allows for DRED not to be in the first frame. */
          first_frame = (i == 0) || (i == dtx_count);
          st->silk_mode.toMono = 0;
          st->nonfinal_frame = i<(nb_frames-1);

          /* When switching from SILK/Hybrid to CELT, only ask for a switch at the last frame */
          frame_to_celt = to_celt && i==nb_frames-1;
          frame_redundancy = redundancy && (frame_to_celt || (!to_celt && i==0));

          curr_max = IMIN(3*st->bitrate_bps/(3*8*st->Fs/enc_frame_size), max_len_sum/nb_frames);
          curr_max = IMIN(max_len_sum-tot_size, curr_max);
          if (analysis_read_pos_bak != -1) {
            is_silence = is_digital_silence(pcm, frame_size, st->channels, lsb_depth);
            /* Get analysis for current frame. */
            tonality_get_info(&st->analysis, &analysis_info, enc_frame_size);
          }

          tmp_len = opus_encode_frame_native(st, pcm+i*(st->channels*enc_frame_size), enc_frame_size, curr_data, curr_max, float_api, first_frame,
          &analysis_info,
          is_silence,
                    frame_redundancy, celt_to_silk, prefill,
                    equiv_rate, frame_to_celt
              );
          if (tmp_len<0)
          {
             RESTORE_STACK;
             return OPUS_INTERNAL_ERROR;
          } else if (tmp_len==1) {
             dtx_count++;
          }
          ret = opus_repacketizer_cat(rp, curr_data, tmp_len);

          if (ret<0)
          {
             RESTORE_STACK;
             return OPUS_INTERNAL_ERROR;
          }
          tot_size += tmp_len;
          curr_data += tmp_len;
       }
       ret = opus_repacketizer_out_range_impl(rp, 0, nb_frames, data, repacketize_len, 0, !st->use_vbr && (dtx_count != nb_frames), NULL, 0);
       if (ret<0)
       {
          ret = OPUS_INTERNAL_ERROR;
       }
       st->silk_mode.toMono = bak_to_mono;
       RESTORE_STACK;
       return ret;
    } else {
      ret = opus_encode_frame_native(st, pcm, frame_size, data, max_data_bytes, float_api, 1,
                &analysis_info,
                is_silence,
                redundancy, celt_to_silk, prefill,
                equiv_rate, to_celt
          );
      RESTORE_STACK;
      return ret;
    }
}
static opus_int32 opus_encode_frame_native(OpusEncoder *st, const opus_val16 *pcm, int frame_size,
                unsigned char *data, opus_int32 max_data_bytes,
                int float_api, int first_frame,
                AnalysisInfo *analysis_info, int is_silence,
                int redundancy, int celt_to_silk, int prefill,
                opus_int32 equiv_rate, int to_celt)
{
    void *silk_enc;
    CELTEncoder *celt_enc;
    const CELTMode *celt_mode;
    int i;
    int ret=0;
    opus_int32 nBytes;
    ec_enc enc;
    int bytes_target;
    int start_band = 0;
    int redundancy_bytes = 0; /* Number of bytes to use for redundancy frame */
    int nb_compr_bytes;
    opus_uint32 redundant_rng = 0;
    int cutoff_Hz;
    int hp_freq_smth1;
    opus_val16 HB_gain;
    int apply_padding;
    int frame_rate;
    int curr_bandwidth;
    int delay_compensation;
    int total_buffer;
    opus_int activity = VAD_NO_DECISION;
    VARDECL(opus_val16, pcm_buf);
    VARDECL(opus_val16, tmp_prefill);
    SAVE_STACK;

    st->rangeFinal = 0;
    silk_enc = (char*)st+st->silk_enc_offset;
    celt_enc = (CELTEncoder*)((char*)st+st->celt_enc_offset);
    celt_encoder_ctl(celt_enc, CELT_GET_MODE(&celt_mode));
    curr_bandwidth = st->bandwidth;
    if (st->application == OPUS_APPLICATION_RESTRICTED_LOWDELAY)
       delay_compensation = 0;
    else
       delay_compensation = st->delay_compensation;
    total_buffer = delay_compensation;

    frame_rate = st->Fs/frame_size;

    if (is_silence)
    {
       activity = !is_silence;
    } else if (analysis_info->valid)
    {
       activity = analysis_info->activity_probability >= DTX_ACTIVITY_THRESHOLD;
       if (!activity)
       {
           /* Mark as active if this noise frame is sufficiently loud */
           opus_val32 noise_energy = compute_frame_energy(pcm, frame_size, st->channels, st->arch);
           activity = st->peak_signal_energy < (PSEUDO_SNR_THRESHOLD * noise_energy);
       }
    }

    /* For the first frame at a new SILK bandwidth */
    if (st->silk_bw_switch)
    {
       redundancy = 1;
       celt_to_silk = 1;
       st->silk_bw_switch = 0;
       /* Do a prefill without resetting the sampling rate control. */
       prefill=2;
    }

    /* If we decided to go with CELT, make sure redundancy is off, no matter what
       we decided earlier. */
    if (st->mode == MODE_CELT_ONLY)
        redundancy = 0;

    if (redundancy)
    {
       redundancy_bytes = compute_redundancy_bytes(max_data_bytes, st->bitrate_bps, frame_rate, st->stream_channels);
       if (redundancy_bytes == 0)
          redundancy = 0;
    }

    /* printf("%d %d %d %d\n", st->bitrate_bps, st->stream_channels, st->mode, curr_bandwidth); */
    bytes_target = IMIN(max_data_bytes-redundancy_bytes, st->bitrate_bps * frame_size / (st->Fs * 8)) - 1;

    data += 1;

    ec_enc_init(&enc, data, max_data_bytes-1);

    ALLOC(pcm_buf, (total_buffer+frame_size)*st->channels, opus_val16);
    OPUS_COPY(pcm_buf, &st->delay_buffer[(st->encoder_buffer-total_buffer)*st->channels], total_buffer*st->channels);

    if (st->mode == MODE_CELT_ONLY)
       hp_freq_smth1 = silk_LSHIFT( silk_lin2log( VARIABLE_HP_MIN_CUTOFF_HZ ), 8 );
    else
       hp_freq_smth1 = silk_LSHIFT( silk_lin2log( VARIABLE_HP_MIN_CUTOFF_HZ ), 8 ); /* SILK not included */

    st->variable_HP_smth2_Q15 = silk_SMLAWB( st->variable_HP_smth2_Q15,
          hp_freq_smth1 - st->variable_HP_smth2_Q15, SILK_FIX_CONST( VARIABLE_HP_SMTH_COEF2, 16 ) );

    /* convert from log scale to Hertz */
    cutoff_Hz = silk_log2lin( silk_RSHIFT( st->variable_HP_smth2_Q15, 8 ) );

    if (st->application == OPUS_APPLICATION_VOIP)
    {
       hp_cutoff(pcm, cutoff_Hz, &pcm_buf[total_buffer*st->channels], st->hp_mem, frame_size, st->channels, st->Fs, st->arch);

    } else {
       dc_reject(pcm, 3, &pcm_buf[total_buffer*st->channels], st->hp_mem, frame_size, st->channels, st->Fs);
    }
    if (float_api)
    {
       opus_val32 sum;
       sum = celt_inner_prod(&pcm_buf[total_buffer*st->channels], &pcm_buf[total_buffer*st->channels], frame_size*st->channels, st->arch);
       /* This should filter out both NaNs and ridiculous signals that could
          cause NaNs further down. */
       if (!(sum < 1e9f) || celt_isnan(sum))
       {
          OPUS_CLEAR(&pcm_buf[total_buffer*st->channels], frame_size*st->channels);
          st->hp_mem[0] = st->hp_mem[1] = st->hp_mem[2] = st->hp_mem[3] = 0;
       }
    }

    /* SILK processing */
    HB_gain = Q15ONE;
    if (st->mode != MODE_CELT_ONLY)
    {
        opus_int32 total_bitRate, celt_rate;
       VARDECL(opus_int16, pcm_silk);
       ALLOC(pcm_silk, st->channels*frame_size, opus_int16);

        /* Distribute bits between SILK and CELT */
        total_bitRate = 8 * bytes_target * frame_rate;
        if( st->mode == MODE_HYBRID ) {
            /* Base rate for SILK */
            st->silk_mode.bitRate = compute_silk_rate_for_hybrid(total_bitRate,
                  curr_bandwidth, st->Fs == 50 * frame_size, st->use_vbr, st->silk_mode.LBRR_coded,
                  st->stream_channels);
            if (!st->energy_masking)
            {
               /* Increasingly attenuate high band when it gets allocated fewer bits */
               celt_rate = total_bitRate - st->silk_mode.bitRate;
               HB_gain = Q15ONE - SHR32(celt_exp2(-celt_rate * QCONST16(1.f/1024, 10)), 1);
            }
        } else {
            /* SILK gets all bits */
            st->silk_mode.bitRate = total_bitRate;
        }

        /* Surround masking for SILK */
        if (st->energy_masking && st->use_vbr && !st->lfe)
        {
           opus_val32 mask_sum=0;
           opus_val16 masking_depth;
           opus_int32 rate_offset;
           int c;
           int end = 17;
           opus_int16 srate = 16000;
           if (st->bandwidth == OPUS_BANDWIDTH_NARROWBAND)
           {
              end = 13;
              srate = 8000;
           } else if (st->bandwidth == OPUS_BANDWIDTH_MEDIUMBAND)
           {
              end = 15;
              srate = 12000;
           }
           for (c=0;c<st->channels;c++)
           {
              for(i=0;i<end;i++)
              {
                 opus_val16 mask;
                 mask = MAX16(MIN16(st->energy_masking[21*c+i],
                        QCONST16(.5f, DB_SHIFT)), -QCONST16(2.0f, DB_SHIFT));
                 if (mask > 0)
                    mask = HALF16(mask);
                 mask_sum += mask;
              }
           }
           /* Conservative rate reduction, we cut the masking in half */
           masking_depth = mask_sum / end*st->channels;
           masking_depth += QCONST16(.2f, DB_SHIFT);
           rate_offset = (opus_int32)PSHR32(MULT16_16(srate, masking_depth), DB_SHIFT);
           rate_offset = MAX32(rate_offset, -2*st->silk_mode.bitRate/3);
           /* Split the rate change between the SILK and CELT part for hybrid. */
           if (st->bandwidth==OPUS_BANDWIDTH_SUPERWIDEBAND || st->bandwidth==OPUS_BANDWIDTH_FULLBAND)
              st->silk_mode.bitRate += 3*rate_offset/5;
           else
              st->silk_mode.bitRate += rate_offset;
        }

        st->silk_mode.payloadSize_ms = 1000 * frame_size / st->Fs;
        st->silk_mode.nChannelsAPI = st->channels;
        st->silk_mode.nChannelsInternal = st->stream_channels;
        if (curr_bandwidth == OPUS_BANDWIDTH_NARROWBAND) {
            st->silk_mode.desiredInternalSampleRate = 8000;
        } else if (curr_bandwidth == OPUS_BANDWIDTH_MEDIUMBAND) {
            st->silk_mode.desiredInternalSampleRate = 12000;
        } else {
            celt_assert( st->mode == MODE_HYBRID || curr_bandwidth == OPUS_BANDWIDTH_WIDEBAND );
            st->silk_mode.desiredInternalSampleRate = 16000;
        }
        if( st->mode == MODE_HYBRID ) {
            /* Don't allow bandwidth reduction at lowest bitrates in hybrid mode */
            st->silk_mode.minInternalSampleRate = 16000;
        } else {
            st->silk_mode.minInternalSampleRate = 8000;
        }

        st->silk_mode.maxInternalSampleRate = 16000;
        if (st->mode == MODE_SILK_ONLY)
        {
           opus_int32 effective_max_rate = frame_rate*max_data_bytes*8;
           if (frame_rate > 50)
              effective_max_rate = effective_max_rate*2/3;
           if (effective_max_rate < 8000)
           {
              st->silk_mode.maxInternalSampleRate = 12000;
              st->silk_mode.desiredInternalSampleRate = IMIN(12000, st->silk_mode.desiredInternalSampleRate);
           }
           if (effective_max_rate < 7000)
           {
              st->silk_mode.maxInternalSampleRate = 8000;
              st->silk_mode.desiredInternalSampleRate = IMIN(8000, st->silk_mode.desiredInternalSampleRate);
           }
        }

        st->silk_mode.useCBR = !st->use_vbr;

        /* Call SILK encoder for the low band */

        /* Max bits for SILK, counting ToC, redundancy bytes, and optionally redundancy. */
        st->silk_mode.maxBits = (max_data_bytes-1)*8;
        if (redundancy && redundancy_bytes >= 2)
        {
           /* Counting 1 bit for redundancy position and 20 bits for flag+size (only for hybrid). */
           st->silk_mode.maxBits -= redundancy_bytes*8 + 1;
           if (st->mode == MODE_HYBRID)
              st->silk_mode.maxBits -= 20;
        }
        if (st->silk_mode.useCBR)
        {
           /* When we're in CBR mode, but we have non-SILK data to encode, switch SILK to VBR with cap to
              save on complexity. Any variations will be absorbed by CELT and/or DRED and we can still
              produce a constant bitrate without wasting bits. */
           if (st->mode == MODE_HYBRID)
           {
              /* Allow SILK to steal up to 25% of the remaining bits */
              opus_int16 other_bits = IMAX(0, st->silk_mode.maxBits - st->silk_mode.bitRate * frame_size / st->Fs);
              st->silk_mode.maxBits = IMAX(0, st->silk_mode.maxBits - other_bits*3/4);
              st->silk_mode.useCBR = 0;
           }
        } else {
           /* Constrained VBR. */
           if (st->mode == MODE_HYBRID)
           {
              /* Compute SILK bitrate corresponding to the max total bits available */
              opus_int32 maxBitRate = compute_silk_rate_for_hybrid(st->silk_mode.maxBits*st->Fs / frame_size,
                    curr_bandwidth, st->Fs == 50 * frame_size, st->use_vbr, st->silk_mode.LBRR_coded,
                    st->stream_channels);
              st->silk_mode.maxBits = maxBitRate * frame_size / st->Fs;
           }
        }

        if (prefill)
        {
            opus_int32 zero=0;
            int prefill_offset;
            /* Use a smooth onset for the SILK prefill to avoid the encoder trying to encode
               a discontinuity. The exact location is what we need to avoid leaving any "gap"
               in the audio when mixing with the redundant CELT frame. Here we can afford to
               overwrite st->delay_buffer because the only thing that uses it before it gets
               rewritten is tmp_prefill[] and even then only the part after the ramp really
               gets used (rather than sent to the encoder and discarded) */
            prefill_offset = st->channels*(st->encoder_buffer-st->delay_compensation-st->Fs/400);
            gain_fade(st->delay_buffer+prefill_offset, st->delay_buffer+prefill_offset,
                  0, Q15ONE, celt_mode->overlap, st->Fs/400, st->channels, celt_mode->window, st->Fs);
            OPUS_CLEAR(st->delay_buffer, prefill_offset);
            for (i=0;i<st->encoder_buffer*st->channels;i++)
                pcm_silk[i] = FLOAT2INT16(st->delay_buffer[i]);
            silk_Encode( silk_enc, &st->silk_mode, pcm_silk, st->encoder_buffer, NULL, &zero, prefill, activity );
            /* Prevent a second switch in the real encode call. */
            st->silk_mode.opusCanSwitch = 0;
        }

        for (i=0;i<frame_size*st->channels;i++)
            pcm_silk[i] = FLOAT2INT16(pcm_buf[total_buffer*st->channels + i]);
        ret = silk_Encode( silk_enc, &st->silk_mode, pcm_silk, frame_size, &enc, &nBytes, 0, activity );
        if( ret ) {
            /*fprintf (stderr, "SILK encode error: %d\n", ret);*/
            /* Handle error */
           RESTORE_STACK;
           return OPUS_INTERNAL_ERROR;
        }

        /* Extract SILK internal bandwidth for signaling in first byte */
        if( st->mode == MODE_SILK_ONLY ) {
            if( st->silk_mode.internalSampleRate == 8000 ) {
               curr_bandwidth = OPUS_BANDWIDTH_NARROWBAND;
            } else if( st->silk_mode.internalSampleRate == 12000 ) {
               curr_bandwidth = OPUS_BANDWIDTH_MEDIUMBAND;
            } else if( st->silk_mode.internalSampleRate == 16000 ) {
               curr_bandwidth = OPUS_BANDWIDTH_WIDEBAND;
            }
        } else {
            celt_assert( st->silk_mode.internalSampleRate == 16000 );
        }

        st->silk_mode.opusCanSwitch = st->silk_mode.switchReady && !st->nonfinal_frame;

        if (nBytes==0)
        {
           st->rangeFinal = 0;
           data[-1] = gen_toc(st->mode, st->Fs/frame_size, curr_bandwidth, st->stream_channels);
           RESTORE_STACK;
           return 1;
        }

        /* FIXME: How do we allocate the redundancy for CBR? */
        if (st->silk_mode.opusCanSwitch)
        {
           redundancy_bytes = compute_redundancy_bytes(max_data_bytes, st->bitrate_bps, frame_rate, st->stream_channels);
           redundancy = (redundancy_bytes != 0);
           celt_to_silk = 0;
           st->silk_bw_switch = 1;
        }
    }

    /* CELT processing */
    {
        int endband=21;

        switch(curr_bandwidth)
        {
            case OPUS_BANDWIDTH_NARROWBAND:
                endband = 13;
                break;
            case OPUS_BANDWIDTH_MEDIUMBAND:
            case OPUS_BANDWIDTH_WIDEBAND:
                endband = 17;
                break;
            case OPUS_BANDWIDTH_SUPERWIDEBAND:
                endband = 19;
                break;
            case OPUS_BANDWIDTH_FULLBAND:
                endband = 21;
                break;
        }
        celt_encoder_ctl(celt_enc, CELT_SET_END_BAND(endband));
        celt_encoder_ctl(celt_enc, CELT_SET_CHANNELS(st->stream_channels));
    }
    celt_encoder_ctl(celt_enc, OPUS_SET_BITRATE(OPUS_BITRATE_MAX));
    if (st->mode != MODE_SILK_ONLY)
    {
        opus_val32 celt_pred=2;
        /* We may still decide to disable prediction later */
        if (st->silk_mode.reducedDependency)
           celt_pred = 0;
        celt_encoder_ctl(celt_enc, CELT_SET_PREDICTION(celt_pred));
    }

    ALLOC(tmp_prefill, st->channels*st->Fs/400, opus_val16);
    if (st->mode != MODE_SILK_ONLY && st->mode != st->prev_mode && st->prev_mode > 0)
    {
       OPUS_COPY(tmp_prefill, &st->delay_buffer[(st->encoder_buffer-total_buffer-st->Fs/400)*st->channels], st->channels*st->Fs/400);
    }

    if (st->channels*(st->encoder_buffer-(frame_size+total_buffer)) > 0)
    {
       OPUS_MOVE(st->delay_buffer, &st->delay_buffer[st->channels*frame_size], st->channels*(st->encoder_buffer-frame_size-total_buffer));
       OPUS_COPY(&st->delay_buffer[st->channels*(st->encoder_buffer-frame_size-total_buffer)],
             &pcm_buf[0],
             (frame_size+total_buffer)*st->channels);
    } else {
       OPUS_COPY(st->delay_buffer, &pcm_buf[(frame_size+total_buffer-st->encoder_buffer)*st->channels], st->encoder_buffer*st->channels);
    }
    /* gain_fade() and stereo_fade() need to be after the buffer copying
       because we don't want any of this to affect the SILK part */
    if( st->prev_HB_gain < Q15ONE || HB_gain < Q15ONE ) {
       gain_fade(pcm_buf, pcm_buf,
             st->prev_HB_gain, HB_gain, celt_mode->overlap, frame_size, st->channels, celt_mode->window, st->Fs);
    }
    st->prev_HB_gain = HB_gain;
    if (st->mode != MODE_HYBRID || st->stream_channels==1)
    {
       if (equiv_rate > 32000)
          st->silk_mode.stereoWidth_Q14 = 16384;
       else if (equiv_rate < 16000)
          st->silk_mode.stereoWidth_Q14 = 0;
       else
          st->silk_mode.stereoWidth_Q14 = 16384 - 2048*(opus_int32)(32000-equiv_rate)/(equiv_rate-14000);
    }
    if( !st->energy_masking && st->channels == 2 ) {
        /* Apply stereo width reduction (at low bitrates) */
        if( st->hybrid_stereo_width_Q14 < (1 << 14) || st->silk_mode.stereoWidth_Q14 < (1 << 14) ) {
            opus_val16 g1, g2;
            g1 = st->hybrid_stereo_width_Q14;
            g2 = (opus_val16)(st->silk_mode.stereoWidth_Q14);
            g1 *= (1.f/16384);
            g2 *= (1.f/16384);
            stereo_fade(pcm_buf, pcm_buf, g1, g2, celt_mode->overlap,
                  frame_size, st->channels, celt_mode->window, st->Fs);
            st->hybrid_stereo_width_Q14 = st->silk_mode.stereoWidth_Q14;
        }
    }

    if ( st->mode != MODE_CELT_ONLY && ec_tell(&enc)+17+20*(st->mode == MODE_HYBRID) <= 8*(max_data_bytes-1))
    {
        /* For SILK mode, the redundancy is inferred from the length */
        if (st->mode == MODE_HYBRID)
           ec_enc_bit_logp(&enc, redundancy, 12);
        if (redundancy)
        {
            int max_redundancy;
            ec_enc_bit_logp(&enc, celt_to_silk, 1);
            if (st->mode == MODE_HYBRID)
            {
               /* Reserve the 8 bits needed for the redundancy length,
                  and at least a few bits for CELT if possible */
               max_redundancy = (max_data_bytes-1)-((ec_tell(&enc)+8+3+7)>>3);
            }
            else
               max_redundancy = (max_data_bytes-1)-((ec_tell(&enc)+7)>>3);
            /* Target the same bit-rate for redundancy as for the rest,
               up to a max of 257 bytes */
            redundancy_bytes = IMIN(max_redundancy, redundancy_bytes);
            redundancy_bytes = IMIN(257, IMAX(2, redundancy_bytes));
            if (st->mode == MODE_HYBRID)
                ec_enc_uint(&enc, redundancy_bytes-2, 256);
        }
    } else {
        redundancy = 0;
    }

    if (!redundancy)
    {
       st->silk_bw_switch = 0;
       redundancy_bytes = 0;
    }
    if (st->mode != MODE_CELT_ONLY)start_band=17;

    if (st->mode == MODE_SILK_ONLY)
    {
        ret = (ec_tell(&enc)+7)>>3;
        ec_enc_done(&enc);
        nb_compr_bytes = ret;
    } else {
        nb_compr_bytes = (max_data_bytes-1)-redundancy_bytes;
        ec_enc_shrink(&enc, nb_compr_bytes);
    }

    if (redundancy || st->mode != MODE_SILK_ONLY)
       celt_encoder_ctl(celt_enc, CELT_SET_ANALYSIS(analysis_info));
    if (st->mode == MODE_HYBRID) {
       SILKInfo info;
       info.signalType = st->silk_mode.signalType;
       info.offset = st->silk_mode.offset;
       celt_encoder_ctl(celt_enc, CELT_SET_SILK_INFO(&info));
    }

    /* 5 ms redundant frame for CELT->SILK */
    if (redundancy && celt_to_silk)
    {
        int err;
        celt_encoder_ctl(celt_enc, CELT_SET_START_BAND(0));
        celt_encoder_ctl(celt_enc, OPUS_SET_VBR(0));
        celt_encoder_ctl(celt_enc, OPUS_SET_BITRATE(OPUS_BITRATE_MAX));
        err = celt_encode_with_ec(celt_enc, pcm_buf, st->Fs/200, data+nb_compr_bytes, redundancy_bytes, NULL);
        if (err < 0)
        {
           RESTORE_STACK;
           return OPUS_INTERNAL_ERROR;
        }
        celt_encoder_ctl(celt_enc, OPUS_GET_FINAL_RANGE(&redundant_rng));
        celt_encoder_ctl(celt_enc, OPUS_RESET_STATE);
    }

    celt_encoder_ctl(celt_enc, CELT_SET_START_BAND(start_band));

    if (st->mode != MODE_SILK_ONLY)
    {
        celt_encoder_ctl(celt_enc, OPUS_SET_VBR(st->use_vbr));
        if (st->mode == MODE_HYBRID)
        {
            if( st->use_vbr ) {
                celt_encoder_ctl(celt_enc, OPUS_SET_BITRATE(st->bitrate_bps-st->silk_mode.bitRate));
                celt_encoder_ctl(celt_enc, OPUS_SET_VBR_CONSTRAINT(0));
            }
        } else {
            if (st->use_vbr)
            {
                celt_encoder_ctl(celt_enc, OPUS_SET_VBR(1));
                celt_encoder_ctl(celt_enc, OPUS_SET_VBR_CONSTRAINT(st->vbr_constraint));
                celt_encoder_ctl(celt_enc, OPUS_SET_BITRATE(st->bitrate_bps));
            }
        }
        if (st->mode != st->prev_mode && st->prev_mode > 0)
        {
           unsigned char dummy[2];
           celt_encoder_ctl(celt_enc, OPUS_RESET_STATE);

           /* Prefilling */
           celt_encode_with_ec(celt_enc, tmp_prefill, st->Fs/400, dummy, 2, NULL);
           celt_encoder_ctl(celt_enc, CELT_SET_PREDICTION(0));
        }
        /* If false, we already busted the budget and we'll end up with a "PLC frame" */
        if (ec_tell(&enc) <= 8*nb_compr_bytes)
        {
           ret = celt_encode_with_ec(celt_enc, pcm_buf, frame_size, NULL, nb_compr_bytes, &enc);
           if (ret < 0)
           {
              RESTORE_STACK;
              return OPUS_INTERNAL_ERROR;
           }
           /* Put CELT->SILK redundancy data in the right place. */
           if (redundancy && celt_to_silk && st->mode==MODE_HYBRID && nb_compr_bytes != ret)
           {
              OPUS_MOVE(data+ret, data+nb_compr_bytes, redundancy_bytes);
              nb_compr_bytes = ret+redundancy_bytes;
           }
        }
    }

    /* 5 ms redundant frame for SILK->CELT */
    if (redundancy && !celt_to_silk)
    {
        int err;
        unsigned char dummy[2];
        int N2, N4;
        N2 = st->Fs/200;
        N4 = st->Fs/400;

        celt_encoder_ctl(celt_enc, OPUS_RESET_STATE);
        celt_encoder_ctl(celt_enc, CELT_SET_START_BAND(0));
        celt_encoder_ctl(celt_enc, CELT_SET_PREDICTION(0));
        celt_encoder_ctl(celt_enc, OPUS_SET_VBR(0));
        celt_encoder_ctl(celt_enc, OPUS_SET_BITRATE(OPUS_BITRATE_MAX));

        if (st->mode == MODE_HYBRID)
        {
           /* Shrink packet to what the encoder actually used. */
           nb_compr_bytes = ret;
           ec_enc_shrink(&enc, nb_compr_bytes);
        }
        /* NOTE: We could speed this up slightly (at the expense of code size) by just adding a function that prefills the buffer */
        celt_encode_with_ec(celt_enc, pcm_buf+st->channels*(frame_size-N2-N4), N4, dummy, 2, NULL);

        err = celt_encode_with_ec(celt_enc, pcm_buf+st->channels*(frame_size-N2), N2, data+nb_compr_bytes, redundancy_bytes, NULL);
        if (err < 0)
        {
           RESTORE_STACK;
           return OPUS_INTERNAL_ERROR;
        }
        celt_encoder_ctl(celt_enc, OPUS_GET_FINAL_RANGE(&redundant_rng));
    }

    /* Signalling the mode in the first byte */
    data--;
    data[0] = gen_toc(st->mode, st->Fs/frame_size, curr_bandwidth, st->stream_channels);

    st->rangeFinal = enc.rng ^ redundant_rng;

    if (to_celt)
        st->prev_mode = MODE_CELT_ONLY;
    else
        st->prev_mode = st->mode;
    st->prev_channels = st->stream_channels;
    st->prev_framesize = frame_size;

    st->first = 0;

    /* DTX decision */
    if (st->use_dtx && (analysis_info->valid || is_silence))
    {
       if (decide_dtx_mode(activity, &st->nb_no_activity_ms_Q1, 2*1000*frame_size/st->Fs))
       {
          st->rangeFinal = 0;
          data[0] = gen_toc(st->mode, st->Fs/frame_size, curr_bandwidth, st->stream_channels);
          RESTORE_STACK;
          return 1;
       }
    } else {
       st->nb_no_activity_ms_Q1 = 0;
    }

    /* In the unlikely case that the SILK encoder busted its target, tell
       the decoder to call the PLC */
    if (ec_tell(&enc) > (max_data_bytes-1)*8)
    {
       if (max_data_bytes < 2)
       {
          RESTORE_STACK;
          return OPUS_BUFFER_TOO_SMALL;
       }
       data[1] = 0;
       ret = 1;
       st->rangeFinal = 0;
    } else if (st->mode==MODE_SILK_ONLY&&!redundancy)
    {
       /*When in LPC only mode it's perfectly
         reasonable to strip off trailing zero bytes as
         the required range decoder behavior is to
         fill these in. This can't be done when the MDCT
         modes are used because the decoder needs to know
         the actual length for allocation purposes.*/
       while(ret>2&&data[ret]==0)ret--;
    }
    /* Count ToC and redundancy */
    ret += 1+redundancy_bytes;
    apply_padding = !st->use_vbr;
    (void)first_frame; /* Avoids a warning about first_frame being unused. */
    if (apply_padding)
    {
       if (opus_packet_pad(data, ret, max_data_bytes) != OPUS_OK)
       {
          RESTORE_STACK;
          return OPUS_INTERNAL_ERROR;
       }
       ret = max_data_bytes;
    }
    RESTORE_STACK;
    return ret;
}
opus_int32 opus_encode(OpusEncoder *st, const opus_int16 *pcm, int analysis_frame_size,
      unsigned char *data, opus_int32 max_data_bytes)
{
   int i, ret;
   int frame_size;
   VARDECL(float, in);
   ALLOC_STACK;

   frame_size = frame_size_select(analysis_frame_size, st->variable_duration, st->Fs);
   if (frame_size <= 0)
   {
      RESTORE_STACK;
      return OPUS_BAD_ARG;
   }
   ALLOC(in, frame_size*st->channels, float);

   for (i=0;i<frame_size*st->channels;i++)
      in[i] = (1.0f/32768)*pcm[i];
   ret = opus_encode_native(st, in, frame_size, data, max_data_bytes, 16,
                            pcm, analysis_frame_size, 0, -2, st->channels, downmix_int, 0);
   RESTORE_STACK;
   return ret;
}
int opus_encoder_ctl(OpusEncoder *st, int request, ...)
{
    int ret;
    CELTEncoder *celt_enc;
    va_list ap;

    ret = OPUS_OK;
    va_start(ap, request);

    celt_enc = (CELTEncoder*)((char*)st+st->celt_enc_offset);

    switch (request)
    {
        case OPUS_SET_APPLICATION_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if (   (value != OPUS_APPLICATION_VOIP && value != OPUS_APPLICATION_AUDIO
                 && value != OPUS_APPLICATION_RESTRICTED_LOWDELAY)
               || (!st->first && st->application != value))
            {
               ret = OPUS_BAD_ARG;
               break;
            }
            st->application = value;
            st->analysis.application = value;
        }
        break;
        case OPUS_GET_APPLICATION_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->application;
        }
        break;
        case OPUS_SET_BITRATE_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if (value != OPUS_AUTO && value != OPUS_BITRATE_MAX)
            {
                if (value <= 0)
                    goto bad_arg;
                else if (value <= 500)
                    value = 500;
                else if (value > (opus_int32)300000*st->channels)
                    value = (opus_int32)300000*st->channels;
            }
            st->user_bitrate_bps = value;
        }
        break;
        case OPUS_GET_BITRATE_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = user_bitrate_to_bitrate(st, st->prev_framesize, 1276);
        }
        break;
        case OPUS_SET_FORCE_CHANNELS_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if((value<1 || value>st->channels) && value != OPUS_AUTO)
            {
               goto bad_arg;
            }
            st->force_channels = value;
        }
        break;
        case OPUS_GET_FORCE_CHANNELS_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->force_channels;
        }
        break;
        case OPUS_SET_MAX_BANDWIDTH_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if (value < OPUS_BANDWIDTH_NARROWBAND || value > OPUS_BANDWIDTH_FULLBAND)
            {
               goto bad_arg;
            }
            st->max_bandwidth = value;
            if (st->max_bandwidth == OPUS_BANDWIDTH_NARROWBAND) {
                st->silk_mode.maxInternalSampleRate = 8000;
            } else if (st->max_bandwidth == OPUS_BANDWIDTH_MEDIUMBAND) {
                st->silk_mode.maxInternalSampleRate = 12000;
            } else {
                st->silk_mode.maxInternalSampleRate = 16000;
            }
        }
        break;
        case OPUS_GET_MAX_BANDWIDTH_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->max_bandwidth;
        }
        break;
        case OPUS_SET_BANDWIDTH_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if ((value < OPUS_BANDWIDTH_NARROWBAND || value > OPUS_BANDWIDTH_FULLBAND) && value != OPUS_AUTO)
            {
               goto bad_arg;
            }
            st->user_bandwidth = value;
            if (st->user_bandwidth == OPUS_BANDWIDTH_NARROWBAND) {
                st->silk_mode.maxInternalSampleRate = 8000;
            } else if (st->user_bandwidth == OPUS_BANDWIDTH_MEDIUMBAND) {
                st->silk_mode.maxInternalSampleRate = 12000;
            } else {
                st->silk_mode.maxInternalSampleRate = 16000;
            }
        }
        break;
        case OPUS_GET_BANDWIDTH_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->bandwidth;
        }
        break;
        case OPUS_SET_DTX_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if(value<0 || value>1)
            {
               goto bad_arg;
            }
            st->use_dtx = value;
        }
        break;
        case OPUS_GET_DTX_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->use_dtx;
        }
        break;
        case OPUS_SET_COMPLEXITY_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if(value<0 || value>10)
            {
               goto bad_arg;
            }
            st->silk_mode.complexity = value;
            celt_encoder_ctl(celt_enc, OPUS_SET_COMPLEXITY(value));
        }
        break;
        case OPUS_GET_COMPLEXITY_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->silk_mode.complexity;
        }
        break;
        case OPUS_SET_INBAND_FEC_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if(value<0 || value>2)
            {
               goto bad_arg;
            }
            st->fec_config = value;
            st->silk_mode.useInBandFEC = (value != 0);
        }
        break;
        case OPUS_GET_INBAND_FEC_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->fec_config;
        }
        break;
        case OPUS_SET_PACKET_LOSS_PERC_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if (value < 0 || value > 100)
            {
               goto bad_arg;
            }
            st->silk_mode.packetLossPercentage = value;
            celt_encoder_ctl(celt_enc, OPUS_SET_PACKET_LOSS_PERC(value));
        }
        break;
        case OPUS_GET_PACKET_LOSS_PERC_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->silk_mode.packetLossPercentage;
        }
        break;
        case OPUS_SET_VBR_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if(value<0 || value>1)
            {
               goto bad_arg;
            }
            st->use_vbr = value;
            st->silk_mode.useCBR = 1-value;
        }
        break;
        case OPUS_GET_VBR_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->use_vbr;
        }
        break;
        case OPUS_SET_VOICE_RATIO_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if (value<-1 || value>100)
            {
               goto bad_arg;
            }
            st->voice_ratio = value;
        }
        break;
        case OPUS_GET_VOICE_RATIO_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->voice_ratio;
        }
        break;
        case OPUS_SET_VBR_CONSTRAINT_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if(value<0 || value>1)
            {
               goto bad_arg;
            }
            st->vbr_constraint = value;
        }
        break;
        case OPUS_GET_VBR_CONSTRAINT_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->vbr_constraint;
        }
        break;
        case OPUS_SET_SIGNAL_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if(value!=OPUS_AUTO && value!=OPUS_SIGNAL_VOICE && value!=OPUS_SIGNAL_MUSIC)
            {
               goto bad_arg;
            }
            st->signal_type = value;
        }
        break;
        case OPUS_GET_SIGNAL_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->signal_type;
        }
        break;
        case OPUS_GET_LOOKAHEAD_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->Fs/400;
            if (st->application != OPUS_APPLICATION_RESTRICTED_LOWDELAY)
                *value += st->delay_compensation;
        }
        break;
        case OPUS_GET_SAMPLE_RATE_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->Fs;
        }
        break;
        case OPUS_GET_FINAL_RANGE_REQUEST:
        {
            opus_uint32 *value = va_arg(ap, opus_uint32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->rangeFinal;
        }
        break;
        case OPUS_SET_LSB_DEPTH_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if (value<8 || value>24)
            {
               goto bad_arg;
            }
            st->lsb_depth=value;
        }
        break;
        case OPUS_GET_LSB_DEPTH_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->lsb_depth;
        }
        break;
        case OPUS_SET_EXPERT_FRAME_DURATION_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if (value != OPUS_FRAMESIZE_ARG    && value != OPUS_FRAMESIZE_2_5_MS &&
                value != OPUS_FRAMESIZE_5_MS   && value != OPUS_FRAMESIZE_10_MS  &&
                value != OPUS_FRAMESIZE_20_MS  && value != OPUS_FRAMESIZE_40_MS  &&
                value != OPUS_FRAMESIZE_60_MS  && value != OPUS_FRAMESIZE_80_MS  &&
                value != OPUS_FRAMESIZE_100_MS && value != OPUS_FRAMESIZE_120_MS)
            {
               goto bad_arg;
            }
            st->variable_duration = value;
        }
        break;
        case OPUS_GET_EXPERT_FRAME_DURATION_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            *value = st->variable_duration;
        }
        break;
        case OPUS_SET_PREDICTION_DISABLED_REQUEST:
        {
           opus_int32 value = va_arg(ap, opus_int32);
           if (value > 1 || value < 0)
              goto bad_arg;
           st->silk_mode.reducedDependency = value;
        }
        break;
        case OPUS_GET_PREDICTION_DISABLED_REQUEST:
        {
           opus_int32 *value = va_arg(ap, opus_int32*);
           if (!value)
              goto bad_arg;
           *value = st->silk_mode.reducedDependency;
        }
        break;
        case OPUS_SET_PHASE_INVERSION_DISABLED_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if(value<0 || value>1)
            {
               goto bad_arg;
            }
            celt_encoder_ctl(celt_enc, OPUS_SET_PHASE_INVERSION_DISABLED(value));
        }
        break;
        case OPUS_GET_PHASE_INVERSION_DISABLED_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
               goto bad_arg;
            }
            celt_encoder_ctl(celt_enc, OPUS_GET_PHASE_INVERSION_DISABLED(value));
        }
        break;
        case OPUS_RESET_STATE:
        {
           void *silk_enc;
           silk_EncControlStruct dummy;
           char *start;
           silk_enc = (char*)st+st->silk_enc_offset;
           tonality_analysis_reset(&st->analysis);

           start = (char*)&st->OPUS_ENCODER_RESET_START;
           OPUS_CLEAR(start, sizeof(OpusEncoder) - (start - (char*)st));

           celt_encoder_ctl(celt_enc, OPUS_RESET_STATE);
           silk_InitEncoder( silk_enc, st->arch, &dummy );
           st->stream_channels = st->channels;
           st->hybrid_stereo_width_Q14 = 1 << 14;
           st->prev_HB_gain = Q15ONE;
           st->first = 1;
           st->mode = MODE_HYBRID;
           st->bandwidth = OPUS_BANDWIDTH_FULLBAND;
           st->variable_HP_smth2_Q15 = silk_LSHIFT( silk_lin2log( VARIABLE_HP_MIN_CUTOFF_HZ ), 8 );
        }
        break;
        case OPUS_SET_FORCE_MODE_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            if ((value < MODE_SILK_ONLY || value > MODE_CELT_ONLY) && value != OPUS_AUTO)
            {
               goto bad_arg;
            }
            st->user_forced_mode = value;
        }
        break;
        case OPUS_SET_LFE_REQUEST:
        {
            opus_int32 value = va_arg(ap, opus_int32);
            st->lfe = value;
            ret = celt_encoder_ctl(celt_enc, OPUS_SET_LFE(value));
        }
        break;
        case OPUS_SET_ENERGY_MASK_REQUEST:
        {
            opus_val16 *value = va_arg(ap, opus_val16*);
            st->energy_masking = value;
            ret = celt_encoder_ctl(celt_enc, OPUS_SET_ENERGY_MASK(value));
        }
        break;
        case OPUS_GET_IN_DTX_REQUEST:
        {
            opus_int32 *value = va_arg(ap, opus_int32*);
            if (!value)
            {
                goto bad_arg;
            }
            if (st->silk_mode.useDTX && (st->prev_mode == MODE_SILK_ONLY || st->prev_mode == MODE_HYBRID)) {
                /* DTX determined by Silk. */
                *value = 0; /* SILK not included */
            }
            else if (st->use_dtx) {
                /* DTX determined by Opus. */
                *value = st->nb_no_activity_ms_Q1 >= NB_SPEECH_FRAMES_BEFORE_DTX*20*2;
            }
            else {
                *value = 0;
            }
        }
        break;
        case CELT_GET_MODE_REQUEST:
        {
           const CELTMode ** value = va_arg(ap, const CELTMode**);
           if (!value)
           {
              goto bad_arg;
           }
           ret = celt_encoder_ctl(celt_enc, CELT_GET_MODE(value));
        }
        break;
        default:
            /* fprintf(stderr, "unknown opus_encoder_ctl() request: %d", request);*/
            ret = OPUS_UNIMPLEMENTED;
            break;
    }
    va_end(ap);
    return ret;
bad_arg:
    va_end(ap);
    return OPUS_BAD_ARG;
}
void opus_encoder_destroy(OpusEncoder *st)
{
    opus_free(st);
}
/* Approximation of 128 * log2() (very close inverse of silk_log2lin()) */
/* Convert input to a log scale    */
opus_int32 silk_lin2log(
    const opus_int32            inLin               /* I  input in linear scale                                         */
)
{
    opus_int32 lz, frac_Q7;

    silk_CLZ_FRAC( inLin, &lz, &frac_Q7 );

    /* Piece-wise parabolic approximation */
    return silk_ADD_LSHIFT32( silk_SMLAWB( frac_Q7, silk_MUL( frac_Q7, 128 - frac_Q7 ), 179 ), 31 - lz, 7 );
}
/* Approximation of 2^() (very close inverse of silk_lin2log()) */
/* Convert input to a linear scale    */
opus_int32 silk_log2lin(
    const opus_int32            inLog_Q7            /* I  input on log scale                                            */
)
{
    opus_int32 out, frac_Q7;

    if( inLog_Q7 < 0 ) {
        return 0;
    } else if ( inLog_Q7 >= 3967 ) {
        return silk_int32_MAX;
    }

    out = silk_LSHIFT( 1, silk_RSHIFT( inLog_Q7, 7 ) );
    frac_Q7 = inLog_Q7 & 0x7F;
    if( inLog_Q7 < 2048 ) {
        /* Piece-wise parabolic approximation */
        out = silk_ADD_RSHIFT32( out, silk_MUL( out, silk_SMLAWB( frac_Q7, silk_SMULBB( frac_Q7, 128 - frac_Q7 ), -174 ) ), 7 );
    } else {
        /* Piece-wise parabolic approximation */
        out = silk_MLA( out, silk_RSHIFT( out, 7 ), silk_SMLAWB( frac_Q7, silk_SMULBB( frac_Q7, 128 - frac_Q7 ), -174 ) );
    }
    return out;
}
/* SILK is not included: libopus only uses it for low bitrates, speech and hybrid mode. */
opus_int silk_Get_Encoder_Size( opus_int *encSizeBytes ) { *encSizeBytes = 0; return 0; }
opus_int silk_InitEncoder( void *encState, int arch, silk_EncControlStruct *encStatus ) { (void)encState; (void)arch; (void)encStatus; return 0; }
opus_int silk_Encode( void *encState, silk_EncControlStruct *encControl, const opus_int16 *samplesIn, opus_int nSamplesIn, ec_enc *psRangeEnc, opus_int32 *nBytesOut, const opus_int prefillFlag, int activity ) {
    (void)encState; (void)encControl; (void)samplesIn; (void)nSamplesIn; (void)psRangeEnc; (void)nBytesOut; (void)prefillFlag; (void)activity;
    return -1;
}

/* ---- internal macros are hidden from the includer ---- */
#undef ABS16
#undef ABS32
#undef ADD16
#undef ADD32
#undef ADD32_ovflw
#undef ALLOC
#undef ALLOC_NONE
#undef ALLOC_STACK
#undef ALLOC_STEPS
#undef ANALYSIS_BUF_SIZE
#undef ANALYSIS_COUNT_MAX
#undef BITRES
#undef CELTEncoder
#undef CELTMode
#undef CELT_GET_MODE
#undef CELT_GET_MODE_REQUEST
#undef CELT_PVQ_U
#undef CELT_PVQ_V
#undef CELT_SET_ANALYSIS
#undef CELT_SET_ANALYSIS_REQUEST
#undef CELT_SET_CHANNELS
#undef CELT_SET_CHANNELS_REQUEST
#undef CELT_SET_END_BAND
#undef CELT_SET_END_BAND_REQUEST
#undef CELT_SET_PREDICTION
#undef CELT_SET_PREDICTION_REQUEST
#undef CELT_SET_SIGNALLING
#undef CELT_SET_SIGNALLING_REQUEST
#undef CELT_SET_SILK_INFO
#undef CELT_SET_SILK_INFO_REQUEST
#undef CELT_SET_START_BAND
#undef CELT_SET_START_BAND_REQUEST
#undef CELT_SIG_SCALE
#undef CHECK_OVERFLOW_OP
#undef COMBFILTER_MAXPERIOD
#undef COMBFILTER_MINPERIOD
#undef C_ADD
#undef C_ADDTO
#undef C_MUL
#undef C_MULBYSCALAR
#undef C_SUB
#undef DETECT_SIZE
#undef DIV32
#undef DIV32_16
#undef DTX_ACTIVITY_THRESHOLD
#undef EC_CLZ
#undef EC_CLZ0
#undef EC_CODE_BITS
#undef EC_CODE_BOT
#undef EC_CODE_EXTRA
#undef EC_CODE_SHIFT
#undef EC_CODE_TOP
#undef EC_ILOG
#undef EC_MINI
#undef EC_SYM_BITS
#undef EC_SYM_MAX
#undef EC_UINT_BITS
#undef EC_WINDOW_SIZE
#undef EPSILON
#undef EXTEND32
#undef EXTRACT16
#undef FINE_OFFSET
#undef FRAC_MUL16
#undef HALF16
#undef HALF32
#undef HALF_OF
#undef IMAX
#undef IMIN
#undef IMUL32
#undef KISS_TWIDDLE_CPX_ALIGNED
#undef LAPLACE_LOG_MINP
#undef LAPLACE_MINP
#undef LAPLACE_NMIN
#undef LEAKAGE_OFFSET
#undef LEAKAGE_SLOPE
#undef LEAK_BANDS
#undef LOG_MAX_PSEUDO
#undef MAC16_16
#undef MAC16_32_Q15
#undef MAX16
#undef MAX32
#undef MAXFACTORS
#undef MAX_CONSECUTIVE_DTX
#undef MAX_ENCODER_BUFFER
#undef MAX_FINE_BITS
#undef MAX_NEURONS
#undef MIN16
#undef MIN32
#undef MODE_CELT_ONLY
#undef MODE_HYBRID
#undef MODE_SILK_ONLY
#undef MSWAP
#undef MULT16_16
#undef MULT16_16_P15
#undef MULT16_16_Q14
#undef MULT16_16_Q15
#undef MULT16_32_Q15
#undef MULT16_32_Q16
#undef MULT32_32_Q31
#undef NB_FRAMES
#undef NB_SPEECH_FRAMES_BEFORE_DTX
#undef NB_TBANDS
#undef NB_TONAL_SKIP_BANDS
#undef NEG16
#undef NEG32_ovflw
#undef NORM_SCALING
#undef OPUS_ARG_NONNULL
#undef OPUS_CLEAR
#undef OPUS_COPY
#undef OPUS_CUSTOM_EXPORT
#undef OPUS_CUSTOM_EXPORT_STATIC
#undef OPUS_CUSTOM_NOSTATIC
#undef OPUS_EXPORT
#undef OPUS_INLINE
#undef OPUS_MOVE
#undef OPUS_RESET_STATE
#undef OPUS_RESTRICT
#undef OPUS_WARN_UNUSED_RESULT
#undef PI
#undef PSEUDO_SNR_THRESHOLD
#undef PSHR32
#undef Q15ONE
#undef Q15_ONE
#undef QCONST16
#undef QCONST32
#undef QTHETA_OFFSET
#undef QTHETA_OFFSET_TWOPHASE
#undef RESTORE_STACK
#undef ROUND16
#undef SATURATE
#undef SAVE_STACK
#undef SCALEIN
#undef SCALE_ENER
#undef SHL16
#undef SHL32
#undef SHR16
#undef SHR32
#undef SILK_FIX_CONST
#undef SPREAD_AGGRESSIVE
#undef SPREAD_LIGHT
#undef SPREAD_NONE
#undef SPREAD_NORMAL
#undef SROUND16
#undef SUB16
#undef SUB32
#undef SUB32_ovflw
#undef S_MUL
#undef TOTAL_MODES
#undef TRANSITION_PENALTY
#undef VAD_NO_DECISION
#undef VARDECL
#undef VARIABLE_HP_MIN_CUTOFF_HZ
#undef VARIABLE_HP_SMTH_COEF2
#undef VERY_SMALL
#undef VSHR32
#undef WEIGHTS_SCALE
#undef __celt_check_analysis_ptr
#undef __celt_check_mode_ptr_ptr
#undef __celt_check_silkinfo_ptr
#undef cA
#undef cB
#undef cC
#undef cE
#undef celt_assert
#undef celt_assert2
#undef celt_cos_norm
#undef celt_div
#undef celt_encoder_ctl
#undef celt_exp2
#undef celt_inner_prod
#undef celt_isnan
#undef celt_log2
#undef celt_pitch_xcorr
#undef celt_rcp
#undef celt_rsqrt
#undef celt_rsqrt_norm
#undef celt_sig_assert
#undef celt_sqrt
#undef clt_mdct_forward
#undef comb_filter_const
#undef dual_inner_prod
#undef float2int
#undef fmadd
#undef frac_div32
#undef is_digital_silence32
#undef kiss_fft_scalar
#undef kiss_twiddle_scalar
#undef op_pvq_search
#undef opus_fft
#undef opus_int
#undef opus_int64
#undef opus_int8
#undef opus_unlikely
#undef silk_ADD32
#undef silk_ADD_LSHIFT32
#undef silk_ADD_RSHIFT32
#undef silk_DIV32_16
#undef silk_LSHIFT
#undef silk_LSHIFT32
#undef silk_MLA
#undef silk_MUL
#undef silk_RSHIFT
#undef silk_RSHIFT32
#undef silk_SMLAWB
#undef silk_SMULBB
#undef silk_SMULWB
#undef silk_SMULWW
#undef silk_assert
#undef silk_int32_MAX
#undef silk_min
#undef xcorr_kernel

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif /* OPUSENC_H */