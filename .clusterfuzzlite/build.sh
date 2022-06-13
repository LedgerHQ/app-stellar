#!/bin/bash -eu

# build fuzzers

pushd fuzz
./build.sh
mv ./cmake-build-fuzz/fuzz_tx $OUT/app-stellar-fuzz-tx
popd

