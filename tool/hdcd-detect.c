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

/** BUILD_HDCD_EXE_COMPAT set uses -k mode by default */
#ifndef BUILD_HDCD_EXE_COMPAT
#define BUILD_HDCD_EXE_COMPAT 0
#else
#define BUILD_HDCD_EXE_COMPAT 1
#endif

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#include "../src/hdcd_simple.h"
#include "wavreader.h"
#include "wavout.h"

#define OPT_KI_SCAN_MAX 384000 /* two full seconds at max rate */

int lv_major = HDCDLIB_VER_MAJOR;
int lv_minor = HDCDLIB_VER_MINOR;

static const char* amode_name[] = {
  "off", "lle", "pe", "cdt", "tgm", "pel", "ltgm"
};

static void usage(const char* name, int kmode) {
    int i;
    if (kmode) {
        fprintf(stderr, "Usage:\n"
            "%s [options] [in.wav]\n", name);
        fprintf(stderr,
            "    input must be a s16le, stereo, 44.1kHz WAV file\n"
            "    output will be s24le, stereo, 44.1kHz\n"
            "\n" );
    } else {
        fprintf(stderr, "Usage:\n"
            "%s [options] [-o out.wav] in.wav\n", name);
        fprintf(stderr,
            "    input must be a s16le, stereo, 44.1kHz WAV file\n"
            "    output will be s24le, stereo, 44.1kHz\n"
            "\n" );
        fprintf(stderr, "Alternate usage:\n"
            "%s [options] -c - <in.wav >out.wav\n", name);
        fprintf(stderr,
            "    When using -c with a pipe (non-seekable),\n"
            "    the wav header will not have a correct 'size',\n"
            "    but will otherwise work\n"
            "\n" );
    }
    fprintf(stderr, "Options:\n"
        "    -h\t\t display usage information\n"
        "    -v\t\t print version and exit\n"
#if (BUILD_HDCD_EXE_COMPAT == 0)
        "    -k\t\t use interface compatible with\n"
        "      \t\t Christoper Key's hdcd.exe\n"
#endif
        "    -q\t\t quiet\n"
        "    -f\t\t force overwrite\n"
        "    -x\t\t return non-zero exit code if HDCD encoding\n"
        "      \t\t was _NOT_ detected\n"
        "    -o <file>\t output file to write\n"
        "    -c\t\t output to stdout\n"
        "    -d\t\t dump full detection data instead of summary\n"
        "    -z <mode>\t analyze modes:\n");
    for(i = 0; i <= 6; i++)
        fprintf(stderr,
        "      \t\t     %s  \t%s\n", amode_name[i], hdcd_str_analyze_mode_desc(i) );
    fprintf(stderr,
        "    -p\t\t output raw s24le PCM samples only without\n"
        "      \t\t any wav header\n");
    if (kmode) {
        fprintf(stderr,
        "    -r\t\t input raw s16le PCM samples, expected to be\n"
        "      \t\t stereo, 44.1kHz (also forces -p)\n"
        "    -a\t\t supress output\n"
        "    -i\t\t identify HDCD, implies -a -x, scans the\n"
        "      \t\t first %d frames (%.0fms at 44.1kHz)\n", OPT_KI_SCAN_MAX, (float)(OPT_KI_SCAN_MAX) / 44100 * 1000 );
    } else {
        fprintf(stderr,
        "    -r\t\t input raw s16le PCM samples, expected to be\n"
        "      \t\t stereo, 44.1kHz (with -k, -r also forces -p)\n");
    }
    fprintf(stderr, "\n");
}

