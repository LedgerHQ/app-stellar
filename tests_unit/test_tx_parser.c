#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <cmocka.h>
#include "stellar/parser.h"

#define MAX_ENVELOPE_SIZE 2048

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

void test_parse_data(void **state) {
    const char *filename = (char *) *state;
    FILE *file = fopen(filename, "rb");
    assert_non_null(file);
    envelope_t envelope;

    memset(&envelope, 0, sizeof(envelope_t));
    uint8_t data[MAX_ENVELOPE_SIZE];
    size_t read_count = fread(data, sizeof(char), MAX_ENVELOPE_SIZE, file);
    assert_true(parse_transaction_envelope(data, read_count, &envelope));
    for (uint8_t i = 0; i < envelope.tx_details.tx.operations_count; i++) {
        assert_true(parse_transaction_operation(data, read_count, &envelope, i));
    }
}

int main() {
    struct CMUnitTest tests[sizeof(testcases) / sizeof(testcases[0])];
    for (int i = 0; i < sizeof(testcases) / sizeof(testcases[0]); i++) {
        tests[i].name = testcases[i];
        tests[i].test_func = test_parse_data;
        tests[i].initial_state = (void *) testcases[i];
        tests[i].setup_func = NULL;
        tests[i].teardown_func = NULL;
    }
    return cmocka_run_group_tests(tests, NULL, NULL);
}