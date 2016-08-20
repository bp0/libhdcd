#!/bin/bash

mkdir -p bin
cd bin

LIBDER=$(pwd)
LIBNAME=libhdcd_decode2

gcc -fPIC -g -c ../hdcd_decode2.c
gcc -shared -O2 -o $LIBNAME.so hdcd_decode2.o -lc -lm
rm hdcd_decode2.o

gcc -o hdcd-detect -O2 ../tool/hdcd-detect.c ../tool/wavreader.c -L$LIBDER -lhdcd_decode2
rm -f hdcd-detect.o wavreader.o

#export LD_LIBRARY_PATH=$LIBDER:$LD_LIBRARY_PATH

cd ..
