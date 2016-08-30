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

struct hdcd_simple {
    hdcd_state_stereo state;
    hdcd_detection_data detect;
    hdcd_log logger;
    int smode;
};

/** set stereo processing mode, only used internally */
static void hdcd_smode(hdcd_simple *s, int mode)
{
    if (!s) return;
    if (mode != 0 && mode != 1) return;
    /* TODO: this could be more careful when switching in
     * the middle of processing, re: last valid tg,
     * but not critical. */
    s->smode = mode;
}

/** create a new hdcd_simple context */
hdcd_simple *hdcd_new(void)
{
    hdcd_simple *s = malloc(sizeof(*s));
    if (s) {
        memset(s, 0, sizeof(*s));
        hdcd_reset(s);
    }
    return s;
}

static void _hdcd_simple_reset(hdcd_state_stereo *state)
{
    if (!state) return;
    _hdcd_reset_stereo_ext(state, 44100, 2000, HDCD_FLAG_TGM_LOG_OFF, HDCD_ANA_OFF, NULL);
}

/** on a song change or something, reset the decoding state */
void hdcd_reset(hdcd_simple *s)
{
    if (!s) return;
    _hdcd_simple_reset(&s->state);
    _hdcd_detect_reset(&s->detect);
    hdcd_smode(s, 1);
    hdcd_analyze_mode(s, 0);
}

static void _hdcd_check_samples(hdcd_simple *s, int *samples, int count) {
    int i, top = 0, bottom = 0;
    if (!s) return;

    /* check if the samples were already converted to s32 */
    for (i = 0; i < count * 2; i++) {
        if (samples[i] > 32767 || samples[i] < -32768) top++;
        if (samples[i] & 0x0000ffff) bottom++;
    }
    if (top) {
        _hdcd_log(&s->logger, "hdcd: s32 samples detected as input, shifting to s16. (%d:%d)\n", top, bottom);
        for (i = 0; i < count * 2; i++)
            samples[i] >>= 16;
    }
}

/** process signed 16-bit samples (stored in 32-bit), interlaced stereo, 44100Hz */
void hdcd_process(hdcd_simple *s, int *samples, int count)
{
    if (!s) return;

    _hdcd_check_samples(s, samples, count);

    if (s->smode)
        /* process stereo channels together */
        _hdcd_process_stereo(&s->state, samples, count);
    else {
        /* independently process each channel */
        _hdcd_process(&s->state.channel[0], samples, count, 2);
        _hdcd_process(&s->state.channel[1], samples + 1, count, 2);
    }
    _hdcd_detect_stereo(&s->state, &s->detect);
}

/*hdcd_dv*/
int hdcd_scan(hdcd_simple *s, int *samples, int count, int ignore_state)
{
    hdcd_state_stereo st;
    hdcd_detection_data d;
    int buf_size = sizeof(int) * count * 2;
    int *samp;
    if (!s) return 0;
    /* The simplest way to do this is to copy everything and process.
     * Perhaps later, a more efficient way can be implemented using
     * calls to _hdcd_scan_stereo() until the first effectual packet
     * is found */
    if (ignore_state) {
        _hdcd_simple_reset(&st);
        _hdcd_detect_reset(&d);
    } else {
        memcpy(&st, &s->state, sizeof(hdcd_state_stereo));
        memcpy(&d, &s->detect, sizeof(hdcd_detection_data));
    }
    if (d.hdcd_detected == HDCD_EFFECTUAL) return
        d.hdcd_detected; /* easy peasy */
    samp = malloc(buf_size);
    if (samp) {
        memcpy(samp, samples, buf_size);
        _hdcd_check_samples(s, samp, count);
        _hdcd_process_stereo(&st, samp, count);
        _hdcd_detect_stereo(&st, &d);
        free(samp);
        return d.hdcd_detected;
    } else return 0;

    /* possible alternate method:
    *samp = samples;
    int c = count;
    while (c) {
        c -= _hdcd_scan_stereo(&st, samp_scan, c);
        if (st.channel[0].count_peak_extend
            || st.channel[1].count_peak_extend
            || st.channel[0].max_gain
            || st.channel[1].max_gain)
            return HDCD_EFFECTUAL;
    }
    if (packets)
        return HDCD_NO_EFFECT;
    else
        return HDCD_NONE;
    */
}

/** free the context when finished */
void hdcd_free(hdcd_simple *s)
{
    if(s) free(s);
}

