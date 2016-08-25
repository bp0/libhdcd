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

#ifndef WAVOUT_H
#define WAVOUT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wavw_t wavw_t;

wavw_t* wav_write_open(const char *file_name, int16_t bits_per_sample, int raw);
int wav_write(wavw_t *wav, const int32_t *samples, int count);
void wav_write_close(wavw_t *wav);

#ifdef __cplusplus
}
#endif

#endif
