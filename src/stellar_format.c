#include "bolos_target.h"

#include "stellar_format.h"
#include "stellar_vars.h"
#include "stellar_api.h"

char opCaption[OPERATION_CAPTION_MAX_SIZE];
char detailCaption[DETAIL_CAPTION_MAX_SIZE];
char detailValue[DETAIL_VALUE_MAX_SIZE];

format_function_t formatter_stack[MAX_FORMATTERS_PER_OPERATION];
int8_t formatter_index;

static void push_to_formatter_stack(format_function_t formatter) {
    if (formatter_index + 1 >= MAX_FORMATTERS_PER_OPERATION) {
        THROW(0x6124);
    }
    formatter_stack[formatter_index + 1] = formatter;
}

static void format_transaction_source(tx_context_t *txCtx) {
    strcpy(detailCaption, "Tx Source");
    print_muxed_account(&txCtx->txDetails.sourceAccount, detailValue, 0, 0);
    push_to_formatter_stack(NULL);
}

static void format_time_bounds_max_time(tx_context_t *txCtx) {
    strcpy(detailCaption, "Time Bounds To");
    print_uint(txCtx->txDetails.timeBounds.maxTime, detailValue, DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_transaction_source);
}

static void format_time_bounds_min_time(tx_context_t *txCtx) {
    strcpy(detailCaption, "Time Bounds From");
    print_uint(txCtx->txDetails.timeBounds.minTime, detailValue, DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_time_bounds_max_time);
}

static void format_time_bounds(tx_context_t *txCtx) {
    if (txCtx->txDetails.hasTimeBounds) {
        format_time_bounds_min_time(txCtx);
    } else {
        format_transaction_source(txCtx);
    }
}

static void format_network(tx_context_t *txCtx) {
    strcpy(detailCaption, "Network");
    strlcpy(detailValue, (char *) PIC(NETWORK_NAMES[txCtx->network]), DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_time_bounds);
}

static void format_fee(tx_context_t *txCtx) {
    strcpy(detailCaption, "Fee");
    Asset asset = {.type = ASSET_TYPE_NATIVE};
    print_amount(txCtx->txDetails.fee, &asset, txCtx->network, detailValue, DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_network);
}

static void format_memo(tx_context_t *txCtx) {
    Memo *memo = &txCtx->txDetails.memo;
    switch (memo->type) {
        case MEMO_ID: {
            strcpy(detailCaption, "Memo ID");
            print_uint(memo->id, detailValue, DETAIL_VALUE_MAX_SIZE);
            break;
        }
        case MEMO_TEXT: {
            strcpy(detailCaption, "Memo Text");
            strlcpy(detailValue, memo->text, MEMO_TEXT_MAX_SIZE + 1);
            break;
        }
        case MEMO_HASH: {
            strcpy(detailCaption, "Memo Hash");
            print_binary_summary(memo->hash, detailValue, HASH_SIZE);
            break;
        }
        case MEMO_RETURN: {
            strcpy(detailCaption, "Memo Return");
            print_binary_summary(memo->hash, detailValue, HASH_SIZE);
            break;
        }
        default: {
            strcpy(detailCaption, "Memo");
            strcpy(detailValue, "[none]");
        }
    }
    push_to_formatter_stack(&format_fee);
}

static void format_confirm_transaction_details(tx_context_t *txCtx) {
    format_memo(txCtx);
}

static void format_operation_source(tx_context_t *txCtx) {
    if (txCtx->opDetails.sourceAccountPresent) {
        strcpy(detailCaption, "Op Source");
        print_muxed_account(&txCtx->opDetails.sourceAccount, detailValue, 0, 0);
        push_to_formatter_stack(&format_confirm_transaction_details);
    } else {
        if (txCtx->opIdx == txCtx->opCount) {
            // last operation: show transaction details
            format_confirm_transaction_details(txCtx);
        } else {
            // more operations: show next operation
            formatter_stack[formatter_index] = NULL;
            set_state_data(true);
        }
    }
}

static void format_bump_sequence_bump_to(tx_context_t *txCtx) {
    strcpy(detailCaption, "Bump To");
    print_int(txCtx->opDetails.bumpSequenceOp.bumpTo, detailValue, DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_operation_source);
}

static void format_bump_sequence(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Bump Sequence");
    push_to_formatter_stack(&format_bump_sequence_bump_to);
}

static void format_inflation(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Inflation");
    push_to_formatter_stack(&format_operation_source);
}

static void format_account_merge_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    print_muxed_account(&txCtx->opDetails.destination, detailValue, 0, 0);
    push_to_formatter_stack(&format_operation_source);
}

static void format_account_merge_detail(tx_context_t *txCtx) {
    strcpy(detailCaption, "Merge Account");
    if (txCtx->opDetails.sourceAccountPresent) {
        print_muxed_account(&txCtx->opDetails.sourceAccount, detailValue, 0, 0);
    } else {
        print_muxed_account(&txCtx->txDetails.sourceAccount, detailValue, 0, 0);
    }
    push_to_formatter_stack(&format_account_merge_destination);
}

