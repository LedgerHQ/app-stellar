from stellar_sdk import (
    parse_transaction_envelope_from_xdr,
    Network,
    InvokeHostFunction,
    FeeBumpTransaction,
)

with open("generic_txs.csv", "r") as f:
    lines = f.readlines()
    for line in lines[1:]:
        xdr = line.split(",")[0]
        tx_envelope = parse_transaction_envelope_from_xdr(
            xdr, Network.PUBLIC_NETWORK_PASSPHRASE
        )
        if isinstance(tx_envelope.transaction, FeeBumpTransaction):
            tx_envelope = tx_envelope.transaction.inner_transaction_envelope
        if isinstance(tx_envelope.transaction.operations[0], InvokeHostFunction):
            print(xdr)

with open("soroban_txs.csv", "r") as f:
    lines = f.readlines()
    for line in lines[1:]:
        xdr = line.split(",")[0]
        tx_envelope = parse_transaction_envelope_from_xdr(
            xdr, Network.PUBLIC_NETWORK_PASSPHRASE
        )
        if isinstance(tx_envelope.transaction, FeeBumpTransaction):
            tx_envelope = tx_envelope.transaction.inner_transaction_envelope

        if not isinstance(tx_envelope.transaction.operations[0], InvokeHostFunction):
            print(xdr)
