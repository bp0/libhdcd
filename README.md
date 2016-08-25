# libhdcd

A stand-alone library for High Definition Compatible Digital (HDCD) decoding and analysis based on foo_hdcd and ffmpeg's af_hdcd. See AUTHORS.

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

Simple API
----------
### Include

    #include <hdcd/hdcd_simple.h>

### Declare and initialize

    hdcd_simple_t *ctx = shdcd_new();

### Each frame
    
    shdcd_process(ctx, samples, count);

### Song change, seek, etc.

    shdcd_reset(ctx);  /* reset the decoder state */

It is best to shdcd_reset() and then seek a couple seconds behind the target to let the the decoder process samples and catch the nearest packet. The samples leading up to the seek target can then be discarded.

### Cleanup
    
    shdcd_free(ctx);

### Analyze mode

A mode to aid in analysis of HDCD encoded audio. In this mode the audio is replaced by a solid tone and the amplitude is adjusted to signal some specified aspect of the process. The output can be loaded in an audio editor alongside the original, where the user can see where different features or states are present.
See hdcd_ana_mode_t in hdcd_analyze.h. See also [HDCD § Analyze mode](http://wiki.hydrogenaud.io/index.php?title=High_Definition_Compatible_Digital#Analyze_mode)

    hdcd_set_analyze_mode(MODE);

    hdcd_set_analyze_mode_stereo(MODE);