static void format_account_merge(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Account Merge");
    push_to_formatter_stack(&format_account_merge_detail);
}

static void format_manage_data_value(tx_context_t *txCtx) {
    strcpy(detailCaption, "Data Value");
    char tmp[89];
    base64_encode(txCtx->opDetails.manageDataOp.dataValue,
                  txCtx->opDetails.manageDataOp.dataValueSize,
                  tmp);
    print_summary(tmp, detailValue, 12, 12);
    push_to_formatter_stack(&format_operation_source);
}

static void format_manage_data_detail(tx_context_t *txCtx) {
    if (txCtx->opDetails.manageDataOp.dataValueSize) {
        strcpy(detailCaption, "Set Data");
        push_to_formatter_stack(&format_manage_data_value);
    } else {
        strcpy(detailCaption, "Remove Data");
        push_to_formatter_stack(&format_operation_source);
    }
    char tmp[65];
    memcpy(tmp, txCtx->opDetails.manageDataOp.dataName, txCtx->opDetails.manageDataOp.dataNameSize);
    tmp[txCtx->opDetails.manageDataOp.dataNameSize] = '\0';
    print_summary(tmp, detailValue, 12, 12);
}

static void format_manage_data(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Manage Data");
    push_to_formatter_stack(&format_manage_data_detail);
}

static void format_allow_trust_authorize(tx_context_t *txCtx) {
    strcpy(detailCaption, "Authorize Flag");
    if (txCtx->opDetails.allowTrustOp.authorize == AUTHORIZED_FLAG) {
        strlcpy(detailValue, "AUTHORIZED_FLAG", DETAIL_VALUE_MAX_SIZE);
    } else if (txCtx->opDetails.allowTrustOp.authorize == AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG) {
        strlcpy(detailValue, "AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG", DETAIL_VALUE_MAX_SIZE);
    } else {
        strlcpy(detailValue, "UNAUTHORIZED_FLAG", DETAIL_VALUE_MAX_SIZE);
    }
    push_to_formatter_stack(&format_operation_source);
}

static void format_allow_trust_asset_code(tx_context_t *txCtx) {
    strcpy(detailCaption, "Asset Code");
    strlcpy(detailValue, txCtx->opDetails.allowTrustOp.assetCode, DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_allow_trust_authorize);
}

static void format_allow_trust_trustor(tx_context_t *txCtx) {
    strcpy(detailCaption, "Trustor");
    print_public_key(txCtx->opDetails.allowTrustOp.trustor, detailValue, 0, 0);
    push_to_formatter_stack(&format_allow_trust_asset_code);
}

static void format_allow_trust(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Allow Trust");
    push_to_formatter_stack(&format_allow_trust_trustor);
}

static void format_set_option_signer_weight(tx_context_t *txCtx) {
    if (txCtx->opDetails.setOptionsOp.signer.weight) {
        strcpy(detailCaption, "Weight");
        print_uint(txCtx->opDetails.setOptionsOp.signer.weight, detailValue, DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_operation_source);
    } else {
        format_operation_source(txCtx);
    }
}

static void format_set_option_signer_detail(tx_context_t *txCtx) {
    strcpy(detailCaption, "Signer Key");
    SignerKey *key = &txCtx->opDetails.setOptionsOp.signer.key;

    switch (key->type) {
        case SIGNER_KEY_TYPE_ED25519: {
            print_public_key(key->data, detailValue, 0, 0);
            break;
        }
        case SIGNER_KEY_TYPE_HASH_X: {
            char tmp[57];
            encode_hash_x_key(key->data, tmp);
            print_summary(tmp, detailValue, 12, 12);
            break;
        }

        case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
            char tmp[57];
            encode_pre_auth_key(key->data, tmp);
            print_summary(tmp, detailValue, 12, 12);
            break;
        }
    }
    push_to_formatter_stack(&format_set_option_signer_weight);
}

static void format_set_option_signer(tx_context_t *txCtx) {
    if (txCtx->opDetails.setOptionsOp.signerPresent) {
        signer_t *signer = &txCtx->opDetails.setOptionsOp.signer;
        if (signer->weight) {
            strcpy(detailCaption, "Add Signer");
        } else {
            strcpy(detailCaption, "Remove Signer");
        }
        switch (signer->key.type) {
            case SIGNER_KEY_TYPE_ED25519: {
                strcpy(detailValue, "Type Public Key");
                break;
            }
            case SIGNER_KEY_TYPE_HASH_X: {
                strcpy(detailValue, "Type Hash(x)");
                break;
            }
            case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
                strcpy(detailValue, "Type Pre-Auth");
                break;
            }
        }
        push_to_formatter_stack(&format_set_option_signer_detail);
    } else {
        format_operation_source(txCtx);
    }
}

static void format_set_option_home_domain(tx_context_t *txCtx) {
    if (txCtx->opDetails.setOptionsOp.homeDomainSize) {
        strcpy(detailCaption, "Home Domain");
        memcpy(detailValue,
               txCtx->opDetails.setOptionsOp.homeDomain,
               txCtx->opDetails.setOptionsOp.homeDomainSize);
        detailValue[txCtx->opDetails.setOptionsOp.homeDomainSize] = '\0';
        push_to_formatter_stack(&format_set_option_signer);
    } else {
        format_set_option_signer(txCtx);
    }
}

