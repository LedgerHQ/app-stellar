import { DEFAULT_START_OPTIONS, ButtonKind, TouchNavigation } from "@zondax/zemu";
import { APP_SEED, models } from "./common";
import * as testCasesFunction from "tests-common";
import { Keypair, StrKey } from "@stellar/stellar-base";
import Str from "@ledgerhq/hw-app-str";
import { StellarUserRefusedError, StellarHashSigningNotEnabledError } from "@ledgerhq/hw-app-str";
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

describe("get public key", () => {
  test.each(models)("get public key without confirmation ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const { rawPublicKey } = await str.getPublicKey("44'/148'/0'", false);
      const result = StrKey.encodeEd25519PublicKey(rawPublicKey);
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
      expect(result).toEqual(kp.publicKey());
    } finally {
      await sim.close();
    }
  });

  test.each(models)("get public key with confirmation - approve ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    try {
      let confirmText = "Approve";
      if (dev.name == "stax") {
        confirmText = "Confirm";
      }
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.ApproveTapButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const result = str.getPublicKey("44'/148'/0'", true);
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await sim.navigateAndCompareUntilText(".", `${dev.prefix.toLowerCase()}-public-key-approve`, confirmText, true);
      const pk = StrKey.encodeEd25519PublicKey((await result).rawPublicKey);
      expect(pk).toEqual(kp.publicKey());
    } finally {
      await sim.close();
    }
  });

  test.each(models)("get public key with confirmation - reject ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    try {
      let confirmText = "Reject";
      if (dev.name == "stax") {
        confirmText = "Confirm";
      }
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      expect(() => str.getPublicKey("44'/148'/0'", true)).rejects.toThrow(StellarUserRefusedError);

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
      expect(() => str.signHash("44'/148'/0'", hash)).rejects.toThrow(StellarHashSigningNotEnabledError);
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
          ButtonKind.ToggleSettingButton1,
        ]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-hash-signing-approve`, settingNav.schedule, true, true);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickBoth(undefined, false);
      }
      const hash = Buffer.from("3389e9f0f1a65f19736cacf544c2e825313e8447f569233bb8db39aa607c8889", "hex");
      const result = str.signHash("44'/148'/0'", hash);
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      let textToFind = "Sign Hash";
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
          ButtonKind.ToggleSettingButton1,
        ]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-hash-signing-reject`, settingNav.schedule, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickBoth(undefined, false);
      }

      const hash = Buffer.from("3389e9f0f1a65f19736cacf544c2e825313e8447f569233bb8db39aa607c8889", "hex");
      expect(() => str.signHash("44'/148'/0'", hash)).rejects.toThrow(StellarUserRefusedError);

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
  describe.each(getTxTestCases())("$caseName", (c) => {
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
            ButtonKind.ToggleSettingButton2,
          ]);
          await sim.navigate(".", `${dev.prefix.toLowerCase()}-${c.filePath}`, settingNav.schedule, true, true);
        } else {
          await sim.clickRight();
          await sim.clickBoth(undefined, false);
          await sim.clickRight();
          await sim.clickBoth(undefined, false);
        }
        const result = str.signTransaction("44'/148'/0'", tx.signatureBase());
        const events = await sim.getEvents();
        await sim.waitForScreenChanges(events);
        // TODO: If set to Sign, it will not pass the test. Is this a bug in Zemu?
        let textToFind = /\bSign\b/;
        if (dev.name == "stax") {
          textToFind = /\bHold to\b/;
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
      let textToFind = "Reject";
      // display sequence
      if (dev.name == "stax") {
        textToFind = "Hold to";
        const settingNav = new TouchNavigation([
          ButtonKind.InfoButton,
          ButtonKind.ToggleSettingButton2,
        ]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-tx-reject`, settingNav.schedule, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
      }

      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(StellarUserRefusedError);

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
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-tx-reject`, settingNav.schedule, true, false);
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

      let textToFind = "Reject";
      // display sequence
      if (dev.name == "stax") {
        textToFind = "Hold to";
        const settingNav = new TouchNavigation([
          ButtonKind.InfoButton,
          ButtonKind.ToggleSettingButton2,
        ]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-fee-bump-tx-reject`, settingNav.schedule, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
      }

      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(StellarUserRefusedError);

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
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-fee-bump-tx-reject`, settingNav.schedule, true, false);
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
      let textToFind = /\bSign\b/;
      if (dev.name == "stax") {
        textToFind = /\bHold to\b/;
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
      let textToFind = /\bSign\b/;
      if (dev.name == "stax") {
        textToFind = /\bHold to\b/;
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
});

describe("soroban auth", () => {
  describe.each(getAuthTestCases())("$caseName", (c) => {
    test.each(models)("device ($dev.name)", async ({ dev, startText }) => {
      const hashIdPreimage = c.txFunction();
      const sim = new Zemu(dev.path);
      try {
        await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
        const transport = await sim.getTransport();
        const str = new Str(transport);
        const result = str.signSorobanAuthorization("44'/148'/0'", hashIdPreimage.toXDR("raw"));
        const events = await sim.getEvents();
        await sim.waitForScreenChanges(events);
        let textToFind = /\bSign\b/;
        if (dev.name == "stax") {
          textToFind = /\bHold to\b/;
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
        const signature = kp.sign(hash(hashIdPreimage.toXDR()))
        expect((await result).signature).toStrictEqual(signature);
      } finally {
        await sim.close();
      }
    });
  });

  test.each(models)("reject soroban auth ($dev.name)", async ({ dev, startText }) => {
    const hashIdPreimage = testCasesFunction.sorobanAuthInvokeContract();
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      expect(() => str.signSorobanAuthorization("44'/148'/0'", hashIdPreimage.toXDR("raw"))).rejects.toThrow(StellarUserRefusedError);

      let textToFind = "Reject";
      if (dev.name == "stax") {
        textToFind = "Sign Soroban Auth?";
      }
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-soroban-auth-reject`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax") {
        const settingNav = new TouchNavigation([ButtonKind.ApproveTapButton]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-soroban-auth-reject`, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });

});

describe("plugin", () => {
  test.each(models)("invoke host function ($dev.name)", async ({ dev, startText, plugin_path }) => {
    const tx = testCasesFunction.opInvokeHostFunctionTestPlugin();
    const sim = new Zemu(dev.path, {
      "StellarTest": plugin_path
    });
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      if (dev.name == "stax") {
        const settingNav = new TouchNavigation([
          ButtonKind.InfoButton,
          ButtonKind.ToggleSettingButton2,
        ]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-plugin-invoke-host-function`, settingNav.schedule, true, true);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
      }
      const result = str.signTransaction("44'/148'/0'", tx.signatureBase());
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      let textToFind = /\bSign\b/;
      if (dev.name == "stax") {
        textToFind = /\bHold to\b/;
      }
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-plugin-invoke-host-function`,
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

  test.each(models)("reject tx ($dev.name)", async ({ dev, startText, plugin_path }) => {
    const tx = testCasesFunction.opInvokeHostFunctionTestPlugin();
    const sim = new Zemu(dev.path, {
      "StellarTest": plugin_path
    });
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      let textToFind = "Reject";
      // display sequence
      if (dev.name == "stax") {
        textToFind = "Hold to";
        const settingNav = new TouchNavigation([
          ButtonKind.InfoButton,
          ButtonKind.ToggleSettingButton2,
        ]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-plugin-invoke-host-function-reject`, settingNav.schedule, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
      }

      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(StellarUserRefusedError);

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-plugin-invoke-host-function-reject`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax") {
        const settingNav = new TouchNavigation([ButtonKind.ApproveTapButton]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-plugin-invoke-host-function-reject`, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });


  test.each(models)("soroban auth ($dev.name)", async ({ dev, startText, plugin_path }) => {
    const hashIdPreimage = testCasesFunction.sorobanAuthInvokeContractTestPlugin();
    const sim = new Zemu(dev.path, {
      "StellarTest": plugin_path
    });
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const result = str.signSorobanAuthorization("44'/148'/0'", hashIdPreimage.toXDR("raw"));
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      let textToFind = /\bSign\b/;
      if (dev.name == "stax") {
        textToFind = /\bHold to\b/;
      }
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-plugin-soroban-auth`,
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


  test.each(models)("reject soroban auth ($dev.name)", async ({ dev, startText, plugin_path }) => {
    const hashIdPreimage = testCasesFunction.sorobanAuthInvokeContractTestPlugin();
    const sim = new Zemu(dev.path, {
      "StellarTest": plugin_path
    });
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      expect(() => str.signSorobanAuthorization("44'/148'/0'", hashIdPreimage.toXDR("raw"))).rejects.toThrow(StellarUserRefusedError);

      let textToFind = "Reject";
      if (dev.name == "stax") {
        textToFind = "Sign Soroban Auth?";
      }
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-plugin-soroban-auth-reject`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax") {
        const settingNav = new TouchNavigation([ButtonKind.ApproveTapButton]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-soroban-auth-reject`, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });
});



function camelToFilePath(str: string) {
  return str.replace(/([A-Z])/g, "-$1").toLowerCase();
}

function getTxTestCases() {
  const casesFunction = Object.keys(testCasesFunction);
  const cases = [];
  for (const rawCase of casesFunction) {
    if (rawCase.startsWith("sorobanAuth")) continue;
    if (rawCase === "opInvokeHostFunctionTestPlugin") continue;
    cases.push({
      caseName: rawCase,
      filePath: camelToFilePath(rawCase),
      txFunction: (testCasesFunction as any)[rawCase], // dirty hack
    });
  }
  return cases;
}

function getAuthTestCases() {
  const casesFunction = Object.keys(testCasesFunction);
  const cases = [];
  for (const rawCase of casesFunction) {
    if (!rawCase.startsWith("sorobanAuth")) continue;
    if (rawCase === "sorobanAuthInvokeContractTestPlugin") continue;
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