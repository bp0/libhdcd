/* ------------------------------------------------------------------
 * Copyright (C) 2016 Burt P.
 * Uses wavreader.c by Martin Storsjo, from libfdk-aac's aac-enc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../hdcd_simple.h"
#include "wavreader.h"

int lv_major = HDCDLIB_VER_MAJOR;
int lv_minor = HDCDLIB_VER_MINOR;

static const char* amode_name[] = {
  "off", "lle", "pe", "cdt", "tgm", "pel", "ltgm"
};

static void usage(const char* name) {
    int i;
    fprintf(stderr, "Usage:\n"
        "%s [options] in.wav [out.wav]\n", name);
    fprintf(stderr,
        "    in.wav must be a s16, stereo, 44100Hz wav file\n"
        "    out.wav will be s24, stereo, 44100Hz\n"
        "\n" );
    fprintf(stderr, "Alternate usage:\n %s [options] - [-] <in.wav [>out.wav]\n", name);
    fprintf(stderr,
        "    When using stdout with a pipe (non-seekable),\n"
        "    the wav header will not have a correct 'size',\n"
        "    but will otherwise work\n"
        "\n" );
    fprintf(stderr, "Options:\n"
        "    -q\t\t quiet\n"
        "    -f\t\t force overwrite\n"
        "    -x\t\t return non-zero exit code if HDCD encoding\n"
        "      \t\t was _NOT_ detected\n"
        "    -a <mode>\t analyze modes:\n");
    for(i = 0; i <= 6; i++)
        fprintf(stderr,
        "      \t\t     %s  \t%s\n", amode_name[i], shdcd_analyze_mode_desc(i) );
    fprintf(stderr,
        "    -p\t\t output raw pcm samples only without any wav header\n"
        "\n");
}

typedef struct {
    FILE *fp;
    uint32_t size;
    int16_t bits_per_sample;
    int16_t bytes_per_sample;

    int raw_pcm_only;

    int length_loc;
    int data_size_loc;
} wavw_t;

static int fwrite_int16el(int16_t v, FILE *fp) {
    const uint8_t b[2] = {
        (uint16_t)v & 0xff,
        (uint16_t)v >> 8,
    };
    return fwrite(&b, 1, 2, fp);
}

static int fwrite_int24el(int32_t v, FILE *fp) {
    const uint8_t b[3] = {
        ((uint32_t)v >> 8) & 0xff,
        ((uint32_t)v >> 16) & 0xff,
        ((uint32_t)v >> 24),
    };
    return fwrite(&b, 1, 3, fp);
}

static int fwrite_int32el(int32_t v, FILE *fp) {
    const uint8_t b[4] = {
        (uint32_t)v & 0xff,
        ((uint32_t)v >> 8) & 0xff,
        ((uint32_t)v >> 16) & 0xff,
        ((uint32_t)v >> 24),
    };
    return fwrite(&b, 1, 4, fp);
}

static wavw_t* wav_write_open(const char *file_name, int16_t bits_per_sample, int raw) {
    int16_t channels = 2;
    int32_t sample_rate = 44100;

    int32_t size = 0;
    int32_t length = size + 44 - 8;
    int32_t bytes_per_second = channels * sample_rate * bits_per_sample/8;
    int16_t bytes_per_sample = channels * bits_per_sample/8;

    wavw_t* wav = malloc(sizeof(wavw_t));
    if (!wav) return NULL;
    memset(wav, 0, sizeof(*wav));

    FILE *fp;
    if (strcmp(file_name, "-") == 0)
        fp = stdout;
    else
        fp = fopen(file_name, "wb");
    if(!fp) {
        free(wav);
        return NULL;
    }

    if (!raw) {
        fwrite("RIFF", 1, 4, fp);
        wav->length_loc = ftell(fp);
        fwrite_int32el(length, fp);
        fwrite("WAVEfmt \x10\x00\x00\x00\x01\x00", 1, 14, fp);
        fwrite_int16el(channels, fp);
        fwrite_int32el(sample_rate, fp);
        fwrite_int32el(bytes_per_second, fp);
        fwrite_int16el(bytes_per_sample, fp);
        fwrite_int16el(bits_per_sample, fp);
        fwrite("data", 1, 4, fp);
        wav->data_size_loc = ftell(fp);
        fwrite_int32el(size, fp);
        wav->raw_pcm_only = raw;
    }
    wav->fp = fp;
    wav->size = size;
    wav->bits_per_sample = bits_per_sample;
    wav->bytes_per_sample = bytes_per_sample;
    wav->raw_pcm_only = raw;
    return wav;
}

static int wav_write(wavw_t *wav, const int32_t *samples, int count) {
    int i;
    size_t elw = 0;

    if (!wav) return 0;
    for (i = 0; i < count; i++)
        if (wav->bits_per_sample == 24)
            elw += fwrite_int24el(samples[i], wav->fp);
        else if (wav->bits_per_sample == 32)
            elw += fwrite_int32el(samples[i], wav->fp);

    wav->size += elw;
    return elw;
}

static void wav_write_close(wavw_t *wav) {
    if (!wav) return;
    if (wav->fp) {
        if (!wav->raw_pcm_only) {
            if (wav->length_loc && fseek(wav->fp, wav->length_loc, SEEK_SET) == 0)
                fwrite_int32el(wav->size + 44 - 8 , wav->fp);
            if (wav->data_size_loc && fseek(wav->fp, wav->data_size_loc, SEEK_SET) == 0)
                fwrite_int32el(wav->size, wav->fp);
        }
        fclose(wav->fp);
    }
    free(wav);
}

int main(int argc, char *argv[]) {
    const char *infile;
    const char *outfile = NULL;
    void *wav;
    wavw_t *wav_out = NULL;

    int format, sample_rate, channels, bits_per_sample;
    int frame_length = 2048;
    int input_size;
    int set_size;
    uint8_t* input_buf;
    int16_t* convert_buf;
    int32_t* process_buf;
    int i, c, read, count, full_count = 0, ver_match;

    int xmode = 0, opt_force = 0, opt_quiet = 0, amode = 0;
    int opt_raw_out = 0;

    hdcd_simple_t *ctx;
    char dstr[256];

    while ((c = getopt(argc, argv, "a:fpqx")) != -1) {
        switch (c) {
            case 'x':
                xmode = 1;
                break;
            case 'q':
                opt_quiet = 1;
                break;
            case 'p':
                opt_raw_out = 1;
                break;
            case 'f':
                opt_force = 1;
                break;
            case 'a':
                if (strcmp(optarg, "off") == 0)
                    amode = SHDCD_ANA_OFF;
                else if (strcmp(optarg, "lle") == 0)
                    amode = SHDCD_ANA_LLE;
                else if (strcmp(optarg, "pe") == 0)
                    amode = SHDCD_ANA_PE;
                else if (strcmp(optarg, "cdt") == 0)
                    amode = SHDCD_ANA_CDT;
                else if (strcmp(optarg, "tgm") == 0)
                    amode = SHDCD_ANA_TGM;
                else if (strcmp(optarg, "pel") == 0)
                    amode = SHDCD_ANA_PEL;
                else if (strcmp(optarg, "ltgm") == 0)
                    amode = SHDCD_ANA_LTGM;
                else
                    amode = atoi(optarg);

                if (amode < 0 || amode > 6 )
                    amode = -1;
                break;
            default:
                usage(argv[0]);
                return 1;
        }
    }

    ver_match = hdcd_lib_version(&lv_major, &lv_minor);

    if (!opt_quiet) fprintf(stderr, "HDCD detect/decode tool\n");
    if (!opt_quiet) fprintf(stderr, "libhdcd version: %d.%d\n\n", lv_major, lv_minor);

    if (!ver_match) {
        if (!opt_quiet)
            fprintf(stderr, "Version mismatch. Built against: %d.%d\n",
                HDCDLIB_VER_MAJOR, HDCDLIB_VER_MINOR);
        if (lv_major != HDCDLIB_VER_MAJOR) {
            if (!opt_quiet)
                fprintf(stderr, "...exiting.\n");
            return 1;
        }
    }

    if (argc - optind < 1) {
        if (!opt_quiet) usage(argv[0]);
        return 1;
    }
    infile = argv[optind];
    if (argc - optind >= 2) outfile = argv[optind + 1];

    wav = wav_read_open(infile);
    if (!wav) {
        if (!opt_quiet) fprintf(stderr, "Unable to open wav file %s\n", infile);
        return 1;
    }
    if (!wav_get_header(wav, &format, &channels, &sample_rate, &bits_per_sample, NULL)) {
        if (!opt_quiet) fprintf(stderr, "Bad wav file %s\n", infile);
        return 1;
    }
    if (format != 1) {
        if (!opt_quiet) fprintf(stderr, "Unsupported WAV format %d\n", format);
        return 1;
    }
    if (bits_per_sample != 16) {
        if (!opt_quiet) fprintf(stderr, "Unsupported WAV sample depth %d\n", bits_per_sample);
        return 1;
    }
    if (channels != 2) {
        if (!opt_quiet) fprintf(stderr, "Unsupported WAV channels %d\n", channels);
        return 1;
    }

    if (outfile) {
        if( !opt_force && strcmp(outfile, "-") != 0 && access( outfile, F_OK ) != -1 ) {
            if (!opt_quiet) fprintf(stderr, "Output file exists, use -f to overwrite\n");
            return 1;
        } else {
            wav_out = wav_write_open(outfile, 24, opt_raw_out);
            if (!wav_out) return 1;
        }
    }

    if (amode == -1) {
        if (!opt_quiet) fprintf(stderr, "Unknown analyze mode\n");
        return 1;
    }

    ctx = shdcd_new();
    if (!opt_quiet) shdcd_default_logger(ctx);
    if (amode) {
        if (!outfile) {
            if (!opt_quiet) fprintf(stderr, "Without an output file, analyze mode does nothing\n");
            return 1;
        }
        if (!opt_quiet) fprintf(stderr, "Analyze mode [%s]: %s\n", amode_name[amode], shdcd_analyze_mode_desc(amode) );
        if (!shdcd_analyze_mode(ctx, amode)) {
            if (!opt_quiet) fprintf(stderr, "Failed to set mode for analyze\n");
            return 1;
        }
    }

    set_size = channels * (bits_per_sample>>3);
    input_size = set_size * frame_length;
    input_buf = (uint8_t*) malloc(input_size);
    convert_buf = (int16_t*) malloc(input_size);
    process_buf = (int32_t*) malloc(input_size * 2);

    while (1) {
        read = wav_read_data(wav, input_buf, input_size);
        /* endian-swap */
        for (i = 0; i < read/2; i++) {
            const uint8_t* in = &input_buf[2*i];
            convert_buf[i] = in[0] | (in[1] << 8);
        }
        count = read / set_size;
        /* s16 pcm to s32 pcm to make room for hdcd expansion */
        for (i = 0; i < count * channels; i++)
            process_buf[i] = convert_buf[i];

        shdcd_process(ctx, process_buf, count);

        if (outfile) wav_write(wav_out, process_buf, count * channels);

        full_count += count;
        if (read < input_size) break; // eof
    }
    shdcd_detect_str(ctx, dstr, sizeof(dstr));
    if (xmode) xmode = !shdcd_detected(ctx); /* return zero if (-x) mode and HDCD not detected */

    if (!opt_quiet) fprintf(stderr, "%d samples, %0.2fs\n", full_count * channels, (float)full_count / (float)sample_rate);
    if (!opt_quiet) fprintf(stderr, "%s\n", dstr);

    free(input_buf);
    free(convert_buf);
    free(process_buf);
    wav_read_close(wav);
    shdcd_free(ctx);
    if (outfile) wav_write_close(wav_out);

    return xmode;
}
