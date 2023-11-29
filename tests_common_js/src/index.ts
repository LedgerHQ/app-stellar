import exp = require("constants");
import {
  Operation,
  TransactionBuilder,
  Networks,
  BASE_FEE,
  Account,
  Memo,
  Keypair,
  Asset,
  StrKey,
  Claimant,
  getLiquidityPoolId,
  LiquidityPoolAsset,
  MuxedAccount,
  LiquidityPoolId,
} from "@stellar/stellar-base";

// mnemonic: 'other base behind follow wet put glad muscle unlock sell income october'
// index 0: GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7 / SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK
// index 1: GDRMNAIPTNIJWJSL6JOF76CJORN47TDVMWERTXO2G2WKOMXGNHUFL5QX / SAE52G23WPAS7MIR2OFGILLICLXXR4K6HSXZHMKD6C33JCAVVILIWYAA
// index 2: GCJBZJSKICFGD3FJMN5RBQIIXYUNVWOI7YAHQZQKK4UAWFGW6TRBRVX3 / SCGYXI6ZHWGD5EPFCFVH37EUHA5BIFNJQJGPMXDKHD4DYA3N2MXMA3NI
// index 3: GCPWZDBYNXVQYOKUWAL34ED3GDKD6UITURJP734A4DPB5EPDSXHAM3KX / SB6YADAUJ6ZCG4X7UAKLBJEXGZZJP3S2JUBSD5ZMQNXUGWPNBECNBE6O
// index 4: GATLHLTLWJVCZ52WDZOVTXFE5YXGEQ6SGEAFEL5J52WIYSGPY7PW7BMV / SBWIYHM4LEWSVHIOJXNP66XDUNME373L25EIDEMFWXNZD56PGXEUWSYG
// index 5: GCJ644IGDW7YFNKHTWSCM37FRMFBQ2EDMZLQM4AUCRBFCW562XXC5OW3 / SA3LRNOYCV4NVVYWLX4P3CXQA3ONKBCQRZDSOVENQ2TCNZRFBEO55FXW
// index 6: GCV6BUTD2REAS3MYMXIFPAMPX24FII3HNHLLESPYLOZDNZAJ4ULXP6KU / SDWURSMB36GGY7DBV2D6QMLG5WAZGFQIZNOSCJZDHC7XEMSFNFX2PEWV
// index 7: GALWXOA7RDHCPT7EXBIVCEPQIDNS5RRD6LJORBIA2HU22ORQ6XH267VE / SAKNBXQWB47HK5IN3VI5VM6UVJTCFRIILUQMLTXVA4KNM36RGME6CSJH
// index 8: GBWFAOQTZVL76F27XDJA2YDH2WKYHHMJAOKHF3B2HFHBMNBNBGMCNKQE / SC3N32WGXFSTQQZ6YNCBLUS6QAD6YUBITJROPQ2UWLMC6HOWZ2N3I5F7
// index 9: GD3OAWFV6M5T2DWVWRQONITXSMWJA2DQ3524H7BQYCNVHJZJFWSN65IA / SAJIU6F4S6ML76JWPBE56XWUIFT77VVPMB6IFTKA6L6EIJ2CFWBFYDWO
const kp0 = Keypair.fromSecret(
  "SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK"
);
const kp1 = Keypair.fromSecret(
  "SAE52G23WPAS7MIR2OFGILLICLXXR4K6HSXZHMKD6C33JCAVVILIWYAA"
);

const kp2 = Keypair.fromSecret(
  "SCGYXI6ZHWGD5EPFCFVH37EUHA5BIFNJQJGPMXDKHD4DYA3N2MXMA3NI"
);

function getCommonTransactionBuilder() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  });
}