static void format_set_option_high_threshold(tx_context_t *txCtx) {
    if (txCtx->opDetails.setOptionsOp.highThresholdPresent) {
        strcpy(detailCaption, "High Threshold");
        print_uint(txCtx->opDetails.setOptionsOp.highThreshold, detailValue, DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_set_option_home_domain);
    } else {
        format_set_option_home_domain(txCtx);
    }
}

static void format_set_option_medium_threshold(tx_context_t *txCtx) {
    if (txCtx->opDetails.setOptionsOp.mediumThresholdPresent) {
        strcpy(detailCaption, "Medium Threshold");
        print_uint(txCtx->opDetails.setOptionsOp.mediumThreshold,
                   detailValue,
                   DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_set_option_high_threshold);
    } else {
        format_set_option_high_threshold(txCtx);
    }
}

static void format_set_option_low_threshold(tx_context_t *txCtx) {
    if (txCtx->opDetails.setOptionsOp.lowThresholdPresent) {
        strcpy(detailCaption, "Low Threshold");
        print_uint(txCtx->opDetails.setOptionsOp.lowThreshold, detailValue, DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_set_option_medium_threshold);
    } else {
        format_set_option_medium_threshold(txCtx);
    }
}

static void format_set_option_master_weight(tx_context_t *txCtx) {
    if (txCtx->opDetails.setOptionsOp.masterWeightPresent) {
        strcpy(detailCaption, "Master Weight");
        print_uint(txCtx->opDetails.setOptionsOp.masterWeight, detailValue, DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_set_option_low_threshold);
    } else {
        format_set_option_low_threshold(txCtx);
    }
}

static void format_set_option_set_flags(tx_context_t *txCtx) {
    if (txCtx->opDetails.setOptionsOp.setFlags) {
        strcpy(detailCaption, "Set Flags");
        print_flags(txCtx->opDetails.setOptionsOp.setFlags, detailValue, DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_set_option_master_weight);
    } else {
        format_set_option_master_weight(txCtx);
    }
}

static void format_set_option_clear_flags(tx_context_t *txCtx) {
    if (txCtx->opDetails.setOptionsOp.clearFlags) {
        strcpy(detailCaption, "Clear Flags");
        print_flags(txCtx->opDetails.setOptionsOp.clearFlags, detailValue, DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_set_option_set_flags);
    } else {
        format_set_option_set_flags(txCtx);
    }
}

static void format_set_option_inflation_destination(tx_context_t *txCtx) {
    if (txCtx->opDetails.setOptionsOp.inflationDestinationPresent) {
        strcpy(detailCaption, "Inflation Dest");
        print_public_key(txCtx->opDetails.setOptionsOp.inflationDestination, detailValue, 0, 0);
        push_to_formatter_stack(&format_set_option_clear_flags);
    } else {
        format_set_option_clear_flags(txCtx);
    }
}

static void format_set_options(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Set Options");
    push_to_formatter_stack(&format_set_option_inflation_destination);
}

static void format_change_trust_limit(tx_context_t *txCtx) {
    strcpy(detailCaption, "Trust Limit");
    if (txCtx->opDetails.changeTrustOp.limit == INT64_MAX) {
        strcpy(detailValue, "[maximum]");
    } else {
        print_amount(txCtx->opDetails.changeTrustOp.limit,
                     NULL,
                     txCtx->network,
                     detailValue,
                     DETAIL_VALUE_MAX_SIZE);
    }
    push_to_formatter_stack(&format_operation_source);
}

static void format_change_trust_detail(tx_context_t *txCtx) {
    if (txCtx->opDetails.changeTrustOp.limit) {
        strcpy(detailCaption, "Change Trust");
        push_to_formatter_stack(&format_change_trust_limit);
    } else {
        strcpy(detailCaption, "Remove Trust");
        push_to_formatter_stack(&format_operation_source);
    }
    uint8_t asset_type = txCtx->opDetails.changeTrustOp.line.type;
    if (asset_type != ASSET_TYPE_CREDIT_ALPHANUM4 && asset_type != ASSET_TYPE_CREDIT_ALPHANUM12) {
        return;
    }
    print_asset_t(&txCtx->opDetails.changeTrustOp.line,
                  txCtx->network,
                  detailValue,
                  DETAIL_VALUE_MAX_SIZE);
}

static void format_change_trust(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Change Trust");
    push_to_formatter_stack(&format_change_trust_detail);
}

static void format_manage_sell_offer_sell(tx_context_t *txCtx) {
    strcpy(detailCaption, "Sell");
    print_amount(txCtx->opDetails.manageSellOfferOp.amount,
                 &txCtx->opDetails.manageSellOfferOp.selling,
                 txCtx->network,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_operation_source);
}

