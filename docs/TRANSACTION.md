# Stellar Transaction Serialization

## XDR parsing

When a transaction is to be signed it is sent to the device as an [XDR](https://tools.ietf.org/html/rfc1832) serialized binary object. To show the transaction details to the user on the device this binary object must be read. This is done by a purpose-built parser shipped with this app.

Due to memory limitations the maximum transaction size is set to 1kb on Nano S and 5kb on Nano S Plus and Nano X. This should be sufficient for most usages, including multi-operation transactions up to 35 operations depending on the size of the operations.

Alternatively the user can enable hash signing. In this mode the transaction XDR is not sent to the device but only the hash of the transaction, which is the basis for a valid signature. In this case details for the transaction cannot be displayed and verified.