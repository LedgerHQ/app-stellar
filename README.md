# Stellar app for the Ledger Nano S

## Introduction

This is a wallet app for the [Ledger Nano S](https://www.ledgerwallet.com/products/ledger-nano-s) that makes it possible to store your [Stellar](https://www.stellar.org/)-based assets on that device.

It is currently a work in progress that is not yet ready to use.

I am simultaneously developing a [Javascript library](https://github.com/lenondupe/stellar-ledger-api) to communicate with this app. There you will find some scripts in order to test the app.

## Building and installing

To build and install the app on your Nano Ledger S you must set up the Nano Ledger S build environment. Please follow the Getting Started instructions at the [Ledger Nano S github repository](https://github.com/LedgerHQ/ledger-nano-s).

Alternatively, if you are on a Mac, you can set up the Vagrant Virtualbox Ledger environment maintained [here](https://github.com/fix/ledger-vagrant). This sets up an Ubuntu virtual machine with the Ledger build environment already set up. Note that at the time of this writing this seems to be the only way to build and install on a Mac. The native Ledger Nano S build environment is currently not working.

The command to compile and load the app onto the device is:

```$ make load```

To remove the app from the device do:

```$ make delete```

## Testing

The `./test` directory contains files for testing the xdr transanction parser. To build and execute the test run `./test.sh`.

### XDR parsing

When a transaction is to be signed it is sent to the device as an [XDR](https://tools.ietf.org/html/rfc1832) serialized binary object. To show the transaction details to the user on the device this XDR blob must be read. This is done by a purpose-built parser shipped with this app. The parser supports only simple payment transactions. Other types of transaction should be signed by sending the transaction hash.

## Key pair validation

The operation to retrieve the public key implements an optional keypair verification method. The operation takes an optional message that it will sign and return the signature along with the public key back to the host. The host can then use the public key to verify the message signature and that way be sure the key pair is valid for the network. This is an additional safety precaution to ensure that transactions with this key pair can be successfully signed.

## Approving a transaction

There are two ways a transaction may be approved by the device. In the case of a simple payment transaction it is sent to the device in its xdr representation. The app then parses the xdr and shows the payment details to the user for approval.
Alternatively, if a transaction contains more than a single operation and/or contains different types of operations a different method can be used that takes only the transaction hash. In this case the details of the operation cannot be shown to the user. A warning is shown that no details are available.