static void format_manage_sell_offer_price(tx_context_t *txCtx) {
    strcpy(detailCaption, "Price");
    uint64_t price = ((uint64_t) txCtx->opDetails.manageSellOfferOp.price.n * 10000000) /
                     txCtx->opDetails.manageSellOfferOp.price.d;
    print_amount(price,
                 &txCtx->opDetails.manageSellOfferOp.buying,
                 txCtx->network,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_manage_sell_offer_sell);
}

static void format_manage_sell_offer_buy(tx_context_t *txCtx) {
    strcpy(detailCaption, "Buy");
    if (txCtx->opDetails.manageSellOfferOp.buying.type == ASSET_TYPE_NATIVE) {
        print_native_asset_code(txCtx->network, detailValue, DETAIL_VALUE_MAX_SIZE);
    } else {
        print_asset_t(&txCtx->opDetails.manageSellOfferOp.buying,
                      txCtx->network,
                      detailValue,
                      DETAIL_VALUE_MAX_SIZE);
    }
    push_to_formatter_stack(&format_manage_sell_offer_price);
}

static void format_manage_sell_offer_detail(tx_context_t *txCtx) {
    if (!txCtx->opDetails.manageSellOfferOp.amount) {
        strcpy(detailCaption, "Remove Offer");
        print_uint(txCtx->opDetails.manageSellOfferOp.offerID, detailValue, DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_operation_source);
    } else {
        if (txCtx->opDetails.manageSellOfferOp.offerID) {
            strcpy(detailCaption, "Change Offer");
            print_uint(txCtx->opDetails.manageSellOfferOp.offerID,
                       detailValue,
                       DETAIL_VALUE_MAX_SIZE);
        } else {
            strcpy(detailCaption, "Create Offer");
            strcpy(detailValue, "Type Active");
        }
        push_to_formatter_stack(&format_manage_sell_offer_buy);
    }
}

static void format_manage_sell_offer(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Manage Sell Offer");
    push_to_formatter_stack(&format_manage_sell_offer_detail);
}

static void format_manage_buy_offer_buy(tx_context_t *txCtx) {
    ManageBuyOfferOp *op = &txCtx->opDetails.manageBuyOfferOp;

    strcpy(detailCaption, "Buy");
    print_amount(op->buyAmount, &op->buying, txCtx->network, detailValue, DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_operation_source);
}

static void format_manage_buy_offer_price(tx_context_t *txCtx) {
    ManageBuyOfferOp *op = &txCtx->opDetails.manageBuyOfferOp;

    strcpy(detailCaption, "Price");
    uint64_t price = ((uint64_t) op->price.n * 10000000) / op->price.d;
    print_amount(price, &op->selling, txCtx->network, detailValue, DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_manage_buy_offer_buy);
}

static void format_manage_buy_offer_sell(tx_context_t *txCtx) {
    ManageBuyOfferOp *op = &txCtx->opDetails.manageBuyOfferOp;

    strcpy(detailCaption, "Sell");
    if (op->selling.type == ASSET_TYPE_NATIVE) {
        print_native_asset_code(txCtx->network, detailValue, DETAIL_VALUE_MAX_SIZE);
    } else {
        print_asset_t(&op->selling, txCtx->network, detailValue, DETAIL_VALUE_MAX_SIZE);
    }
    push_to_formatter_stack(&format_manage_buy_offer_price);
}

static void format_manage_buy_offer_detail(tx_context_t *txCtx) {
    ManageBuyOfferOp *op = &txCtx->opDetails.manageBuyOfferOp;

    if (op->buyAmount == 0) {
        strcpy(detailCaption, "Remove Offer");
        print_uint(op->offerID, detailValue, DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_operation_source);  // TODO
    } else {
        if (op->offerID) {
            strcpy(detailCaption, "Change Offer");
            print_uint(op->offerID, detailValue, DETAIL_VALUE_MAX_SIZE);
        } else {
            strcpy(detailCaption, "Create Offer");
            strcpy(detailValue, "Type Active");
        }
        push_to_formatter_stack(&format_manage_buy_offer_sell);
    }
}

static void format_manage_buy_offer(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Manage Buy Offer");
    push_to_formatter_stack(&format_manage_buy_offer_detail);
}

static void format_create_passive_sell_offer_sell(tx_context_t *txCtx) {
    strcpy(detailCaption, "Sell");
    print_amount(txCtx->opDetails.createPassiveSellOfferOp.amount,
                 &txCtx->opDetails.createPassiveSellOfferOp.selling,
                 txCtx->network,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_operation_source);
}

static void format_create_passive_sell_offer_price(tx_context_t *txCtx) {
    strcpy(detailCaption, "Price");

    CreatePassiveSellOfferOp *op = &txCtx->opDetails.createPassiveSellOfferOp;
    uint64_t price = ((uint64_t) op->price.n * 10000000) / op->price.d;
    print_amount(price, &op->buying, txCtx->network, detailValue, DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_create_passive_sell_offer_sell);
}

