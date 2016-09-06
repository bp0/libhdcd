#!/bin/sh

# wrapper for testing the windows build via wine
# run build-win.sh before running tests

wine "./win-bin/hdcd-detect.exe" $@