int main(int argc, char *argv[]) {
    const char *infile = NULL;
    const char *outfile = NULL;
    const char dashfile[] = "-";
    void *wav = NULL;
    FILE *fp_raw_in = NULL;
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
    int kmode = BUILD_HDCD_EXE_COMPAT,
        opt_ka = 0, opt_ks = 0, opt_kr = 0, opt_ki = 0;
    int opt_help = 0, opt_dump_detect = 0;
    int opt_raw_out = 0, opt_raw_in = 0, raw_rate = 44100;
    int opt_nop = 0, opt_testing = 0;
    int dv; /* used with opt_testing */

    hdcd_simple *ctx;
    char dstr[256];

    while ((c = getopt(argc, argv, "acde:fhijkno:pqrsvxz:")) != -1) {
        switch (c) {
            case 'x':
                xmode = 1;
                break;
            case 'v':
                printf("libhdcd %d.%d\n", HDCDLIB_VER_MAJOR, HDCDLIB_VER_MINOR);
                exit(0);
                break;
            case 'q':
                opt_quiet = 1;
                break;
            case 'p':
                opt_raw_out = 1;
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'n':
                opt_nop = 1;
                break;
            case 'k':
                kmode = 1;
                break;
            case 'i':
                opt_ki = 1;
                xmode = 1;
                opt_ka = 1;
                opt_quiet = 1;
                break;
            case 'j':
                opt_testing = 1;
                break;
            case 'h':
                opt_help = 1;
                break;
            case 'f':
                opt_force = 1;
                break;
            case 'e':
                raw_rate = atoi(optarg);
                break;
            case 'c':
                outfile = "-";
                break;
            case 'r':
                opt_raw_in = 1;
                opt_kr = 1;
                break;
            case 's':
                opt_ks = 1;
                break;
            case 'd':
                opt_dump_detect = 1;
                break;
            case 'a':
                opt_ka = 1;
                break;
            case 'z':
                if (strcmp(optarg, "off") == 0)
                    amode = HDCD_ANA_OFF;
                else if (strcmp(optarg, "lle") == 0)
                    amode = HDCD_ANA_LLE;
                else if (strcmp(optarg, "pe") == 0)
                    amode = HDCD_ANA_PE;
                else if (strcmp(optarg, "cdt") == 0)
                    amode = HDCD_ANA_CDT;
                else if (strcmp(optarg, "tgm") == 0)
                    amode = HDCD_ANA_TGM;
                else if (strcmp(optarg, "pel") == 0)
                    amode = HDCD_ANA_PEL;
                else if (strcmp(optarg, "ltgm") == 0)
                    amode = HDCD_ANA_LTGM;
                else
                    amode = atoi(optarg);

                if (amode < 0 || amode > 6 )
                    amode = -1;
                break;
            default:
                usage(argv[0], kmode);
                return 1;
        }
    }

    if (argc - optind >= 1)
        infile = argv[optind];

    if (kmode) {
        /* kmode default is -q, requires -s to print messages */
        if (!opt_quiet) opt_quiet = opt_ks;
        if (!outfile) outfile = dashfile;
        /* kmode -a suppress output specified */
        if (opt_ka) outfile = NULL;
        /* kmode defaults to reading from stdin */
        if (!infile) infile = dashfile;
        /* kmode -r means raw in and out,
         * normal mode only raw in, requires -p for raw out */
        if (opt_kr) opt_raw_out = 1;
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

    if (!opt_quiet && kmode) fprintf(stderr, "hdcd.exe compatible mode\n");

    if (opt_help) {
        usage(argv[0], kmode);
        return 0;
    }

    if (!kmode && !infile) {
        usage(argv[0], kmode);
        return 1;
    }

    if (opt_raw_in) {
        format = 1;
        channels = 2;
        sample_rate = raw_rate;
        bits_per_sample = 16;
        if (strcmp(infile, "-") == 0) {
            fp_raw_in = stdin;
#ifdef _WIN32
            setmode(fileno(stdin), O_BINARY);
#endif
        } else {
            fp_raw_in = fopen(infile, "rb");
            if (!fp_raw_in) {
                if (!opt_quiet) fprintf(stderr, "Unable to open raw pcm file %s\n", infile);
                return 1;
            }
        }
    } else {
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
    }

    if (outfile) {
        if( !opt_force && strcmp(outfile, "-") != 0 && access( outfile, F_OK ) != -1 ) {
            if (!opt_quiet) fprintf(stderr, "Output file exists, use -f to overwrite\n");
            return 1;
        } else {
            wav_out = wav_write_open(outfile, (opt_nop ? 16 : 24), sample_rate, opt_raw_out);
            if (!wav_out) return 1;
        }
    }

    if (amode == -1) {
        if (!opt_quiet) fprintf(stderr, "Unknown analyze mode\n");
        return 1;
    }

    ctx = hdcd_new();
    if (!hdcd_set_sample_rate(ctx, sample_rate)) {
        if (!opt_quiet) fprintf(stderr, "Unusable sample rate %d\n", sample_rate);
        return 1;
    }
    if (!opt_quiet) hdcd_logger_default(ctx);
    if (amode) {
        if (!outfile) {
            if (!opt_quiet) fprintf(stderr, "Without an output file, analyze mode does nothing\n");
            return 1;
        }
        if (!opt_quiet) fprintf(stderr, "Analyze mode [%s]: %s\n", amode_name[amode], hdcd_str_analyze_mode_desc(amode) );
        if (!hdcd_analyze_mode(ctx, amode)) {
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
        if (opt_raw_in)
            read = fread(input_buf, 1, input_size, fp_raw_in);
         else
            read = wav_read_data(wav, input_buf, input_size);

        /* endian-swap */
        for (i = 0; i < read/2; i++) {
            const uint8_t* in = &input_buf[2*i];
            convert_buf[i] = in[0] | (in[1] << 8);
        }
        count = read / set_size;

        /* if there isn't a full set, then forget the last one */
        if (read % set_size) count--;
        if (count < 0) count = 0;

        /* s16 pcm to s32 pcm to make room for hdcd expansion */
        for (i = 0; i < count * channels; i++)
            process_buf[i] = convert_buf[i];

        if (!opt_nop) {
            /* in -j testing mode only */
            if (opt_testing)
                dv = hdcd_scan(ctx, process_buf, count, 0);

            hdcd_process(ctx, process_buf, count);

            /* in -j testing mode only */
            if (opt_testing)
                if (dv != hdcd_detected(ctx) )
                    fprintf(stderr,
                        "hdcd_scan() result did not match hdcd_process(): %d:%d\n",
                        dv, hdcd_detected(ctx) );
        }


        if (outfile) {
            wav_write(wav_out, process_buf, count * channels);
        }

        full_count += count;
        if (opt_ki) {
            /* -i mode, break when HDCD is discovered*/
            if (hdcd_detected(ctx))
                break;
            /* limit scanning to first OPT_KI_SCAN_MAX samples */
            if (full_count >= OPT_KI_SCAN_MAX)
                break;
        }
        if (read < input_size) break; /* eof */
    }
    if (xmode) xmode = !hdcd_detected(ctx); /* return non-zero if (-x) mode and HDCD not detected */

    if (opt_ki) {
        /* strings are exactly those given by Key's hdcd.exe -i */
        if (hdcd_detected(ctx))
            fprintf(stderr, "HDCD Detected\n");
        else
            fprintf(stderr, "HDCD not detected\n");
    }

    if (!opt_quiet) fprintf(stderr, "%d samples, %0.2fs\n", full_count * channels, (float)full_count / (float)sample_rate);
    if (!opt_quiet) {
        if (opt_dump_detect) {
            hdcd_logger_dump_state(ctx);
            int det = hdcd_detected(ctx);
            int pf = hdcd_detect_packet_type(ctx);
            int pe = hdcd_detect_peak_extend(ctx);
            fprintf(stderr, ".hdcd_encoding: [%d] %s\n", det, hdcd_str_detect(det) );
            fprintf(stderr, ".packet_type: [%d] %s\n", pf, hdcd_str_pformat(pf) );
            fprintf(stderr, ".total_packets: %d\n", hdcd_detect_total_packets(ctx) );
            fprintf(stderr, ".errors: %d\n", hdcd_detect_errors(ctx) );
            fprintf(stderr, ".peak_extend: [%d] %s\n", pe, hdcd_str_pe(pe) );
            fprintf(stderr, ".uses_transient_filter: %d\n", hdcd_detect_uses_transient_filter(ctx) );
            fprintf(stderr, ".max_gain_adjustment: %0.1f dB\n", hdcd_detect_max_gain_adjustment(ctx) );
            fprintf(stderr, ".cdt_expirations: %d\n", hdcd_detect_cdt_expirations(ctx) );
        } else {
            hdcd_detect_str(ctx, dstr, sizeof(dstr));
            fprintf(stderr, "%s\n", dstr);
        }
    }

    free(input_buf);
    free(convert_buf);
    free(process_buf);
    if (opt_raw_in)
        fclose(fp_raw_in);
    else
        wav_read_close(wav);
    hdcd_free(ctx);
    if (outfile) wav_write_close(wav_out);

    return xmode;
}