static void format_create_passive_sell_offer_buy(tx_context_t *txCtx) {
    strcpy(detailCaption, "Buy");
    if (txCtx->opDetails.createPassiveSellOfferOp.buying.type == ASSET_TYPE_NATIVE) {
        print_native_asset_code(txCtx->network, detailValue, DETAIL_VALUE_MAX_SIZE);
    } else {
        print_asset_t(&txCtx->opDetails.createPassiveSellOfferOp.buying,
                      txCtx->network,
                      detailValue,
                      DETAIL_VALUE_MAX_SIZE);
    }
    push_to_formatter_stack(&format_create_passive_sell_offer_price);
}

static void format_create_passive_sell_offer(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Create Passive Sell Offer");
    push_to_formatter_stack(&format_create_passive_sell_offer_buy);
}

static void format_path_via(tx_context_t *txCtx) {
    if (txCtx->opDetails.pathPaymentStrictReceiveOp.pathLen) {
        strcpy(detailCaption, "Via");
        uint8_t i;
        for (i = 0; i < txCtx->opDetails.pathPaymentStrictReceiveOp.pathLen; i++) {
            char asset_name[12 + 1];
            Asset *asset = &txCtx->opDetails.pathPaymentStrictReceiveOp.path[i];
            if (strlen(detailValue) != 0) {
                strlcat(detailValue, ", ", DETAIL_VALUE_MAX_SIZE);
            }
            print_asset_name(asset, txCtx->network, asset_name, sizeof(asset_name));
            strlcat(detailValue, asset_name, DETAIL_VALUE_MAX_SIZE);
        }
        push_to_formatter_stack(&format_operation_source);
    } else {
        format_operation_source(txCtx);
    }
}

static void format_path_payment_strict_receive_receive(tx_context_t *txCtx) {
    strcpy(detailCaption, "Receive");
    print_amount(txCtx->opDetails.pathPaymentStrictReceiveOp.destAmount,
                 &txCtx->opDetails.pathPaymentStrictReceiveOp.destAsset,
                 txCtx->network,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_path_via);
}

static void format_path_payment_strict_receive_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    print_muxed_account(&txCtx->opDetails.pathPaymentStrictReceiveOp.destination,
                        detailValue,
                        0,
                        0);
    push_to_formatter_stack(&format_path_payment_strict_receive_receive);
}

static void format_path_payment_strict_receive_send(tx_context_t *txCtx) {
    strcpy(detailCaption, "Send Max");
    print_amount(txCtx->opDetails.pathPaymentStrictReceiveOp.sendMax,
                 &txCtx->opDetails.pathPaymentStrictReceiveOp.sendAsset,
                 txCtx->network,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_path_payment_strict_receive_destination);
}

static void format_path_payment_strict_receive(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Path Payment Strict Receive");
    push_to_formatter_stack(&format_path_payment_strict_receive_send);
}

static void format_path_payment_strict_send_receive(tx_context_t *txCtx) {
    strcpy(detailCaption, "Receive Min");
    print_amount(txCtx->opDetails.pathPaymentStrictReceiveOp.destAmount,
                 &txCtx->opDetails.pathPaymentStrictReceiveOp.destAsset,
                 txCtx->network,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_path_via);
}

static void format_path_payment_strict_send_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    print_muxed_account(&txCtx->opDetails.pathPaymentStrictReceiveOp.destination,
                        detailValue,
                        0,
                        0);
    push_to_formatter_stack(&format_path_payment_strict_send_receive);
}

static void format_path_payment_strict_send(tx_context_t *txCtx) {
    strcpy(detailCaption, "Send");
    print_amount(txCtx->opDetails.pathPaymentStrictSendOp.sendAmount,
                 &txCtx->opDetails.pathPaymentStrictSendOp.sendAsset,
                 txCtx->network,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_path_payment_strict_send_destination);
}

static void format_payment_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    print_muxed_account(&txCtx->opDetails.payment.destination, detailValue, 0, 0);
    push_to_formatter_stack(&format_operation_source);
}

static void format_payment_send(tx_context_t *txCtx) {
    strcpy(detailCaption, "Send");
    print_amount(txCtx->opDetails.payment.amount,
                 &txCtx->opDetails.payment.asset,
                 txCtx->network,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_payment_destination);
}

static void format_payment(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Payment");
    push_to_formatter_stack(&format_payment_send);
}

static void format_create_account_amount(tx_context_t *txCtx) {
    strcpy(detailCaption, "Starting Balance");
    Asset asset = {.type = ASSET_TYPE_NATIVE};
    print_amount(txCtx->opDetails.createAccount.startingBalance,
                 &asset,
                 txCtx->network,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_operation_source);
}

static void format_create_account_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    print_public_key(txCtx->opDetails.createAccount.destination, detailValue, 0, 0);
    push_to_formatter_stack(&format_create_account_amount);
}

static void format_create_account(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Create Account");
    push_to_formatter_stack(&format_create_account_destination);
}

void format_create_claimable_balance_warning(tx_context_t *txCtx) {
    (void) txCtx;
    // TODO: The claimant can be very complicated. I haven't figured out how to
    // display it for the time being, so let's display an WARNING here first.
    strcpy(detailCaption, "WARNING");
    strcpy(detailValue, "Currently does not support displaying claimant details");
    push_to_formatter_stack(&format_operation_source);
}

