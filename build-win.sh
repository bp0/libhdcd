#!/bin/bash

LIBNAME=libhdcd_decode2

mkdir -p win-bin
cd win-bin

MGCC=i686-w64-mingw32-gcc
"$MGCC" -c ../hdcd_decode2.c
"$MGCC" -shared -o $LIBNAME.dll hdcd_decode2.o -Wl,--out-implib,$LIBNAME.a
rm -f hdcd_decode2.o

"$MGCC" -c ../tool/hdcd-detect.c ../tool/wavreader.c
"$MGCC" -o hdcd-detect.exe hdcd-detect.o wavreader.o -L. -l$LIBNAME
rm -f hdcd-detect.o wavreader.o

cd ..
