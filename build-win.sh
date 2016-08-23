#!/bin/bash

LIBNAME=libhdcd

mkdir -p win-bin
cd win-bin

MGCC=i686-w64-mingw32-gcc
"$MGCC" -c ../hdcd_decode2.c ../hdcd_simple.c ../hdcd_libversion.c
"$MGCC" -shared -o $LIBNAME.dll hdcd_decode2.o  hdcd_libversion.o  hdcd_simple.o -Wl,--out-implib,$LIBNAME.a
rm -f hdcd_decode2.o hdcd_simple.o hdcd_libversion.o

"$MGCC" -c ../tool/hdcd-detect.c ../tool/wavreader.c
"$MGCC" -o hdcd-detect.exe hdcd-detect.o wavreader.o -L. -l$LIBNAME
rm -f hdcd-detect.o wavreader.o

cd ..
