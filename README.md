# libhdcd

A stand-alone library for High Definition Compatible Digital (HDCD) decoding
and analysis based on foo_hdcd and ffmpeg's af_hdcd. See AUTHORS.

Features
--------

* HDCD decoding
* Optional HDCD detection code
* Optional Analyze mode
* Optional logging callback interface

Building
--------

In order to build the library from **git** you need [GNU build system][autotools]:

    autoreconf -ivf

And then use the normal configure & make:

    ./configure
    make
    make install

[autotools]: https://autotools.io

CLI Tool
--------

tool/ contains source for an HDCD detection/decoding command-line interface
utility that employs libhdcd, called hdcd-detect.

See `hdcd-detect -h` for usage.

The Windows binary package includes a special build of hdcd-detect called
hdcd.exe that attempts to be a drop-in replacement for Key's original hdcd.exe,
but with all the recent improvements. This compatibility mode can be used in
hdcd-detect using the -k option. The primary difference is that the default
behavior is to read from stdin, and write to stdout.

Simple API
----------
### Include

    #include <hdcd/hdcd_simple.h>

### Declare and initialize

    hdcd_simple *ctx = hdcd_new();

### Each frame

    /* copy s16 samples into int32_t
     * example:
     *
     * int32_t samples[nb_samples * channels];
     * for (i = 0; i < nb_samples * channels; i++)
     *     samples[i] = s16_samples[i];
     */

    /* process will expand s16 into s32 */
    hdcd_process(ctx, samples, nb_samples);

### Song change, seek, etc.

    hdcd_reset(ctx);  /* reset the decoder state */

It is best to hdcd_reset() and then seek a couple seconds behind the target
to let the the decoder process samples and catch the nearest packet. The
samples leading up to the seek target can then be discarded.

### Cleanup

    hdcd_free(ctx);

### HDCD detection

Detection data is available after a call to hdcd_process().

    hdcd_dv dv;
    dv = hdcd_detected(ctx);   /* see hdcd_dv in hdcd_detect.h */

    /* get a string with an HDCD detection summary */
    char dstr[256];
    hdcd_detect_str(ctx, dstr, sizeof(dstr));

Complete detection stats are available. See hdcd_simple.h and hdcd_detect.h for related functions and types.

    /* get individual detection values */
    hdcd_pf pf = hdcd_detect_packet_type(ctx);           /* see hdcd_pf in hdcd_detect.h */
    int packets = int hdcd_detect_total_packets(ctx);    /* valid packets */
    int errors = hdcd_detect_errors(ctx);                /* detectable errors */
    hdcd_pe pe = hdcd_detect_peak_extend(ctx);           /* see hdcd_pe in hdcd_detect.h */
    int tf = hdcd_detect_uses_transient_filter(ctx);     /* bool, 1 if the tf flag was detected */
    float mga = float hdcd_detect_max_gain_adjustment(ctx); /* in dB, expected in the range -7.5 to 0.0 */
    int cdt_exp = hdcd_detect_cdt_expirations(ctx);         /* -1 for never set, 0 for set but never expired */

### Analyze mode

A mode to aid in analysis of HDCD encoded audio. In this mode the audio is
replaced by a solid tone and the amplitude is adjusted to signal some specified
aspect of the process. The output can be loaded in an audio editor alongside
the original, where the user can see where different features or states are
present. See hdcd_ana_mode in hdcd_analyze.h.
See also [HDCD § Analyze mode](http://wiki.hydrogenaud.io/index.php?title=High_Definition_Compatible_Digital#Analyze_mode)

    hdcd_analyze_mode(MODE);
