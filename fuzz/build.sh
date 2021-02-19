#!/usr/bin/env bash

SCRIPTDIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"
BUILDDIR=$SCRIPTDIR/cmake-build-fuzz

# Compile fuzzer
rm -rf "$BUILDDIR"
mkdir "$BUILDDIR"
cd "$BUILDDIR" || exit

cmake -DCMAKE_C_COMPILER=clang -DFUZZ=1 ../../tests
make clean
make fuzz_tx
