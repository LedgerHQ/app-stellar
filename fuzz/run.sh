#!/usr/bin/env bash

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
BUILDDIR=$SCRIPTDIR/cmake-build-fuzz
CORPUSDIR=$SCRIPTDIR/../tests/testcases
INPUTDIR=$SCRIPTDIR/inputs

mkdir -p "$INPUTDIR"
cp "$CORPUSDIR"/*.raw "$INPUTDIR"

"$BUILDDIR"/fuzz_tx "$INPUTDIR" > /dev/null
