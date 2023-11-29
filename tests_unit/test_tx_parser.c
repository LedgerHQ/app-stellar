#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <cmocka.h>

#include "transaction/transaction_parser.h"

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

static void parse_tx(const char *filename) {
    FILE *f = fopen(filename, "rb");
    assert_non_null(f);
    tx_ctx_t tx_info;
    memset(&tx_info, 0, sizeof(tx_ctx_t));
    tx_info.raw_size = fread(tx_info.raw, 1, RAW_TX_MAX_SIZE, f);
    if (!parse_tx_xdr(tx_info.raw, tx_info.raw_size, &tx_info)) {
        fail_msg("parse %s failed!", filename);
    }
}

void test_parse() {
    for (int i = 0; i < sizeof(testcases) / sizeof(testcases[0]); i++) {
        parse_tx(testcases[i]);
    }
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_parse)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
