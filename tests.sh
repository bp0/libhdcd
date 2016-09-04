#!/bin/bash

# Run tests on the HDCD decoder library, against previous results

if [ -d "/run/shm" ]; then
    TMP="/run/shm"
elif [ -d "/dev/shm" ]; then
    TMP="/dev/shm"
else
    TMP="/tmp"
fi
echo "TMP is $TMP"

HDCD_DETECT="./hdcd-detect"
MD5SUM=$(which md5sum)
if [ -z "$MD5SUM" ]; then
    echo "Requires md5sum"
    exit 1;
fi

EXIT_CODE=0
TESTS=0
PASSED=0

die_on_fail() {
    EXIT_CODE=$EXIT_CODE # nop in case the next line is commented
    # if prefer die on fail then un-comment the next line
    exit $EXIT_CODE
}

test_pipes() {
    ((TESTS++))
    TFILE="test/hdcd.raw"
    THASH="5db465a58d2fd0d06ca944b883b33476"
    TOUT="$TMP/hdcd_tests_pipes_$$"
    echo "-test-pipes:"
    "$HDCD_DETECT" -qcrp - <"$TFILE" |"$MD5SUM" >"$TOUT.md5"
    "$HDCD_DETECT" -kqcr <"$TFILE" |"$MD5SUM" >"$TOUT.md5.k"
    sed -i -e "s#^\([0-9a-f]*\).*#\1#" "$TOUT.md5"
    sed -i -e "s#^\([0-9a-f]*\).*#\1#" "$TOUT.md5.k"
    echo "$THASH" >"$TOUT.md5.target"
    RESULT=$(diff "$TOUT.md5" "$TOUT.md5.target")
    RESULTK=$(diff "$TOUT.md5.k" "$TOUT.md5.target")
    if [ -n "$RESULT" ] || [ -n "$RESULTK" ]; then
        echo "N: $RESULT"
        echo "K: $RESULTK"
        echo "-- FAILED [md5_result]"
        EXIT_CODE=1
        die_on_fail
    else
        echo "-- PASSED"
        ((PASSED++))
    fi
    rm -f "$TOUT.md5" "$TOUT.md5.k" "$TOUT.md5.target"
}

do_test() {
    TOPT="-j $1"
    TFILE="test/$2"
    THASH="$3"
    TOUT="$TMP/hdcd_tests_$$.wav"
    TEXI="$4"
    if [ -z "$TEXI" ]; then TEXI=0; fi
    TTIT="$5"
    if [ -z "$TTIT" ]; then TTIT="$2"; fi
    ((TESTS++))
    echo "$TTIT:"
    echo "   bin = $HDCD_DETECT" >"$TOUT.summary"
    echo "   opt = $TOPT" >>"$TOUT.summary"
    echo "   -o = $TOUT" >>"$TOUT.summary"
    echo "   test_file = $TFILE" >>"$TOUT.summary"
    "$HDCD_DETECT" $TOPT -o "$TOUT" "$TFILE"
    HDEX=$?
    if ((HDEX != TEXI)); then
        cat "$TOUT.summary"
        echo "hdcd-detect returned unexpected exit code: $HDEX (wanted: $TEXI)"
        echo "-- FAILED [exit_code]"
        EXIT_CODE=1
        die_on_fail
    else
        "$MD5SUM" "$TOUT" >"$TOUT.md5"
        sed -i -e "s#^\([0-9a-f]*\).*#\1#" "$TOUT.md5"
        echo "$THASH" >"$TOUT.md5.target"
        RESULT=$(diff "$TOUT.md5" "$TOUT.md5.target")
        if [ -n "$RESULT" ]; then
            cat "$TOUT.summary"
            echo "$RESULT"
            echo "-- FAILED [md5_result]"
            EXIT_CODE=1
            die_on_fail
        else
            echo "-- PASSED"
            ((PASSED++))
        fi
    fi
    rm -f "$TOUT" "$TOUT.md5" "$TOUT.md5.target" "$TOUT.summary"
}

mkmix() {
    cd test
    bash ./mkmix.sh
    cd ..
}

verify_files() {
    echo "Verifying test files..."
    cd test
    md5sum -c file_sums
    SR=$?
    cd ..
    if ((SR != 0)); then
        exit 1;
    fi
}

verify_files

mkmix

test_pipes

# format:
#   do_test <options> <test_file> <md5_result> [<exit_code> [<test_title>]]

