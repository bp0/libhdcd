#!/bin/bash

# Run tests on the HDCD decoder library, against previous results

TMP="/tmp"
#TMP="/run/shm"

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
    exit EXIT_CODE
}

do_test() {
    TOPT="$1"
    TFILE="test/$2"
    THASH="$3"
    TOUT="$TMP/hdcd_tests_$$.wav"
    TEXI="$4"
    if [ -z "$TEXI" ]; then TEXI=0; fi
    TTIT="$5"
    if [ -z "$TTIT" ]; then TTIT="$2"; fi
    ((TESTS++))
    echo "$TTIT:"
    "$HDCD_DETECT" $TOPT -o "$TOUT" "$TFILE"
    HDEX=$?
    if ((HDEX != TEXI)); then
        echo "hdcd-detect returned unexpected exit code: $HDEX (wanted: $TEXI)"
        echo "   bin = $HDCD_DETECT"
        echo "   opt = $TOPT"
        echo "   -o = $TOUT"
        echo "   test_file = $TFILE"
        echo "-- FAILED [exit_code]"
        EXIT_CODE=1
        die_on_fail
    else
        "$MD5SUM" "$TOUT" >"$TOUT.md5"
        sed -i -e "s#^\([0-9a-f]*\).*#\1#" "$TOUT.md5"
        echo "$THASH" >"$TOUT.md5.target"
        RESULT=$(diff "$TOUT.md5" "$TOUT.md5.target")
        if [ -n "$RESULT" ]; then
            echo "$RESULT"
            echo "-- FAILED [md5_result]"
            EXIT_CODE=1
            die_on_fail
        else
            echo "-- PASSED"
            ((PASSED++))
        fi
    fi
    rm -f "$TOUT" "$TOUT.md5" "$TOUT.md5.target"
}

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

# ava16 is not HDCD, but has a coincidental valid HDCD packet that
# applies -6dB in one channel for a short time if target_gain matching is
# not happening. HDCD should be "not detected"
do_test "-qx" "ava16.wav"     "a44fea1a2c825ed24f57f35a328d7874" 1

# analyzer tests
do_test "-qx -z pe"     "hdcd-all.wav"  "769ce35ba609d6cf90f525db3be6cc92" 0 "analyzer-pe"
do_test "-qxp -z pe"    "hdcd.wav"      "81a4f00f85a585bc0198e9a0670a8cde" 0 "analyzer-pe-fate-match"
do_test "-qx -z lle"    "hdcd-all.wav"  "cbec0b7d20475c4c0b0e341e3b354bd4" 0 "analyzer-lle"
do_test "-qx -z cdt"    "hdcd-all.wav"  "3a3410e9a0646ea9a25e9ac2124cbeea" 0 "analyzer-cdt"
do_test "-qx -z tgm"    "hdcd-tgm.wav"  "45df3641d1023fed4621c4ae6ee45800" 0 "analyzer-tgm"
do_test "-qx -z pel"    "ava16.wav"     "74bb1f47db2bcf0c35d38437f147ab8f" 1 "analyzer-pel"
do_test "-qx -z tgm"    "hdcd-ftm.wav"  "d4559c4149cb6629ea7a5cde775e380e" 0 "analyzer-tgm-for-the-masses"
do_test "-qx -z ltgm"   "hdcd-ftm.wav"  "7656237a5f8171d58113b5fe5b718738" 0 "analyzer-lgtm-for-the-masses"
do_test "-qx -z ltgm"   "ava16.wav"     "5479204e34eeedf714122c314f779c83" 1 "analyzer-lgtm-nch-process-false-positive"
do_test "-qx -z cdt"    "ava16.wav"     "d54f7e54623b6bfbe78d127e69fefe6e" 1 "analyzer-cdt-nch-process-false-positive"


echo "passed: $PASSED / $TESTS"
echo "exit: $EXIT_CODE"
exit $EXIT_CODE
