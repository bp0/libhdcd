#!/bin/bash

# Run tests on the HDCD decoder library, against previous results

HDCD_DETECT="./hdcd-detect"
TMP="/tmp"
#TMP="/run/shm"

EXIT_CODE=0

do_test() {
    TOPT="$1"
    TFILE="test/$2.wav"
    THASH="$3"
    TOUT="$TMP/hdcd_tests_$$.wav"
    TEXI="$4"
    if [ -z "$TEXI" ]; then TEXI=0; fi
    TTIT="$5"
    if [ -z "$TTIT" ]; then TTIT="$2"; fi
    echo "$TTIT:"
    "$HDCD_DETECT" $TOPT "$TFILE" "$TOUT"
    HDEX=$?
    if ((HDEX != TEXI)); then
        echo "hdcd-detect returned unexpected exit code: $HDEX (wanted: $TEXI)"
        echo "   bin = $HDCD_DETECT"
        echo "   opt = $TOPT"
        echo "   tfile = $TFILE"
        echo "   temp = $TOUT"
        echo "-- FAILED [exit_code]"
        EXIT_CODE=1
    else
        md5sum "$TOUT" >"$TOUT.md5"
        sed -i -e "s#^\([0-9a-f]*\).*#\1#" "$TOUT.md5"
        echo "$THASH" >"$TOUT.md5.target"
        RESULT=$(diff "$TOUT.md5" "$TOUT.md5.target")
        if [ -n "$RESULT" ]; then
            echo "$RESULT"
            echo "-- FAILED [md5_result]"
            EXIT_CODE=1
        else
            echo "-- PASSED"
        fi
    fi
    rm -f "$TOUT" "$TOUT.md5" "$TOUT.md5.target"
}

# format:
#   do_test <options> <test_file> <md5_result> [<exit_code> [<test_title>]]

# hdcd.wav has PE only
do_test "-qx" "hdcd"      "835d9eca6c8e762f512806b0eeac42bd" 0

# hdcd-all.wav has PE, LLE, and TF
do_test "-qx" "hdcd-all"  "da671fe3351ffc6e156913b88911829c" 0

# hdcd-err.wav has encoding errors
do_test "-qx" "hdcd-err"  "fbc703becf0502e4f1c44c9af8f7db8d" 0

# hdcd-ftm.wav is from For the Masses (1998), a notorious HDCD-CD.
do_test "-qx" "hdcd-ftm"  "c7b16edf2b7c36531b551f791da986f6" 0 "for-the-masses"

# hdcd-tgm has a very short target gain mismatch
do_test "-qx" "hdcd-tgm"  "f3cf4d7fbe2ffbab53a3698730c140d1" 0

# ava16 is not HDCD, but has a coincidental valid HDCD packet that
# applies -6dB in one channel for a short time if target_gain matching is
# not happening. HDCD should be "not detected"
do_test "-qx" "ava16"     "a44fea1a2c825ed24f57f35a328d7874" 1


exit $EXIT_CODE
