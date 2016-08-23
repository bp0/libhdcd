HDCD decoder code, originally in foo_hdcd, then developed further as part of FFmpeg, then made into this generic form.

Building
--------

In order to build the library from **git** you need [GNU build system][autotools]:

    autoreconf -ivf

And then use the normal configure & make:

    ./configure
    make
    make install

[autotools]: https://autotools.io

Features
--------

* HDCD decoding
* Optional HDCD detection code
* Optional Analyze mode
* Optional logging callback interface

Simple API
----------
### Include

    #include <hdcd/hdcd_simple.h>

### Declare and initialize

    hdcd_simple_t ctx;
    ctx = shdcd_new();

### Each frame
    
    shdcd_process(ctx, samples, count);

### Cleanup
    
    shdcd_free(ctx);

Full API
--------

### Include
    #include <hdcd/hdcd_decode2.h>

### For any number of channels, process one at a time.

##### Declare
    hdcd_state_t state[MAX_CHANNELS];

#### Initialize
    /* foreach(channel) */
        hdcd_reset(&state[channel], sample_rate);

#### Each frame
    /* foreach(channel) */
        hdcd_process(&state[channel], samples, count, nb_channels);

### When there are exactly two stereo channels, target_gain matching is enabled.

#### Declare
    hdcd_state_stereo_t state_stereo;

#### Initialize
    hdcd_reset_stereo(&state_stereo, sample_rate);

#### Each frame
    hdcd_process_stereo(&state_stereo, samples, count);

### HDCD detection functions

#### Declare
    hdcd_detection_data_t detect;

#### Initialize
    hdcd_detect_reset(&detect);

#### Each frame (n-channel)
    hdcd_detect_start(&detect);
    /* foreach(channel) */
        hdcd_process(&state[channel], samples, count, nb_channels);
        hdcd_detect_onech(&state[channel], &detect);
    hdcd_detect_end(&detect, nb_channels);

#### Each frame (stereo)
    hdcd_process_stereo(&state_stereo, samples, count);
    hdcd_detect_stereo(&state_stereo, &detect);

#### Use detection data
    char dstr[256];
    hdcd_detect_str(&detect, dstr, sizeof(dstr));
    printf("%s\n", dstr);

Example output:
> HDCD detected: yes (B:3893), peak_extend: enabled permanently, max_gain_adj: -4.0 dB, transient_filter: detected, detectable errors: 0

See `hdcd_detect_str()` in hdcd_decode2.c for an example of using individual `hdcd_detection_t` members.

### Analyze mode

A mode to aid in analysis of HDCD encoded audio. In this mode the audio is replaced by a solid tone and the amplitude is adjusted to signal some specified aspect of the process. The output can be loaded in an audio editor alongside the original, where the user can see where different features or states are present.
See hdcd_ana_mode_t in hdcd_decoder2.h. See also [HDCD § Analyze mode](http://wiki.hydrogenaud.io/index.php?title=High_Definition_Compatible_Digital#Analyze_mode)

    hdcd_set_analyze_mode(MODE);

    hdcd_set_analyze_mode_stereo(MODE);

