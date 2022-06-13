#!/bin/bash -eu

# build fuzzers

pushd fuzz
./build.sh
#pwd
#ls
#ls ./cmake-build-fuzz
#ldd ./cmake-build-fuzz/fuzz_tx
#ls -lah /lib/x86_64-linux-gnu/libbsd.so.0
mv ./cmake-build-fuzz/fuzz_tx $OUT/app-stellar-fuzz-tx
popd