static void format_create_claimable_balance_balance(tx_context_t *txCtx) {
    strcpy(detailCaption, "Balance");
    print_amount(txCtx->opDetails.createClaimableBalanceOp.amount,
                 &txCtx->opDetails.createClaimableBalanceOp.asset,
                 txCtx->network,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_create_claimable_balance_warning);
}

static void format_create_claimable_balance(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Create Claimable Balance");
    push_to_formatter_stack(&format_create_claimable_balance_balance);
}

static void format_claim_claimable_balance_balance_id(tx_context_t *txCtx) {
    strcpy(detailCaption, "Balance ID");
    print_claimable_balance_id(&txCtx->opDetails.claimClaimableBalanceOp.balanceID, detailValue);
    push_to_formatter_stack(&format_operation_source);
}

static void format_claim_claimable_balance(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Claim Claimable Balance");
    push_to_formatter_stack(&format_claim_claimable_balance_balance_id);
}

static void format_claim_claimable_balance_sponsored_id(tx_context_t *txCtx) {
    strcpy(detailCaption, "Sponsored ID");
    print_public_key(txCtx->opDetails.beginSponsoringFutureReservesOp.sponsoredID,
                     detailValue,
                     0,
                     0);
    push_to_formatter_stack(&format_operation_source);
}

static void format_begin_sponsoring_future_reserves(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Begin Sponsoring Future Reserves");
    push_to_formatter_stack(&format_claim_claimable_balance_sponsored_id);
}

static void format_end_sponsoring_future_reserves(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "End Sponsoring Future Reserves");
    push_to_formatter_stack(&format_operation_source);
}

static void format_revoke_sponsorship_account(tx_context_t *txCtx) {
    strcpy(detailCaption, "Account ID");
    print_public_key(txCtx->opDetails.revokeSponsorshipOp.ledgerKey.account.accountID,
                     detailValue,
                     0,
                     0);
    push_to_formatter_stack(&format_operation_source);
}

static void format_revoke_sponsorship_trust_line_asset(tx_context_t *txCtx) {
    strcpy(detailCaption, "Asset");
    print_asset_t(&txCtx->opDetails.revokeSponsorshipOp.ledgerKey.trustLine.asset,
                  txCtx->network,
                  detailValue,
                  DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_operation_source);
}

static void format_revoke_sponsorship_trust_line_account(tx_context_t *txCtx) {
    strcpy(detailCaption, "Account ID");
    print_public_key(txCtx->opDetails.revokeSponsorshipOp.ledgerKey.trustLine.accountID,
                     detailValue,
                     0,
                     0);
    push_to_formatter_stack(&format_revoke_sponsorship_trust_line_asset);
}
static void format_revoke_sponsorship_offer_offer_id(tx_context_t *txCtx) {
    strcpy(detailCaption, "Offer ID");
    print_uint(txCtx->opDetails.revokeSponsorshipOp.ledgerKey.offer.offerID,
               detailValue,
               DETAIL_VALUE_MAX_SIZE);

    push_to_formatter_stack(&format_operation_source);
}

static void format_revoke_sponsorship_offer_seller_id(tx_context_t *txCtx) {
    strcpy(detailCaption, "Seller ID");
    print_public_key(txCtx->opDetails.revokeSponsorshipOp.ledgerKey.offer.sellerID,
                     detailValue,
                     0,
                     0);
    push_to_formatter_stack(&format_revoke_sponsorship_offer_offer_id);
}

static void format_revoke_sponsorship_data_data_name(tx_context_t *txCtx) {
    strcpy(detailCaption, "Data Name");
    char tmp[65];
    memcpy(tmp,
           txCtx->opDetails.revokeSponsorshipOp.ledgerKey.data.dataName,
           txCtx->opDetails.revokeSponsorshipOp.ledgerKey.data.dataNameSize);
    tmp[txCtx->opDetails.revokeSponsorshipOp.ledgerKey.data.dataNameSize] = '\0';
    strcpy(detailValue, tmp);
    push_to_formatter_stack(&format_operation_source);
}

static void format_revoke_sponsorship_data_account(tx_context_t *txCtx) {
    strcpy(detailCaption, "Account ID");
    print_public_key(txCtx->opDetails.revokeSponsorshipOp.ledgerKey.data.accountID,
                     detailValue,
                     0,
                     0);
    push_to_formatter_stack(&format_revoke_sponsorship_data_data_name);
}

static void format_revoke_sponsorship_claimable_balance(tx_context_t *txCtx) {
    strcpy(detailCaption, "Balance ID");
    print_claimable_balance_id(
        &txCtx->opDetails.revokeSponsorshipOp.ledgerKey.claimableBalance.balanceId,
        detailValue);
    push_to_formatter_stack(&format_operation_source);
}

