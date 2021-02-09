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
    print_public_key(txCtx->txDetails.source, detailValue, 0, 0);
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
    strlcpy(detailValue,
            (char *) PIC(NETWORK_NAMES[txCtx->txDetails.network]),
            DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_time_bounds);
}

static void format_fee(tx_context_t *txCtx) {
    strcpy(detailCaption, "Fee");
    char nativeAssetCode[7];
    print_native_asset_code(txCtx->txDetails.network, nativeAssetCode, sizeof(nativeAssetCode));
    print_amount(txCtx->txDetails.fee, nativeAssetCode, detailValue, DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_network);
}

static void format_memo(tx_context_t *txCtx) {
    switch (txCtx->txDetails.memo.type) {
        case MEMO_TYPE_ID: {
            strcpy(detailCaption, "Memo ID");
            strlcpy(detailValue, txCtx->txDetails.memo.data, DETAIL_VALUE_MAX_SIZE);
            break;
        }
        case MEMO_TYPE_TEXT: {
            strcpy(detailCaption, "Memo Text");
            strlcpy(detailValue, txCtx->txDetails.memo.data, DETAIL_VALUE_MAX_SIZE);
            break;
        }
        case MEMO_TYPE_HASH: {
            strcpy(detailCaption, "Memo Hash");
            print_summary(txCtx->txDetails.memo.data, detailValue, 8, 6);
            break;
        }
        case MEMO_TYPE_RETURN: {
            strcpy(detailCaption, "Memo Return");
            print_summary(txCtx->txDetails.memo.data, detailValue, 8, 6);
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
    if (txCtx->opDetails.sourcePresent) {
        strcpy(detailCaption, "Op Source");
        print_public_key(txCtx->opDetails.source, detailValue, 0, 0);
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

static void format_bump_sequence(tx_context_t *txCtx) {
    strcpy(detailCaption, "Bump Sequence");
    print_int(txCtx->opDetails.op.bumpSequence.bumpTo, detailValue, DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_operation_source);
}

static void format_inflation(tx_context_t *txCtx) {
    (void) txCtx;
    strcpy(opCaption, "Run Inflation");
    push_to_formatter_stack(&format_operation_source);
}

static void format_account_merge_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    print_public_key(txCtx->opDetails.op.accountMerge.destination, detailValue, 0, 0);
    push_to_formatter_stack(&format_operation_source);
}

static void format_account_merge(tx_context_t *txCtx) {
    strcpy(detailCaption, "Merge Account");
    if (txCtx->opDetails.sourcePresent) {
        print_public_key(txCtx->opDetails.source, detailValue, 0, 0);
    } else {
        print_public_key(txCtx->txDetails.source, detailValue, 0, 0);
    }
    push_to_formatter_stack(&format_account_merge_destination);
}

static void format_manage_data_value(tx_context_t *txCtx) {
    strcpy(detailCaption, "Data Value");
    char tmp[89];
    base64_encode(txCtx->opDetails.op.manageData.dataValue,
                  txCtx->opDetails.op.manageData.dataValueSize,
                  tmp);
    print_summary(tmp, detailValue, 12, 12);
    push_to_formatter_stack(&format_operation_source);
}

static void format_manage_data(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.manageData.dataValueSize) {
        strcpy(detailCaption, "Set Data");
        push_to_formatter_stack(&format_manage_data_value);
    } else {
        strcpy(detailCaption, "Remove Data");
        push_to_formatter_stack(&format_operation_source);
    }
    char tmp[65];
    memcpy(tmp,
           txCtx->opDetails.op.manageData.dataName,
           txCtx->opDetails.op.manageData.dataNameSize);
    tmp[txCtx->opDetails.op.manageData.dataNameSize] = '\0';
    print_summary(tmp, detailValue, 12, 12);
}

static void format_allow_trust_trustee(tx_context_t *txCtx) {
    strcpy(detailCaption, "Account ID");
    print_public_key(txCtx->opDetails.op.allowTrust.trustee, detailValue, 0, 0);
    push_to_formatter_stack(&format_operation_source);
}

static void format_allow_trust(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.allowTrust.authorize) {
        strcpy(detailCaption, "Allow Trust");
    } else {
        strcpy(detailCaption, "Revoke Trust");
    }
    strlcpy(detailValue, txCtx->opDetails.op.allowTrust.assetCode, DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_allow_trust_trustee);
}

static void format_set_option_signer_weight(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.signer.weight) {
        strcpy(detailCaption, "Weight");
        print_uint(txCtx->opDetails.op.setOptions.signer.weight,
                   detailValue,
                   DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_operation_source);
    } else {
        format_operation_source(txCtx);
    }
}

