#!/bin/bash

# build for coverage testing, run tests, generate html

CFLAGS="-O0 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -Werror=missing-prototypes -fprofile-arcs -ftest-coverage -g"
LFLAGS="-lgcov --coverage"
LIBNAME=libhdcd

if [ -z `which lcov` ]; then echo "Needs lcov"; exit 1; fi
if [ -z `which genhtml` ]; then echo "Needs genhtml"; exit 1; fi

mkdir -p gcov-bin
cd gcov-bin

rm *.gcda *.gcno

gcc $CFLAGS -fPIC -c ../src/hdcd_decode2.c ../src/hdcd_simple.c ../src/hdcd_libversion.c ../src/hdcd_analyze_tonegen.c ../src/hdcd_strings.c
ar crsu $LIBNAME.a hdcd_decode2.o hdcd_libversion.o hdcd_simple.o hdcd_analyze_tonegen.o hdcd_strings.o
gcc -shared -s -o $LIBNAME.dll hdcd_decode2.o hdcd_libversion.o hdcd_simple.o hdcd_analyze_tonegen.o hdcd_strings.o

rm -f hdcd-detect.o wavreader.o wavout.o
rm -f hdcd_decode2.o hdcd_simple.o hdcd_libversion.o hdcd_analyze_tonegen.o hdcd_strings.o

gcc $CFLAGS -c ../tool/hdcd-detect.c ../tool/wavio.c
gcc -s -o hdcd-detect-gcov hdcd-detect.o wavio.o $LIBNAME.a $LFLAGS
rm -f hdcd-detect.o wavio.o

cd ..
./tests.sh -bin gcov-bin/hdcd-detect-gcov
gcov-bin/hdcd-detect-gcov -h 2>/dev/null
lcov --capture --directory gcov-bin --output-file coverage.info
genhtml coverage.info --output-directory lcov-html