static void format_revoke_sponsorship_claimable_signer_signer_key_detail(tx_context_t *txCtx) {
    strcpy(detailCaption, "Signer Key");
    SignerKey *key = &txCtx->opDetails.revokeSponsorshipOp.signer.signerKey;

    switch (key->type) {
        case SIGNER_KEY_TYPE_ED25519: {
            print_public_key(key->data, detailValue, 0, 0);
            break;
        }
        case SIGNER_KEY_TYPE_HASH_X: {
            char tmp[57];
            encode_hash_x_key(key->data, tmp);
            print_summary(tmp, detailValue, 12, 12);
            break;
        }

        case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
            char tmp[57];
            encode_pre_auth_key(key->data, tmp);
            print_summary(tmp, detailValue, 12, 12);
            break;
        }
    }
    push_to_formatter_stack(&format_operation_source);
}

static void format_revoke_sponsorship_claimable_signer_signer_key_type(tx_context_t *txCtx) {
    strcpy(detailCaption, "Signer Key Type");
    switch (txCtx->opDetails.revokeSponsorshipOp.signer.signerKey.type) {
        case SIGNER_KEY_TYPE_ED25519: {
            strcpy(detailValue, "Public Key");
            break;
        }
        case SIGNER_KEY_TYPE_HASH_X: {
            strcpy(detailValue, "Hash(x)");
            break;
        }
        case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
            strcpy(detailValue, "Pre-Auth");
            break;
        }
    }

    push_to_formatter_stack(&format_revoke_sponsorship_claimable_signer_signer_key_detail);
}

static void format_revoke_sponsorship_claimable_signer_account(tx_context_t *txCtx) {
    strcpy(detailCaption, "Account ID");
    print_public_key(txCtx->opDetails.revokeSponsorshipOp.signer.accountID, detailValue, 0, 0);
    push_to_formatter_stack(&format_revoke_sponsorship_claimable_signer_signer_key_type);
}

static void format_revoke_sponsorship(tx_context_t *txCtx) {
    strcpy(detailCaption, "Operation Type");
    if (txCtx->opDetails.revokeSponsorshipOp.type == REVOKE_SPONSORSHIP_SIGNER) {
        strcpy(detailValue, "Revoke Sponsorship (SIGNER_KEY)");
        push_to_formatter_stack(&format_revoke_sponsorship_claimable_signer_account);
    } else {
        switch (txCtx->opDetails.revokeSponsorshipOp.ledgerKey.type) {
            case ACCOUNT:
                strcpy(detailValue, "Revoke Sponsorship (ACCOUNT)");
                push_to_formatter_stack(&format_revoke_sponsorship_account);
                break;
            case OFFER:
                strcpy(detailValue, "Revoke Sponsorship (OFFER)");
                push_to_formatter_stack(&format_revoke_sponsorship_offer_seller_id);
                break;
            case TRUSTLINE:
                strcpy(detailValue, "Revoke Sponsorship (TRUSTLINE)");
                push_to_formatter_stack(&format_revoke_sponsorship_trust_line_account);
                break;
            case DATA:
                strcpy(detailValue, "Revoke Sponsorship (DATA)");
                push_to_formatter_stack(&format_revoke_sponsorship_data_account);
                break;
            case CLAIMABLE_BALANCE:
                strcpy(detailValue, "Revoke Sponsorship (CLAIMABLE_BALANCE)");
                push_to_formatter_stack(&format_revoke_sponsorship_claimable_balance);
                break;
        }
    }
}

static void format_clawback_from(tx_context_t *txCtx) {
    strcpy(detailCaption, "From");
    print_muxed_account(&txCtx->opDetails.clawbackOp.from, detailValue, 0, 0);
    push_to_formatter_stack(&format_operation_source);
}

static void format_clawback_amount(tx_context_t *txCtx) {
    strcpy(detailCaption, "Clawback Balance");
    print_amount(txCtx->opDetails.clawbackOp.amount,
                 &txCtx->opDetails.clawbackOp.asset,
                 txCtx->network,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_clawback_from);
}

static void format_clawback(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Clawback");
    push_to_formatter_stack(&format_clawback_amount);
}

static void format_clawback_claimable_balance_balance_id(tx_context_t *txCtx) {
    strcpy(detailCaption, "Balance ID");
    print_claimable_balance_id(&txCtx->opDetails.clawbackClaimableBalanceOp.balanceID, detailValue);
    push_to_formatter_stack(&format_operation_source);
}

static void format_clawback_claimable_balance(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Clawback Claimable Balance");
    push_to_formatter_stack(&format_clawback_claimable_balance_balance_id);
}

static void format_set_trust_line_set_flags(tx_context_t *txCtx) {
    strcpy(detailCaption, "Set Flags");
    if (txCtx->opDetails.setTrustLineFlagsOp.setFlags) {
        print_trust_line_flags(txCtx->opDetails.setTrustLineFlagsOp.setFlags,
                               detailValue,
                               DETAIL_VALUE_MAX_SIZE);
    } else {
        strcpy(detailValue, "[none]");
    }
    push_to_formatter_stack(&format_operation_source);
}

static void format_set_trust_line_clear_flags(tx_context_t *txCtx) {
    strcpy(detailCaption, "Clear Flags");
    if (txCtx->opDetails.setTrustLineFlagsOp.clearFlags) {
        print_trust_line_flags(txCtx->opDetails.setTrustLineFlagsOp.clearFlags,
                               detailValue,
                               DETAIL_VALUE_MAX_SIZE);
    } else {
        strcpy(detailValue, "[none]");
    }
    push_to_formatter_stack(&format_set_trust_line_set_flags);
}

