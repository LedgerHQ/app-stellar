#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include "stellar_api.h"
#include "stellar_format.h"

stellar_context_t ctx;
tx_context_t tx_ctx;

static const char *testcases[] = {
    "../testcases/feeBumpTxSimple.raw",
    "../testcases/feeBumpTxSimpleMuxedFeeSource.raw",
    "../testcases/txAccountMergeMuxedDestination.raw",
    "../testcases/txAccountMerge.raw",
    "../testcases/txAllowTrustAuthorized.raw",
    "../testcases/txAllowTrustAuthorizedToMaintainLiabilities.raw",
    "../testcases/txAllowTrustUnauthorized.raw",
    "../testcases/txBeginSponsoringFutureReserves.raw",
    "../testcases/txBumpSequence.raw",
    "../testcases/txChangeOffer.raw",
    "../testcases/txChangeTrust.raw",
    "../testcases/txChangeTrustLiquidityPoolAsset.raw",
    "../testcases/txClaimClaimableBalance.raw",
    "../testcases/txClawbackClaimableBalance.raw",
    "../testcases/txClawbackMuxedFrom.raw",
    "../testcases/txClawback.raw",
    "../testcases/txCreateAccount.raw",
    "../testcases/txCreateClaimableBalancePredicateAnd.raw",
    "../testcases/txCreateClaimableBalancePredicateBeforeAbs.raw",
    "../testcases/txCreateClaimableBalancePredicateBeforeRel.raw",
    "../testcases/txCreateClaimableBalancePredicateMultiClaimant.raw",
    "../testcases/txCreateClaimableBalancePredicateNot.raw",
    "../testcases/txCreateClaimableBalancePredicateOr.raw",
    "../testcases/txCreateClaimableBalancePredicateUnconditional.raw",
    "../testcases/txCreateOffer2.raw",
    "../testcases/txCreateOffer.raw",
    "../testcases/txCustomAsset12.raw",
    "../testcases/txCustomAsset4.raw",
    "../testcases/txEndSponsoringFutureReserves.raw",
    "../testcases/txInflation.raw",
    "../testcases/txLiquidityPoolDeposit.raw",
    "../testcases/txLiquidityPoolWithdraw.raw",
    "../testcases/txManageBuyOffer.raw",
    "../testcases/txMemoHash.raw",
    "../testcases/txMemoId.raw",
    "../testcases/txMemoText.raw",
    "../testcases/txMultiOp.raw",
    "../testcases/txMultiOpWithSource.raw",
    "../testcases/txOpSourceMuxedDestination.raw",
    "../testcases/txOpSource.raw",
    "../testcases/txPassiveOffer.raw",
    "../testcases/txPathPaymentStrictReceiveMuxedDestination.raw",
    "../testcases/txPathPaymentStrictReceiveEmptyPath.raw",
    "../testcases/txPathPaymentStrictReceive.raw",
    "../testcases/txPathPaymentStrictSendMuxedDestination.raw",
    "../testcases/txPathPaymentStrictSendEmptyPath.raw",
    "../testcases/txPathPaymentStrictSend.raw",
    "../testcases/txPaymentMuxedDestination.raw",
    "../testcases/txRemoveData.raw",
    "../testcases/txRevokeSponsorshipLiquidityPool.raw",
    "../testcases/txRemoveOffer.raw",
    "../testcases/txRemoveTrust.raw",
    "../testcases/txRemoveTrustLiquidityPoolAsset.raw",
    "../testcases/txRevokeSponsorshipAccount.raw",
    "../testcases/txRevokeSponsorshipClaimableBalance.raw",
    "../testcases/txRevokeSponsorshipData.raw",
    "../testcases/txRevokeSponsorshipOffer.raw",
    "../testcases/txRevokeSponsorshipSignerEd25519PublicKey.raw",
    "../testcases/txRevokeSponsorshipSignerHashX.raw",
    "../testcases/txRevokeSponsorshipSignerPreAuth.raw",
    "../testcases/txRevokeSponsorshipTrustLine.raw",
    "../testcases/txRevokeSponsorshipTrustLineLiquidityPoolID.raw",
    "../testcases/txSetAllOptions.raw",
    "../testcases/txSetData.raw",
    "../testcases/txSetOptionsEmptyBody.raw",
    "../testcases/txSetOptionsNoSigner.raw",
    "../testcases/txSetOptionsRemoveHomeDomain.raw",
    "../testcases/txSetSomeOptions.raw",
    "../testcases/txSetTrustLineFlags.raw",
    "../testcases/txSimpleMuxedSource.raw",
    "../testcases/txSimple.raw",
    "../testcases/txTimeBounds.raw",
    "../testcases/txTimeBoundsMaxIsZero.raw",
    "../testcases/txTimeBoundsMinIsZero.raw",
    NULL,
};

static void load_transaction_data(const char *filename, tx_context_t *txCtx) {
    FILE *f = fopen(filename, "rb");
    assert_non_null(f);

    txCtx->rawLength = fread(txCtx->raw, 1, MAX_RAW_TX, f);
    assert_int_not_equal(txCtx->rawLength, 0);
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
    uint8_t opCount = ctx.req.tx.txDetails.opCount;
    current_data_index = 0;
    get_result_filename(filename, path, sizeof(path));

    FILE *fp = fopen(path, "r");
    assert_non_null(fp);

    set_state_data(true);

    while ((opCount != 0 && current_data_index < opCount) ||
           formatter_stack[formatter_index] != NULL) {
        assert_non_null(fgets(line, sizeof(line), fp));

        char *expected_title = line;
        char *expected_value = strstr(line, "; ");
        assert_non_null(expected_value);

        *expected_value = '\x00';
        assert_string_equal(expected_title, detailCaption);

        expected_value += 2;
        char *p = strchr(expected_value, '\n');
        if (p != NULL) {
            *p = '\x00';
        }
        assert_string_equal(expected_title, detailCaption);
        assert_string_equal(expected_value, detailValue);

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
    memset(&ctx.req.tx, 0, sizeof(ctx.req.tx));

    load_transaction_data(filename, &ctx.req.tx);

    ctx.state = STATE_APPROVE_TX;
    assert_true(parse_tx_xdr(ctx.req.tx.raw, ctx.req.tx.rawLength, &ctx.req.tx));

    ctx.state = STATE_APPROVE_TX;

    check_transaction_results(filename);
}

void test_transactions(void **state) {
    (void) state;

    for (const char **testcase = testcases; *testcase != NULL; testcase++) {
        test_tx(*testcase);
    }
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_transactions),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
