#!/bin/bash

gcc hdcd-detect.c wavreader.c ../hdcd_decode2.c -lm -o hdcd-detect
rm -f hdcd-detect.o wavreader.o