// Operation Testcases
export function opCreateAccount() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.createAccount({
        destination: kp1.publicKey(),
        startingBalance: "100",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opPaymentAssetNative() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "922337203685.4775807",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opPaymentAssetAlphanum4() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        amount: "922337203685.4775807",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opPaymentAssetAlphanum12() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: new Asset(
          "BANANANANANA",
          "GCDGPFKW2LUJS2ESKAS42HGOKC6VWOKEJ44TQ3ZXZAMD4ZM5FVHJHPJS"
        ),
        amount: "922337203685.4775807",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opPaymentWithMuxedDestination() {
  const muxedAccount = new MuxedAccount(
    new Account(kp1.publicKey(), "0"),
    "10000"
  ).accountId();

  return getCommonTransactionBuilder()
    .addOperation(
      Operation.payment({
        destination: muxedAccount,
        asset: Asset.native(),
        amount: "922337203685.4775807",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opPathPaymentStrictReceive() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.pathPaymentStrictReceive({
        sendAsset: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        sendMax: "1",
        destination: kp1.publicKey(),
        destAsset: Asset.native(),
        destAmount: "123456789.334",
        path: [
          new Asset(
            "USDC",
            "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
          ),
          new Asset(
            "PANDA",
            "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z"
          ),
        ],
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opPathPaymentStrictReceiveWithEmptyPath() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.pathPaymentStrictReceive({
        sendAsset: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        sendMax: "1",
        destination: kp1.publicKey(),
        destAsset: Asset.native(),
        destAmount: "123456789.334",
        path: [],
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opPathPaymentStrictReceiveWithMuxedDestination() {
  const muxedAccount = new MuxedAccount(
    new Account(kp1.publicKey(), "0"),
    "10000"
  ).accountId();
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.pathPaymentStrictReceive({
        sendAsset: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        sendMax: "1",
        destination: muxedAccount,
        destAsset: Asset.native(),
        destAmount: "123456789.334",
        path: [
          new Asset(
            "USDC",
            "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
          ),
          new Asset(
            "PANDA",
            "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z"
          ),
        ],
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opManageSellOfferCreate() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.manageSellOffer({
        selling: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        buying: Asset.native(),
        amount: "988448423.2134",
        price: "0.0001234",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opManageSellOfferUpdate() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.manageSellOffer({
        selling: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        buying: Asset.native(),
        amount: "988448423.2134",
        price: "0.0001234",
        offerId: "7123456",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opManageSellOfferDelete() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.manageSellOffer({
        selling: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        buying: Asset.native(),
        amount: "0",
        price: "0.0001234",
        offerId: "7123456",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opCreatePassiveSellOffer() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.createPassiveSellOffer({
        selling: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        buying: Asset.native(),
        amount: "988448423.2134",
        price: "0.0001234",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opSetOptions() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.setOptions({
        inflationDest: kp1.publicKey(),
        clearFlags: 8, // TODO: SDK do not support multiple flags
        setFlags: 1,
        masterWeight: 255,
        lowThreshold: 10,
        medThreshold: 20,
        highThreshold: 30,
        homeDomain: "stellar.org",
        signer: {
          ed25519PublicKey: kp2.publicKey(),
          weight: 10,
        },
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opSetOptionsWithEmptyBody() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.setOptions({
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opSetOptionsAddPublicKeySigner() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.setOptions({
        signer: {
          ed25519PublicKey: kp1.publicKey(),
          weight: 10,
        },
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opSetOptionsRemovePublicKeySigner() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.setOptions({
        signer: {
          ed25519PublicKey: kp1.publicKey(),
          weight: 0,
        },
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opSetOptionsAddHashXSigner() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.setOptions({
        signer: {
          sha256Hash: StrKey.decodeSha256Hash(
            "XDNA2V62PVEFBZ74CDJKTUHLY4Y7PL5UAV2MAM4VWF6USFE3SH235FXL"
          ),
          weight: 10,
        },
        source: kp0.publicKey(),
      })
    )
    .build();
}


export function opSetOptionsRemoveHashXSigner() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.setOptions({
        signer: {
          sha256Hash: StrKey.decodeSha256Hash(
            "XDNA2V62PVEFBZ74CDJKTUHLY4Y7PL5UAV2MAM4VWF6USFE3SH235FXL"
          ),
          weight: 0,
        },
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opSetOptionsAddPreAuthTxSigner() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.setOptions({
        signer: {
          preAuthTx: StrKey.decodePreAuthTx(
            "TDNA2V62PVEFBZ74CDJKTUHLY4Y7PL5UAV2MAM4VWF6USFE3SH234BSS"
          ),
          weight: 10,
        },
        source: kp0.publicKey(),
      })
    )
    .build();
}


export function opSetOptionsRemovePreAuthTxSigner() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.setOptions({
        signer: {
          preAuthTx: StrKey.decodePreAuthTx(
            "TDNA2V62PVEFBZ74CDJKTUHLY4Y7PL5UAV2MAM4VWF6USFE3SH234BSS"
          ),
          weight: 0,
        },
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opSetOptionsAddEd25519SignerPayloadSigner() {
  // We cannot build such a transaction directly through the js-stellar-base
  // from stellar_sdk import *

  // kp0 = Keypair.from_secret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK")
  // kp1 = Keypair.from_secret("SAE52G23WPAS7MIR2OFGILLICLXXR4K6HSXZHMKD6C33JCAVVILIWYAA")

  // account = Account(kp0.public_key, 103720918407102567)
  // tx = (
  //     TransactionBuilder(account, Network.PUBLIC_NETWORK_PASSPHRASE, 100)
  //     .add_text_memo("hello world")
  //     .add_time_bounds(0, 1670818332)
  //     .append_set_options_op(signer=Signer(signer_key=SignerKey.ed25519_signed_payload(
  //         SignedPayloadSigner(kp1.public_key, b'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ000123456789')),
  //         weight=10
  //     ), source=kp0.public_key).build()
  // )
  // xdr = tx.to_xdr()
  // PDRMNAIPTNIJWJSL6JOF76CJORN47TDVMWERTXO2G2WKOMXGNHUFKAAAABAGCYTDMRSWMZ3INFVGW3DNNZXXA4LSON2HK5TXPB4XUQKCINCEKRSHJBEUUS2MJVHE6UCRKJJVIVKWK5MFSWRQGAYDCMRTGQ2TMNZYHF52U
  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAAGQBcH2gMW7AaAAAAAEAAAAAAAAAAAAAAABjlqocAAAAAQAAAAtoZWxsbyB3b3JsZAAAAAABAAAAAQAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAAAUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAAD4saBD5tQmyZL8lxf+El0W8/MdWWJGd3aNqynMuZp6FUAAABAYWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWjAwMDEyMzQ1Njc4OQAAAAoAAAAAAAAAAA==";
  return TransactionBuilder.fromXDR(xdr, Networks.PUBLIC);
}


export function opSetOptionsRemoveEd25519SignerPayloadSigner() {
  // We cannot build such a transaction directly through the js-stellar-base
  // from stellar_sdk import *

  // kp0 = Keypair.from_secret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK")
  // kp1 = Keypair.from_secret("SAE52G23WPAS7MIR2OFGILLICLXXR4K6HSXZHMKD6C33JCAVVILIWYAA")

  // account = Account(kp0.public_key, 103720918407102567)
  // tx = (
  //     TransactionBuilder(account, Network.PUBLIC_NETWORK_PASSPHRASE, 100)
  //     .add_text_memo("hello world")
  //     .add_time_bounds(0, 1670818332)
  //     .append_set_options_op(signer=Signer(signer_key=SignerKey.ed25519_signed_payload(
  //         SignedPayloadSigner(kp1.public_key, b'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ000123456789')),
  //         weight=0
  //     ), source=kp0.public_key).build()
  // )
  // xdr = tx.to_xdr()
  // PDRMNAIPTNIJWJSL6JOF76CJORN47TDVMWERTXO2G2WKOMXGNHUFKAAAABAGCYTDMRSWMZ3INFVGW3DNNZXXA4LSON2HK5TXPB4XUQKCINCEKRSHJBEUUS2MJVHE6UCRKJJVIVKWK5MFSWRQGAYDCMRTGQ2TMNZYHF52U
  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAAGQBcH2gMW7AaAAAAAEAAAAAAAAAAAAAAABjlqocAAAAAQAAAAtoZWxsbyB3b3JsZAAAAAABAAAAAQAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAAAUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAAD4saBD5tQmyZL8lxf+El0W8/MdWWJGd3aNqynMuZp6FUAAABAYWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWjAwMDEyMzQ1Njc4OQAAAAAAAAAAAAAAAA==";
  return TransactionBuilder.fromXDR(xdr, Networks.PUBLIC);
}


export function opChangeTrustAddTrustLine() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.changeTrust({
        asset: new Asset(
          "USDC",
          "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        ),
        limit: "922337203680.9999999",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opChangeTrustRemoveTrustLine() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.changeTrust({
        asset: new Asset(
          "USD",
          "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        ),
        limit: "0",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opChangeTrustWithLiquidityPoolAssetAddTrustLine() {
  const asset1 = new Asset(
    "BTC",
    "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
  );
  const asset2 = new Asset(
    "USDC",
    "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
  );
  const asset = new LiquidityPoolAsset(asset1, asset2, 30);

  return getCommonTransactionBuilder()
    .addOperation(
      Operation.changeTrust({
        asset: asset,
        limit: "922337203680.9999999",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opChangeTrustWithLiquidityPoolAssetRemoveTrustLine() {
  const asset1 = new Asset(
    "BTC",
    "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
  );
  const asset2 = new Asset(
    "USDC",
    "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
  );
  const asset = new LiquidityPoolAsset(asset1, asset2, 30);

  return getCommonTransactionBuilder()
    .addOperation(
      Operation.changeTrust({
        asset: asset,
        limit: "0",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opAllowTrustDeauthorize() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.allowTrust({
        trustor: kp1.publicKey(),
        assetCode: "USD",
        authorize: 0,
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opAllowTrustAuthorize() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.allowTrust({
        trustor: kp1.publicKey(),
        assetCode: "USD",
        authorize: 1,
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opAllowTrustAuthorizeToMaintainLiabilities() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.allowTrust({
        trustor: kp1.publicKey(),
        assetCode: "USD",
        authorize: 2,
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opAccountMerge() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.accountMerge({
        destination: kp1.publicKey(),
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opAccountMergeWithMuxedDestination() {
  const muxedAccount = new MuxedAccount(
    new Account(kp1.publicKey(), "0"),
    "10000"
  ).accountId();
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.accountMerge({
        destination: muxedAccount,
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opInflation() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.inflation({
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opManageDataAdd() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.manageData({
        name: "Ledger Stellar App abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcda",
        value:
          "Hello Stellar! abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcda",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opManageDataAddWithUnprintableData() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.manageData({
        name: "Ledger Stellar App abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcda",
        value: Buffer.from("这是一条测试消息 hey", "utf8"),
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opManageDataRemove() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.manageData({
        name: "Ledger Stellar App abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcda",
        value: null,
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opBumpSequence() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "9223372036854775807",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opManageBuyOfferCreate() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.manageBuyOffer({
        selling: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        buying: Asset.native(),
        buyAmount: "988448111.2222",
        price: "0.0001011",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opManageBuyOfferUpdate() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.manageBuyOffer({
        selling: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        buying: Asset.native(),
        buyAmount: "988448111.2222",
        price: "0.0001011",
        offerId: "3523456",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opManageBuyOfferDelete() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.manageBuyOffer({
        selling: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        buying: Asset.native(),
        buyAmount: "0",
        price: "0.0001011",
        offerId: "3523456",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opPathPaymentStrictSend() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.pathPaymentStrictSend({
        sendAsset: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        sendAmount: "0.985",
        destination: kp1.publicKey(),
        destAsset: Asset.native(),
        destMin: "123456789.987",
        path: [
          new Asset(
            "USDC",
            "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
          ),
          new Asset(
            "PANDA",
            "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z"
          ),
        ],
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opPathPaymentStrictSendWithEmptyPath() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.pathPaymentStrictSend({
        sendAsset: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        sendAmount: "0.985",
        destination: kp1.publicKey(),
        destAsset: Asset.native(),
        destMin: "123456789.987",
        path: [],
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opPathPaymentStrictSendWithMuxedDestination() {
  const muxedAccount = new MuxedAccount(
    new Account(kp1.publicKey(), "0"),
    "10000"
  ).accountId();
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.pathPaymentStrictSend({
        sendAsset: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        sendAmount: "0.985",
        destination: muxedAccount,
        destAsset: Asset.native(),
        destMin: "123456789.987",
        path: [
          new Asset(
            "USDC",
            "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
          ),
          new Asset(
            "PANDA",
            "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z"
          ),
        ],
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opCreateClaimableBalance() {
  const claimants = [
    new Claimant(kp1.publicKey(), Claimant.predicateUnconditional()),
    new Claimant(
      kp2.publicKey(),
      Claimant.predicateAnd(
        Claimant.predicateOr(
          Claimant.predicateBeforeAbsoluteTime("1629344902"),
          Claimant.predicateBeforeAbsoluteTime("1629300000")
        ),
        Claimant.predicateNot(Claimant.predicateBeforeRelativeTime("180"))
      )
    ),
  ];

  return getCommonTransactionBuilder()
    .addOperation(
      Operation.createClaimableBalance({
        asset: new Asset(
          "USDC",
          "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        ),
        amount: "100",
        claimants: claimants,
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opClaimClaimableBalance() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.claimClaimableBalance({
        balanceId:
          "00000000da0d57da7d4850e7fc10d2a9d0ebc731f7afb40574c03395b17d49149b91f5be",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opBeginSponsoringFutureReserves() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.beginSponsoringFutureReserves({
        sponsoredId: kp1.publicKey(),
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opEndSponsoringFutureReserves() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.endSponsoringFutureReserves({
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opRevokeSponsorshipAccount() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.revokeAccountSponsorship({
        account: kp1.publicKey(),
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opRevokeSponsorshipTrustLineWithAsset() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.revokeTrustlineSponsorship({
        account: kp1.publicKey(),
        asset: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opRevokeSponsorshipTrustLineWithLiquidityPoolId() {
  const asset1 = new Asset(
    "BTC",
    "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
  );
  const asset2 = new Asset(
    "USDC",
    "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
  );

  const asset = new LiquidityPoolAsset(asset1, asset2, 30);
  const poolId = getLiquidityPoolId(
    "constant_product",
    asset.getLiquidityPoolParameters()
  );

  const id = new LiquidityPoolId(poolId.toString("hex"));

  return getCommonTransactionBuilder()
    .addOperation(
      Operation.revokeTrustlineSponsorship({
        account: kp1.publicKey(),
        asset: id,
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opRevokeSponsorshipOffer() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.revokeOfferSponsorship({
        seller: kp1.publicKey(),
        offerId: "123456",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opRevokeSponsorshipData() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.revokeDataSponsorship({
        account: kp1.publicKey(),
        name: "Ledger Stellar App abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcda",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opRevokeSponsorshipClaimableBalance() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.revokeClaimableBalanceSponsorship({
        balanceId:
          "00000000da0d57da7d4850e7fc10d2a9d0ebc731f7afb40574c03395b17d49149b91f5be",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opRevokeSponsorshipLiquidityPool() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.revokeLiquidityPoolSponsorship({
        liquidityPoolId:
          "dd7b1ab831c273310ddbec6f97870aa83c2fbd78ce22aded37ecbf4f3380fac7",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opRevokeSponsorshipEd25519PublicKeySigner() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.revokeSignerSponsorship({
        signer: {
          ed25519PublicKey: kp2.publicKey(),
        },
        account: kp1.publicKey(),
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opRevokeSponsorshipHashXSigner() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.revokeSignerSponsorship({
        signer: {
          sha256Hash: StrKey.decodeSha256Hash(
            "XDNA2V62PVEFBZ74CDJKTUHLY4Y7PL5UAV2MAM4VWF6USFE3SH235FXL"
          ),
        },
        account: kp1.publicKey(),
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opRevokeSponsorshipPreAuthTxSigner() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.revokeSignerSponsorship({
        signer: {
          preAuthTx: StrKey.decodePreAuthTx(
            "TDNA2V62PVEFBZ74CDJKTUHLY4Y7PL5UAV2MAM4VWF6USFE3SH234BSS"
          ),
        },
        account: kp1.publicKey(),
        source: kp0.publicKey(),
      })
    )
    .build();
}


// TODO: We need to add the corresponding support in the SDK.
// export function opRevokeSponsorshipEd25519SignedPayloadSigner() {
//   // We cannot build such a transaction directly through the js-stellar-base
//   // from stellar_sdk import *
//   // from stellar_sdk.operation.revoke_sponsorship import RevokeSponsorshipType, Signer
//   // tx = (
//   //   TransactionBuilder(account, Network.PUBLIC_NETWORK_PASSPHRASE, 100)
//   //   .add_text_memo("hello world")
//   //   .add_time_bounds(0, 1670818332)
//   //   .append_operation(
//   //       RevokeSponsorship(
//   //           RevokeSponsorshipType.SIGNER,
//   //           account_id=None,
//   //           trustline=None,
//   //           offer=None,
//   //           data=None,
//   //           claimable_balance_id=None,
//   //           liquidity_pool_id=None,
//   //           signer=Signer(
//   //               kp1.public_key,
//   //               SignerKey.ed25519_signed_payload(
//   //                   SignedPayloadSigner(
//   //                       kp1.public_key,
//   //                       b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ000123456789",
//   //                   )
//   //               ),
//   //           ),
//   //           source=kp0.public_key,
//   //       )
//   //   )
//   //   .build()
//   // )
//   // xdr = tx.to_xdr()
// }

export function opClawback() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.clawback({
        asset: new Asset(
          "USDC",
          "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        ),
        from: kp1.publicKey(),
        amount: "1000.85",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opClawbackWithMuxedFrom() {
  const muxedAccount = new MuxedAccount(
    new Account(kp1.publicKey(), "0"),
    "10000"
  ).accountId();
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.clawback({
        asset: new Asset(
          "USDC",
          "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        ),
        from: muxedAccount,
        amount: "1000.85",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opClawbackClaimableBalance() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.clawbackClaimableBalance({
        balanceId:
          "00000000da0d57da7d4850e7fc10d2a9d0ebc731f7afb40574c03395b17d49149b91f5be",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opSetTrustLineFlagsUnauthorized() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.setTrustLineFlags({
        trustor: kp1.publicKey(),
        asset: new Asset(
          "USDC",
          "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        ),
        flags: {
          authorized: false,
          authorizedToMaintainLiabilities: false,
          clawbackEnabled: false,
        },
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opSetTrustLineFlagsAuthorized() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.setTrustLineFlags({
        trustor: kp1.publicKey(),
        asset: new Asset(
          "USDC",
          "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        ),
        flags: {
          authorized: true,
          authorizedToMaintainLiabilities: true,
          clawbackEnabled: false,
        },
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opSetTrustLineFlagsAuthorizedToMaintainLiabilities() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.setTrustLineFlags({
        trustor: kp1.publicKey(),
        asset: new Asset(
          "USDC",
          "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        ),
        flags: {
          authorized: false,
          authorizedToMaintainLiabilities: true,
          clawbackEnabled: false,
        },
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opSetTrustLineFlagsAuthorizedAndClawbackEnabled() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.setTrustLineFlags({
        trustor: kp1.publicKey(),
        asset: new Asset(
          "USDC",
          "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        ),
        flags: {
          authorized: true,
          authorizedToMaintainLiabilities: false,
          clawbackEnabled: true,
        },
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opLiquidityPoolDeposit() {
  const asset1 = new Asset(
    "BTC",
    "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
  );
  const asset2 = new Asset(
    "USDC",
    "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
  );

  const asset = new LiquidityPoolAsset(asset1, asset2, 30);
  const poolId = getLiquidityPoolId(
    "constant_product",
    asset.getLiquidityPoolParameters()
  );

  return getCommonTransactionBuilder()
    .addOperation(
      Operation.liquidityPoolDeposit({
        liquidityPoolId: poolId.toString("hex"),
        maxAmountA: "1000000",
        maxAmountB: "0.2321",
        minPrice: "14324232.23",
        maxPrice: "10000000.00",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opLiquidityPoolWithdraw() {
  const asset1 = new Asset(
    "BTC",
    "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
  );
  const asset2 = new Asset(
    "USDC",
    "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
  );

  const asset = new LiquidityPoolAsset(asset1, asset2, 30);
  const poolId = getLiquidityPoolId(
    "constant_product",
    asset.getLiquidityPoolParameters()
  );

  return getCommonTransactionBuilder()
    .addOperation(
      Operation.liquidityPoolWithdraw({
        liquidityPoolId: poolId.toString("hex"),
        amount: "5000",
        minAmountA: "10000",
        minAmountB: "20000",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opInvokeHostFunctionUploadWasm() {
  /**
   * soroban --very-verbose contract deploy \
    --wasm ./increment/target/wasm32-unknown-unknown/release/soroban_increment_contract.wasm  \
    --source SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK \
    --rpc-url https://soroban-testnet.stellar.org \
    --network-passphrase 'Test SDF Network ; September 2015'
   */
  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAcdcALZ/tAAAAAQAAAAAAAAAAAAAAAQAAAAEAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAAYAAAAAgAAAkwAYXNtAQAAAAEVBGACfn4BfmADfn5+AX5gAAF+YAAAAhkEAWwBMAAAAWwBMQAAAWwBXwABAWwBOAAAAwUEAgMDAwUDAQAQBhkDfwFBgIDAAAt/AEGAgMAAC38AQYCAwAALBzUFBm1lbW9yeQIACWluY3JlbWVudAAEAV8ABwpfX2RhdGFfZW5kAwELX19oZWFwX2Jhc2UDAgqnAQSSAQIBfwF+QQAhAAJAAkACQEKOutCvhtQ5QgIQgICAgABCAVINAEKOutCvhtQ5QgIQgYCAgAAiAUL/AYNCBFINASABQiCIpyEACyAAQQFqIgBFDQFCjrrQr4bUOSAArUIghkIEhCIBQgIQgoCAgAAaQoSAgICgBkKEgICAwAwQg4CAgAAaIAEPCwAACxCFgICAAAALCQAQhoCAgAAACwQAAAALAgALAHMOY29udHJhY3RzcGVjdjAAAAAAAAAAQEluY3JlbWVudCBpbmNyZW1lbnRzIGFuIGludGVybmFsIGNvdW50ZXIsIGFuZCByZXR1cm5zIHRoZSB2YWx1ZS4AAAAJaW5jcmVtZW50AAAAAAAAAAAAAAEAAAAEAB4RY29udHJhY3RlbnZtZXRhdjAAAAAAAAAAFAAAADkAcw5jb250cmFjdG1ldGF2MAAAAAAAAAAFcnN2ZXIAAAAAAAAGMS43My4wAAAAAAAAAAAACHJzc2RrdmVyAAAAMzIwLjAuMC1yYzIjMDk5MjQxM2Y5YjA1ZTViZmIxZjg3MmJjZTk5ZTg5ZDkxMjliMmU2MQAAAAAAAAAAAQAAAAAAAAABAAAABxPhaFi95KtQoAbb8HFyKI8+wZ2GQNGoUwFsYMFcJREXAAAAAAAYZjYAAAKwAAAAAAAAAAAAAAAMAAAAAU5Gbs0AAABA4Qgx6lFhpvkLoEOoHEV2O/B0+ALtMwuX4Kh3iPmI4CtYXBFNMUmKDnKvsiZE/moqNtxyD8Ce0ZblL6rhjCCaBA==";
  return TransactionBuilder.fromXDR(xdr, Networks.TESTNET);
}

export function opInvokeHostFunctionCreateContractWasmId() {
  /**
 * soroban --very-verbose contract deploy \
  --wasm ./increment/target/wasm32-unknown-unknown/release/soroban_increment_contract.wasm  \
  --source SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK \
  --rpc-url https://soroban-testnet.stellar.org \
  --network-passphrase 'Test SDF Network ; September 2015'
 */
  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAB7iQALZ/tAAAAAgAAAAAAAAAAAAAAAQAAAAAAAAAYAAAAAQAAAAAAAAAAAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NqDN1ZmOZBULw7RQpLx1hMwklzeYyod2tz7XGLOGlAnsAAAAAE+FoWL3kq1CgBtvwcXIojz7BnYZA0ahTAWxgwVwlERcAAAABAAAAAAAAAAEAAAAAAAAAAAAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzagzdWZjmQVC8O0UKS8dYTMJJc3mMqHdrc+1xizhpQJ7AAAAABPhaFi95KtQoAbb8HFyKI8+wZ2GQNGoUwFsYMFcJREXAAAAAAAAAAEAAAAAAAAAAQAAAAcT4WhYveSrUKAG2/BxciiPPsGdhkDRqFMBbGDBXCURFwAAAAEAAAAGAAAAASDg5o1dgbNGaFKMfUZ6NtiiHIsKeZQjTddBtARypKjgAAAAFAAAAAEAAbYGAAAC4AAAAGgAAAAAAACh8wAAAAFORm7NAAAAQB9/IVcMlt2Uo2f5SSwDUXEimmUOQMqgz2baGQlL6a4aH/Jyqpm5sGsraNzDlbu6W5VIcHkEtuGVY+d7kPNFHwU=";
  return TransactionBuilder.fromXDR(xdr, Networks.TESTNET);
}

export function opInvokeHostFunctionCreateContractNewAsset() {
  // import time

  // from stellar_sdk import (
  //     Keypair,
  //     Network,
  //     SorobanServer,
  //     StrKey,
  //     TransactionBuilder,
  // )
  // from stellar_sdk import xdr as stellar_xdr
  // from stellar_sdk.exceptions import PrepareTransactionException
  // from stellar_sdk.soroban_rpc import GetTransactionStatus, SendTransactionStatus

  // secret = "SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK"
  // rpc_server_url = "https://soroban-testnet.stellar.org:443"
  // network_passphrase = Network.TESTNET_NETWORK_PASSPHRASE

  // kp = Keypair.from_secret(secret)
  // soroban_server = SorobanServer(rpc_server_url)
  // source = soroban_server.load_account(kp.public_key)

  // tx = (
  //     TransactionBuilder(source, network_passphrase)
  //     .add_time_bounds(0, 0)
  //     .append_create_stellar_asset_contract_from_address_op(address=kp.public_key, salt=b'c' * 32, source=kp.public_key)
  //     .build()
  // )

  // try:
  //     tx = soroban_server.prepare_transaction(tx)
  // except PrepareTransactionException as e:
  //     print(f"Got exception: {e.simulate_transaction_response}")
  //     raise e

  // print(tx.to_xdr())

  // tx.sign(kp)

  // send_transaction_data = soroban_server.send_transaction(tx)
  // print(f"sent transaction: {send_transaction_data}")
  // if send_transaction_data.status != SendTransactionStatus.PENDING:
  //     raise Exception("send transaction failed")

  // while True:
  //     print("waiting for transaction to be confirmed...")
  //     get_transaction_data = soroban_server.get_transaction(send_transaction_data.hash)
  //     if get_transaction_data.status != GetTransactionStatus.NOT_FOUND:
  //         break
  //     time.sleep(3)

  // print(f"transaction: {get_transaction_data}")

  // if get_transaction_data.status == GetTransactionStatus.SUCCESS:
  //     assert get_transaction_data.result_meta_xdr is not None
  //     transaction_meta = stellar_xdr.TransactionMeta.from_xdr(
  //         get_transaction_data.result_meta_xdr
  //     )
  //     result = transaction_meta.v3.soroban_meta.return_value.address.contract_id.hash  # type: ignore
  //     contract_id = StrKey.encode_contract(result)
  //     print(f"contract id: {contract_id}")
  // else:
  //     print(f"Transaction failed: {get_transaction_data.result_xdr}")

  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQABUbYALZ/tAAAACgAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAABAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAAGAAAAAEAAAAAAAAAAAAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzWNjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjAAAAAQAAAAEAAAAAAAAAAQAAAAAAAAAAAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2MAAAABAAAAAAAAAAEAAAAAAAAAAAAAAAEAAAAGAAAAAVvz5/0ZDBy7NBxhMS/CR+rdii7rHMTLZO5cDByDoxtUAAAAFAAAAAEAAXf0AAAAMAAAAEgAAAAAAACSxAAAAAA=";
  return TransactionBuilder.fromXDR(xdr, Networks.TESTNET);
}

export function opInvokeHostFunctionCreateContractWrapAsset() {
  /**
 * soroban contract deploy \
  --wasm ./increment/target/wasm32-unknown-unknown/release/soroban_increment_contract.wasm  \
  --source SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK \
  --rpc-url https://soroban-testnet.stellar.org \
  --network-passphrase 'Test SDF Network ; September 2015'
 */
  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAGY4YALZ/tAAAAAwAAAAAAAAAAAAAAAQAAAAAAAAAYAAAAAQAAAAEAAAACTEVER0VSAAAAAAAAAAAAAOLGgQ+bUJsmS/JcX/hJdFvPzHVliRnd2jaspzLmaehVAAAAAQAAAAAAAAABAAAAAAAAAAAAAAABAAAABgAAAAH1fw7+2c6SK9x+aL47FkMYOgIZwujCGmG2Ve5S7lUfFgAAABQAAAABAAOluwAAADAAAAHsAAAAAAABWgMAAAABTkZuzQAAAEAScsqMRjAFnQsVoZjDSfYjEGAOPXjgZaFvxaN1EuE0q/zuF+uukosgx7UaxNR7R1xvz0Rk27VtC+E0X/SNcWIC";
  return TransactionBuilder.fromXDR(xdr, Networks.TESTNET);
}

export function opInvokeHostFunctionUnverifiedContract() {
  /**
   * soroban --very-verbose contract invoke \
    --id CAQOBZUNLWA3GRTIKKGH2RT2G3MKEHELBJ4ZII2N25A3IBDSUSUOAUQU  \
    --source SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK \
    --rpc-url https://soroban-testnet.stellar.org \
    --network-passphrase 'Test SDF Network ; September 2015' \
    -- \
    increment
   */
  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQABxvgALZ/tAAAABAAAAAAAAAAAAAAAAQAAAAAAAAAYAAAAAAAAAAEg4OaNXYGzRmhSjH1GejbYohyLCnmUI03XQbQEcqSo4AAAAAlpbmNyZW1lbnQAAAAAAAAAAAAAAAAAAAEAAAAAAAAAAQAAAAcT4WhYveSrUKAG2/BxciiPPsGdhkDRqFMBbGDBXCURFwAAAAEAAAAGAAAAASDg5o1dgbNGaFKMfUZ6NtiiHIsKeZQjTddBtARypKjgAAAAFAAAAAEAGQovAAADSAAAAIQAAAAAAAANSQAAAAFORm7NAAAAQMeYqOX1HnwH9heyEgce5OcjQEakm+vFFqtXBEdaHMqDvMBVCcy4u8WhVAbOWCvNQf+/wjIaj03un47sRyLJtwc=";
  return TransactionBuilder.fromXDR(xdr, Networks.TESTNET);
}

export function opInvokeHostFunctionUnverifiedContractWithTransferFunction() {
  // from stellar_sdk import Keypair, Network, SorobanServer, TransactionBuilder, scval

  // rpc_server_url = "https://soroban-testnet.stellar.org:443"
  // soroban_server = SorobanServer(rpc_server_url)
  // network_passphrase = Network.TESTNET_NETWORK_PASSPHRASE

  // alice_kp = Keypair.from_secret(
  //     "SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK"
  // )  # GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7
  // bob_kp = Keypair.from_secret(
  //     "SAE52G23WPAS7MIR2OFGILLICLXXR4K6HSXZHMKD6C33JCAVVILIWYAA"
  // )  # GDRMNAIPTNIJWJSL6JOF76CJORN47TDVMWERTXO2G2WKOMXGNHUFL5QX
  // native_token_contract_id = "CAQOBZUNLWA3GRTIKKGH2RT2G3MKEHELBJ4ZII2N25A3IBDSUSUOAUQU"

  // alice_source = soroban_server.load_account(alice_kp.public_key)

  // args = [
  //     scval.to_address(alice_kp.public_key),  # from
  //     scval.to_address(bob_kp.public_key),  # spender
  //     scval.to_int128(100 * 10 ** 7),  # amount, 100 XLM
  //     scval.to_uint32(2990592)
  // ]

  // tx = (
  //     TransactionBuilder(alice_source, network_passphrase, base_fee=500)
  //     .add_time_bounds(0, 0)
  //     .append_invoke_contract_function_op(
  //         contract_id=native_token_contract_id,
  //         function_name="approve",
  //         parameters=args,
  //     )
  //     .build()
  // )

  // print(f"Unsigned XDR:\n{tx.to_xdr()}")
  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAAfQALZ/tAAAACAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAAAAAAAGAAAAAAAAAABIODmjV2Bs0ZoUox9Rno22KIciwp5lCNN10G0BHKkqOAAAAAHYXBwcm92ZQAAAAAEAAAAEgAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAASAAAAAAAAAADixoEPm1CbJkvyXF/4SXRbz8x1ZYkZ3do2rKcy5mnoVQAAAAoAAAAAAAAAAAAAAAA7msoAAAAAAwAtogAAAAAAAAAAAAAAAAA=";
  return TransactionBuilder.fromXDR(xdr, Networks.TESTNET);
}

export function opInvokeHostFunctionUnverifiedContractWithApproveFunction() {
  //   from stellar_sdk import Keypair, Network, SorobanServer, TransactionBuilder, scval

  // rpc_server_url = "https://soroban-testnet.stellar.org:443"
  // soroban_server = SorobanServer(rpc_server_url)
  // network_passphrase = Network.TESTNET_NETWORK_PASSPHRASE

  // alice_kp = Keypair.from_secret(
  //     "SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK"
  // )  # GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7
  // bob_kp = Keypair.from_secret(
  //     "SAE52G23WPAS7MIR2OFGILLICLXXR4K6HSXZHMKD6C33JCAVVILIWYAA"
  // )  # GDRMNAIPTNIJWJSL6JOF76CJORN47TDVMWERTXO2G2WKOMXGNHUFL5QX
  // native_token_contract_id = "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"

  // alice_source = soroban_server.load_account(alice_kp.public_key)

  // args = [
  //     scval.to_address(alice_kp.public_key),  # from
  //     scval.to_address(bob_kp.public_key),  # spender
  //     scval.to_int128(100 * 10 ** 7),  # amount, 100 XLM
  //     scval.to_uint32(2990592)
  // ]

  // tx = (
  //     TransactionBuilder(alice_source, network_passphrase, base_fee=500)
  //     .add_time_bounds(0, 0)
  //     .append_invoke_contract_function_op(
  //         contract_id=native_token_contract_id,
  //         function_name="mock",
  //         parameters=args,
  //         source=alice_kp.public_key
  //     )
  //     .build()
  // )

  // print(f"Unsigned XDR:\n{tx.to_xdr()}")
  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAAfQALZ/tAAAACwAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAABAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAAGAAAAAAAAAAB15KLcsJwPM/q9+uf9O9NUEpVqLl5/JtFDqLIQrTRzmEAAAAEbW9jawAAAAQAAAASAAAAAAAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAABIAAAAAAAAAAOLGgQ+bUJsmS/JcX/hJdFvPzHVliRnd2jaspzLmaehVAAAACgAAAAAAAAAAAAAAADuaygAAAAADAC2iAAAAAAAAAAAAAAAAAA==";
  return TransactionBuilder.fromXDR(xdr, Networks.TESTNET);
}

export function opInvokeHostFunctionAssetTransfer() {
  // import time

  // from stellar_sdk import Keypair, Network, SorobanServer, TransactionBuilder, scval
  // from stellar_sdk import xdr as stellar_xdr
  // from stellar_sdk.exceptions import PrepareTransactionException
  // from stellar_sdk.soroban_rpc import GetTransactionStatus, SendTransactionStatus

  // rpc_server_url = "https://soroban-testnet.stellar.org:443"
  // soroban_server = SorobanServer(rpc_server_url)
  // network_passphrase = Network.TESTNET_NETWORK_PASSPHRASE

  // alice_kp = Keypair.from_secret(
  //     "SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK"
  // )  # GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7
  // bob_kp = Keypair.from_secret(
  //     "SAE52G23WPAS7MIR2OFGILLICLXXR4K6HSXZHMKD6C33JCAVVILIWYAA"
  // )  # GDRMNAIPTNIJWJSL6JOF76CJORN47TDVMWERTXO2G2WKOMXGNHUFL5QX
  // native_token_contract_id = "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"

  // alice_source = soroban_server.load_account(alice_kp.public_key)

  // args = [
  //     scval.to_address(alice_kp.public_key),  # from
  //     scval.to_address(bob_kp.public_key),  # to
  //     scval.to_int128(100 * 10 ** 7),  # amount, 100 XLM
  // ]

  // tx = (
  //     TransactionBuilder(alice_source, network_passphrase, base_fee=500)
  //     .add_time_bounds(0, 0)
  //     .append_invoke_contract_function_op(
  //         contract_id=native_token_contract_id,
  //         function_name="transfer",
  //         parameters=args,
  //         source=alice_kp.public_key
  //     )
  //     .build()
  // )

  // try:
  //     tx = soroban_server.prepare_transaction(tx)
  // except PrepareTransactionException as e:
  //     print(f"Got exception: {e.simulate_transaction_response}")
  //     raise e

  // print(f"Unsigned XDR:\n{tx.to_xdr()}")

  // tx.sign(alice_kp)
  // print(f"Signed XDR:\n{tx.to_xdr()}")

  // send_transaction_data = soroban_server.send_transaction(tx)
  // print(f"sent transaction: {send_transaction_data}")
  // if send_transaction_data.status != SendTransactionStatus.PENDING:
  //     raise Exception("send transaction failed")
  // while True:
  //     print("waiting for transaction to be confirmed...")
  //     get_transaction_data = soroban_server.get_transaction(send_transaction_data.hash)
  //     if get_transaction_data.status != GetTransactionStatus.NOT_FOUND:
  //         break
  //     time.sleep(3)

  // print(f"transaction: {get_transaction_data}")

  // if get_transaction_data.status == GetTransactionStatus.SUCCESS:
  //     assert get_transaction_data.result_meta_xdr is not None
  //     transaction_meta = stellar_xdr.TransactionMeta.from_xdr(
  //         get_transaction_data.result_meta_xdr
  //     )
  //     if transaction_meta.v3.soroban_meta.return_value.type == stellar_xdr.SCValType.SCV_VOID:  # type: ignore[union-attr]
  //         print("send success")
  // else:
  //     print(f"Transaction failed: {get_transaction_data.result_xdr}")

  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQACOxwALZ/tAAAACwAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAABAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAAGAAAAAAAAAAB15KLcsJwPM/q9+uf9O9NUEpVqLl5/JtFDqLIQrTRzmEAAAAIdHJhbnNmZXIAAAADAAAAEgAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAASAAAAAAAAAADixoEPm1CbJkvyXF/4SXRbz8x1ZYkZ3do2rKcy5mnoVQAAAAoAAAAAAAAAAAAAAAA7msoAAAAAAQAAAAAAAAAAAAAAAdeSi3LCcDzP6vfrn/TvTVBKVai5efybRQ6iyEK00c5hAAAACHRyYW5zZmVyAAAAAwAAABIAAAAAAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAAEgAAAAAAAAAA4saBD5tQmyZL8lxf+El0W8/MdWWJGd3aNqynMuZp6FUAAAAKAAAAAAAAAAAAAAAAO5rKAAAAAAAAAAABAAAAAAAAAAEAAAAGAAAAAdeSi3LCcDzP6vfrn/TvTVBKVai5efybRQ6iyEK00c5hAAAAFAAAAAEAAAACAAAAAAAAAADixoEPm1CbJkvyXF/4SXRbz8x1ZYkZ3do2rKcy5mnoVQAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0ABBGDAAACFAAAAOwAAAAAAAAAOwAAAAA=";
  return TransactionBuilder.fromXDR(xdr, Networks.TESTNET);
}

export function opInvokeHostFunctionAssetApprove() {
  // import time

  // from stellar_sdk import Keypair, Network, SorobanServer, TransactionBuilder, scval
  // from stellar_sdk import xdr as stellar_xdr
  // from stellar_sdk.exceptions import PrepareTransactionException
  // from stellar_sdk.soroban_rpc import GetTransactionStatus, SendTransactionStatus

  // rpc_server_url = "https://soroban-testnet.stellar.org:443"
  // soroban_server = SorobanServer(rpc_server_url)
  // network_passphrase = Network.TESTNET_NETWORK_PASSPHRASE

  // alice_kp = Keypair.from_secret(
  //     "SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK"
  // )  # GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7
  // bob_kp = Keypair.from_secret(
  //     "SAE52G23WPAS7MIR2OFGILLICLXXR4K6HSXZHMKD6C33JCAVVILIWYAA"
  // )  # GDRMNAIPTNIJWJSL6JOF76CJORN47TDVMWERTXO2G2WKOMXGNHUFL5QX
  // native_token_contract_id = "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"

  // alice_source = soroban_server.load_account(alice_kp.public_key)

  // args = [
  //     scval.to_address(alice_kp.public_key),  # from
  //     scval.to_address(bob_kp.public_key),  # spender
  //     scval.to_int128(1000 * 10 ** 7),  # amount, 1000 XLM
  //     scval.to_uint32(2999592)
  // ]

  // tx = (
  //     TransactionBuilder(alice_source, network_passphrase, base_fee=500)
  //     .add_time_bounds(0, 0)
  //     .append_invoke_contract_function_op(
  //         contract_id=native_token_contract_id,
  //         function_name="approve",
  //         parameters=args,
  //         source=alice_kp.public_key
  //     )
  //     .build()
  // )

  // try:
  //     tx = soroban_server.prepare_transaction(tx)
  // except PrepareTransactionException as e:
  //     print(f"Got exception: {e.simulate_transaction_response}")
  //     raise e

  // print(f"Unsigned XDR:\n{tx.to_xdr()}")

  // tx.sign(alice_kp)
  // print(f"Signed XDR:\n{tx.to_xdr()}")

  // send_transaction_data = soroban_server.send_transaction(tx)
  // print(f"sent transaction: {send_transaction_data}")
  // if send_transaction_data.status != SendTransactionStatus.PENDING:
  //     raise Exception("send transaction failed")
  // while True:
  //     print("waiting for transaction to be confirmed...")
  //     get_transaction_data = soroban_server.get_transaction(send_transaction_data.hash)
  //     if get_transaction_data.status != GetTransactionStatus.NOT_FOUND:
  //         break
  //     time.sleep(3)

  // print(f"transaction: {get_transaction_data}")

  // if get_transaction_data.status == GetTransactionStatus.SUCCESS:
  //     assert get_transaction_data.result_meta_xdr is not None
  //     transaction_meta = stellar_xdr.TransactionMeta.from_xdr(
  //         get_transaction_data.result_meta_xdr
  //     )
  //     if transaction_meta.v3.soroban_meta.return_value.type == stellar_xdr.SCValType.SCV_VOID:  # type: ignore[union-attr]
  //         print("send success")
  // else:
  //     print(f"Transaction failed: {get_transaction_data.result_xdr}")
  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQADJ94ALZ/tAAAADAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAABAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAAGAAAAAAAAAAB15KLcsJwPM/q9+uf9O9NUEpVqLl5/JtFDqLIQrTRzmEAAAAHYXBwcm92ZQAAAAAEAAAAEgAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAASAAAAAAAAAADixoEPm1CbJkvyXF/4SXRbz8x1ZYkZ3do2rKcy5mnoVQAAAAoAAAAAAAAAAAAAAAJUC+QAAAAAAwAtxSgAAAABAAAAAAAAAAAAAAAB15KLcsJwPM/q9+uf9O9NUEpVqLl5/JtFDqLIQrTRzmEAAAAHYXBwcm92ZQAAAAAEAAAAEgAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAASAAAAAAAAAADixoEPm1CbJkvyXF/4SXRbz8x1ZYkZ3do2rKcy5mnoVQAAAAoAAAAAAAAAAAAAAAJUC+QAAAAAAwAtxSgAAAAAAAAAAQAAAAAAAAABAAAABgAAAAHXkotywnA8z+r365/0701QSlWouXn8m0UOoshCtNHOYQAAABQAAAABAAAAAQAAAAYAAAAB15KLcsJwPM/q9+uf9O9NUEpVqLl5/JtFDqLIQrTRzmEAAAAQAAAAAQAAAAIAAAAPAAAACUFsbG93YW5jZQAAAAAAABEAAAABAAAAAgAAAA8AAAAEZnJvbQAAABIAAAAAAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAADwAAAAdzcGVuZGVyAAAAABIAAAAAAAAAAOLGgQ+bUJsmS/JcX/hJdFvPzHVliRnd2jaspzLmaehVAAAAAAAEyB8AAAFYAAABLAAAAAAAAHGtAAAAAA==";
  return TransactionBuilder.fromXDR(xdr, Networks.TESTNET);
}

export function opInvokeHostFunctionScvals() {
  // from stellar_sdk import *
  // from stellar_sdk import xdr

  // kp0 = Keypair.from_secret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK")
  // source = Account(kp0.public_key, 1234567890)

  // scvals = [
  //     scval.to_bool(True),
  //     scval.to_void(),
  //     scval.to_uint32(1234),
  //     scval.to_int32(12345),
  //     scval.to_uint64(23432453),
  //     scval.to_int64(454546),
  //     scval.to_timepoint(2356623562),
  //     scval.to_duration(34543643),
  //     scval.to_uint128(43543645645645),
  //     scval.to_int128(43543645645645),
  //     scval.to_uint256(24358729874358025473024572),
  //     scval.to_int256(24358729874358025473024572),
  //     scval.to_bytes(b"this is test bytes"),
  //     scval.to_string("hello this is test string"),
  //     scval.to_symbol("testfunc"),
  //     scval.to_vec([scval.to_bool(True), scval.to_bool(False)]),
  //     scval.to_map(
  //         {
  //             scval.to_symbol("true"): scval.to_bool(True),
  //             scval.to_symbol("false"): scval.to_bool(False),
  //         }
  //     ),
  //     scval.to_address(kp0.public_key),
  //     scval.to_address("CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"),
  //     xdr.SCVal(xdr.SCValType.SCV_LEDGER_KEY_CONTRACT_INSTANCE),
  //     xdr.SCVal(
  //         xdr.SCValType.SCV_LEDGER_KEY_NONCE, nonce_key=xdr.SCNonceKey(xdr.Int64(100))
  //     ),
  //     xdr.SCVal(
  //         xdr.SCValType.SCV_CONTRACT_INSTANCE,
  //         instance=xdr.SCContractInstance(
  //             executable=xdr.ContractExecutable(
  //                 xdr.ContractExecutableType.CONTRACT_EXECUTABLE_STELLAR_ASSET
  //             ),
  //             storage=None,
  //         ),
  //     ),
  //     xdr.SCVal(
  //         xdr.SCValType.SCV_CONTRACT_INSTANCE,
  //         instance=xdr.SCContractInstance(
  //             executable=xdr.ContractExecutable(
  //                 xdr.ContractExecutableType.CONTRACT_EXECUTABLE_WASM,
  //                 wasm_hash=xdr.Hash(
  //                     b"\xcf\x88\x84S\xd6`V\xbc\xb6\xaeY*\x91\x90s\xb5\x93\xb5\x96[\xff\xcb\xcf\xc3\x04\xacGT\x9e\xac\xda\xd6"
  //                 ),
  //             ),
  //             storage=scval.to_map(
  //                 {
  //                     scval.to_symbol("true"): scval.to_bool(True),
  //                     scval.to_symbol("false"): scval.to_bool(False),
  //                 }
  //             ).map,
  //         ),
  //     ),
  // ]
  // tx = (
  //     TransactionBuilder(source, Network.TESTNET_NETWORK_PASSPHRASE, 500)
  //     .append_invoke_contract_function_op(
  //         contract_id="CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC",
  //         function_name="test",
  //         parameters=scvals,
  //     )
  //     .add_time_bounds(0, 0)
  //     .build()
  // )

  // print(tx.to_xdr())
  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAAfQAAAAASZYC0wAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAAAAAAAGAAAAAAAAAAB15KLcsJwPM/q9+uf9O9NUEpVqLl5/JtFDqLIQrTRzmEAAAAEdGVzdAAAABcAAAAAAAAAAQAAAAEAAAADAAAE0gAAAAQAADA5AAAABQAAAAABZY0FAAAABgAAAAAABu+SAAAABwAAAACMdzjKAAAACAAAAAACDxgbAAAACQAAAAAAAAAAAAAnmkuH600AAAAKAAAAAAAAAAAAACeaS4frTQAAAAsAAAAAAAAAAAAAAAAAAAAAAAAAAAAUJilkdtaR8ljiPAAAAAwAAAAAAAAAAAAAAAAAAAAAAAAAAAAUJilkdtaR8ljiPAAAAA0AAAASdGhpcyBpcyB0ZXN0IGJ5dGVzAAAAAAAOAAAAGWhlbGxvIHRoaXMgaXMgdGVzdCBzdHJpbmcAAAAAAAAPAAAACHRlc3RmdW5jAAAAEAAAAAEAAAACAAAAAAAAAAEAAAAAAAAAAAAAABEAAAABAAAAAgAAAA8AAAAEdHJ1ZQAAAAAAAAABAAAADwAAAAVmYWxzZQAAAAAAAAAAAAAAAAAAEgAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAASAAAAAdeSi3LCcDzP6vfrn/TvTVBKVai5efybRQ6iyEK00c5hAAAAFAAAABUAAAAAAAAAZAAAABMAAAABAAAAAAAAABMAAAAAz4iEU9ZgVry2rlkqkZBztZO1llv/y8/DBKxHVJ6s2tYAAAABAAAAAgAAAA8AAAAEdHJ1ZQAAAAAAAAABAAAADwAAAAVmYWxzZQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
  return TransactionBuilder.fromXDR(xdr, Networks.TESTNET);
}

export function opExtendFootprintTtl() {
  /**
   * soroban --very-verbose contract bump --ledgers-to-expire 130816 \
    --durability persistent --id CACEIKVZTU7Z6VKNISE3OO5MXSCKUC7HC2FNCWRO2HJMWSUPUWHDLSJE \
    --source SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK \
    --rpc-url https://soroban-testnet.stellar.org:443 \
    --network-passphrase 'Test SDF Network ; September 2015'
   */
  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAn9IALZ/tAAAABQAAAAAAAAAAAAAAAQAAAAAAAAAZAAAAAAAB/wAAAAABAAAAAAAAAAEAAAAGAAAAAQREKrmdP59VTUSJtzusvISqC+cWitFaLtHSy0qPpY41AAAAFAAAAAEAAAAAAAAAAAAAAJgAAAAAAAAAAAAAdogAAAABTkZuzQAAAEAQIX09qLt+SIcA7sOc7XGSWjK98FFURHW77g8uWm4lQirDqZU51B0uatCZe90mSt+RK7r7it3I92JSUL1Ba+EA";
  return TransactionBuilder.fromXDR(xdr, Networks.TESTNET);
}

export function opRestoreFootprint() {
  // TODO: add soroban resource
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.restoreFootprint({
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opWithEmptySource() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "922337203685.4775807",
      })
    )
    .build();
}

export function opWithMuxedSource() {
  const muxedAccount = new MuxedAccount(
    new Account(kp0.publicKey(), "0"),
    "10000"
  ).accountId();

  return getCommonTransactionBuilder()
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "922337203685.4775807",
        source: muxedAccount,
      })
    )
    .build();
}

export function txMemoNone() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txMemoId() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.id("18446744073709551615"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txMemoText() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world 123456789 123456"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txMemoTextUnprintable() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: new Memo("text", Buffer.from("这是一条测试消息 hey", "utf8")),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txMemoHash() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.hash(
      "573c10b148fc4bc7db97540ce49da22930f4bcd48a060dc7347be84ea9f52d9f"
    ),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txMemoReturnHash() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.return(
      "573c10b148fc4bc7db97540ce49da22930f4bcd48a060dc7347be84ea9f52d9f"
    ),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondWithAllItems() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
      minTime: 1657951297, // 2022-07-16T06:01:37+00:00
    },
    ledgerbounds: {
      minLedger: 40351800,
      maxLedger: 40352000,
    },
    minAccountSequence: "103420918407103888",
    minAccountSequenceAge: 1649239999,
    minAccountSequenceLedgerGap: 30,
    extraSigners: [
      "GBJCHUKZMTFSLOMNC7P4TS4VJJBTCYL3XKSOLXAUJSD56C4LHND5TWUC",
      "PA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAQACAQDAQCQMBYIBEFAWDANBYHRAEISCMKBKFQXDAMRUGY4DUPB6IBZGM",
    ],
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondIsNone() {
  // We cannot build such a transaction directly through the js-stellar-base
  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAAGQBcH2gMW7AaAAAAAAAAAAEVzwQsUj8S8fbl1QM5J2iKTD0vNSKBg3HNHvoTqn1LZ8AAAABAAAAAQAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAAAEAAAAA4saBD5tQmyZL8lxf+El0W8/MdWWJGd3aNqynMuZp6FUAAAAAAAAAAACYloAAAAAAAAAAAA==";
  return TransactionBuilder.fromXDR(xdr, Networks.PUBLIC);
}

export function txCondTimeBounds() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
      minTime: 1657951297, // 2022-07-16T06:01:37+00:00
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondTimeBoundsMaxIsZero() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 1670818332, // 2022-12-12T04:12:12+00:00
      maxTime: 0,
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondTimeBoundsMinIsZero() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1657951297, // 2022-07-16T06:01:37+00:00
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondTimeBoundsAreZero() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 0,
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondTimeBoundsIsNone() {
  // We cannot build such a transaction directly through the js-stellar-base
  const xdr = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAAGQBcH2gMW7AaAAAAAIAAAAAAAAAAQJnuDgCZ7kAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAARXPBCxSPxLx9uXVAzknaIpMPS81IoGDcc0e+hOqfUtnwAAAAEAAAABAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAAAQAAAADixoEPm1CbJkvyXF/4SXRbz8x1ZYkZ3do2rKcy5mnoVQAAAAAAAAAAAJiWgAAAAAAAAAAA"
  return TransactionBuilder.fromXDR(xdr, Networks.PUBLIC);
}

export function txCondLedgerBounds() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 0,
    },
    ledgerbounds: {
      minLedger: 40351800,
      maxLedger: 40352000,
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondLedgerBoundsMaxIsZero() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 0,
    },
    ledgerbounds: {
      minLedger: 40351800,
      maxLedger: 0,
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondLedgerBoundsMinIsZero() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 0,
    },
    ledgerbounds: {
      minLedger: 0,
      maxLedger: 40352000,
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondLedgerBoundsAreZero() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 0,
    },
    ledgerbounds: {
      minLedger: 0,
      maxLedger: 0,
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondMinAccountSequence() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 0,
    },
    minAccountSequence: "103420918407103888",
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondMinAccountSequenceAge() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 0,
    },
    minAccountSequenceAge: 1649239999,
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondMinAccountSequenceLedgerGap() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 0,
    },
    minAccountSequenceLedgerGap: 30,
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondExtraSignersWithOneSigner() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 0,
    },
    extraSigners: [
      "PA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAQACAQDAQCQMBYIBEFAWDANBYHRAEISCMKBKFQXDAMRUGY4DUPB6IBZGM",
    ],
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txCondExtraSignersWithTwoSigners() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 0,
    },
    extraSigners: [
      "PA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAQACAQDAQCQMBYIBEFAWDANBYHRAEISCMKBKFQXDAMRUGY4DUPB6IBZGM",
      "GBJCHUKZMTFSLOMNC7P4TS4VJJBTCYL3XKSOLXAUJSD56C4LHND5TWUC",
    ],
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txMultiOperations() {
  return getCommonTransactionBuilder()
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "922337203685.4775807",
        source: kp0.publicKey(),
      })
    )
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: new Asset(
          "BTC",
          "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        ),
        amount: "922337203685.4775807",
        source: kp0.publicKey(),
      })
    )
    .addOperation(
      Operation.setOptions({
        homeDomain: "stellar.org",
      })
    )
    .build();
}

export function txCustomBaseFee() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: "1275",
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .addOperation(
      Operation.payment({
        destination: kp2.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txWithMuxedSource() {
  const muxedAccount = new MuxedAccount(
    new Account(kp0.publicKey(), "103720918407102567"),
    "10000"
  );

  return new TransactionBuilder(muxedAccount, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txNetworkPublic() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txNetworkTestnet() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.TESTNET,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txNetworkCustom() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: "Custom Network; July 2022",
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function feeBumpTx() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  const innerTx = new TransactionBuilder(account, {
    fee: "50",
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .addOperation(
      Operation.payment({
        destination: kp2.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
  innerTx.sign(kp0);
  const feeBumpTx = TransactionBuilder.buildFeeBumpTransaction(
    kp0,
    "750",
    innerTx,
    Networks.PUBLIC
  );
  return feeBumpTx;
}

export function feeBumpTxWithMuxedFeeSource() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  const innerTx = new TransactionBuilder(account, {
    fee: "50",
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.payment({
        destination: kp1.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .addOperation(
      Operation.payment({
        destination: kp2.publicKey(),
        asset: Asset.native(),
        amount: "1",
        source: kp0.publicKey(),
      })
    )
    .build();
  innerTx.sign(kp0);
  innerTx.sign(kp1);

  const muxedAccount = new MuxedAccount(
    new Account(kp0.publicKey(), "103720918407102567"),
    "10000"
  ).accountId();
  const feeBumpTx = TransactionBuilder.buildFeeBumpTransaction(
    muxedAccount,
    "750",
    innerTx,
    Networks.PUBLIC
  );
  return feeBumpTx;
}

export function txSourceOmitSourceEqualSigner() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function txSourceOmitSourceNotEqualSigner() {
  const account = new Account(kp1.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp1.publicKey(),
      })
    )
    .build();
}

export function txSourceOmitMuxedSourceEqualSigner() {
  const muxedAccount = new MuxedAccount(
    new Account(kp0.publicKey(), "103720918407102567"),
    "10000"
  );
  return new TransactionBuilder(muxedAccount, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function feeBumpTxOmitFeeSourceEqualSigner() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  const innerTx = new TransactionBuilder(account, {
    fee: "50",
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp0.publicKey(),
      })
    )
    .build();
  innerTx.sign(kp0);
  const feeBumpTx = TransactionBuilder.buildFeeBumpTransaction(
    kp0,
    "750",
    innerTx,
    Networks.PUBLIC
  );
  return feeBumpTx;
}

export function feeBumpTxOmitFeeSourceNotEqualSigner() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  const innerTx = new TransactionBuilder(account, {
    fee: "50",
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp0.publicKey(),
      })
    )
    .build();
  innerTx.sign(kp0);
  const feeBumpTx = TransactionBuilder.buildFeeBumpTransaction(
    kp1,
    "750",
    innerTx,
    Networks.PUBLIC
  );
  return feeBumpTx;
}

export function feeBumpTxOmitMuxedFeeSourceEqualSigner() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  const innerTx = new TransactionBuilder(account, {
    fee: "50",
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp0.publicKey(),
      })
    )
    .build();
  innerTx.sign(kp0);

  const muxedAccount = new MuxedAccount(
    new Account(kp0.publicKey(), "103720918407102567"),
    "10000"
  ).accountId();
  const feeBumpTx = TransactionBuilder.buildFeeBumpTransaction(
    muxedAccount,
    "750",
    innerTx,
    Networks.PUBLIC
  );
  return feeBumpTx;
}

export function opSourceOmitTxSourceEqualOpSourceEqualSigner() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opSourceOmitTxSourceEqualOpSourceNotEqualSigner() {
  const account = new Account(kp1.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp1.publicKey(),
      })
    )
    .build();
}

export function opSourceOmitOpSourceEqualSignerNotEqualTxSource() {
  const account = new Account(kp1.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp0.publicKey(),
      })
    )
    .build();
}

export function opSourceOmitTxSourceEqualSignerNotEqualOpSource() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp1.publicKey(),
      })
    )
    .build();
}

export function opSourceOmitTxMuxedSourceEqualOpMuxedSourceEqualSigner() {
  const muxedAccount = new MuxedAccount(
    new Account(kp0.publicKey(), "103720918407102567"),
    "10000"
  );
  return new TransactionBuilder(muxedAccount, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: muxedAccount.accountId(),
      })
    )
    .build();
}

export function opSourceOmitTxSourceEqualOpMuxedSourceEqualSigner() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  const muxedAccount = new MuxedAccount(account, "10000");

  return new TransactionBuilder(account, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: muxedAccount.accountId(),
      })
    )
    .build();
}

export function opSourceOmitTxMuxedSourceEqualOpSourceEqualSigner() {
  const account = new Account(kp0.publicKey(), "103720918407102567");
  const muxedAccount = new MuxedAccount(account, "10000");
  return new TransactionBuilder(muxedAccount, {
    fee: BASE_FEE,
    networkPassphrase: Networks.PUBLIC,
    memo: Memo.text("hello world"),
    timebounds: {
      minTime: 0,
      maxTime: 1670818332, // 2022-12-12T04:12:12+00:00
    },
  })
    .addOperation(
      Operation.bumpSequence({
        bumpTo: "1232134324234",
        source: kp0.publicKey(),
      })
    )
    .build();
}