static void format_set_option_signer_detail(tx_context_t *txCtx) {
    strcpy(detailCaption, "Signer Key");
    switch (txCtx->opDetails.op.setOptions.signer.type) {
        case SIGNER_KEY_TYPE_ED25519: {
            print_public_key(txCtx->opDetails.op.setOptions.signer.data, detailValue, 0, 0);
            break;
        }
        case SIGNER_KEY_TYPE_HASH_X: {
            char tmp[57];
            encode_hash_x_key(txCtx->opDetails.op.setOptions.signer.data, tmp);
            print_summary(tmp, detailValue, 12, 12);
            break;
        }

        case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
            char tmp[57];
            encode_pre_auth_key(txCtx->opDetails.op.setOptions.signer.data, tmp);
            print_summary(tmp, detailValue, 12, 12);
            break;
        }
    }
    push_to_formatter_stack(&format_set_option_signer_weight);
}

static void format_set_option_signer(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.signerPresent) {
        if (txCtx->opDetails.op.setOptions.signer.weight) {
            strcpy(detailCaption, "Add Signer");
        } else {
            strcpy(detailCaption, "Remove Signer");
        }
        switch (txCtx->opDetails.op.setOptions.signer.type) {
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
    if (txCtx->opDetails.op.setOptions.homeDomainSize) {
        strcpy(detailCaption, "Home Domain");
        memcpy(detailValue,
               txCtx->opDetails.op.setOptions.homeDomain,
               txCtx->opDetails.op.setOptions.homeDomainSize);
        detailValue[txCtx->opDetails.op.setOptions.homeDomainSize] = '\0';
        push_to_formatter_stack(&format_set_option_signer);
    } else {
        format_set_option_signer(txCtx);
    }
}

static void format_set_option_high_threshold(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.highThresholdPresent) {
        strcpy(detailCaption, "High Threshold");
        print_uint(txCtx->opDetails.op.setOptions.highThreshold,
                   detailValue,
                   DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_set_option_home_domain);
    } else {
        format_set_option_home_domain(txCtx);
    }
}

static void format_set_option_medium_threshold(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.mediumThresholdPresent) {
        strcpy(detailCaption, "Medium Threshold");
        print_uint(txCtx->opDetails.op.setOptions.mediumThreshold,
                   detailValue,
                   DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_set_option_high_threshold);
    } else {
        format_set_option_high_threshold(txCtx);
    }
}

static void format_set_option_low_threshold(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.lowThresholdPresent) {
        strcpy(detailCaption, "Low Threshold");
        print_uint(txCtx->opDetails.op.setOptions.lowThreshold, detailValue, DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_set_option_medium_threshold);
    } else {
        format_set_option_medium_threshold(txCtx);
    }
}

static void format_set_option_master_weight(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.masterWeightPresent) {
        strcpy(detailCaption, "Master Weight");
        print_uint(txCtx->opDetails.op.setOptions.masterWeight, detailValue, DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_set_option_low_threshold);
    } else {
        format_set_option_low_threshold(txCtx);
    }
}

static void format_set_option_set_flags(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.setFlags) {
        strcpy(detailCaption, "Set Flags");
        print_flags(txCtx->opDetails.op.setOptions.setFlags, detailValue, DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_set_option_master_weight);
    } else {
        format_set_option_master_weight(txCtx);
    }
}

static void format_set_option_clear_flags(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.clearFlags) {
        strcpy(detailCaption, "Clear Flags");
        print_flags(txCtx->opDetails.op.setOptions.clearFlags, detailValue, DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_set_option_set_flags);
    } else {
        format_set_option_set_flags(txCtx);
    }
}

