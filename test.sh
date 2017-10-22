#!/bin/bash
mkdir -p obj
gcc test/parsertest.c src/stlr_utils.c src/base32.c src/crc16.c src/xdr_parser.c test/test_utils.c -o obj/parsertest -I src/ -I test/ -D TEST
./obj/parsertest test/simpleTx.hex
