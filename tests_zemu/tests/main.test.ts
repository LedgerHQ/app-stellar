import { DEFAULT_START_OPTIONS, ButtonKind, TouchNavigation, INavElement } from "@zondax/zemu";
import { APP_SEED, models } from "./common";
import * as testCasesFunction from "tests-common";
import { Keypair, StrKey } from "@stellar/stellar-base";
import Str from "@ledgerhq/hw-app-str";
import { StellarUserRefusedError } from "@ledgerhq/hw-app-str";
import Zemu from "@zondax/zemu";
import { sha256 } from 'sha.js'
import { ActionKind, TModel } from "@zondax/zemu/dist/types";


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

test.concurrent.each(models)("can start and stop container ($dev.name)", async ({ dev, startText }) => {
  const sim = new Zemu(dev.path);
  try {
    await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
  } finally {
    await sim.close();
  }
});

describe("get public key", () => {
  test.concurrent.each(models)("get public key without confirmation ($dev.name)", async ({ dev, startText }) => {
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

  test.concurrent.each(models)("get public key with confirmation - approve ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    const testCaseName = `${dev.prefix.toLowerCase()}-public-key-approve`;
    try {
      const confirmText = dev.name.startsWith("nano") ? "Approve" : "Confirm";
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.ApproveTapButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const result = str.getPublicKey("44'/148'/0'", true);
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await sim.navigateAndCompareUntilText(".", testCaseName, confirmText, true);
      const pk = StrKey.encodeEd25519PublicKey((await result).rawPublicKey);
      expect(pk).toEqual(kp.publicKey());
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)("get public key with confirmation - reject ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    const testCaseName = `${dev.prefix.toLowerCase()}-public-key-reject`;
    try {
      const confirmText = dev.name.startsWith("nano") ? "Reject" : "Confirm";
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      expect(() => str.getPublicKey("44'/148'/0'", true)).rejects.toThrow(StellarUserRefusedError);

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await sim.navigateAndCompareUntilText(".", testCaseName, confirmText, true);
    } finally {
      await sim.close();
    }
  });
});

describe("hash signing", () => {
  // TODO: skip for now, see https://github.com/LedgerHQ/ledger-secure-sdk/issues/737
  test.concurrent.each(models)("approve ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    const testCaseName = `${dev.prefix.toLowerCase()}-hash-signing-approve`;
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      const hash = Buffer.from("3389e9f0f1a65f19736cacf544c2e825313e8447f569233bb8db39aa607c8889", "hex");
      const result = str.signHash("44'/148'/0'", hash);
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      // accept risk
      await acceptRisk(sim, dev.name, testCaseName);
      await sim.deleteEvents();

      const textToFind = dev.name.startsWith("nano") ? "Sign Hash" : "Hold to";
      await sim.navigateAndCompareUntilText(".", testCaseName, textToFind, true);
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
      expect((await result).signature).toStrictEqual(kp.sign(hash));
    } finally {
      await sim.close();
    }
  });

  // TODO: skip for now, see https://github.com/LedgerHQ/ledger-secure-sdk/issues/737
  test.concurrent.each(models)("reject ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    const testCaseName = `${dev.prefix.toLowerCase()}-hash-signing-reject`;
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      const hash = Buffer.from("3389e9f0f1a65f19736cacf544c2e825313e8447f569233bb8db39aa607c8889", "hex");
      expect(() => str.signHash("44'/148'/0'", hash)).rejects.toThrow(StellarUserRefusedError);

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);

      // accept risk
      await acceptRisk(sim, dev.name, testCaseName);
      await sim.deleteEvents();
      const textToFind = dev.name.startsWith("nano") ? "Reject" : "Hold to";
      await sim.navigateAndCompareUntilText(".", testCaseName, textToFind, true);
      if (dev.name == "stax" || dev.name == "flex") {
        const settingNav = new TouchNavigation(dev.name, [ButtonKind.ApproveTapButton]);
        await sim.navigate(".", testCaseName, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)("refuse risk ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    const testCaseName = `${dev.prefix.toLowerCase()}-hash-signing-refuse-risk`;
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      const hash = Buffer.from("3389e9f0f1a65f19736cacf544c2e825313e8447f569233bb8db39aa607c8889", "hex");
      expect(() => str.signHash("44'/148'/0'", hash)).rejects.toThrow(StellarUserRefusedError);

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await refuseRisk(sim, dev.name, testCaseName);
    } finally {
      await sim.close();
    }
  });
});

