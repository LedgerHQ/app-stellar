#!/bin/bash
set -e

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
BUILDDIR="$SCRIPTDIR/cmake-build-fuzz-coverage"
CORPUSDIR="$SCRIPTDIR/corpus"
HTMLCOVDIR="$SCRIPTDIR/html-coverage"

# Compile the fuzzer with code coverage support
rm -rf "$BUILDDIR" "$HTMLCOVDIR"
cmake -DBOLOS_SDK=/opt/ledger-secure-sdk -DCMAKE_C_COMPILER=clang -DCODE_COVERAGE=1 -B"$BUILDDIR" -H.
cmake --build "$BUILDDIR" --target fuzz_tx

# Run the fuzzer on the corpus files
export LLVM_PROFILE_FILE="$BUILDDIR/fuzz_tx.%p.profraw"
# "$BUILDDIR/fuzz_tx" "$CORPUSDIR"/*
"$BUILDDIR/fuzz_tx" -rss_limit_mb=1024 -max_len=20000 -max_total_time=600 -print_final_stats=1 "$CORPUSDIR" -jobs=4 -workers=4
llvm-profdata merge --sparse "$BUILDDIR"/fuzz_tx.*.profraw -o "$BUILDDIR/fuzz_tx.profdata"

# Exclude lib_standard_app directory, base32 and base64 code from coverage report
llvm-cov show "$BUILDDIR/fuzz_tx" -instr-profile="$BUILDDIR/fuzz_tx.profdata" -show-line-counts-or-regions -output-dir="$HTMLCOVDIR" -format=html -ignore-filename-regex="(.*lib_standard_app.*)|(.*libstellar/base64\.c.*)|(.*libstellar/base32\.c.*)"
llvm-cov report "$BUILDDIR/fuzz_tx" -instr-profile="$BUILDDIR/fuzz_tx.profdata" -ignore-filename-regex="(.*lib_standard_app.*)|(.*libstellar/base64\.c.*)|(.*libstellar/base32\.c.*)"
