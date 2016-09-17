/* ------------------------------------------------------------------
 * Copyright (C) 2016 Burt P.
 * Based on wavreader.c, copyright (C) 2009 Martin Storsjo,
 * from libfdk-aac's aac-enc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#ifndef WAVIO_H
#define WAVIO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wavio wavio;

wavio* wav_write_open(const char *filename, int channels, int sample_rate, int bits_per_sample, int raw);
int wav_write_samples(wavio *wav, const int32_t *samples, int nb_samples);

wavio* wav_read_open(const char *filename); /* dumber, but working with pipes version */
wavio* wav_read_open_ms(const char *filename); /* Martin Storsjo's version */
wavio* wav_read_open_raw(const char *filename, int channels, int sample_rate, int bits_per_sample);
int wav_read(wavio* wav, unsigned char* data, unsigned int length);
int wav_read_samples(wavio* wav, int32_t* samples, int nb_samples);

int wav_get_header(wavio* wav, int* format, int* channels, int* sample_rate, int* bits_per_sample, int *valid_bits_per_sample, unsigned int* data_length);
void wav_close(wavio *wav);

void wavio_dump(wavio *wav, const char *tag);

#ifdef __cplusplus
}
#endif

#endif


