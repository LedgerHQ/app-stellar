# Stellar app for the Ledger Nano S

## Introduction

This is a wallet app for the [Ledger Nano S](https://www.ledgerwallet.com/products/ledger-nano-s).

It is currently work in progress.

I am simultaneously developing a [Javascript library](https://github.com/lenondupe/stellar-ledger-api) to communicate with this app. There you will find some scripts in order to test the app.

## Building and installing

To build and install the app on your Nano Ledger S you must set up the Nano Ledger S build environment. Please follow the Getting Started instructions at the [Ledger Nano S github repository](https://github.com/LedgerHQ/ledger-nano-s).

Alternatively, if you are on a Mac, you can set up the Vagrant Virtualbox Ledger environment maintained [here](https://github.com/fix/ledger-vagrant). This sets up an Ubuntu virtual machine with the Ledger build environment already set up. Note that at the time of this writing this seems to be the only way to build and install on a Mac. The native Ledger Nano S build environment is currently not working.

## Testing

The `./test` directory contains files for testing the xdr transanction parser. To build and execute the test run `./test.sh`.

### XDR parsing

When a transaction is to be signed it is sent to the device as an [XDR](https://tools.ietf.org/html/rfc1832) serialized blob. To show the transaction details to the user on the device this XDR blob must be read. This is done by a purpose-built parser shipped with this app. The parser currently supports only the most simple payment transaction. Hopefully it will be possible to support all possible transactions in the future. When a transaction is sent to the device that is not supported an error code is returned.

## Approving a transaction

When a transaction is sent to the device the details of the transaction are shown on the device screen one by one in an automatic loop until you either press approve (right button) or reject (left button).

## Known issues

* The recipient address is currently not shown correctly due to a bug in the base32 encoder. While the encoder runs fine when testing on the local machine, some characters are mixed up when it is run on the device.

* Amounts that take up more space than the size of the screen should be shown in a ticker.
