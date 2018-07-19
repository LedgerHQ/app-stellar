/*******************************************************************************
 *   Ledger Stellar App
 *   (c) 2017-2018 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/
#ifndef TEST
#include "bolos_target.h"
#include "os.h"
#endif
#include "stellar_types.h"

#ifdef TARGET_BLUE

#include "stellar_format.h"
#include "stellar_api.h"
#include "string.h"

char opCaption[20];
char detailCaption[20];
char detailValue[89];

volatile format_function_t formatter;

void format_sequence_number(tx_context_t *txCtx) {
    strcpy(detailCaption, "Sequence Number");
    print_uint(txCtx->txDetails.sequenceNumber, detailValue);
    formatter = NULL;
}

void format_transaction_source(tx_context_t *txCtx) {
    strcpy(detailCaption, "Tx Source");
    encode_public_key(txCtx->txDetails.source, detailValue);
    formatter = &format_sequence_number;
}

void format_time_bounds(tx_context_t *txCtx) {
    if (txCtx->txDetails.hasTimeBounds) {
        strcpy(detailCaption, "Time Bounds");
        strcpy(detailValue, "From ");
        print_uint(txCtx->txDetails.timeBounds.minTime, detailValue+strlen(detailValue));
        strcpy(detailValue+strlen(detailValue), " To ");
        print_uint(txCtx->txDetails.timeBounds.maxTime, detailValue+strlen(detailValue));
        formatter = &format_transaction_source;
    } else {
        format_transaction_source(txCtx);
    }
}

void format_network(tx_context_t *txCtx) {
    strcpy(detailCaption, "Network");
    strcpy(detailValue, ((char *)PIC(NETWORK_NAMES[txCtx->txDetails.network])));
    formatter = &format_time_bounds;
}

void format_fee(tx_context_t *txCtx) {
    strcpy(detailCaption, "Fee");
    char nativeAssetCode[7];
    print_native_asset_code(txCtx->txDetails.network, nativeAssetCode);
    print_amount(txCtx->txDetails.fee, nativeAssetCode, detailValue);
    formatter = &format_network;
}

void format_memo(tx_context_t *txCtx) {
    switch (txCtx->txDetails.memo.type) {
        case MEMO_TYPE_ID: {
            strcpy(detailCaption, "Memo Id");
            strcpy(detailValue, txCtx->txDetails.memo.data);
            break;
        }
        case MEMO_TYPE_TEXT: {
            strcpy(detailCaption, "Memo Text");
            strcpy(detailValue, txCtx->txDetails.memo.data);
            break;
        }
        case MEMO_TYPE_HASH: {
            strcpy(detailCaption, "Memo Hash");
            strcpy(detailValue, txCtx->txDetails.memo.data);
            break;
        }
        case MEMO_TYPE_RETURN: {
            strcpy(detailCaption, "Memo Return");
            strcpy(detailValue, txCtx->txDetails.memo.data);
            break;
        }
        default: {
            strcpy(detailCaption, "Memo");
            strcpy(detailValue, "[none]");
        }
    }
    formatter = &format_fee;
}

void format_confirm_transaction_details(tx_context_t *txCtx) {
    format_memo(txCtx);
}

void format_operation_source(tx_context_t *txCtx) {
    if (txCtx->opDetails.sourcePresent) {
        strcpy(detailCaption, "Op Source");
        encode_public_key(txCtx->opDetails.source, detailValue);
    }
    formatter = NULL;
}

void format_bump_sequence(tx_context_t *txCtx) {
    strcpy(opCaption, "Bump Sequence");
    strcpy(detailCaption, "Bump To");
    print_int(txCtx->opDetails.op.bumpSequence.bumpTo, detailValue);
    formatter = &format_operation_source;
}

void format_inflation(tx_context_t *txCtx) {
    strcpy(opCaption, "Run Inflation");
    formatter = &format_operation_source;
}

void format_account_merge_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    encode_public_key(txCtx->opDetails.op.accountMerge.destination, detailValue);
    formatter = &format_operation_source;
}

void format_account_merge(tx_context_t *txCtx) {
    strcpy(opCaption, "Merge Account");
    strcpy(detailCaption, "Account ID");
    if (txCtx->opDetails.sourcePresent) {
        encode_public_key(txCtx->opDetails.source, detailValue);
    } else {
        encode_public_key(txCtx->txDetails.source, detailValue);
    }
    formatter = &format_account_merge_destination;
}

void format_manage_data_value(tx_context_t *txCtx) {
    strcpy(detailCaption, "Data Value");
    base64_encode(txCtx->opDetails.op.manageData.dataValue, txCtx->opDetails.op.manageData.dataValueSize, detailValue);
    formatter = &format_operation_source;
}

void format_manage_data(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.manageData.dataValueSize) {
        strcpy(opCaption, "Set Data");
        formatter = &format_manage_data_value;
    } else {
        strcpy(opCaption, "Remove Data");
        formatter = &format_operation_source;
    }
    strcpy(detailCaption, "Data Key");
    memcpy(detailValue, txCtx->opDetails.op.manageData.dataName, txCtx->opDetails.op.manageData.dataNameSize);
    detailValue[txCtx->opDetails.op.manageData.dataNameSize] = '\0';
}

void format_allow_trust_trustee(tx_context_t *txCtx) {
    strcpy(detailCaption, "Account ID");
    encode_public_key(txCtx->opDetails.op.allowTrust.trustee, detailValue);
    formatter = &format_operation_source;
}

void format_allow_trust(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.allowTrust.authorize) {
        strcpy(opCaption, "Allow Trust");
    } else {
        strcpy(opCaption, "Revoke Trust");
    }
    strcpy(detailCaption, "Asset Code");
    strcpy(detailValue, txCtx->opDetails.op.allowTrust.assetCode);
    formatter = &format_allow_trust_trustee;
}

void format_set_option_signer(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.signerPresent) {
        if (txCtx->opDetails.op.setOptions.signer.weight) {
            strcpy(detailCaption, "Add Signer");
        } else {
            strcpy(detailCaption, "Remove Signer");
        }
        char *type;
        char signer[30];
        switch (txCtx->opDetails.op.setOptions.signer.type) {
        case SIGNER_KEY_TYPE_ED25519: {
            type = "Public Key";
            print_public_key(txCtx->opDetails.op.setOptions.signer.data, signer, 12, 12);
            break;
        }
        case SIGNER_KEY_TYPE_HASH_X: {
            type = "Hash(x)";
            char tmp[57];
            encode_hash_x_key(txCtx->opDetails.op.setOptions.signer.data, tmp);
            print_summary(tmp, signer, 12, 12);
            break;
        }
        case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
            type = "Pre-Auth";
            char tmp[57];
            encode_pre_auth_key(txCtx->opDetails.op.setOptions.signer.data, tmp);
            print_summary(tmp, signer, 12, 12);
            break;
        }
        default: {
            type = "Unknown Type";
            strcpy(signer, "Unknown signer");
        }
        }
        char weight[4];
        print_uint(txCtx->opDetails.op.setOptions.signer.weight, weight);
        strcpy(detailValue, type);
        strcpy(detailValue+strlen(detailValue), ": ");
        strcpy(detailValue+strlen(detailValue), signer);
        if (txCtx->opDetails.op.setOptions.signer.weight) {
            strcpy(detailValue+strlen(detailValue), "; weight = ");
            strcpy(detailValue+strlen(detailValue), weight);
        }
        formatter = &format_operation_source;
    } else {
        format_operation_source(txCtx);
    }
}

void format_set_option_home_domain(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.homeDomainSize) {
        strcpy(detailCaption, "Home Domain");
        memcpy(detailValue, txCtx->opDetails.op.setOptions.homeDomain, txCtx->opDetails.op.setOptions.homeDomainSize);
        detailValue[txCtx->opDetails.op.setOptions.homeDomainSize] = '\0';
        formatter = &format_set_option_signer;
    } else {
        format_set_option_signer(txCtx);
    }
}

void format_set_option_thresholds(tx_context_t *txCtx) {
    bool thresholdsPresent = false;
    if (txCtx->opDetails.op.setOptions.lowThresholdPresent) {
        thresholdsPresent = true;
        strcpy(detailValue, "low: ");
        print_uint(txCtx->opDetails.op.setOptions.lowThreshold, detailValue+strlen(detailValue));
    }
    if (txCtx->opDetails.op.setOptions.mediumThresholdPresent) {
        thresholdsPresent = true;
        uint8_t len = strlen(detailValue);
        if (len) {
            strcpy(detailValue+len, "; ");
            len += 2;
        }
        strcpy(detailValue+len, "medium: ");
        print_uint(txCtx->opDetails.op.setOptions.mediumThreshold, detailValue+strlen(detailValue));
    }
    if (txCtx->opDetails.op.setOptions.highThresholdPresent) {
        thresholdsPresent = true;
        uint8_t len = strlen(detailValue);
        if (len) {
            strcpy(detailValue+len, "; ");
            len += 2;
        }
        strcpy(detailValue+len, "high: ");
        print_uint(txCtx->opDetails.op.setOptions.highThreshold, detailValue+strlen(detailValue));
    }
    if (thresholdsPresent) {
        strcpy(detailCaption, "Thresholds");
        formatter = &format_set_option_home_domain;
    } else {
        format_set_option_home_domain(txCtx);
    }
}

void format_set_option_master_weight(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.masterWeightPresent) {
        strcpy(detailCaption, "Master Weight");
        print_uint(txCtx->opDetails.op.setOptions.masterWeight, detailValue);
        formatter = &format_set_option_thresholds;
    } else {
        format_set_option_thresholds(txCtx);
    }
}

void format_set_option_flags(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.clearFlags || txCtx->opDetails.op.setOptions.setFlags) {
        strcpy(detailCaption, "Account Flags");
        if (txCtx->opDetails.op.setOptions.clearFlags) {
            print_flags(txCtx->opDetails.op.setOptions.clearFlags, detailValue, '-');
        }
        if (txCtx->opDetails.op.setOptions.setFlags) {
            print_flags(txCtx->opDetails.op.setOptions.setFlags, detailValue, '+');
        }
        formatter = &format_set_option_master_weight;
    } else {
        format_set_option_master_weight(txCtx);
    }
}

void format_set_option_inflation_destination(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.inflationDestinationPresent) {
        strcpy(detailCaption, "Inflation Dest");
        encode_public_key(txCtx->opDetails.op.setOptions.inflationDestination, detailValue);
        formatter = &format_set_option_flags;
    } else {
        format_set_option_flags(txCtx);
    }
}

void format_set_options(tx_context_t *txCtx) {
    strcpy(opCaption, "Set Options");
    format_set_option_inflation_destination(txCtx);
}

void format_change_trust_limit(tx_context_t *txCtx) {
    strcpy(detailCaption, "Limit");
    if (txCtx->opDetails.op.changeTrust.limit == INT64_MAX) {
        strcpy(detailValue, "[maximum]");
    } else {
        print_amount(txCtx->opDetails.op.changeTrust.limit, NULL, detailValue);
    }
    formatter = &format_operation_source;
}

void format_change_trust(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.changeTrust.limit) {
        strcpy(opCaption, "Change Trust");
        formatter = &format_change_trust_limit;
    } else {
        strcpy(opCaption, "Remove Trust");
        formatter = &format_operation_source;
    }
    strcpy(detailCaption, "Asset");
    print_asset_t(&txCtx->opDetails.op.changeTrust.asset, detailValue);
}

void format_manage_offer_sell(tx_context_t *txCtx) {
    strcpy(detailCaption, "Sell");
    print_amount(txCtx->opDetails.op.manageOffer.amount, txCtx->opDetails.op.manageOffer.selling.code, detailValue);
    formatter = &format_operation_source;
}

void format_manage_offer_price(tx_context_t *txCtx) {
    strcpy(detailCaption, "Price");
    uint64_t price = ((uint64_t)txCtx->opDetails.op.manageOffer.price.numerator * 10000000) /
        txCtx->opDetails.op.manageOffer.price.denominator;
    print_amount(price, txCtx->opDetails.op.manageOffer.buying.code, detailValue);
    formatter = &format_manage_offer_sell;
}

void format_manage_offer_buy(tx_context_t *txCtx) {
    strcpy(detailCaption, "Buy");
    if (txCtx->opDetails.op.manageOffer.buying.type == ASSET_TYPE_NATIVE) {
        print_native_asset_code(txCtx->txDetails.network, detailValue);
    } else {
        print_asset_t(&txCtx->opDetails.op.manageOffer.buying, detailValue);
    }
    formatter = &format_manage_offer_price;
}

void format_manage_offer(tx_context_t *txCtx) {
    if (!txCtx->opDetails.op.manageOffer.amount) {
        strcpy(opCaption, "Remove Offer");
        strcpy(detailCaption, "Offer ID");
        print_uint(txCtx->opDetails.op.manageOffer.offerId, detailValue);
        formatter = &format_operation_source;
    } else {
        if (txCtx->opDetails.op.manageOffer.offerId) {
            strcpy(opCaption, "Change Offer");
            strcpy(detailCaption, "Offer ID");
            print_uint(txCtx->opDetails.op.manageOffer.offerId, detailValue);
        } else {
            strcpy(opCaption, "Create Offer");
            strcpy(detailCaption, "Offer Type");
            if (txCtx->opDetails.op.manageOffer.active) {
                strcpy(detailValue, "Active");
            } else {
                strcpy(detailValue, "Passive");
            }
        }
        formatter = &format_manage_offer_buy;
    }
}

void format_path_via(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.pathPayment.pathLen) {
        strcpy(detailCaption, "Via");
        uint8_t i;
        for (i = 0; i < txCtx->opDetails.op.pathPayment.pathLen; i++) {
            asset_t asset = txCtx->opDetails.op.pathPayment.path[i];
            uint8_t len = strlen(detailValue);
            if (len) {
                strcpy(detailValue+len, ", ");
                len += 2;
            }
            strcpy(detailValue+len, asset.code);
        }
        formatter = &format_operation_source;
    } else {
        format_operation_source(txCtx);
    }
}

void format_path_receive(tx_context_t *txCtx) {
    strcpy(detailCaption, "Receive");
    print_amount(txCtx->opDetails.op.pathPayment.destAmount, txCtx->opDetails.op.pathPayment.destAsset.code, detailValue);
    formatter = &format_path_via;
}

void format_path_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    encode_public_key(txCtx->opDetails.op.pathPayment.destination, detailValue);
    formatter = &format_path_receive;
}

void format_path_payment(tx_context_t *txCtx) {
    strcpy(opCaption, "Path Payment");
    strcpy(detailCaption, "Send Max");
    print_amount(txCtx->opDetails.op.pathPayment.sendMax, txCtx->opDetails.op.pathPayment.sourceAsset.code, detailValue);
    formatter = &format_path_destination;
}

void format_payment_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    encode_public_key(txCtx->opDetails.op.payment.destination, detailValue);
    formatter = &format_operation_source;
}

void format_payment(tx_context_t *txCtx) {
    strcpy(opCaption, "Payment");
    strcpy(detailCaption, "Amount");
    print_amount(txCtx->opDetails.op.payment.amount, txCtx->opDetails.op.payment.asset.code, detailValue);
    formatter = &format_payment_destination;
}

void format_create_account_amount(tx_context_t *txCtx) {
    strcpy(detailCaption, "Starting Balance");
    char nativeAssetCode[7];
    print_native_asset_code(txCtx->txDetails.network, nativeAssetCode);
    print_amount(txCtx->opDetails.op.createAccount.amount, nativeAssetCode, detailValue);
    formatter = &format_operation_source;
}

void format_create_account(tx_context_t *txCtx) {
    strcpy(opCaption, "Create Account");
    strcpy(detailCaption, "Account ID");
    encode_public_key(txCtx->opDetails.op.createAccount.accountId, detailValue);
    formatter = &format_create_account_amount;
}

const format_function_t formatters[12] = {
    &format_create_account,
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
    &format_bump_sequence
};

void format_confirm_operation(tx_context_t *txCtx) {
    ((format_function_t)PIC(formatters[txCtx->opDetails.type]))(txCtx);
}

void format_confirm_transaction(tx_context_t *txCtx) {
    formatter = &format_confirm_operation;
}

void format_confirm_hash_detail(tx_context_t *txCtx) {
    strcpy(detailCaption, "Hash");
    print_binary_summary(txCtx->hash, detailValue, 32);
    formatter = &format_confirm_hash;
}

void format_confirm_hash_warning(tx_context_t *txCtx) {
    strcpy(detailCaption, "WARNING");
    strcpy(detailValue, "No details available");
    formatter = &format_confirm_hash_detail;
}

void format_confirm_hash(tx_context_t *txCtx) {
    formatter = &format_confirm_hash_warning;
}

#endif
