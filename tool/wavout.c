/* ------------------------------------------------------------------
 * Copyright (C) 2016 Burt P.
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
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "wavout.h"

typedef struct wavw_t {
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

wavw_t* wav_write_open(const char *file_name, int16_t bits_per_sample, int raw) {
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
    if (strcmp(file_name, "-") == 0) {
        fp = stdout;
#ifdef _WIN32
        setmode(fileno(stdout), O_BINARY);
#endif
    } else
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

int wav_write(wavw_t *wav, const int32_t *samples, int count) {
    int i;
    size_t elw = 0;

    if (!wav) return 0;
    for (i = 0; i < count; i++)
        if (wav->bits_per_sample == 24)
            elw += fwrite_int24el(samples[i], wav->fp);
        else if (wav->bits_per_sample == 32)
            elw += fwrite_int32el(samples[i], wav->fp);
        else if (wav->bits_per_sample == 16)
            elw += fwrite_int16el(samples[i], wav->fp);

    wav->size += elw;
    return elw;
}

void wav_write_close(wavw_t *wav) {
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
