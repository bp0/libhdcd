#!/bin/bash

mkdir -p bin
cd bin

gcc ../tool/hdcd-detect.c ../tool/wavreader.c ../hdcd_decode2.c -lm -o hdcd-detect.static
rm -f *.o

cd ..
