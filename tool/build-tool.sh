#!/bin/bash

gcc hdcd-detect.c wavreader.c ../hdcd_decode2.c -lm -o hdcd_detect
