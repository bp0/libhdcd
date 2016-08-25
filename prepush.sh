#!/bin/bash

# before pushing, test things

./autogen.sh && ./configure && make && ./tests.sh

