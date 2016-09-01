#!/bin/bash

echo "{ start test/mkmix.sh }"
rm -f hdcd-mix.raw
../hdcd-detect -qpn -o hdcd-mix0.raw hdcd-pfa.wav
../hdcd-detect -ri hdcd-mix0.raw
../hdcd-detect -qpn -o hdcd-mix1.raw hdcd-tgm.wav
../hdcd-detect -ri hdcd-mix1.raw
../hdcd-detect -qpn -o hdcd-mix2.raw ava16.wav
../hdcd-detect -ri hdcd-mix2.raw
cat hdcd-mix0.raw hdcd-mix1.raw hdcd-mix2.raw >hdcd-mix.raw
../hdcd-detect -rd hdcd-mix.raw 2>&1 | grep "^\."
rm hdcd-mix{0,1,2}.raw
echo "{ end test/mkmix.sh }"
