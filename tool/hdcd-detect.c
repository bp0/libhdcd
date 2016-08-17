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
#include "../hdcd_decode2.h"
#include "wavreader.h"

void usage(const char* name) {
    fprintf(stderr, "%s in.wav\n", name);
}

int main(int argc, char *argv[]) {
    const char *infile;
    void *wav;
    int format, sample_rate, channels, bits_per_sample;
    int frame_length = 2048;
    int input_size;
    int set_size;
    uint8_t* input_buf;
    int16_t* convert_buf;
    int32_t* process_buf;
    int i, read, count;

    hdcd_detection_data_t detect;
    hdcd_state_stereo_t state_stereo;
    char dstr[256];

    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }
    infile = argv[1];

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

    hdcd_reset_stereo(&state_stereo, sample_rate);
    hdcd_detect_reset(&detect);

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
        if (read < input_size) break; // eof
    }
    hdcd_detect_str(&detect, dstr, sizeof(dstr));
    printf("%s\n", dstr);

    free(input_buf);
    free(process_buf);
    wav_read_close(wav);

    return 0;
}

