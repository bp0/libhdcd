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
#include "../hdcd_decode2.h"
#include "wavreader.h"

int lv_major = HDCD_DECODE2_VER_MAJOR;
int lv_minor = HDCD_DECODE2_VER_MINOR;

void usage(const char* name) {
    fprintf(stderr, "Usage:\n %s in.wav [out.wav]\n", name);
    fprintf(stderr, "    in.wav must be a s16, stereo, 44100Hz wav file\n");
    fprintf(stderr, "    out.wav will be s32; will not prompt for overwrite\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Alternate usage:\n %s - [-] <in.wav [>out.wav]\n", name);
    fprintf(stderr,
        "    When using stdout with a pipe (non-seekable),\n"
        "    the wav header will not have a correct 'size',\n"
        "    but will otherwise work.\n");
}

typedef struct {
    FILE *fp;
    uint32_t size;
    int16_t bits_per_sample;
    int16_t bytes_per_sample;

    int length_loc;
    int data_size_loc;
} wavw_t;

int fwrite_int16el(int16_t v, FILE *fp) {
    const uint8_t b[2] = {
        (uint16_t)v & 0xff,
        (uint16_t)v >> 8,
    };
    return fwrite(&b, 1, 2, fp);
}

int fwrite_int24el(int32_t v, FILE *fp) {
    const uint8_t b[3] = {
        ((uint32_t)v >> 8) & 0xff,
        ((uint32_t)v >> 16) & 0xff,
        ((uint32_t)v >> 24),
    };
    return fwrite(&b, 1, 3, fp);
}

int fwrite_int32el(int32_t v, FILE *fp) {
    const uint8_t b[4] = {
        (uint32_t)v & 0xff,
        ((uint32_t)v >> 8) & 0xff,
        ((uint32_t)v >> 16) & 0xff,
        ((uint32_t)v >> 24),
    };
    return fwrite(&b, 1, 4, fp);
}

wavw_t* wav_write_open(const char *file_name, int16_t bits_per_sample) {
    int16_t channels = 2;
    int32_t sample_rate = 44100;

    int32_t size = 0;
    int32_t length = size + 44 - 8;
    int32_t bytes_per_second = channels * sample_rate * bits_per_sample/8;
    int16_t bytes_per_sample = channels * bits_per_sample/8;

    wavw_t* wav = malloc(sizeof(wavw_t));
    if (!wav) return NULL;

    FILE *fp;
    if (strcmp(file_name, "-") == 0)
        fp = stdout;
    else
        fp = fopen(file_name, "wb");
    if(!fp) {
        free(wav);
        return NULL;
    }
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
    wav->fp = fp;
    wav->size = size;
    wav->bits_per_sample = bits_per_sample;
    wav->bytes_per_sample = bytes_per_sample;
    return wav;
}

int wav_write(wavw_t *wav, const int32_t *samples, int count) {
    int i;
    size_t elw = 0;

    if (!wav) return 1;
    for (i = 0; i < count; i++)
        if (wav->bits_per_sample == 24)
            elw += fwrite_int24el(samples[i], wav->fp);
        else if (wav->bits_per_sample == 32)
            elw += fwrite_int32el(samples[i], wav->fp);

    wav->size += elw;
}

int wav_write_close(wavw_t *wav) {
    if ( fseek(wav->fp, wav->length_loc, SEEK_SET) == 0)
        fwrite_int32el(wav->size + 44 - 8 , wav->fp);
    if ( fseek(wav->fp, wav->data_size_loc, SEEK_SET) == 0)
        fwrite_int32el(wav->size, wav->fp);
    fclose(wav->fp);
    free(wav);
}

int main(int argc, char *argv[]) {
    const char *infile;
    const char *outfile = NULL;
    void *wav;
    wavw_t *wav_out;

    int format, sample_rate, channels, bits_per_sample;
    int frame_length = 2048;
    int input_size;
    int set_size;
    uint8_t* input_buf;
    int16_t* convert_buf;
    int32_t* process_buf;
    int i, read, count, full_count = 0;

    hdcd_detection_data_t detect;
    hdcd_state_stereo_t state_stereo;
    hdcd_log_t logger;
    char dstr[256];

    int ver_match = hdcd_lib_version(&lv_major, &lv_minor);

    fprintf(stderr, "HDCD detect/decode tool\n");
    fprintf(stderr, "libhdcd version: %d.%d\n\n", lv_major, lv_minor);

    if (!ver_match) {
        fprintf(stderr, "Version mismatch. Built against: %d.%d\n",
            HDCD_DECODE2_VER_MAJOR, HDCD_DECODE2_VER_MINOR);
        if (lv_major != HDCD_DECODE2_VER_MAJOR) {
            fprintf(stderr, "...exiting.\n");
            return 1;
        }
    }

    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }
    infile = argv[1];
    if (argc == 3) outfile = argv[2];

    wav = wav_read_open(infile);
    if (!wav) {
        fprintf(stderr, "Unable to open wav file %s\n", infile);
        return 1;
    }
    if (!wav_get_header(wav, &format, &channels, &sample_rate, &bits_per_sample, NULL)) {
        fprintf(stderr, "Bad wav file %s\n", infile);
        return 1;
    }
    if (format != 1) {
        fprintf(stderr, "Unsupported WAV format %d\n", format);
        return 1;
    }
    if (bits_per_sample != 16) {
        fprintf(stderr, "Unsupported WAV sample depth %d\n", bits_per_sample);
        return 1;
    }
    if (channels != 2) {
        fprintf(stderr, "Unsupported WAV channels %d\n", channels);
        return 1;
    }

    if (outfile) {
        wav_out = wav_write_open(outfile, 24);
        if (!wav_out) return 1;
    }

    hdcd_reset_stereo(&state_stereo, sample_rate);
    hdcd_detect_reset(&detect);
    hdcd_log_init(&logger);
    hdcd_attach_logger_stereo(&state_stereo, &logger);

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

        hdcd_process_stereo(&state_stereo, process_buf, count);
        hdcd_detect_stereo(&state_stereo, &detect);

        if (outfile) wav_write(wav_out, process_buf, count * channels);

        full_count += count;
        if (read < input_size) break; // eof
    }
    hdcd_detect_str(&detect, dstr, sizeof(dstr));
    fprintf(stderr, "%d samples, %0.2fs\n", full_count * channels, (float)full_count / (float)sample_rate);
    fprintf(stderr, "%s\n", dstr);

    free(input_buf);
    free(process_buf);
    wav_read_close(wav);
    if (outfile) wav_write_close(wav_out);

    return 0;
}

