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
#include <string.h>
#include "hdcd_decode2.h"
#include "hdcd_simple.h"

struct hdcd_simple_t {
    hdcd_state_stereo_t state;
    hdcd_detection_data_t detect;
    hdcd_log_t logger;
    int smode;
};

/** set stereo processing mode, only used internally */
static void shdcd_smode(hdcd_simple_t *s, int mode)
{
    if (!s) return;
    if (mode != 0 && mode != 1) return;
    /* TODO: this needs to be more careful when switching in
     * the middle of processing */
    s->smode = mode;
}

/** create a new hdcd_simple context */
hdcd_simple_t *shdcd_new(void)
{
    hdcd_simple_t *s = malloc(sizeof(*s));
    if (s) {
        memset(s, 0, sizeof(*s));
        shdcd_reset(s);
    }
    return s;
}

/** on a song change or something, reset the decoding state */
void shdcd_reset(hdcd_simple_t *s)
{
    if (!s) return;
    hdcd_reset_stereo_ext(&s->state, 44100, 2000, HDCD_FLAG_TGM_LOG_OFF, HDCD_ANA_OFF, NULL);
    shdcd_smode(s, 1);
    shdcd_analyze_mode(s, 0);
}

/** process signed 16-bit samples (stored in 32-bit), interlaced stereo, 44100Hz */
void shdcd_process(hdcd_simple_t *s, int *samples, int count)
{
    if (!s) return;
    if (s->smode)
        /* process stereo channels together */
        hdcd_process_stereo(&s->state, samples, count);
    else {
        /* independently process each channel */
        hdcd_process(&s->state.channel[0], samples, count, 2);
        hdcd_process(&s->state.channel[1], samples + 1, count, 2);
    }
    hdcd_detect_stereo(&s->state, &s->detect);
}

/** free the context when finished */
void shdcd_free(hdcd_simple_t *s)
{
    if(s) free(s);
}

/** Is HDCD encoding detected? */
int shdcd_detected(hdcd_simple_t *s)
{
    if (!s) return 0;
    return s->detect.hdcd_detected;
}

/** get a string with an HDCD detection summary */
void shdcd_detect_str(hdcd_simple_t *s, char *str, int maxlen)
{
    if (!s || !str) return;
    hdcd_detect_str(&s->detect, str, maxlen);
}

int shdcd_attach_logger(hdcd_simple_t *s, hdcd_log_callback func, void *priv)
{
    if (!s) return 0;
    if (!func) return 0;
    hdcd_log_init_ext(&s->logger, func, priv);
    hdcd_attach_logger_stereo(&s->state, &s->logger);
    return 1;
}

void shdcd_default_logger(hdcd_simple_t *s)
{
    if (!s) return;
    hdcd_log_init_ext(&s->logger, NULL, NULL);
    hdcd_attach_logger_stereo(&s->state, &s->logger);
}

void shdcd_detach_logger(hdcd_simple_t *s)
{
    if (!s) return;
    /* just reset to the default and then disable */
    hdcd_log_init_ext(&s->logger, NULL, NULL);
    hdcd_attach_logger_stereo(&s->state, &s->logger);
    hdcd_log_disable(&s->logger);
}

int shdcd_analyze_mode(hdcd_simple_t *s, int mode)
{
    if (!s) return 0;

    /* clear HDCD_FLAG_FORCE_PE for all, and set it
     * in the one mode that will use it  */
    s->state.channel[0].decoder_options &= ~HDCD_FLAG_FORCE_PE;
    s->state.channel[1].decoder_options &= ~HDCD_FLAG_FORCE_PE;

    switch(mode) {
        case SHDCD_ANA_OFF:     /* identical to HDCD_ANA_OFF */
        case SHDCD_ANA_LLE:     /* identical to HDCD_ANA_LLE */
        case SHDCD_ANA_PE:      /* identical to HDCD_ANA_PE  */
        case SHDCD_ANA_CDT:     /* identical to HDCD_ANA_CDT */
        case SHDCD_ANA_TGM:     /* identical to HDCD_ANA_TGM */
            shdcd_smode(s, 1);
            hdcd_set_analyze_mode_stereo(&s->state, mode);
            return 1;
        case SHDCD_ANA_PEL:     /* HDCD_ANA_PE + HDCD_FLAG_FORCE_PE */
            shdcd_smode(s, 1);
            s->state.channel[0].decoder_options |= HDCD_FLAG_FORCE_PE;
            s->state.channel[1].decoder_options |= HDCD_FLAG_FORCE_PE;
            hdcd_set_analyze_mode_stereo(&s->state, HDCD_ANA_PE);
            return 1;
        case SHDCD_ANA_LTGM: /* HDCD_ANA_LLE + stereo_mode off */
            shdcd_smode(s, 0);
            hdcd_set_analyze_mode_stereo(&s->state, HDCD_ANA_LLE);
            return 1;
    }
    return 0;
}

const char* shdcd_analyze_mode_desc(int mode)
{
    static const char * const ana_mode_str[] = {
        HDCD_ANA_OFF_DESC,
        HDCD_ANA_LLE_DESC,
        HDCD_ANA_PE_DESC,
        HDCD_ANA_CDT_DESC,
        HDCD_ANA_TGM_DESC,
        SHDCD_ANA_PEL_DESC,
        SHDCD_ANA_LTGM_DESC,
    };
    if (mode < 0 || mode > 6) return "???";
    return ana_mode_str[mode];
}
