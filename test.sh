#!/bin/bash
mkdir -p obj
gcc test/parsertest.c src/stellar_utils.c src/stellar_parser.c test/test_utils.c -o obj/parsertest -I src/ -I test/ -D TEST
./obj/parsertest test/txSimple.hex
./obj/parsertest test/txMemoId.hex
./obj/parsertest test/txMemoText.hex
./obj/parsertest test/txMemoHash.hex
./obj/parsertest test/txOpAccountId.hex
./obj/parsertest test/txCustomAsset4.hex
./obj/parsertest test/txCustomAsset12.hex
./obj/parsertest test/txTimeBounds.hex
./obj/parsertest test/txChangeTrust.hex
./obj/parsertest test/txManageOffer.hex
./obj/parsertest test/txCreateAccount.hex
./obj/parsertest test/txPathPayment.hex
./obj/parsertest test/txAccountMerge.hex
./obj/parsertest test/txManageData.hex
./obj/parsertest test/txAllowTrust.hex
./obj/parsertest test/txSetAllOptions.hex
./obj/parsertest test/txSetSomeOptions.hex
./obj/parsertest test/txMultiOp2.hex
gcc test/utilstest.c src/stellar_utils.c test/test_utils.c -o obj/utilstest -I src/ -I test/ -D TEST
./obj/utilstest
