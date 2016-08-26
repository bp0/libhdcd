#!/bin/bash

# before pushing, test things

make clean
./autogen.sh && ./configure && make && ./tests.sh