describe("transactions", () => {
  describe.each(getTxTestCases())("$caseName", (c) => {
    test.concurrent.each(models)("device ($dev.name)", async ({ dev, startText }) => {
      const tx = c.txFunction();
      const sim = new Zemu(dev.path);
      const testCaseName = `${dev.prefix.toLowerCase()}-${c.filePath}`;
      try {
        await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
        const transport = await sim.getTransport();
        const str = new Str(transport);

        // enable custom contracts and seqence number
        const testsNeedEnableCustomContracts = [
          "opInvokeHostFunctionAssetApprove",
          "opInvokeHostFunctionAssetTransfer",
          "opInvokeHostFunctionScvalsCase0",
          "opInvokeHostFunctionScvalsCase1",
          "opInvokeHostFunctionScvalsCase2",
          "opInvokeHostFunctionTestPlugin",
          "opInvokeHostFunctionWithAuth",
          "opInvokeHostFunctionWithAuthAndNoArgs",
          "opInvokeHostFunctionWithAuthAndNoArgsAndNoSource",
          "opInvokeHostFunctionWithComplexSubInvocation",
          "opInvokeHostFunctionWithoutArgs",
          "opInvokeHostFunctionWithoutAuthAndNoSource"
        ];
        await enableSequence(sim, dev.name, testCaseName);

        const result = str.signTransaction("44'/148'/0'", tx.signatureBase());
        const events = await sim.getEvents();
        await sim.waitForScreenChanges(events);

        // accept risk
        if (testsNeedEnableCustomContracts.includes(c.caseName)) {
          await acceptRisk(sim, dev.name, testCaseName);
          await sim.deleteEvents();
        }

        // TODO: If set to Sign, it will not pass the test. Is this a bug in Zemu?
        const textToFind = dev.name.startsWith("nano") ? /\bSign\b/ : /\bHold to\b/;
        await sim.navigateAndCompareUntilText(
          ".",
          testCaseName,
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

  test.concurrent.each(models)("reject tx ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.txNetworkPublic();
    const sim = new Zemu(dev.path);
    const testCaseName = `${dev.prefix.toLowerCase()}-tx-reject`;
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      await enableSequence(sim, dev.name, testCaseName);

      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(StellarUserRefusedError);

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      const textToFind = dev.name.startsWith("nano") ? "Reject" : "Sign transaction?";
      await sim.navigateAndCompareUntilText(
        ".",
        testCaseName,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax" || dev.name == "flex") {
        const settingNav = new TouchNavigation(dev.name, [ButtonKind.ApproveTapButton]);
        await sim.navigate(".", testCaseName, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)("reject fee bump tx ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.feeBumpTx();
    const sim = new Zemu(dev.path);
    const testCaseName = `${dev.prefix.toLowerCase()}-fee-bump-tx-reject`;
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      await enableSequence(sim, dev.name, testCaseName);

      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(StellarUserRefusedError);

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      const textToFind = dev.name.startsWith("nano") ? "Reject" : "Sign transaction?";
      await sim.navigateAndCompareUntilText(
        ".",
        testCaseName,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax" || dev.name == "flex") {
        const settingNav = new TouchNavigation(dev.name, [ButtonKind.ApproveTapButton]);
        await sim.navigate(".", testCaseName, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)("hide sequence tx ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.txNetworkPublic();
    const sim = new Zemu(dev.path);
    const testCaseName = `${dev.prefix.toLowerCase()}-tx-hide-sequence`;
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      const result = str.signTransaction("44'/148'/0'", tx.signatureBase());
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      const textToFind = dev.name.startsWith("nano") ? /\bSign\b/ : /\bHold to\b/;
      await sim.navigateAndCompareUntilText(
        ".",
        testCaseName,
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

  test.concurrent.each(models)("hide sequence fee bump tx ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.feeBumpTx();
    const sim = new Zemu(dev.path);
    const testCaseName = `${dev.prefix.toLowerCase()}-fee-bump-tx-hide-sequence`;
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      const result = str.signTransaction("44'/148'/0'", tx.signatureBase());
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      const textToFind = dev.name.startsWith("nano") ? /\bSign\b/ : /\bHold to\b/;
      await sim.navigateAndCompareUntilText(
        ".",
        testCaseName,
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

  test.concurrent.each(models)("refuse risk ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.opInvokeHostFunctionScvalsCase0();
    const sim = new Zemu(dev.path);
    const testCaseName = `${dev.prefix.toLowerCase()}-tx-refuse-risk`;
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(StellarUserRefusedError);
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await refuseRisk(sim, dev.name, testCaseName);
    } finally {
      await sim.close();
    }
  });
});

describe("soroban auth", () => {
  describe.each(getAuthTestCases())("$caseName", (c) => {
    test.concurrent.each(models)("device ($dev.name)", async ({ dev, startText }) => {
      const hashIdPreimage = c.txFunction();
      const sim = new Zemu(dev.path);
      const testCaseName = `${dev.prefix.toLowerCase()}-${c.filePath}`;
      try {
        await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
        const transport = await sim.getTransport();
        const str = new Str(transport);

        // enable custom contracts
        const testsNeedEnableCustomContracts = [
          "sorobanAuthInvokeContract",
          "sorobanAuthInvokeContractWithComplexSubInvocation",
          "sorobanAuthInvokeContractWithoutArgs",
          "sorobanAuthPublic",
          "sorobanAuthTestnet",
          "sorobanAuthUnknownNetwork"
        ];
        const result = str.signSorobanAuthorization("44'/148'/0'", hashIdPreimage.toXDR("raw"));
        const events = await sim.getEvents();
        await sim.waitForScreenChanges(events);
        if (testsNeedEnableCustomContracts.includes(c.caseName)) {
          await acceptRisk(sim, dev.name, testCaseName);
          await sim.deleteEvents();
        }
        const textToFind = dev.name.startsWith("nano") ? /\bSign\b/ : /\bHold to\b/;
        await sim.navigateAndCompareUntilText(
          ".",
          testCaseName,
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

  test.concurrent.each(models)("reject soroban auth ($dev.name)", async ({ dev, startText }) => {
    const hashIdPreimage = testCasesFunction.sorobanAuthInvokeContract();
    const sim = new Zemu(dev.path);
    const testCaseName = `${dev.prefix.toLowerCase()}-soroban-auth-reject`;
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      expect(() => str.signSorobanAuthorization("44'/148'/0'", hashIdPreimage.toXDR("raw"))).rejects.toThrow(StellarUserRefusedError);

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);

      await acceptRisk(sim, dev.name, testCaseName);
      await sim.deleteEvents();
      const textToFind = dev.name.startsWith("nano") ? "Reject" : "Sign Soroban Auth?";
      await sim.navigateAndCompareUntilText(
        ".",
        testCaseName,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax" || dev.name == "flex") {
        const settingNav = new TouchNavigation(dev.name, [ButtonKind.ApproveTapButton]);
        await sim.navigate(".", testCaseName, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)("refuse risk ($dev.name)", async ({ dev, startText }) => {
    const hashIdPreimage = testCasesFunction.sorobanAuthInvokeContract();
    const sim = new Zemu(dev.path);
    const testCaseName = `${dev.prefix.toLowerCase()}-soroban-auth-refuse-risk`;
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      expect(() => str.signSorobanAuthorization("44'/148'/0'", hashIdPreimage.toXDR("raw"))).rejects.toThrow(StellarUserRefusedError);
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      await refuseRisk(sim, dev.name, testCaseName);
    } finally {
      await sim.close();
    }
  });
});

describe("plugin", () => {
  test.concurrent.each(models)("invoke host function ($dev.name)", async ({ dev, startText, plugin_path }) => {
    const tx = testCasesFunction.opInvokeHostFunctionTestPlugin();
    const sim = new Zemu(dev.path, {
      "StellarTest": plugin_path
    });
    const testCaseName = `${dev.prefix.toLowerCase()}-plugin-invoke-host-function`;
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const result = str.signTransaction("44'/148'/0'", tx.signatureBase());
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      const textToFind = dev.name.startsWith("nano") ? /\bSign\b/ : /\bHold to\b/;
      await sim.navigateAndCompareUntilText(
        ".",
        testCaseName,
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

  test.concurrent.each(models)("reject tx ($dev.name)", async ({ dev, startText, plugin_path }) => {
    const tx = testCasesFunction.opInvokeHostFunctionTestPlugin();
    const testCaseName = `${dev.prefix.toLowerCase()}-plugin-invoke-host-function-reject`;
    const sim = new Zemu(dev.path, {
      "StellarTest": plugin_path
    });
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(StellarUserRefusedError);
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      const textToFind = dev.name.startsWith("nano") ? "Reject" : "Sign transaction?";
      await sim.navigateAndCompareUntilText(
        ".",
        testCaseName,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax" || dev.name == "flex") {
        const settingNav = new TouchNavigation(dev.name, [ButtonKind.ApproveTapButton]);
        await sim.navigate(".", testCaseName, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });


  test.concurrent.each(models)("soroban auth ($dev.name)", async ({ dev, startText, plugin_path }) => {
    const hashIdPreimage = testCasesFunction.sorobanAuthInvokeContractTestPlugin();
    const sim = new Zemu(dev.path, {
      "StellarTest": plugin_path
    });
    const testCaseName = `${dev.prefix.toLowerCase()}-plugin-soroban-auth`;
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const result = str.signSorobanAuthorization("44'/148'/0'", hashIdPreimage.toXDR("raw"));
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      const textToFind = dev.name.startsWith("nano") ? /\bSign\b/ : /\bHold to\b/;
      await sim.navigateAndCompareUntilText(
        ".",
        testCaseName,
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


  test.concurrent.each(models)("reject soroban auth ($dev.name)", async ({ dev, startText, plugin_path }) => {
    const hashIdPreimage = testCasesFunction.sorobanAuthInvokeContractTestPlugin();
    const testCaseName = `${dev.prefix.toLowerCase()}-plugin-soroban-auth-reject`;
    const sim = new Zemu(dev.path, {
      "StellarTest": plugin_path
    });
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      expect(() => str.signSorobanAuthorization("44'/148'/0'", hashIdPreimage.toXDR("raw"))).rejects.toThrow(StellarUserRefusedError);

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      const textToFind = dev.name.startsWith("nano") ? "Reject" : "Sign Soroban Auth?";
      await sim.navigateAndCompareUntilText(
        ".",
        testCaseName,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax" || dev.name == "flex") {
        const settingNav = new TouchNavigation(dev.name, [ButtonKind.ApproveTapButton]);
        await sim.navigate(".", testCaseName, settingNav.schedule, true, false);
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

async function enableSequence(sim: Zemu, device: TModel, testCaseName: string) {
  if (device == "stax" || device == "flex") {
    const settingNav = new TouchNavigation(device, [
      ButtonKind.InfoButton,
      ButtonKind.ToggleSettingButton1,
    ]);
    await sim.navigate(".", testCaseName, settingNav.schedule, true, false);
  } else {
    await sim.clickRight(undefined, true);
    await sim.clickBoth(undefined, true);
    await sim.clickBoth(undefined, true);
  }
}

async function acceptRisk(sim: Zemu, device: TModel, testCaseName: string) {
  if (device == "stax" || device == "flex") {
    const acceptRisk = new TouchNavigation(device, [
      ButtonKind.ConfirmNoButton,
      ButtonKind.ConfirmYesButton,
    ]);
    await sim.navigate(".", testCaseName, acceptRisk.schedule, true, false);
  } else {
    await sim.clickRight(undefined, true);
    await sim.clickRight(undefined, true);
    await sim.clickRight(undefined, true);
    await sim.clickRight(undefined, true);
    await sim.clickBoth(undefined, true);
  }
}

async function refuseRisk(sim: Zemu, device: TModel, testCaseName: string) {
  if (device == "stax" || device == "flex") {
    const acceptRisk = new TouchNavigation(device, [
      ButtonKind.ConfirmNoButton,
      ButtonKind.ConfirmNoButton,
    ]);
    await sim.navigate(".", testCaseName, acceptRisk.schedule, true, false);
  } else {
    await sim.clickRight(undefined, true);
    await sim.clickRight(undefined, true);
    await sim.clickRight(undefined, true);
    await sim.clickRight(undefined, true);
    await sim.clickRight(undefined, true);
    await sim.clickBoth(undefined, true);
  }
}
