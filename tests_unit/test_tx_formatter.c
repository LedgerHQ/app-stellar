#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <cmocka.h>

#include "stellar/formatter.h"
#include "stellar/parser.h"

#define MAX_ENVELOPE_SIZE 2048
#define MAX_CAPTION_SIZE  21
#define MAX_VALUE_SIZE    105

const char *testcases[] = {
    "../testcases/opCreateAccount.raw",
    "../testcases/opPaymentAssetNative.raw",
    "../testcases/opPaymentAssetAlphanum4.raw",
    "../testcases/opPaymentAssetAlphanum12.raw",
    "../testcases/opPaymentWithMuxedDestination.raw",
    "../testcases/opRestoreFootprint.raw",
    "../testcases/opPathPaymentStrictReceive.raw",
    "../testcases/opPathPaymentStrictReceiveWithEmptyPath.raw",
    "../testcases/opPathPaymentStrictReceiveWithMuxedDestination.raw",
    "../testcases/opManageSellOfferCreate.raw",
    "../testcases/opManageSellOfferUpdate.raw",
    "../testcases/opManageSellOfferDelete.raw",
    "../testcases/opCreatePassiveSellOffer.raw",
    "../testcases/opSetOptions.raw",
    "../testcases/opSetOptionsWithEmptyBody.raw",
    "../testcases/opSetOptionsAddPublicKeySigner.raw",
    "../testcases/opSetOptionsRemovePublicKeySigner.raw",
    "../testcases/opSetOptionsAddHashXSigner.raw",
    "../testcases/opSetOptionsRemoveHashXSigner.raw",
    "../testcases/opSetOptionsAddPreAuthTxSigner.raw",
    "../testcases/opSetOptionsRemovePreAuthTxSigner.raw",
    "../testcases/opSetOptionsAddEd25519SignerPayloadSigner.raw",
    "../testcases/opSetOptionsRemoveEd25519SignerPayloadSigner.raw",
    "../testcases/opChangeTrustAddTrustLine.raw",
    "../testcases/opChangeTrustRemoveTrustLine.raw",
    "../testcases/opChangeTrustWithLiquidityPoolAssetAddTrustLine.raw",
    "../testcases/opChangeTrustWithLiquidityPoolAssetRemoveTrustLine.raw",
    "../testcases/opAllowTrustDeauthorize.raw",
    "../testcases/opAllowTrustAuthorize.raw",
    "../testcases/opAllowTrustAuthorizeToMaintainLiabilities.raw",
    "../testcases/opAccountMerge.raw",
    "../testcases/opAccountMergeWithMuxedDestination.raw",
    "../testcases/opInflation.raw",
    "../testcases/opInvokeHostFunctionAssetApprove.raw",
    "../testcases/opInvokeHostFunctionScvalsCase0.raw",
    "../testcases/opInvokeHostFunctionScvalsCase1.raw",
    "../testcases/opInvokeHostFunctionScvalsCase2.raw",
    "../testcases/opInvokeHostFunctionScvalsCase3.raw",
    "../testcases/opInvokeHostFunctionAssetTransfer.raw",
    "../testcases/opInvokeHostFunctionCreateContractNewAsset.raw",
    "../testcases/opInvokeHostFunctionCreateContractWasmId.raw",
    "../testcases/opInvokeHostFunctionCreateContractWrapAsset.raw",
    "../testcases/opInvokeHostFunctionWithoutArgs.raw",
    "../testcases/opInvokeHostFunctionUploadWasm.raw",
    "../testcases/opInvokeHostFunctionWithAuth.raw",
    "../testcases/opInvokeHostFunctionWithAuthAndNoArgsAndNoSource.raw",
    "../testcases/opInvokeHostFunctionWithAuthAndNoArgs.raw",
    "../testcases/opInvokeHostFunctionWithoutAuthAndNoSource.raw",
    "../testcases/opInvokeHostFunctionTransferXlm.raw",
    "../testcases/opInvokeHostFunctionTransferUsdc.raw",
    "../testcases/opInvokeHostFunctionApproveUsdc.raw",
    "../testcases/opInvokeHostFunctionWithComplexSubInvocation.raw",
    "../testcases/opInvokeHostFunctionTestPlugin.raw",
    "../testcases/opManageDataAdd.raw",
    "../testcases/opManageDataAddWithUnprintableData.raw",
    "../testcases/opManageDataRemove.raw",
    "../testcases/opBumpSequence.raw",
    "../testcases/opManageBuyOfferCreate.raw",
    "../testcases/opManageBuyOfferUpdate.raw",
    "../testcases/opManageBuyOfferDelete.raw",
    "../testcases/opPathPaymentStrictSend.raw",
    "../testcases/opPathPaymentStrictSendWithEmptyPath.raw",
    "../testcases/opPathPaymentStrictSendWithMuxedDestination.raw",
    "../testcases/opCreateClaimableBalance.raw",
    "../testcases/opClaimClaimableBalance.raw",
    "../testcases/opBeginSponsoringFutureReserves.raw",
    "../testcases/opEndSponsoringFutureReserves.raw",
    "../testcases/opExtendFootprintTtl.raw",
    "../testcases/opRevokeSponsorshipAccount.raw",
    "../testcases/opRevokeSponsorshipTrustLineWithAsset.raw",
    "../testcases/opRevokeSponsorshipTrustLineWithLiquidityPoolId.raw",
    "../testcases/opRevokeSponsorshipOffer.raw",
    "../testcases/opRevokeSponsorshipData.raw",
    "../testcases/opRevokeSponsorshipClaimableBalance.raw",
    "../testcases/opRevokeSponsorshipLiquidityPool.raw",
    "../testcases/opRevokeSponsorshipEd25519PublicKeySigner.raw",
    "../testcases/opRevokeSponsorshipHashXSigner.raw",
    "../testcases/opRevokeSponsorshipPreAuthTxSigner.raw",
    "../testcases/opClawback.raw",
    "../testcases/opClawbackWithMuxedFrom.raw",
    "../testcases/opClawbackClaimableBalance.raw",
    "../testcases/opSetTrustLineFlagsUnauthorized.raw",
    "../testcases/opSetTrustLineFlagsAuthorized.raw",
    "../testcases/opSetTrustLineFlagsAuthorizedToMaintainLiabilities.raw",
    "../testcases/opSetTrustLineFlagsAuthorizedAndClawbackEnabled.raw",
    "../testcases/opLiquidityPoolDeposit.raw",
    "../testcases/opLiquidityPoolWithdraw.raw",
    "../testcases/opWithEmptySource.raw",
    "../testcases/opWithMuxedSource.raw",
    "../testcases/txMemoNone.raw",
    "../testcases/txMemoId.raw",
    "../testcases/txMemoText.raw",
    "../testcases/txMemoTextUnprintable.raw",
    "../testcases/txMemoHash.raw",
    "../testcases/txMemoReturnHash.raw",
    "../testcases/txCondWithAllItems.raw",
    "../testcases/txCondIsNone.raw",
    "../testcases/txCondTimeBounds.raw",
    "../testcases/txCondTimeBoundsMaxIsZero.raw",
    "../testcases/txCondTimeBoundsMinIsZero.raw",
    "../testcases/txCondTimeBoundsAreZero.raw",
    "../testcases/txCondTimeBoundsIsNone.raw",
    "../testcases/txCondLedgerBounds.raw",
    "../testcases/txCondLedgerBoundsMaxIsZero.raw",
    "../testcases/txCondLedgerBoundsMinIsZero.raw",
    "../testcases/txCondLedgerBoundsAreZero.raw",
    "../testcases/txCondMinAccountSequence.raw",
    "../testcases/txCondMinAccountSequenceAge.raw",
    "../testcases/txCondMinAccountSequenceLedgerGap.raw",
    "../testcases/txCondExtraSignersWithOneSigner.raw",
    "../testcases/txCondExtraSignersWithTwoSigners.raw",
    "../testcases/txMultiOperations.raw",
    "../testcases/txCustomBaseFee.raw",
    "../testcases/txWithMuxedSource.raw",
    "../testcases/txNetworkPublic.raw",
    "../testcases/txNetworkTestnet.raw",
    "../testcases/txNetworkCustom.raw",
    "../testcases/feeBumpTx.raw",
    "../testcases/feeBumpTxWithMuxedFeeSource.raw",
    "../testcases/txSourceOmitSourceEqualSigner.raw",
    "../testcases/txSourceOmitSourceNotEqualSigner.raw",
    "../testcases/txSourceOmitMuxedSourceEqualSigner.raw",
    "../testcases/feeBumpTxOmitFeeSourceEqualSigner.raw",
    "../testcases/feeBumpTxOmitFeeSourceNotEqualSigner.raw",
    "../testcases/feeBumpTxOmitMuxedFeeSourceEqualSigner.raw",
    "../testcases/opSourceOmitTxSourceEqualOpSourceEqualSigner.raw",
    "../testcases/opSourceOmitTxSourceEqualOpSourceNotEqualSigner.raw",
    "../testcases/opSourceOmitOpSourceEqualSignerNotEqualTxSource.raw",
    "../testcases/opSourceOmitTxSourceEqualSignerNotEqualOpSource.raw",
    "../testcases/opSourceOmitTxMuxedSourceEqualOpMuxedSourceEqualSigner.raw",
    "../testcases/opSourceOmitTxSourceEqualOpMuxedSourceEqualSigner.raw",
    "../testcases/opSourceOmitTxMuxedSourceEqualOpSourceEqualSigner.raw",
};