static void format_set_option_inflation_destination(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.inflationDestinationPresent) {
        strcpy(detailCaption, "Inflation Dest");
        print_public_key(txCtx->opDetails.op.setOptions.inflationDestination, detailValue, 0, 0);
        push_to_formatter_stack(&format_set_option_clear_flags);
    } else {
        format_set_option_clear_flags(txCtx);
    }
}

static void format_set_options(tx_context_t *txCtx) {
    format_set_option_inflation_destination(txCtx);
}

static void format_change_trust_limit(tx_context_t *txCtx) {
    strcpy(detailCaption, "Trust Limit");
    if (txCtx->opDetails.op.changeTrust.limit == INT64_MAX) {
        strcpy(detailValue, "[maximum]");
    } else {
        print_amount(txCtx->opDetails.op.changeTrust.limit,
                     NULL,
                     detailValue,
                     DETAIL_VALUE_MAX_SIZE);
    }
    push_to_formatter_stack(&format_operation_source);
}

static void format_change_trust(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.changeTrust.limit) {
        strcpy(detailCaption, "Change Trust");
        push_to_formatter_stack(&format_change_trust_limit);
    } else {
        strcpy(detailCaption, "Remove Trust");
        push_to_formatter_stack(&format_operation_source);
    }
    uint8_t asset_type = txCtx->opDetails.op.changeTrust.asset.type;
    if (asset_type != ASSET_TYPE_CREDIT_ALPHANUM4 && asset_type != ASSET_TYPE_CREDIT_ALPHANUM12) {
        return;
    }
    print_asset_t(&txCtx->opDetails.op.changeTrust.asset, detailValue, DETAIL_VALUE_MAX_SIZE);
}

static void format_manage_offer_sell(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.manageOffer.buy) {
        strcpy(detailCaption, "Buy");
        print_amount(txCtx->opDetails.op.manageOffer.amount,
                     txCtx->opDetails.op.manageOffer.buying.code,
                     detailValue,
                     DETAIL_VALUE_MAX_SIZE);
    } else {
        strcpy(detailCaption, "Sell");
        print_amount(txCtx->opDetails.op.manageOffer.amount,
                     txCtx->opDetails.op.manageOffer.selling.code,
                     detailValue,
                     DETAIL_VALUE_MAX_SIZE);
    }
    push_to_formatter_stack(&format_operation_source);
}

static void format_manage_offer_price(tx_context_t *txCtx) {
    strcpy(detailCaption, "Price");
    uint64_t price = ((uint64_t) txCtx->opDetails.op.manageOffer.price.numerator * 10000000) /
                     txCtx->opDetails.op.manageOffer.price.denominator;
    if (txCtx->opDetails.op.manageOffer.buy) {
        print_amount(price,
                     txCtx->opDetails.op.manageOffer.selling.code,
                     detailValue,
                     DETAIL_VALUE_MAX_SIZE);
    } else {
        print_amount(price,
                     txCtx->opDetails.op.manageOffer.buying.code,
                     detailValue,
                     DETAIL_VALUE_MAX_SIZE);
    }
    push_to_formatter_stack(&format_manage_offer_sell);
}

static void format_manage_offer_buy(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.manageOffer.buy) {
        strcpy(detailCaption, "Sell");
        if (txCtx->opDetails.op.manageOffer.selling.type == ASSET_TYPE_NATIVE) {
            print_native_asset_code(txCtx->txDetails.network, detailValue, DETAIL_VALUE_MAX_SIZE);
        } else {
            print_asset_t(&txCtx->opDetails.op.manageOffer.selling,
                          detailValue,
                          DETAIL_VALUE_MAX_SIZE);
        }
    } else {
        strcpy(detailCaption, "Buy");
        if (txCtx->opDetails.op.manageOffer.buying.type == ASSET_TYPE_NATIVE) {
            print_native_asset_code(txCtx->txDetails.network, detailValue, DETAIL_VALUE_MAX_SIZE);
        } else {
            print_asset_t(&txCtx->opDetails.op.manageOffer.buying,
                          detailValue,
                          DETAIL_VALUE_MAX_SIZE);
        }
    }
    push_to_formatter_stack(&format_manage_offer_price);
}

