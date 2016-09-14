#!/bin/bash

# Run tests on the HDCD decoder library, against previous results
AST=""
case $1 in
    -bin)
        HDCD_DETECT="$2"
        AST="(bin: \"$HDCD_DETECT\")"
    ;;
    -win)
        HDCD_DETECT="./wine-hdcd-detect.sh"
        AST="(bin: \"$HDCD_DETECT\")"
    ;;
    *)
        HDCD_DETECT="./hdcd-detect"
    ;;
esac

if [ ! -f "$HDCD_DETECT" ]; then
    echo "Not found: \"$HDCD_DETECT\""
    exit 1;
fi
echo "BIN is \"$HDCD_DETECT\""
"$HDCD_DETECT" -v

if [ -d "/run/shm" ]; then
    TMP="/run/shm"
elif [ -d "/dev/shm" ]; then
    TMP="/dev/shm"
else
    TMP="/tmp"
fi
echo "TMP is $TMP"

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
    TFILE="test/hdcd.wav"
    THASH="5db465a58d2fd0d06ca944b883b33476"
    TFILEK="test/hdcd.raw"
    THASHK="5db465a58d2fd0d06ca944b883b33476"
    TOUT="$TMP/hdcd_tests_pipes_$$"
    echo "-test-pipes:"
    cat "$TFILE"  | "$HDCD_DETECT" -qcp - |"$MD5SUM" >"$TOUT.md5"
    cat "$TFILEK" | "$HDCD_DETECT" -kqcr  |"$MD5SUM" >"$TOUT.md5.k"
    sed -i -e "s#^\([0-9a-f]*\).*#\1#" "$TOUT.md5"
    sed -i -e "s#^\([0-9a-f]*\).*#\1#" "$TOUT.md5.k"
    echo "$THASH" >"$TOUT.md5.target"
    echo "$THASHK" >"$TOUT.md5.k.target"
    RESULT=$(diff "$TOUT.md5" "$TOUT.md5.target")
    RESULTK=$(diff "$TOUT.md5.k" "$TOUT.md5.k.target")
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
    rm -f "$TOUT.md5" "$TOUT.md5.k" "$TOUT.md5.target" "$TOUT.md5.k.target"
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
    if [ -n "$THASH" ]; then
        echo "   -o = $TOUT" >>"$TOUT.summary"
    fi
    echo "   test_file = $TFILE" >>"$TOUT.summary"
    if [ -n "$THASH" ]; then
        "$HDCD_DETECT" $TOPT -o "$TOUT" "$TFILE"
        HDEX=$?
    else
        "$HDCD_DETECT" $TOPT "$TFILE"
        HDEX=$?
    fi
    if ((HDEX != TEXI)); then
        cat "$TOUT.summary"
        echo "hdcd-detect returned unexpected exit code: $HDEX (wanted: $TEXI)"
        echo "-- FAILED [exit_code]"
        EXIT_CODE=1
        die_on_fail
    else
        if [ -n "$THASH" ]; then
            "$MD5SUM" "$TOUT" >"$TOUT.md5"
            sed -i -e "s#^\([0-9a-f]*\).*#\1#" "$TOUT.md5"
            echo "$THASH" >"$TOUT.md5.target"
            RESULT=$(diff "$TOUT.md5" "$TOUT.md5.target")
        else
            RESULT=""
        fi
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
#   do_test <options> <test_file> <md5_result> <exit_code> [<test_title>]

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

# test for differences from ffmpeg amode
do_test "-qxp -z pe"    "hdcd.wav"      "63b534310b3d993fa421a18da9552096" 0 "analyzer-pe-fate-match"

# pretend the raw source is some other sample rate, as I don't have any actual non-44.1kHz HDCD to test
do_test "-qxpr -e 48000"  "hdcd.raw"      "5db465a58d2fd0d06ca944b883b33476" 0 "rate-48000"

# using a test sample with cdt expirations to catch a cdt period calculation change
do_test "-qxpr -z cdt -e 48000"  "hdcd-mix.raw"  "7eb08190462b8ebd0ce17d363600dc28" 0 "rate-48000-cdt"
do_test "-qxpr -z cdt -e 88200"  "hdcd-mix.raw"  "5902691d220dbeba69693ada7756a842" 0 "rate-88200-cdt"
do_test "-qxpr -z cdt -e 96000"  "hdcd-mix.raw"  "82b757c2b7480d623e5e9f2f99f61a3f" 0 "rate-96000-cdt"
do_test "-qxpr -z cdt -e 176400" "hdcd-mix.raw"  "1afa2b589dc51ea8bb9e7cb07d363f04" 0 "rate-176400-cdt"
do_test "-qxpr -z cdt -e 192000" "hdcd-mix.raw"  "2f431c0df402438b919f804082eab6b6" 0 "rate-192000-cdt"

# 20-bit and 24-bit
do_test "-qx -e 44100:20" "hdcd20.wav" "" 0 "hdcd-20bit-yes"
do_test "-qx"             "hdcd20.wav" "" 1 "hdcd-20bit-no"
do_test "-qx"             "hdcd24.wav" "" 0 "hdcd-24bit"

echo "passed: $PASSED / $TESTS $AST"
echo "exit: $EXIT_CODE"
exit $EXIT_CODE