static void format_set_trust_line_asset(tx_context_t *txCtx) {
    strcpy(detailCaption, "Asset");
    print_asset_t(&txCtx->opDetails.setTrustLineFlagsOp.asset,
                  txCtx->network,
                  detailValue,
                  DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_set_trust_line_clear_flags);
}

static void format_set_trust_line_trustor(tx_context_t *txCtx) {
    strcpy(detailCaption, "Trustor");
    print_public_key(txCtx->opDetails.setTrustLineFlagsOp.trustor, detailValue, 0, 0);
    push_to_formatter_stack(&format_set_trust_line_asset);
}

static void format_set_trust_line_flags(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "Operation Type");
    strcpy(detailValue, "Set Trust Line Flags");
    push_to_formatter_stack(&format_set_trust_line_trustor);
}

static const format_function_t formatters[] = {
    &format_create_account,
    &format_payment,
    &format_path_payment_strict_receive,
    &format_manage_sell_offer,
    &format_create_passive_sell_offer,
    &format_set_options,
    &format_change_trust,
    &format_allow_trust,
    &format_account_merge,
    &format_inflation,
    &format_manage_data,
    &format_bump_sequence,
    &format_manage_buy_offer,
    &format_path_payment_strict_send,
    &format_create_claimable_balance,
    &format_claim_claimable_balance,
    &format_begin_sponsoring_future_reserves,
    &format_end_sponsoring_future_reserves,
    &format_revoke_sponsorship,
    &format_clawback,
    &format_clawback_claimable_balance,
    &format_set_trust_line_flags,
};

void format_confirm_operation(tx_context_t *txCtx) {
    if (txCtx->opCount > 1) {
        size_t len;
        strcpy(opCaption, "Operation ");
        len = strlen(opCaption);
        print_uint(txCtx->opIdx, opCaption + len, OPERATION_CAPTION_MAX_SIZE - len);
        strlcat(opCaption, " of ", sizeof(opCaption));
        len = strlen(opCaption);
        print_uint(txCtx->opCount, opCaption + len, OPERATION_CAPTION_MAX_SIZE - len);
        push_to_formatter_stack(((format_function_t) PIC(formatters[txCtx->opDetails.type])));
    } else {
        ((format_function_t) PIC(formatters[txCtx->opDetails.type]))(txCtx);
    }
}

static void format_confirm_hash_detail(tx_context_t *txCtx) {
    strcpy(detailCaption, "Hash");
    print_binary_summary(txCtx->hash, detailValue, 32);
    push_to_formatter_stack(NULL);
}

void format_confirm_hash_warning(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(detailCaption, "WARNING");
    strcpy(detailValue, "No details available");
    push_to_formatter_stack(&format_confirm_hash_detail);
}

uint8_t current_data_index;

format_function_t get_formatter(tx_context_t *txCtx, bool forward) {
    switch (ctx.state) {
        case STATE_APPROVE_TX: {  // classic tx
            if (!forward) {
                if (current_data_index ==
                    0) {  // if we're already at the beginning of the buffer, return NULL
                    return NULL;
                }
                // rewind to tx beginning if we're requesting a previous operation
                txCtx->offset = 0;
                txCtx->opIdx = 0;
            }

            while (current_data_index > txCtx->opIdx) {
                if (!parse_tx_xdr(txCtx->raw, txCtx->rawLength, txCtx)) {
                    return NULL;
                }
            }
            return &format_confirm_operation;
        }
        case STATE_APPROVE_TX_HASH: {
            if (!forward) {
                return NULL;
            }
            return &format_confirm_hash_warning;
        }
        default:
            THROW(0x6123);
    }
    return NULL;
}

void ui_approve_tx_next_screen(tx_context_t *txCtx) {
    if (!formatter_stack[formatter_index]) {
        MEMCLEAR(formatter_stack);
        formatter_index = 0;
        current_data_index++;
        formatter_stack[0] = get_formatter(txCtx, true);
    }
}

void ui_approve_tx_prev_screen(tx_context_t *txCtx) {
    if (formatter_index == -1) {
        MEMCLEAR(formatter_stack);
        formatter_index = 0;
        current_data_index--;
        formatter_stack[0] = get_formatter(txCtx, false);
    }
}

void set_state_data(bool forward) {
    if (forward) {
        ui_approve_tx_next_screen(&ctx.req.tx);
    } else {
        ui_approve_tx_prev_screen(&ctx.req.tx);
    }

    // Apply last formatter to fill the screen's buffer
    if (formatter_stack[formatter_index]) {
        MEMCLEAR(detailCaption);
        MEMCLEAR(detailValue);
        MEMCLEAR(opCaption);
        formatter_stack[formatter_index](&ctx.req.tx);

        if (opCaption[0] != '\0') {
            strlcpy(detailCaption, opCaption, sizeof(detailCaption));
            detailValue[0] = ' ';
        }
    }
}
