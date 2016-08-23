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

#ifdef __cplusplus
extern "C" {
#endif

typedef void* hdcd_simple_t;

/** create a new hdcd_simple context */
hdcd_simple_t shdcd_new();
/** process 16-bit samples (stored in 32-bit), interlaced stereo, 44100Hz */
void shdcd_process(hdcd_simple_t ctx, int *samples, int count);
/** on a song change or something, reset the decoding state */
void shdcd_reset(hdcd_simple_t ctx);
/** free the context when finished */
void shdcd_free(hdcd_simple_t ctx);


/** is HDCD encoding detected? */
int shdcd_detected(hdcd_simple_t ctx);
/** get a string with an HDCD detection summary */
void shdcd_detect_str(hdcd_simple_t ctx, char *str, int maxlen); /* [256] should be enough */


/** set a logging callback or use the default (print to stderr) */
typedef void (*hdcd_log_callback)(const void *priv, const char* fmt, va_list args);
int shdcd_attach_logger(hdcd_simple_t ctx, hdcd_log_callback func, void *priv);
void shdcd_default_logger(hdcd_simple_t ctx);
void shdcd_detach_logger(hdcd_simple_t ctx);


#ifdef __cplusplus
}
#endif

#endif

