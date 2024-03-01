# Release Notes

## v5.1.0

### Updated
- Add support for Stellar Soroban.
- Fix some bugs.
- Remove the soon-to-be-deprecated SDK calls.

## v4.0.0

### Updated
- Added a `Sequence Number` setting: `Displayed` or `NOT Displayed`, default to `NOT Displayed`.
- Optimized the display of amount. (ex. `10000000 XLM` -> `10,000,000 XLM`)
- Optimized the display of offer price.
- Optimize the display of `Memo Text` and `Manage Data Value`, if they are printable ASCII characters, they will be printed directly, otherwise display the base64 encoded summary.
- In some common operations, the prompt for the operation type was removed.
- Other UX improvements.
- Other bugfixes.
- Refactored this app based on [app-boilerplate](https://github.com/ledgerhq/app-boilerplate).
- Refactored unit tests and added full e2e tests.
- Added APDU documentation.

### Breaking Changes
- Removed keypair validation in `GET_PUBLIC_KEY` command. If necessary, we recommend that you ask the user to confirm the address on the device.
- Removed support for the `KEEP_ALIVE` command at the app layer.

## v3.0 Multi-operation details support and U2F connection keep-alive

Support for showing details of multi-operation transactions has been added. Previously, multi-operation transactions could only be signed using the hash-signing method. Transaction envelopes of up to 1.5kb in size are supported*, covering most Stellar usage scenarios.

A U2F connection keep-alive has been implemented to prevent connections from timing out after 30 seconds when using Chrome.

Transaction details now include operation source (if specified), transaction source, transaction time bounds (if specified), sequence number, memo type, base64-encoded managed data values. Detail display for set options has been improved: flags are now in human readable format, thresholds and master weight are represented separately, and add and remove signer is more descriptive too with the pre-auth and hash(x) signer keys shown in their base32-encoded form. 

Support for new Stellar operation 'Bump Sequence' was added.

The native asset is no longer assumed to be XLM when showing amounts. Instead, amounts in native assets are qualified as XLM only if the network id matches either Stellar public network or test network ids. Otherwise native amounts are qualified as 'native'.

This release also uses the new SDK built-in U2F support which means that the user no longer needs to set the browser mode when switching between host applications that use different transport protocols.

\*To give an idea: this allows for 25 XLM payment, 23 change trust operations, 17 manage offer operations where one of the assets is native, 15 set options operations where the master weight, a threshold and a signer are specified, 9 set options operations where all the options are specified, or 5 path payment operations where both send and receive assets are non-native and two hops are specified.

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
