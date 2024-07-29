"""A script is used to parse the raw data under the `./tests_unit/testcases/` folder into Base64 XDR data."""

from stellar_sdk import (
    FeeBumpTransaction,
    FeeBumpTransactionEnvelope,
    Transaction,
    TransactionEnvelope,
    Network,
)
from stellar_sdk.xdr import HashIDPreimage

import base64
import sys


def get_network_passphrase(network_id: bytes):
    if network_id == Network.public_network().network_id():
        return Network.PUBLIC_NETWORK_PASSPHRASE
    elif network_id == Network.testnet_network().network_id():
        return Network.TESTNET_NETWORK_PASSPHRASE
    else:
        return "Unknown Network"


def parse_tx_from_raw_xdr(raw_xdr: bytes):
    network_id, tx = raw_xdr[:32], base64.b64encode(raw_xdr[36:]).decode()
    network_passphrase = get_network_passphrase(network_id)
    try:
        tx = Transaction.from_xdr(tx)
        return TransactionEnvelope(tx, network_passphrase)
    except Exception as e:
        tx = FeeBumpTransaction.from_xdr(tx, network_passphrase)
        return FeeBumpTransactionEnvelope(tx, network_passphrase)


def parse_auth_from_raw_xdr(raw_xdr: bytes):
    return HashIDPreimage.from_xdr_bytes(raw_xdr)


def main():
    if len(sys.argv) != 2:
        print("Usage: python raw_data_to_base64py <raw_file_path>")
        sys.exit(1)
    file_path = sys.argv[1]

    try:
        with open(file_path, "rb") as file:
            raw_data = file.read()
    except FileNotFoundError:
        print(f"File not found: {file_path}")
        sys.exit(1)
    except IOError:
        print(f"Error reading file: {file_path}")
        sys.exit(1)

    try:
        tx_envelope = parse_tx_from_raw_xdr(raw_data)
        print(tx_envelope.to_xdr())
        return
    except Exception as e:
        pass

    try:
        auth = parse_auth_from_raw_xdr(raw_data)
        print(auth.to_xdr())
        return
    except Exception:
        pass

    print("Unable to parse the raw data.")


if __name__ == "__main__":
    main()