static void format_manage_offer(tx_context_t *txCtx) {
    if (!txCtx->opDetails.op.manageOffer.amount) {
        strcpy(detailCaption, "Remove Offer");
        print_uint(txCtx->opDetails.op.manageOffer.offerId, detailValue, DETAIL_VALUE_MAX_SIZE);
        push_to_formatter_stack(&format_operation_source);
    } else {
        if (txCtx->opDetails.op.manageOffer.offerId) {
            strcpy(detailCaption, "Change Offer");
            print_uint(txCtx->opDetails.op.manageOffer.offerId, detailValue, DETAIL_VALUE_MAX_SIZE);
        } else {
            strcpy(detailCaption, "Create Offer");
            if (txCtx->opDetails.op.manageOffer.active) {
                strcpy(detailValue, "Type Active");
            } else {
                strcpy(detailValue, "Type Passive");
            }
        }
        push_to_formatter_stack(&format_manage_offer_buy);
    }
}

static void format_path_via(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.pathPayment.pathLen) {
        strcpy(detailCaption, "Via");
        uint8_t i;
        for (i = 0; i < txCtx->opDetails.op.pathPayment.pathLen; i++) {
            asset_t asset = txCtx->opDetails.op.pathPayment.path[i];
            if (strlen(detailValue) != 0) {
                strlcat(detailValue, ", ", DETAIL_VALUE_MAX_SIZE);
            }
            strlcat(detailValue, asset.code, DETAIL_VALUE_MAX_SIZE);
        }
        push_to_formatter_stack(&format_operation_source);
    } else {
        format_operation_source(txCtx);
    }
}

static void format_path_receive(tx_context_t *txCtx) {
    strcpy(detailCaption, "Receive");
    print_amount(txCtx->opDetails.op.pathPayment.destAmount,
                 txCtx->opDetails.op.pathPayment.destAsset.code,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_path_via);
}

static void format_path_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    print_public_key(txCtx->opDetails.op.pathPayment.destination, detailValue, 0, 0);
    push_to_formatter_stack(&format_path_receive);
}

static void format_path_payment(tx_context_t *txCtx) {
    strcpy(detailCaption, "Send Max");
    print_amount(txCtx->opDetails.op.pathPayment.sendMax,
                 txCtx->opDetails.op.pathPayment.sourceAsset.code,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_path_destination);
}

static void format_payment_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    print_public_key(txCtx->opDetails.op.payment.destination, detailValue, 0, 0);
    push_to_formatter_stack(&format_operation_source);
}

static void format_payment(tx_context_t *txCtx) {
    strcpy(detailCaption, "Send");
    print_amount(txCtx->opDetails.op.payment.amount,
                 txCtx->opDetails.op.payment.asset.code,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_payment_destination);
}

static void format_create_account_amount(tx_context_t *txCtx) {
    strcpy(detailCaption, "Starting Balance");
    char nativeAssetCode[7];
    print_native_asset_code(txCtx->txDetails.network, nativeAssetCode, sizeof(nativeAssetCode));
    print_amount(txCtx->opDetails.op.createAccount.amount,
                 nativeAssetCode,
                 detailValue,
                 DETAIL_VALUE_MAX_SIZE);
    push_to_formatter_stack(&format_operation_source);
}

static void format_create_account(tx_context_t *txCtx) {
    strcpy(detailCaption, "Create Account");
    print_public_key(txCtx->opDetails.op.createAccount.accountId, detailValue, 0, 0);
    push_to_formatter_stack(&format_create_account_amount);
}

static const format_function_t formatters[13] = {&format_create_account,
                                                 &format_payment,
                                                 &format_path_payment,
                                                 &format_manage_offer,
                                                 &format_manage_offer,
                                                 &format_set_options,
                                                 &format_change_trust,
                                                 &format_allow_trust,
                                                 &format_account_merge,
                                                 &format_inflation,
                                                 &format_manage_data,
                                                 &format_bump_sequence,
                                                 &format_manage_offer};

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
            PRINTF("caption: %s\n", detailCaption);
        } else if (detailCaption[0] != '\0' && detailValue[0] != '\0') {
            PRINTF("caption: %s\n", detailCaption);
            PRINTF("details: %s\n", detailValue);
        }
    }
}