/** Is HDCD encoding detected? */
/*hdcd_dv*/
int hdcd_detected(hdcd_simple *s)
{
    if (!s) return 0;
    return s->detect.hdcd_detected;
}

/*hdcd_pf*/
int hdcd_detect_packet_type(hdcd_simple *ctx)
{ if (ctx) return ctx->detect.packet_type; else return 0; }

int hdcd_detect_total_packets(hdcd_simple *ctx)
{ if (ctx) return ctx->detect.total_packets; else return 0; }

int hdcd_detect_errors(hdcd_simple *ctx)
{ if (ctx) return ctx->detect.errors; else return 0; }

/*hdcd_pe*/
int hdcd_detect_peak_extend(hdcd_simple *ctx)
{ if (ctx) return ctx->detect.peak_extend; else return 0; }

int hdcd_detect_uses_transient_filter(hdcd_simple *ctx)
{ if (ctx) return ctx->detect.uses_transient_filter; else return 0; }

float hdcd_detect_max_gain_adjustment(hdcd_simple *ctx)
{ if (ctx) return ctx->detect.max_gain_adjustment; else return 0.0; }

int hdcd_detect_cdt_expirations(hdcd_simple *ctx)
{ if (ctx) return ctx->detect.cdt_expirations; else return -1; }

/** get a string with an HDCD detection summary */
void hdcd_detect_str(hdcd_simple *s, char *str, int maxlen)
{
    if (!s || !str) return;
    _hdcd_detect_str(&s->detect, str, maxlen);
}

int hdcd_logger_attach(hdcd_simple *s, hdcd_log_callback func, void *priv)
{
    if (!s) return 0;
    if (!func) return 0;
    _hdcd_log_init_ext(&s->logger, func, priv);
    _hdcd_attach_logger_stereo(&s->state, &s->logger);
    return 1;
}

void hdcd_logger_default(hdcd_simple *s)
{
    if (!s) return;
    _hdcd_log_init_ext(&s->logger, NULL, NULL);
    _hdcd_attach_logger_stereo(&s->state, &s->logger);
}

void hdcd_logger_detach(hdcd_simple *s)
{
    if (!s) return;
    /* just reset to the default and then disable */
    _hdcd_log_init_ext(&s->logger, NULL, NULL);
    _hdcd_attach_logger_stereo(&s->state, &s->logger);
    _hdcd_log_disable(&s->logger);
}

int hdcd_analyze_mode(hdcd_simple *s, int mode)
{
    if (!s) return 0;

    /* clear HDCD_FLAG_FORCE_PE for all, and set it
     * in the one mode that will use it  */
    s->state.channel[0].decoder_options &= ~HDCD_FLAG_FORCE_PE;
    s->state.channel[1].decoder_options &= ~HDCD_FLAG_FORCE_PE;

    switch(mode) {
        case HDCD_ANA_OFF:
        case HDCD_ANA_LLE:
        case HDCD_ANA_PE:
        case HDCD_ANA_CDT:
        case HDCD_ANA_TGM:
            hdcd_smode(s, 1);
            _hdcd_set_analyze_mode_stereo(&s->state, mode);
            return 1;
        case HDCD_ANA_PEL:     /* HDCD_ANA_PE + HDCD_FLAG_FORCE_PE */
            hdcd_smode(s, 1);
            s->state.channel[0].decoder_options |= HDCD_FLAG_FORCE_PE;
            s->state.channel[1].decoder_options |= HDCD_FLAG_FORCE_PE;
            _hdcd_set_analyze_mode_stereo(&s->state, HDCD_ANA_PE);
            return 1;
        case HDCD_ANA_LTGM:   /* HDCD_ANA_LLE + stereo_mode off */
            hdcd_smode(s, 0);
            _hdcd_set_analyze_mode_stereo(&s->state, HDCD_ANA_LLE);
            return 1;
    }
    return 0;
}

const char* hdcd_str_analyze_mode_desc(hdcd_ana_mode mode)
{
    static const char * const ana_mode_str[] = {
        HDCD_ANA_OFF_DESC,
        HDCD_ANA_LLE_DESC,
        HDCD_ANA_PE_DESC,
        HDCD_ANA_CDT_DESC,
        HDCD_ANA_TGM_DESC,
        HDCD_ANA_PEL_DESC,
        HDCD_ANA_LTGM_DESC,
    };
    if (mode < 0 || mode > 6) return "";
    return ana_mode_str[mode];
}
