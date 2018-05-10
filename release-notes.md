# Release Notes

## v3.0 Multi operation support and U2F connection keep-alive

Support for showing details of multi-operation transactions has been added. Previously, multi-operation transactions could only be signed using the hash-signing method. Transaction envelopes of up to 1kb in size are supported. Most smart contract scenario's should not require larger transactions.

A U2F connection keep-alive has been implemented to prevent connections from timing out after 30 seconds when using browser mode on Chrome.

## v2.1 Hash signing support

This release adds a mode to sign the hash of the transaction. Transaction details are not shown in this mode.

## v2.0 Ledger Blue support

This release features full support for displaying the details for all operations possible on the Stellar network and adds Ledger Blue support.
Operations added are: path payment, create passive offer, set options, allow/revoke trust, account merge, inflation, and manage data.

- Ledger Blue support
- Display details for all types of operations
- Display full memo and hash type memo as hash summary
- Fix 'Price is not correctly qualified for Offer operations'

## v1.1
Initial release