static bool is_string_empty(const char *str) {
    return str == NULL || str[0] == '\0';
}

static void get_result_filename(const char *filename, char *path, size_t size) {
    strncpy(path, filename, size);

    char *ext = strstr(path, ".raw");
    assert_non_null(ext);
    memcpy(ext, ".txt", 4);
}

void test_format_envelope(void **state) {
    const char *filename = (char *) *state;
    char result_filename[1024] = {0};
    get_result_filename(filename, result_filename, sizeof(result_filename));

    FILE *file = fopen(filename, "rb");
    assert_non_null(file);

    envelope_t envelope;

    memset(&envelope, 0, sizeof(envelope_t));
    uint8_t data[MAX_ENVELOPE_SIZE];
    size_t read_count = fread(data, sizeof(char), MAX_ENVELOPE_SIZE, file);

    assert_true(parse_transaction_envelope(data, read_count, &envelope));

    char caption[MAX_CAPTION_SIZE];
    char value[MAX_VALUE_SIZE];
    uint8_t signing_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                             0xbd, 0x59, 0xce, 0xa9, 0x7,  0x9e, 0x7c, 0xc7, 0xc,  0xe7, 0xb1,
                             0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};

    formatter_data_t fdata = {.raw_data = data,
                              .raw_data_len = read_count,
                              .envelope = &envelope,
                              .signing_key = signing_key,
                              .caption = caption,
                              .value = value,
                              .value_len = MAX_VALUE_SIZE,
                              .caption_len = MAX_CAPTION_SIZE,
                              .display_sequence = true};

    char output[4096] = {0};
    bool data_exists = true;
    bool is_op_header = false;
    reset_formatter();
    while (true) {
        assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
        if (!data_exists) {
            break;
        }
        char temp[256] = {0};
        sprintf(temp,
                "%s;%s%s\n",
                fdata.caption,
                is_string_empty(fdata.value) ? "" : " ",
                fdata.value);
        strcat(output, temp);
    }

    char expected_result[4096] = {0};
    FILE *result_file = fopen(result_filename, "r");
    assert_non_null(result_file);
    fread(expected_result, sizeof(char), 4096, result_file);
    assert_string_equal(output, expected_result);

    fclose(file);
    fclose(result_file);
}

