/*
 *  Copyright (C) 2010, Chris Moeller,
 *  All rights reserved.
 *  Optimizations by Gumboot
 *  Additional work by Burt P.
 *  Original code reverse engineered from HDCD decoder library by Christopher Key,
 *  which was likely reverse engineered from Windows Media Player.
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

/*
 * HDCD is High Definition Compatible Digital
 * http://wiki.hydrogenaud.io/index.php?title=High_Definition_Compatible_Digital
 *
 * More information about HDCD-encoded audio CDs:
 * http://www.audiomisc.co.uk/HFN/HDCD/Enigma.html
 * http://www.audiomisc.co.uk/HFN/HDCD/Examined.html
 */

#ifndef _HDCD_DECODE2_H_
#define _HDCD_DECODE2_H_

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/********************* optional logging ************************/

typedef void (*hdcd_log_callback)(const void *priv, const char* fmt, va_list args);

typedef struct {
    int enable;
    void *priv;
    hdcd_log_callback log_func;
} hdcd_log_t;

void hdcd_log_init(hdcd_log_t *log, hdcd_log_callback func, void *priv);
void hdcd_log(hdcd_log_t *log, const char* fmt, ...);
void hdcd_log_enable(hdcd_log_t *log);
void hdcd_log_disable(hdcd_log_t *log);

/********************* optional analyze mode *******************/

typedef enum {
    HDCD_ANA_OFF    = 0,
    HDCD_ANA_LLE    = 1,
    HDCD_ANA_PE     = 2,
    HDCD_ANA_CDT    = 3,
    HDCD_ANA_TGM    = 4,
} hdcd_ana_mode_t;

#define HDCD_ANA_OFF_DESC "disabled"
#define HDCD_ANA_LLE_DESC "gain adjustment level at each sample"
#define HDCD_ANA_PE_DESC  "samples where peak extend occurs"
#define HDCD_ANA_CDT_DESC "samples where the code detect timer is active"
#define HDCD_ANA_TGM_DESC "samples where the target gain does not match between channels"

static const char* hdcd_str_ana_mode(hdcd_ana_mode_t v);

/********************* decoding ********************************/

#define HDCD_FLAG_FORCE_PE         128

typedef struct {
    uint8_t decoder_options;  /**< as flags HDCD_FLAG_* */

    uint64_t window;
    unsigned char readahead;

    /** arg is set when a packet prefix is found.
     *  control is the active control code, where
     *  bit 0-3: target_gain, 4-bit (3.1) fixed-point value
     *  bit 4  : peak_extend
     *  bit 5  : transient_filter
     *  bit 6,7: always zero */
    uint8_t arg, control;
    unsigned int sustain, sustain_reset; /**< code detect timer */

    int running_gain; /**< 11-bit (3.8) fixed point, extended from target_gain */

    /** counters */
    int code_counterA;            /**< 8-bit format packet */
    int code_counterA_almost;     /**< looks like an A code, but a bit expected to be 0 is 1 */
    int code_counterB;            /**< 16-bit format packet, 8-bit code, 8-bit XOR of code */
    int code_counterB_checkfails; /**< looks like a B code, but doesn't pass the XOR check */
    int code_counterC;            /**< packet prefix was found, expect a code */
    int code_counterC_unmatched;  /**< told to look for a code, but didn't find one */
    int count_peak_extend;        /**< valid packets where peak_extend was enabled */
    int count_transient_filter;   /**< valid packets where filter was detected */
    /** target_gain is a 4-bit (3.1) fixed-point value, always
     *  negative, but stored positive.
     *  The 16 possible values range from -7.5 to 0.0 dB in
     *  steps of 0.5, but no value below -6.0 dB should appear. */
    int gain_counts[16];
    int max_gain;
    /** occurences of code detect timer expiring without detecting
     *  a code. -1 for timer never set. */
    int count_sustain_expired;

    hdcd_log_t *log;
    hdcd_ana_mode_t ana_mode;
    int _ana_snb;
} hdcd_state_t;

typedef struct {
    hdcd_state_t channel[2];
    hdcd_ana_mode_t ana_mode;
    int val_target_gain;
    int count_tg_mismatch;
} hdcd_state_stereo_t;

typedef enum {
    HDCD_NONE            = 0,
    HDCD_NO_EFFECT       = 1,  /**< HDCD packets appear, but all control codes are NOP */
    HDCD_EFFECTUAL       = 2,  /**< HDCD packets appear, and change the output in some way */
} hdcd_detection_t;

void hdcd_reset(hdcd_state_t *state, unsigned rate);
void hdcd_reset_ext(hdcd_state_t *state, unsigned rate, int sustain_period_ms, uint8_t flags, hdcd_ana_mode_t analyze_mode, hdcd_log_t *log);
void hdcd_set_analyze_mode(hdcd_state_t *state, hdcd_ana_mode_t mode);

void hdcd_process(hdcd_state_t *state, int *samples, int count, int stride);

void hdcd_reset_stereo(hdcd_state_stereo_t *state, unsigned rate);
void hdcd_reset_stereo_ext(hdcd_state_stereo_t *state, unsigned rate, int sustain_period_ms, uint8_t flags, hdcd_ana_mode_t analyze_mode, hdcd_log_t *log);
void hdcd_process_stereo(hdcd_state_stereo_t *state, int *samples, int count);
void hdcd_set_analyze_mode_stereo(hdcd_state_stereo_t *state, hdcd_ana_mode_t mode);

/********************* optional detection and stats ************/

typedef enum {
    HDCD_PE_NEVER        = 0, /**< All valid packets have PE set to off */
    HDCD_PE_INTERMITTENT = 1, /**< Some valid packets have PE set to on */
    HDCD_PE_PERMANENT    = 2, /**< All valid packets have PE set to on  */
} hdcd_pe_t;

typedef enum {
    HDCD_PVER_NONE       = 0, /**< No packets discovered */
    HDCD_PVER_A          = 1, /**< Packets of type A (8-bit control) discovered */
    HDCD_PVER_B          = 2, /**< Packets of type B (8-bit control, 8-bit XOR) discovered */
    HDCD_PVER_MIX        = 3, /**< Packets of type A and B discovered, most likely an error? */
} hdcd_pf_t;

typedef struct {
    hdcd_detection_t hdcd_detected;
    hdcd_pf_t packet_type;
    int total_packets;         /**< valid packets */
    int errors;                /**< detectable errors */
    hdcd_pe_t peak_extend;
    int uses_transient_filter;
    float max_gain_adjustment; /**< in dB, expected in the range -7.5 to 0.0 */
    int cdt_expirations;       /**< -1 for never set, 0 for set but never expired */

    int _active_count;         /**< used internally */
} hdcd_detection_data_t;

void hdcd_detect_reset(hdcd_detection_data_t *detect);

void hdcd_detect_start(hdcd_detection_data_t *detect);
void hdcd_detect_onech(hdcd_state_t *state, hdcd_detection_data_t *detect);
void hdcd_detect_end(hdcd_detection_data_t *detect, int channels);
/* combines _start() _onech()(x2) _end */
void hdcd_detect_stereo(hdcd_state_stereo_t *state, hdcd_detection_data_t *detect);

static const char* hdcd_detect_str_pe(hdcd_pe_t v);
static const char* hdcd_detect_str_pformat(hdcd_pf_t v);
void hdcd_detect_str(hdcd_detection_data_t *detect, char *str, int maxlen); /* [256] should be enough */

void hdcd_dump_state_to_log(hdcd_state_t *state, int channel);

#ifdef __cplusplus
}
#endif

#endif

