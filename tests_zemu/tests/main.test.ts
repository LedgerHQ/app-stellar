import { DEFAULT_START_OPTIONS, ButtonKind, TouchNavigation } from "@zondax/zemu";
import { APP_SEED, models } from "./common";
import * as testCasesFunction from "tests-common";
import { Keypair, xdr } from "@stellar/stellar-base";
import Str from "@ledgerhq/hw-app-str";
import Zemu from "@zondax/zemu";
import { sha256 } from 'sha.js'

beforeAll(async () => {
  await Zemu.checkAndPullImage();
});

jest.setTimeout(1000 * 60 * 60);

let defaultOptions = {
  ...DEFAULT_START_OPTIONS,
  logging: true,
  custom: `-s "${APP_SEED}"`,
  X11: false,
};

test.each(models)("can start and stop container ($dev.name)", async ({ dev, startText }) => {
  const sim = new Zemu(dev.path);
  try {
    await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
  } finally {
    await sim.close();
  }
});

test.each(models)("app version ($dev.name)", async ({ dev, startText }) => {
  const sim = new Zemu(dev.path);
  try {
    await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
    const transport = await sim.getTransport();
    const str = new Str(transport);
    const result = await str.getAppConfiguration();
    expect(result.version).toBe("5.0.3");
  } finally {
    await sim.close();
  }
});

describe("get public key", () => {
  test.each(models)("get public key without confirmation ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const result = await str.getPublicKey("44'/148'/0'", false, false);
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");

      expect(result).toStrictEqual({
        publicKey: kp.publicKey(),
        raw: kp.rawPublicKey(),
      });
    } finally {
      await sim.close();
    }
  });

  test.each(models)("get public key with confirmation - approve ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    try {
      let confirmText = "Approve";
      if (dev.name == "stax") {
        confirmText = "Address";
      }
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.ApproveTapButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const result = str.getPublicKey("44'/148'/0'", false, true);
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await sim.navigateAndCompareUntilText(".", `${dev.prefix.toLowerCase()}-public-key-approve`, confirmText, true);
      expect(result).resolves.toStrictEqual({
        publicKey: kp.publicKey(),
        raw: kp.rawPublicKey(),
      });
    } finally {
      await sim.close();
    }
  });

  test.each(models)("get public key with confirmation - reject ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    try {
      let confirmText = "Reject";
      if (dev.name == "stax") {
        confirmText = "Address";
      }
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      // TODO: Maybe we should throw a more specific exception in @ledgerhq/hw-app-str
      expect(() => str.getPublicKey("44'/148'/0'", false, true)).rejects.toThrow(
        "Ledger device: Condition of use not satisfied (denied by the user?) (0x6985)"
      );

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await sim.navigateAndCompareUntilText(".", `${dev.prefix.toLowerCase()}-public-key-reject`, confirmText, true);
    } finally {
      await sim.close();
    }
  });
});