void test_formatter_forward(void **state) {
    (void) state;

    const char *filename = "../testcases/txMultiOperations.raw";

    FILE *file = fopen(filename, "rb");
    assert_non_null(file);

    envelope_t envelope;

    memset(&envelope, 0, sizeof(envelope_t));
    uint8_t data[MAX_ENVELOPE_SIZE];
    size_t read_count = fread(data, sizeof(char), MAX_ENVELOPE_SIZE, file);

    assert_true(parse_transaction_envelope(data, read_count, &envelope));

    char caption[MAX_CAPTION_SIZE];
    char value[MAX_VALUE_SIZE];
    uint8_t signing_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                             0xbd, 0x59, 0xce, 0xa9, 0x7,  0x9e, 0x7c, 0xc7, 0xc,  0xe7, 0xb1,
                             0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};

    formatter_data_t fdata = {.raw_data = data,
                              .raw_data_len = read_count,
                              .envelope = &envelope,
                              .signing_key = signing_key,
                              .caption = caption,
                              .value = value,
                              .value_len = MAX_VALUE_SIZE,
                              .caption_len = MAX_CAPTION_SIZE,
                              .display_sequence = true};

    bool data_exists = false;
    bool is_op_header = false;
    reset_formatter();

    // Flow:
    // Memo Text; hello world
    // Max Fee; 0.00003 XLM
    // Sequence Num; 103720918407102568
    // Valid Before (UTC); 2022-12-12 04:12:12
    // Tx Source; GDUTHC..XM2FN7
    // Operation; 1 of 3
    // Send; 922,337,203,685.4775807 XLM
    // Destination; GDRMNAIPTNIJWJSL6JOF76CJORN47TDVMWERTXO2G2WKOMXGNHUFL5QX
    // Op Source; GDUTHC..XM2FN7
    // Operation; 2 of 3
    // Send; 922,337,203,685.4775807 BTC@GAT..MTCH
    // Destination; GDRMNAIPTNIJWJSL6JOF76CJORN47TDVMWERTXO2G2WKOMXGNHUFL5QX
    // Op Source; GDUTHC..XM2FN7
    // Operation; 3 of 3
    // Operation Type; Set Options
    // Home Domain; stellar.org

    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Memo Text");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Max Fee");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Sequence Num");
    assert_true(get_next_data(&fdata, false, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Memo Text");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Max Fee");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Sequence Num");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Valid Before (UTC)");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Tx Source");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_true(is_op_header);
    assert_string_equal(fdata.caption, "Operation");
    assert_string_equal(fdata.value, "1 of 3");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Send");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Destination");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Op Source");

    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_true(is_op_header);
    assert_string_equal(fdata.caption, "Operation");
    assert_string_equal(fdata.value, "2 of 3");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Send");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Destination");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Op Source");

    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_true(is_op_header);
    assert_string_equal(fdata.caption, "Operation");
    assert_string_equal(fdata.value, "3 of 3");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Operation Type");
    assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Home Domain");

    assert_true(get_next_data(&fdata, false, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_true(is_op_header);
    assert_string_equal(fdata.caption, "Operation");
    assert_string_equal(fdata.value, "3 of 3");

    assert_true(get_next_data(&fdata, false, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_true(is_op_header);
    assert_string_equal(fdata.caption, "Operation");
    assert_string_equal(fdata.value, "2 of 3");

    assert_true(get_next_data(&fdata, false, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_true(is_op_header);
    assert_string_equal(fdata.caption, "Operation");
    assert_string_equal(fdata.value, "1 of 3");

    assert_true(get_next_data(&fdata, false, &data_exists, &is_op_header));
    assert_true(data_exists);
    assert_false(is_op_header);
    assert_string_equal(fdata.caption, "Memo Text");

    assert_true(get_next_data(&fdata, false, &data_exists, &is_op_header));
    assert_false(data_exists);
}

int main() {
    size_t testcases_len = sizeof(testcases) / sizeof(testcases[0]) + 1;
    struct CMUnitTest tests[testcases_len];
    for (int i = 0; i < testcases_len - 1; i++) {
        tests[i].name = testcases[i];
        tests[i].test_func = test_format_envelope;
        tests[i].initial_state = (void *) testcases[i];
        tests[i].setup_func = NULL;
        tests[i].teardown_func = NULL;
    }
    tests[testcases_len - 1].name = "test_formatter_forward";
    tests[testcases_len - 1].test_func = test_formatter_forward;
    tests[testcases_len - 1].initial_state = NULL;
    tests[testcases_len - 1].setup_func = NULL;
    tests[testcases_len - 1].teardown_func = NULL;
    return cmocka_run_group_tests(tests, NULL, NULL);
}
