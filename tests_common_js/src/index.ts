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
} from "stellar-base";

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