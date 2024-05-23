#!/bin/bash -eu

# build fuzzers

pushd fuzz
cmake -DBOLOS_SDK=../BOLOS_SDK -Bbuild -H.
make -C build
mv ./build/fuzz_tx $OUT/app-stellar-fuzz-tx
popd