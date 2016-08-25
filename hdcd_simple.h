/*
 *  Copyright (C) 2016, Burt P.,
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. The names of its contributors may not be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _HDCD_SIMPLE_H_
#define _HDCD_SIMPLE_H_

#include <stdarg.h>
#include "hdcd_libversion.h"
#include "hdcd_detect.h"         /* enums for various detection values */
#include "hdcd_analyze.h"        /* enums and definitions for analyze modes */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hdcd_simple_t hdcd_simple_t;

/** create a new hdcd_simple context */
hdcd_simple_t *shdcd_new(void);
/** process 16-bit samples (stored in 32-bit), interlaced stereo, 44100Hz.
 *  the samples will be converted in place to 32-bit samples. */
void shdcd_process(hdcd_simple_t *ctx, int *samples, int count);
/** on a song change or something, reset the decoding state */
void shdcd_reset(hdcd_simple_t *ctx);
/** free the context when finished */
void shdcd_free(hdcd_simple_t *ctx);


/** is HDCD encoding detected? */
/*hdcd_detection_t*/ int shdcd_detected(hdcd_simple_t *ctx);
/** get a string with an HDCD detection summary */
void shdcd_detect_str(hdcd_simple_t *ctx, char *str, int maxlen); /* [256] should be enough */
/** get individual detection values */
/*hdcd_pf_t*/ int shdcd_detect_packet_type(hdcd_simple_t *ctx);
              int shdcd_detect_total_packets(hdcd_simple_t *ctx);         /**< valid packets */
              int shdcd_detect_errors(hdcd_simple_t *ctx);                /**< detectable errors */
/*hdcd_pe_t*/ int shdcd_detect_peak_extend(hdcd_simple_t *ctx);
              int shdcd_detect_uses_transient_filter(hdcd_simple_t *ctx);
              float shdcd_detect_max_gain_adjustment(hdcd_simple_t *ctx); /**< in dB, expected in the range -7.5 to 0.0 */
              int shdcd_detect_cdt_expirations(hdcd_simple_t *ctx);       /**< -1 for never set, 0 for set but never expired */


/** set a logging callback or use the default (print to stderr) */
typedef void (*hdcd_log_callback)(const void *priv, const char* fmt, va_list args);
int shdcd_attach_logger(hdcd_simple_t *ctx, hdcd_log_callback func, void *priv);
void shdcd_default_logger(hdcd_simple_t *ctx);
void shdcd_detach_logger(hdcd_simple_t *ctx);


/** Analyze mode(s)
 *
 *   In analyze mode, the audio is replaced by a solid tone, and
 *   amplitude is changed to signal when the specified feature is
 *   used, or some decoding state exists.
 * 0-4 match the HDCD_ANA_* values
 * 5,6 are additional modes that are required because direct acces to
 *     the internal state is not available in the simple API. */
typedef enum {
    SHDCD_ANA_OFF     = 0, /**< disabled */
    SHDCD_ANA_LLE     = 1, /**< LLE matching level at each sample */
    SHDCD_ANA_PE      = 2, /**< samples where PE occurs */
    SHDCD_ANA_CDT     = 3, /**< samples where the code detect timer is active */
    SHDCD_ANA_TGM     = 4, /**< samples where target_gain is not matching in each channel */
    SHDCD_ANA_PEL     = 5, /**< any samples above PE level */
    SHDCD_ANA_LTGM    = 6, /**< LLE level in each channel at each sample */
} shdcd_ana_mode_t;
/* HDCD_ANA_*_DESC can be used for 0-4
 * the two extra modes are described here: */
#define SHDCD_ANA_PEL_DESC  "any samples above peak extend level"
#define SHDCD_ANA_LTGM_DESC "gain adjustment level at each sample, in each channel"

/** set the analyze mode */
int shdcd_analyze_mode(hdcd_simple_t *ctx, int mode);
/** get a nice description of what a mode does */
const char* shdcd_analyze_mode_desc(int mode);

#ifdef __cplusplus
}
#endif

#endif

