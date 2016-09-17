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
# output as wav to test the wav writer
do_test "-qx"  "hdcd.wav"     "a5bc65fa360802a19403fa4afd6338af" 0 "hdcd-output-wav"

# hdcd-all.wav has PE, LLE, and TF
do_test "-qxp" "hdcd-all.wav"  "e8cdf508b7805ed49aaba2f3e12c1bfe" 0

# hdcd-err.wav has encoding errors
do_test "-qxp" "hdcd-err.wav"  "0f7c3581950e57564d72ad96200bb648" 0

# hdcd-ftm.wav is from For the Masses (1998), a notorious HDCD-CD.
do_test "-qxp" "hdcd-ftm.wav"  "c8c094ad88f43cb9eda1fa2d9b121664" 0 "for-the-masses"

# hdcd-tgm has a very short target gain mismatch
do_test "-qxp" "hdcd-tgm.wav"  "c512b16b00f867105c68ef93fea08fb7" 0

# hdcd-pfa has packet format A and is "special mode" with a max gain adjust of -7.0dB
do_test "-qxp" "hdcd-pfa.wav"  "760628f8e3c81e7f7f94fdf594decd61" 0 "pfa-special-mode"

# ava16 is not HDCD, but has a coincidental valid HDCD packet that
# applies -6dB in one channel for a short time if target_gain matching is
# not happening. HDCD should be "not detected"
do_test "-qxp" "ava16.wav"     "b179a5cc8d8a0b8461236807ae55bdd3" 1

# tests -n (nop) in mkmix, then tests mixed packet format
do_test "-qxrp"  "hdcd-mix.raw"  "6a3cf7f920f419477ada264cc63b40da" 0 "hdt-nop-cat-mixed-pf"

# another way that "verify files before testing" might have worked
do_test "-qxrpn" "hdcd.raw"      "ca3ce18c754bd008d7f308c2892cb795" 1 "hdt-nop"

# analyzer tests
do_test "-qxp  -z pe"     "hdcd-all.wav"  "6c180e3e734f0d670221e99109a9460d" 0 "analyzer-pe"
do_test "-qxp  -z lle"    "hdcd-all.wav"  "a097872ca7f7ffc94b133a84accc928f" 0 "analyzer-lle"
do_test "-qxrp -z cdt"    "hdcd-mix.raw"  "b10600fc87011c6cc6c048d1a4e7da8d" 0 "analyzer-cdt"
do_test "-qxp  -z tgm"    "hdcd-tgm.wav"  "5dbaeb406e30ce1398eca609c9f357aa" 0 "analyzer-tgm"
do_test "-qxp  -z pel"    "ava16.wav"     "f79cb2b91e21d2b1fa07d007a71c6bc9" 1 "analyzer-pel"
do_test "-qxp  -z tgm"    "hdcd-ftm.wav"  "b44f967875b4c2f2225dbebb16d3e3df" 0 "analyzer-tgm-for-the-masses"
do_test "-qxp  -z ltgm"   "hdcd-ftm.wav"  "0eab0a7d41fd8e0c35266cbd8cb834ac" 0 "analyzer-lgtm-for-the-masses"
do_test "-qxp  -z ltgm"   "ava16.wav"     "33c2de6018e2debab2b3384696c83abb" 1 "analyzer-lgtm-nch-process-false-positive"
do_test "-qxp  -z cdt"    "ava16.wav"     "d1a8dab7a6fb718d5e09031b45e8b5b5" 1 "analyzer-cdt-nch-process-false-positive"

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
do_test "-qx"             "hdcd20.wav"     "" 0 "hdcd-20bit"
do_test "-qx"             "hdcd24.wav"     "" 0 "hdcd-24bit"
# actual 20-bit, but the WAV header claims 24-bit, and HDCD isn't found
do_test "-qx"             "hdcd20in24.wav" "" 1 "hdcd-20bit-in24-no"
# specifiy 20-bit, and HDCD is found
do_test "-qx -e 44100:20" "hdcd20in24.wav" "" 0 "hdcd-20bit-in24-yes"

echo "passed: $PASSED / $TESTS $AST"
echo "exit: $EXIT_CODE"
exit $EXIT_CODE
