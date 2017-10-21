#!/bin/bash
gcc test/parsertest.c src/stlr_utils.c src/base32.c src/crc16.c src/xdr_parser.c -o obj/parsertest -I src/ -D TEST
./obj/parsertest test/simpleTx.hex