# these (hopefully) match:
#  1.  ffmpeg -i test/hdcd.wav -af hdcd -f s24le md5:
#  2.  hdcd-detect -pc test/hdcd.wav |md5sum
#  3.  hdcd-detect -kpc <test/hdcd.wav |md5sum
do_test "-qxp"  "hdcd.wav"     "5db465a58d2fd0d06ca944b883b33476" 0 "hdcd-output-pcm-only"
do_test "-ksxp" "hdcd.wav"     "5db465a58d2fd0d06ca944b883b33476" 0 "hdcd-output-pcm-only-cjk-hdcd-compat"
do_test "-ksxr" "hdcd.raw"     "5db465a58d2fd0d06ca944b883b33476" 0 "hdcd-output-pcm-only-cjk-hdcd-compat-raw"

# hdcd.wav has PE only
do_test "-qx"  "hdcd.wav"     "835d9eca6c8e762f512806b0eeac42bd" 0 "hdcd-output-wav"

# hdcd-all.wav has PE, LLE, and TF
do_test "-qx" "hdcd-all.wav"  "da671fe3351ffc6e156913b88911829c" 0

# hdcd-err.wav has encoding errors
do_test "-qx" "hdcd-err.wav"  "fbc703becf0502e4f1c44c9af8f7db8d" 0

# hdcd-ftm.wav is from For the Masses (1998), a notorious HDCD-CD.
do_test "-qx" "hdcd-ftm.wav"  "c7b16edf2b7c36531b551f791da986f6" 0 "for-the-masses"

# hdcd-tgm has a very short target gain mismatch
do_test "-qx" "hdcd-tgm.wav"  "f3cf4d7fbe2ffbab53a3698730c140d1" 0

# hdcd-pfa has packet format A and is "special mode" with a max gain adjust of -7.0dB
do_test "-qxp" "hdcd-pfa.wav"  "760628f8e3c81e7f7f94fdf594decd61" 0 "pfa-special-mode"

# ava16 is not HDCD, but has a coincidental valid HDCD packet that
# applies -6dB in one channel for a short time if target_gain matching is
# not happening. HDCD should be "not detected"
do_test "-qx" "ava16.wav"     "a44fea1a2c825ed24f57f35a328d7874" 1

# tests -n (nop) in mkmix, then tests mixed packet format
do_test "-qxrp"  "hdcd-mix.raw"  "6a3cf7f920f419477ada264cc63b40da" 0 "hdt-nop-cat-mixed-pf"

# another way that "verify files before testing" might have worked
do_test "-qxrpn" "hdcd.raw"      "ca3ce18c754bd008d7f308c2892cb795" 1 "hdt-nop"

# analyzer tests
do_test "-qx -z pe"     "hdcd-all.wav"  "8dad6ce72136e973f7d5ee61e35b501c" 0 "analyzer-pe"
do_test "-qx -z lle"    "hdcd-all.wav"  "26cf3b5a5ae999c6358f4ea79b01c38a" 0 "analyzer-lle"
do_test "-qxr -z cdt"   "hdcd-mix.raw"  "ec9963ed629ee020a593a047612fcc9f" 0 "analyzer-cdt"
do_test "-qx -z tgm"    "hdcd-tgm.wav"  "6e1a07529d4cfb16e8e3639d75a7083d" 0 "analyzer-tgm"
do_test "-qx -z pel"    "ava16.wav"     "63b6f873049794eb7a899d9a5c6c8a5e" 1 "analyzer-pel"
do_test "-qx -z tgm"    "hdcd-ftm.wav"  "aba5385269206f2dc63b04c4dfdd05c9" 0 "analyzer-tgm-for-the-masses"
do_test "-qx -z ltgm"   "hdcd-ftm.wav"  "f386afaa1a1cd4e47c21f0b8dc541e21" 0 "analyzer-lgtm-for-the-masses"
do_test "-qx -z ltgm"   "ava16.wav"     "aed4aa16a7cb6c13ec72d7715ab78a10" 1 "analyzer-lgtm-nch-process-false-positive"
do_test "-qx -z cdt"    "ava16.wav"     "eb93df7b31c6b68bd64aff314ca1bc23" 1 "analyzer-cdt-nch-process-false-positive"

# this will not match after the change to the tone generator in libhdcd, unless ffmpeg af_hdcd's
# is changed in the same way.
#do_test "-qxp -z pe"    "hdcd.wav"      "81a4f00f85a585bc0198e9a0670a8cde" 0 "analyzer-pe-fate-match"

echo "passed: $PASSED / $TESTS"
echo "exit: $EXIT_CODE"
exit $EXIT_CODE
