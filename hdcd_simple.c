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

#include <stdlib.h>
#include "hdcd_decode2.h"
#include "hdcd_simple.h"

typedef struct {
    hdcd_state_stereo_t state;
    hdcd_detection_data_t detect;
    hdcd_log_t logger;
} hdcd_simple_ctx_t;

/** create a new hdcd_simple context */
hdcd_simple_t shdcd_new()
{
    hdcd_simple_ctx_t *s = malloc(sizeof(hdcd_simple_ctx_t));
    if (s)
        shdcd_reset(s);
    return s;
}

/** on a song change or something, reset the decoding state */
void shdcd_reset(hdcd_simple_t ctx)
{
    hdcd_simple_ctx_t *s = ctx;
    if (!s) return;
    hdcd_reset_stereo_ext(&s->state, 44100, 2000, HDCD_FLAG_TGM_LOG_OFF, HDCD_ANA_OFF, NULL);
}

/** process signed 16-bit samples (stored in 32-bit), interlaced stereo, 44100Hz */
void shdcd_process(hdcd_simple_t ctx, int *samples, int count)
{
    hdcd_simple_ctx_t *s = ctx;
    if (!ctx) return;
    hdcd_process_stereo(&s->state, samples, count);
    hdcd_detect_stereo(&s->state, &s->detect);
}

/** free the context when finished */
void shdcd_free(hdcd_simple_t ctx)
{
    hdcd_simple_ctx_t *s = ctx;
    if(s) free(s);
    ctx = NULL;
}

/** Is HDCD encoding detected? */
int shdcd_detected(hdcd_simple_t ctx)
{
    hdcd_simple_ctx_t *s = ctx;
    if (!s) return 0;
    return s->detect.hdcd_detected;
}

/** get a string with an HDCD detection summary */
void shdcd_detect_str(hdcd_simple_t ctx, char *str, int maxlen)
{
    hdcd_simple_ctx_t *s = ctx;
    if (!s || !str) return;
    hdcd_detect_str(&s->detect, str, maxlen);
}

int shdcd_attach_logger(hdcd_simple_t ctx, hdcd_log_callback func, void *priv)
{
    hdcd_simple_ctx_t *s = ctx;
    if (!s) return -1;
    hdcd_log_init_ext(&s->logger, func, priv);
    hdcd_attach_logger_stereo(&s->state, &s->logger);
}

void shdcd_default_logger(hdcd_simple_t ctx)
{
    hdcd_simple_ctx_t *s = ctx;
    if (!s) return;
    hdcd_log_init_ext(&s->logger, NULL, NULL);
    hdcd_attach_logger_stereo(&s->state, &s->logger);
}

void shdcd_detach_logger(hdcd_simple_t ctx) {
    hdcd_simple_ctx_t *s = ctx;
    if (!s) return;
    /* just reset to the default and then disable */
    hdcd_log_init_ext(&s->logger, NULL, NULL);
    hdcd_attach_logger_stereo(&s->state, &s->logger);
    hdcd_log_disable(&s->logger);
}
