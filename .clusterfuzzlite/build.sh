#!/bin/bash -eu

# build fuzzers

pushd fuzz
cmake -DCMAKE_C_COMPILER=clang -Bbuild -H.
make -C build
mv ./build/fuzz_tx $OUT/app-stellar-fuzz-tx
popd