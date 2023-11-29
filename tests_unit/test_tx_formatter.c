#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <cmocka.h>

#include "transaction/transaction_parser.h"
#include "transaction/transaction_formatter.h"

static const char *testcases[] = {
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
    "../testcases/opInvokeHostFunctionScvals.raw",
    "../testcases/opInvokeHostFunctionAssetTransfer.raw",
    "../testcases/opInvokeHostFunctionCreateContractNewAsset.raw",
    "../testcases/opInvokeHostFunctionCreateContractWasmId.raw",
    "../testcases/opInvokeHostFunctionCreateContractWrapAsset.raw",
    "../testcases/opInvokeHostFunctionUnverifiedContract.raw",
    "../testcases/opInvokeHostFunctionUnverifiedContractWithApproveFunction.raw",
    "../testcases/opInvokeHostFunctionUnverifiedContractWithTransferFunction.raw",
    "../testcases/opInvokeHostFunctionUploadWasm.raw",
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

static void load_transaction_data(const char *filename, tx_ctx_t *tx_ctx) {
    FILE *f = fopen(filename, "rb");
    assert_non_null(f);

    tx_ctx->raw_size = fread(tx_ctx->raw, 1, RAW_TX_MAX_SIZE, f);
    assert_int_not_equal(tx_ctx->raw_size, 0);
    fclose(f);
}

static void get_result_filename(const char *filename, char *path, size_t size) {
    strncpy(path, filename, size);

    char *ext = strstr(path, ".raw");
    assert_non_null(ext);
    memcpy(ext, ".txt", 4);
}

static void check_transaction_results(const char *filename) {
    char path[1024];
    char line[4096];
    uint8_t op_cnt = G_context.tx_info.tx_details.operations_count;
    G.ui.current_data_index = 0;
    get_result_filename(filename, path, sizeof(path));

    FILE *fp = fopen(path, "r");
    assert_non_null(fp);

    set_state_data(true);

    while ((op_cnt != 0 && G.ui.current_data_index < op_cnt) ||
           formatter_stack[formatter_index] != NULL) {
        assert_non_null(fgets(line, sizeof(line), fp));

        char *expected_title = line;
        char *expected_value = strstr(line, "; ");
        assert_non_null(expected_value);

        *expected_value = '\x00';
        assert_string_equal(expected_title, G.ui.detail_caption);

        expected_value += 2;
        char *p = strchr(expected_value, '\n');
        if (p != NULL) {
            *p = '\x00';
        }
        assert_string_equal(expected_title, G.ui.detail_caption);
        assert_string_equal(expected_value, G.ui.detail_value);

        formatter_index++;

        if (formatter_stack[formatter_index] != NULL) {
            set_state_data(true);
        }
    }
    assert_int_equal(fgets(line, sizeof(line), fp), 0);
    assert_int_equal(feof(fp), 1);
    fclose(fp);
}

static void test_tx(const char *filename) {
    memset(&G_context.tx_info, 0, sizeof(G_context.tx_info));

    load_transaction_data(filename, &G_context.tx_info);

    // GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7
    uint8_t public_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                            0xbd, 0x59, 0xce, 0xa9, 0x7,  0x9e, 0x7c, 0xc7, 0xc,  0xe7, 0xb1,
                            0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};
    assert_true(
        parse_tx_xdr(G_context.tx_info.raw, G_context.tx_info.raw_size, &G_context.tx_info));
    memcpy(G_context.raw_public_key, public_key, sizeof(public_key));

    check_transaction_results(filename);
}

void test_transactions(void **state) {
    (void) state;

    for (int i = 0; i < sizeof(testcases) / sizeof(testcases[0]); i++) {
        test_tx(testcases[i]);
    }
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_transactions),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