describe("hash signing", () => {
  test.each(models)("hash signing mode is not enabled ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const hash = Buffer.from("3389e9f0f1a65f19736cacf544c2e825313e8447f569233bb8db39aa607c8889", "hex");
      expect(() => str.signHash("44'/148'/0'", hash)).rejects.toThrow(
        new Error("Hash signing not allowed. Have you enabled it in the app settings?")
      );
    } finally {
      await sim.close();
    }
  });

  test.each(models)("approve ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      // enable hash signing
      if (dev.name == "stax") {
        const settingNav = new TouchNavigation([
          ButtonKind.InfoButton,
          ButtonKind.NavRightButton,
          ButtonKind.ToggleSettingButton1,
        ]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-hash-signing-approve`, settingNav.schedule, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickBoth(undefined, false);
      }
      const hash = Buffer.from("3389e9f0f1a65f19736cacf544c2e825313e8447f569233bb8db39aa607c8889", "hex");
      const result = str.signHash("44'/148'/0'", hash);
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      let textToFind = "Approve";
      if (dev.name == "stax") {
        textToFind = "Hold to";
      }
      await sim.navigateAndCompareUntilText(".", `${dev.prefix.toLowerCase()}-hash-signing-approve`, textToFind, true);
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
      expect((await result).signature).toStrictEqual(kp.sign(hash));
    } finally {
      await sim.close();
    }
  });

  test.each(models)("reject ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      let textToFind = "Reject";
      // enable hash signing
      if (dev.name == "stax") {
        textToFind = "Hold to";
        const settingNav = new TouchNavigation([
          ButtonKind.InfoButton,
          ButtonKind.NavRightButton,
          ButtonKind.ToggleSettingButton1,
        ]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-hash-signing-reject`, settingNav.schedule, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickBoth(undefined, false);
      }

      const hash = Buffer.from("3389e9f0f1a65f19736cacf544c2e825313e8447f569233bb8db39aa607c8889", "hex");
      expect(() => str.signHash("44'/148'/0'", hash)).rejects.toThrow(new Error("Transaction approval request was rejected"));

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await sim.navigateAndCompareUntilText(".", `${dev.prefix.toLowerCase()}-hash-signing-reject`, textToFind, true);
      if (dev.name == "stax") {
        const settingNav = new TouchNavigation([ButtonKind.ApproveTapButton]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-hash-signing-reject`, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });
});

describe("transactions", () => {
  describe.each(getTestCases())("$caseName", (c) => {
    test.each(models)("device ($dev.name)", async ({ dev, startText }) => {
      const tx = c.txFunction();
      const sim = new Zemu(dev.path);
      try {
        await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
        const transport = await sim.getTransport();
        const str = new Str(transport);
        if (dev.name == "stax") {
          const settingNav = new TouchNavigation([
            ButtonKind.InfoButton,
            ButtonKind.NavRightButton,
            ButtonKind.ToggleSettingButton2,
            ButtonKind.ToggleSettingButton3,
          ]);
          await sim.navigate(".", `tx`, settingNav.schedule, true, false);
        } else {
          await sim.clickRight();
          await sim.clickBoth(undefined, false);
          await sim.clickRight();
          await sim.clickBoth(undefined, false);
          await sim.clickRight();
          await sim.clickBoth(undefined, false);
        }
        const result = str.signTransaction("44'/148'/0'", tx.signatureBase());
        const events = await sim.getEvents();
        await sim.waitForScreenChanges(events);
        let textToFind = "Finalize";
        if (dev.name == "stax") {
          textToFind = "Hold to";
        }
        await sim.navigateAndCompareUntilText(
          ".",
          `${dev.prefix.toLowerCase()}-${c.filePath}`,
          textToFind,
          true,
          undefined,
          1000 * 60 * 60
        );
        const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
        tx.sign(kp);
        expect((await result).signature).toStrictEqual(tx.signatures[0].signature());
      } finally {
        await sim.close();
      }
    });
  });

  test.each(models)("reject tx ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.txNetworkPublic();
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      let textToFind = "Cancel";
      // display sequence
      if (dev.name == "stax") {
        textToFind = "Hold to";
        const settingNav = new TouchNavigation([
          ButtonKind.InfoButton,
          ButtonKind.NavRightButton,
          ButtonKind.ToggleSettingButton3,
        ]);
        await sim.navigate(".", `reject tx`, settingNav.schedule, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickRight();
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
      }

      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(
        new Error("Transaction approval request was rejected")
      );

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-tx-reject`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax") {
        const settingNav = new TouchNavigation([ButtonKind.ApproveTapButton]);
        await sim.navigate(".", `reject tx`, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });

  test.each(models)("reject fee bump tx ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.feeBumpTx();
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      let textToFind = "Cancel";
      // display sequence
      if (dev.name == "stax") {
        textToFind = "Hold to";
        const settingNav = new TouchNavigation([
          ButtonKind.InfoButton,
          ButtonKind.NavRightButton,
          ButtonKind.ToggleSettingButton3,
        ]);
        await sim.navigate(".", `reject fee bump tx`, settingNav.schedule, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickRight();
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
      }

      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(
        new Error("Transaction approval request was rejected")
      );

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-fee-bump-tx-reject`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax") {
        const settingNav = new TouchNavigation([ButtonKind.ApproveTapButton]);
        await sim.navigate(".", `reject fee bump tx`, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });

  test.each(models)("hide sequence tx ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.txNetworkPublic();
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      const result = str.signTransaction("44'/148'/0'", tx.signatureBase());
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      let textToFind = "Finalize";
      if (dev.name == "stax") {
        textToFind = "Hold to";
      }
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-tx-hide-sequence`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
      tx.sign(kp);
      expect((await result).signature).toStrictEqual(tx.signatures[0].signature());
    } finally {
      await sim.close();
    }
  });

  test.each(models)("hide sequence fee bump tx ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.feeBumpTx();
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      const result = str.signTransaction("44'/148'/0'", tx.signatureBase());
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      let textToFind = "Finalize";
      if (dev.name == "stax") {
        textToFind = "Hold to";
      }
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-fee-bump-tx-hide-sequence`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );

      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
      tx.sign(kp);
      expect((await result).signature).toStrictEqual(tx.signatures[0].signature());
    } finally {
      await sim.close();
    }
  });

  test.each(models)("disable custom contract support ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.opInvokeHostFunctionUnverifiedContract()
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(
        new Error("Custom contract mode is not enabled")
      );
    } finally {
      await sim.close();
    }
  });
});

describe("soroban auth", () => {
  test.each(models)("custom contract ($dev.name)", async ({ dev, startText }) => {
    const hashIdPreimage = xdr.HashIdPreimage.fromXDR("AAAACc7gMC1ZhE0yvcqRXIID3USzP7t+3BkFHqN6vt8o7NRyAAAAAAAAANUAAAB7AAAAAAAAAAHXkotywnA8z+r365/0701QSlWouXn8m0UOoshCtNHOYQAAAAR0ZXN0AAAABwAAABIAAAAAAAAAAJR2U+bMMdA0nH7HAYNhr5Op1xUBrfPK8nO8iTBa1KacAAAAEgAAAAAAAAAAWLfEosjyl6qPPSRxKB/fzOyv5I5WYzE+wY4Spz7KmKEAAAANAAAA3GhlbGxvIHdvcmxkaGVsbG8gd29ybGRoZWxsbyB3b3JsZGhlbGxvIHdvcmxkaGVsbG8gd29ybGRoZWxsbyB3b3JsZGhlbGxvIHdvcmxkaGVsbG8gd29ybGRoZWxsbyB3b3JsZGhlbGxvIHdvcmxkaGVsbG8gd29ybGRoZWxsbyB3b3JsZGhlbGxvIHdvcmxkaGVsbG8gd29ybGRoZWxsbyB3b3JsZGhlbGxvIHdvcmxkaGVsbG8gd29ybGRoZWxsbyB3b3JsZGhlbGxvIHdvcmxkaGVsbG8gd29ybGQAAAAQAAAAAQAAAAMAAAAGAAAAAAAAAOgAAAAGAAAAAAAAACsAAAAGAAAAAAAAAdIAAAARAAAAAQAAAAIAAAAGAAAAAAAAAAEAAAAGAAAAAAAAAAIAAAAGAAAAAAAAAAMAAAAGAAAAAAAAAAQAAAAGAAAAAAAAKGcAAAAUAAAAAgAAAAAAAAAB15KLcsJwPM/q9+uf9O9NUEpVqLl5/JtFDqLIQrTRzmEAAAAEc3ViMQAAAAEAAAASAAAAAAAAAACUdlPmzDHQNJx+xwGDYa+TqdcVAa3zyvJzvIkwWtSmnAAAAAEAAAAAAAAAAdeSi3LCcDzP6vfrn/TvTVBKVai5efybRQ6iyEK00c5hAAAABHN1YjMAAAABAAAAEgAAAAAAAAAAlHZT5swx0DScfscBg2Gvk6nXFQGt88ryc7yJMFrUppwAAAAAAAAAAAAAAAHXkotywnA8z+r365/0701QSlWouXn8m0UOoshCtNHOYQAAAARzdWIyAAAAAQAAABIAAAAAAAAAAJR2U+bMMdA0nH7HAYNhr5Op1xUBrfPK8nO8iTBa1KacAAAAAA==", "base64")
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      if (dev.name == "stax") {
        const settingNav = new TouchNavigation([
          ButtonKind.InfoButton,
          ButtonKind.NavRightButton,
          ButtonKind.ToggleSettingButton2,
          ButtonKind.ToggleSettingButton3,
        ]);
        await sim.navigate(".", `tx`, settingNav.schedule, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
      }
      const result = str.signSorobanAuthoration("44'/148'/0'", hashIdPreimage.toXDR());
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      let textToFind = "Finalize";
      if (dev.name == "stax") {
        textToFind = "Hold to";
      }
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-soroban-auth-custom-contract`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
      const signature = kp.sign(hash(hashIdPreimage.toXDR()))
      expect((await result).signature).toStrictEqual(signature);
    } finally {
      await sim.close();
    }
  });

  test.each(models)("disable custom contract support ($dev.name)", async ({ dev, startText }) => {
    const hashIdPreimage = xdr.HashIdPreimage.fromXDR("AAAACc7gMC1ZhE0yvcqRXIID3USzP7t+3BkFHqN6vt8o7NRyAAAAAEl1bUUCEMifAAAAAAAAAAHXkotywnA8z+r365/0701QSlWouXn8m0UOoshCtNHOYQAAAApjdXN0b21mdW5jAAAAAAACAAAAEgAAAAAAAAAArNCtx3gjgAC9trrSLpGZI4l/eofjEM7xUF5sCwtxVCUAAAAKAAAAAAAAAAAAAAACaUQFAAAAAAA=", "base64")
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      expect(() => str.signSorobanAuthoration("44'/148'/0'", hashIdPreimage.toXDR())).rejects.toThrow(
        new Error("Custom contract mode is not enabled")
      );
    } finally {
      await sim.close();
    }
  })

  test.each(models)("asset transfer ($dev.name)", async ({ dev, startText }) => {
    // from stellar_sdk import *
    // from stellar_sdk import xdr

    // kp = Keypair.from_mnemonic_phrase("sock divorce large flag accident car innocent step success grid attitude race")

    // data = xdr.HashIDPreimage(
    //     xdr.EnvelopeType.ENVELOPE_TYPE_SOROBAN_AUTHORIZATION,
    //     soroban_authorization=xdr.HashIDPreimageSorobanAuthorization(
    //         network_id=xdr.Hash(Network(Network.TESTNET_NETWORK_PASSPHRASE).network_id()),
    //         nonce=xdr.Int64(1232432453),
    //         signature_expiration_ledger=xdr.Uint32(34654367),
    //         invocation=xdr.SorobanAuthorizedInvocation(function=xdr.SorobanAuthorizedFunction(
    //             xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
    //             contract_fn=xdr.InvokeContractArgs(
    //                 contract_address=Address(
    //                     "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC").to_xdr_sc_address(),
    //                 function_name=scval.to_symbol("transfer").sym,
    //                 args=[
    //                     scval.to_address("GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"),  # from
    //                     scval.to_address("GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"),  # to
    //                     scval.to_int128(103560 * 10 ** 5),  # amount
    //                 ]
    //             )), sub_invocations=[])
    //     )
    // )

    // print(data.to_xdr())

    const hashIdPreimage = xdr.HashIdPreimage.fromXDR("AAAACc7gMC1ZhE0yvcqRXIID3USzP7t+3BkFHqN6vt8o7NRyAAAAAEl1bUUCEMifAAAAAAAAAAHXkotywnA8z+r365/0701QSlWouXn8m0UOoshCtNHOYQAAAAh0cmFuc2ZlcgAAAAMAAAASAAAAAAAAAACs0K3HeCOAAL22utIukZkjiX96h+MQzvFQXmwLC3FUJQAAABIAAAAAAAAAAHmloTuvVXFjeiFXxq/7dJHXxEVO7NKwt+QOZwI/CW+sAAAACgAAAAAAAAAAAAAAAmlEBQAAAAAA", "base64")
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const result = str.signSorobanAuthoration("44'/148'/0'", hashIdPreimage.toXDR());
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      let textToFind = "Finalize";
      if (dev.name == "stax") {
        textToFind = "Hold to";
      }
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-soroban-auth-asset-transfer`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
      const signature = kp.sign(hash(hashIdPreimage.toXDR()))
      expect((await result).signature).toStrictEqual(signature);
    } finally {
      await sim.close();
    }
  });

  test.each(models)("asset approve ($dev.name)", async ({ dev, startText }) => {
    // from stellar_sdk import *
    // from stellar_sdk import xdr

    // kp = Keypair.from_mnemonic_phrase("sock divorce large flag accident car innocent step success grid attitude race")

    // data = xdr.HashIDPreimage(
    //     xdr.EnvelopeType.ENVELOPE_TYPE_SOROBAN_AUTHORIZATION,
    //     soroban_authorization=xdr.HashIDPreimageSorobanAuthorization(
    //         network_id=xdr.Hash(Network(Network.PUBLIC_NETWORK_PASSPHRASE).network_id()),
    //         nonce=xdr.Int64(1232432453),
    //         signature_expiration_ledger=xdr.Uint32(34654367),
    //         invocation=xdr.SorobanAuthorizedInvocation(function=xdr.SorobanAuthorizedFunction(
    //             xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
    //             contract_fn=xdr.InvokeContractArgs(
    //                 contract_address=Address(
    //                     "CAUIKL3IYGMERDRUN6YSCLWVAKIFG5Q4YJHUKM4S4NJZQIA3BAS6OJPK").to_xdr_sc_address(),
    //                 function_name=scval.to_symbol("approve").sym,
    //                 args=[
    //                     scval.to_address("GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"),  # from
    //                     scval.to_address("GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"),  # spender
    //                     scval.to_int128(10000 * 10 ** 7),  # amount, 100 AQUA
    //                     scval.to_uint32(356654),  # live_until_ledger
    //                 ]
    //             )), sub_invocations=[])
    //     )
    // )

    // print(data.to_xdr())

    const hashIdPreimage = xdr.HashIdPreimage.fromXDR("AAAACXrDOZdUTjF10ma9AiQ5sizbFlCMARY/JuXLKj4QRal5AAAAAEl1bUUCEMifAAAAAAAAAAEohS9owZhIjjRvsSEu1QKQU3Ycwk9FM5LjU5ggGwgl5wAAAAdhcHByb3ZlAAAAAAQAAAASAAAAAAAAAACs0K3HeCOAAL22utIukZkjiX96h+MQzvFQXmwLC3FUJQAAABIAAAAAAAAAAHmloTuvVXFjeiFXxq/7dJHXxEVO7NKwt+QOZwI/CW+sAAAACgAAAAAAAAAAAAAAF0h26AAAAAADAAVxLgAAAAA=", "base64")
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const result = str.signSorobanAuthoration("44'/148'/0'", hashIdPreimage.toXDR());
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      let textToFind = "Finalize";
      if (dev.name == "stax") {
        textToFind = "Hold to";
      }
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-soroban-auth-asset-approve`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
      const signature = kp.sign(hash(hashIdPreimage.toXDR()))
      expect((await result).signature).toStrictEqual(signature);
    } finally {
      await sim.close();
    }
  });

  test.each(models)("create contract ($dev.name)", async ({ dev, startText }) => {
    // import os

    // from stellar_sdk import *
    // from stellar_sdk import xdr

    // kp = Keypair.from_mnemonic_phrase("sock divorce large flag accident car innocent step success grid attitude race")

    // create_contract = stellar_xdr.CreateContractArgs(
    //     contract_id_preimage=stellar_xdr.ContractIDPreimage(
    //         stellar_xdr.ContractIDPreimageType.CONTRACT_ID_PREIMAGE_FROM_ADDRESS,
    //         from_address=stellar_xdr.ContractIDPreimageFromAddress(
    //             address=Address("GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34").to_xdr_sc_address(),
    //             salt=stellar_xdr.Uint256(os.urandom(32)),
    //         ),
    //     ),
    //     executable=stellar_xdr.ContractExecutable(
    //         stellar_xdr.ContractExecutableType.CONTRACT_EXECUTABLE_WASM,
    //         stellar_xdr.Hash(os.urandom(32)),
    //     ),
    // )
    // data = xdr.HashIDPreimage(
    //     xdr.EnvelopeType.ENVELOPE_TYPE_SOROBAN_AUTHORIZATION,
    //     soroban_authorization=xdr.HashIDPreimageSorobanAuthorization(
    //         network_id=xdr.Hash(Network(Network.TESTNET_NETWORK_PASSPHRASE).network_id()),
    //         nonce=xdr.Int64(1232432453),
    //         signature_expiration_ledger=xdr.Uint32(34654367),
    //         invocation=xdr.SorobanAuthorizedInvocation(function=xdr.SorobanAuthorizedFunction(
    //             xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CREATE_CONTRACT_HOST_FN,
    //             create_contract_host_fn=create_contract), sub_invocations=[])
    //     )
    // )

    // print(data.to_xdr())
    const hashIdPreimage = xdr.HashIdPreimage.fromXDR("AAAACc7gMC1ZhE0yvcqRXIID3USzP7t+3BkFHqN6vt8o7NRyAAAAAEl1bUUCEMifAAAAAQAAAAAAAAAAAAAAAHmloTuvVXFjeiFXxq/7dJHXxEVO7NKwt+QOZwI/CW+sZtJrYZLQhUNQ41DfRYSHnaHrciqZinqVKPFRKq8TKTIAAAAAtYMmZe+dL7n/Rl8va30Nong7L5JZ1zFQSNKKkrOVJhgAAAAA", "base64")
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const result = str.signSorobanAuthoration("44'/148'/0'", hashIdPreimage.toXDR());
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      let textToFind = "Finalize";
      if (dev.name == "stax") {
        textToFind = "Hold to";
      }
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-soroban-auth-create-contract`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
      const signature = kp.sign(hash(hashIdPreimage.toXDR()))
      expect((await result).signature).toStrictEqual(signature);
    } finally {
      await sim.close();
    }
  });
})

function camelToFilePath(str: string) {
  return str.replace(/([A-Z])/g, "-$1").toLowerCase();
}

function getTestCases() {
  const casesFunction = Object.keys(testCasesFunction);
  const cases = [];
  for (const rawCase of casesFunction) {
    cases.push({
      caseName: rawCase,
      filePath: camelToFilePath(rawCase),
      txFunction: (testCasesFunction as any)[rawCase], // dirty hack
    });
  }
  return cases;
}

function hash(data: Buffer) {
  const hasher = new sha256()
  hasher.update(data)
  return hasher.digest()
}