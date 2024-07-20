import { DEFAULT_START_OPTIONS, ButtonKind, TouchNavigation, INavElement } from "@zondax/zemu";
import { APP_SEED, models } from "./common";
import * as testCasesFunction from "tests-common";
import { Keypair, StrKey } from "@stellar/stellar-base";
import Str from "@ledgerhq/hw-app-str";
import { StellarUserRefusedError, StellarHashSigningNotEnabledError } from "@ledgerhq/hw-app-str";
import Zemu from "@zondax/zemu";
import { sha256 } from 'sha.js'
import { ActionKind } from "@zondax/zemu/dist/types";


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
    try {
      const confirmText = dev.name.startsWith("nano") ? "Approve" : "Confirm";
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

  test.concurrent.each(models)("get public key with confirmation - reject ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    try {
      const confirmText = dev.name.startsWith("nano") ? "Reject" : "Confirm";
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
  test.concurrent.each(models)("hash signing mode is not enabled ($dev.name)", async ({ dev, startText }) => {
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

  test.concurrent.each(models)("approve ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      // enable hash signing
      if (dev.name == "stax") {
        const settingNav = [
          ...new TouchNavigation("stax", [ButtonKind.InfoButton]).schedule,
          StaxSettingToggleHashSigning,
          ...new TouchNavigation("stax", [ButtonKind.ConfirmYesButton]).schedule
        ]
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-hash-signing-approve`, settingNav, true, false);
      } else if (dev.name == "flex") {
        const settingNav = [
          ...new TouchNavigation("flex", [ButtonKind.InfoButton]).schedule,
          FlexSettingToggleHashSigning,
          ...new TouchNavigation("flex", [ButtonKind.ConfirmYesButton]).schedule
        ]
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-hash-signing-approve`, settingNav, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
      }

      const hash = Buffer.from("3389e9f0f1a65f19736cacf544c2e825313e8447f569233bb8db39aa607c8889", "hex");
      const result = str.signHash("44'/148'/0'", hash);
      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      // accept risk
      if (dev.name == "stax" || dev.name == "flex") {
        const acceptRisk = new TouchNavigation(dev.name, [
          ButtonKind.ConfirmNoButton,
          ButtonKind.ConfirmYesButton,
        ]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-hash-signing-approve`, acceptRisk.schedule, true, false);
      }
      const textToFind = dev.name.startsWith("nano") ? "Sign Hash" : "Hold to";
      await sim.navigateAndCompareUntilText(".", `${dev.prefix.toLowerCase()}-hash-signing-approve`, textToFind, true);
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK");
      expect((await result).signature).toStrictEqual(kp.sign(hash));
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)("reject ($dev.name)", async ({ dev, startText }) => {
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      // enable hash signing
      if (dev.name == "stax") {
        const settingNav = [
          ...new TouchNavigation("stax", [ButtonKind.InfoButton]).schedule,
          StaxSettingToggleHashSigning,
          ...new TouchNavigation("stax", [ButtonKind.ConfirmYesButton]).schedule
        ]
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-hash-signing-approve`, settingNav, true, false);
      } else if (dev.name == "flex") {
        const settingNav = [
          ...new TouchNavigation("flex", [ButtonKind.InfoButton]).schedule,
          FlexSettingToggleHashSigning,
          ...new TouchNavigation("flex", [ButtonKind.ConfirmYesButton]).schedule
        ]
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-hash-signing-approve`, settingNav, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
      }

      const hash = Buffer.from("3389e9f0f1a65f19736cacf544c2e825313e8447f569233bb8db39aa607c8889", "hex");
      expect(() => str.signHash("44'/148'/0'", hash)).rejects.toThrow(StellarUserRefusedError);

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);

      // accept risk
      if (dev.name == "stax" || dev.name == "flex") {
        const acceptRisk = new TouchNavigation(dev.name, [
          ButtonKind.ConfirmNoButton,
          ButtonKind.ConfirmYesButton,
        ]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-hash-signing-approve`, acceptRisk.schedule, true, false);
      }

      const textToFind = dev.name.startsWith("nano") ? "Reject" : "Hold to";
      await sim.navigateAndCompareUntilText(".", `${dev.prefix.toLowerCase()}-hash-signing-reject`, textToFind, true);
      if (dev.name == "stax" || dev.name == "flex") {
        const settingNav = new TouchNavigation(dev.name, [ButtonKind.ApproveTapButton]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-hash-signing-reject`, settingNav.schedule, true, false);
      }
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
      try {
        await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
        const transport = await sim.getTransport();
        const str = new Str(transport);

        // enable custom contracts and seqence number
        if (dev.name == "stax") {
          const settingNav = [
            ...new TouchNavigation("stax", [ButtonKind.InfoButton]).schedule,
            StaxSettingToggleCustomContracts,
            ...new TouchNavigation("stax", [ButtonKind.ConfirmYesButton]).schedule,
            StaxSettingToggleSequence,
          ];
          await sim.navigate(".", `${dev.prefix.toLowerCase()}-${c.filePath}`, settingNav, true, false);
        } else if (dev.name == "flex") {
          const settingNav = [
            ...new TouchNavigation("flex", [ButtonKind.InfoButton]).schedule,
            FlexSettingToggleCustomContracts,
            ...new TouchNavigation("flex", [ButtonKind.ConfirmYesButton, ButtonKind.SettingsNavRightButton]).schedule,
            FlexSettingToggleSequence,
          ];
          await sim.navigate(".", `${dev.prefix.toLowerCase()}-${c.filePath}`, settingNav, true, false);
        } else {
          await sim.clickRight();
          await sim.clickBoth(undefined, false);
          await sim.clickBoth(undefined, false);
          await sim.clickRight();
          await sim.clickRight();
          await sim.clickBoth(undefined, false);
        }

        const result = str.signTransaction("44'/148'/0'", tx.signatureBase());
        const events = await sim.getEvents();
        await sim.waitForScreenChanges(events);

        // accept risk
        if (dev.name == "stax" || dev.name == "flex") {
          if (c.caseName.includes("InvokeHostFunction")
            && !c.caseName.includes("Create")
            && !c.caseName.includes("Upload")
            && !c.caseName.includes("Xlm")
            && !c.caseName.includes("Usdc")
          ) {
            const acceptRisk = new TouchNavigation(dev.name, [
              ButtonKind.ConfirmNoButton,
              ButtonKind.ConfirmYesButton,
            ]);
            await sim.navigate(".", `${dev.prefix.toLowerCase()}-${c.filePath}`, acceptRisk.schedule, true, false);
          }
        }

        // TODO: If set to Sign, it will not pass the test. Is this a bug in Zemu?
        const textToFind = dev.name.startsWith("nano") ? /\bSign\b/ : /\bHold to\b/;
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

  test.concurrent.each(models)("reject tx ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.txNetworkPublic();
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      // enable custom contracts and seqence number
      if (dev.name == "stax") {
        const settingNav = [
          ...new TouchNavigation("stax", [ButtonKind.InfoButton]).schedule,
          StaxSettingToggleCustomContracts,
          ...new TouchNavigation("stax", [ButtonKind.ConfirmYesButton]).schedule,
          StaxSettingToggleSequence,
        ];
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-tx-reject`, settingNav, true, false);
      } else if (dev.name == "flex") {
        const settingNav = [
          ...new TouchNavigation("flex", [ButtonKind.InfoButton]).schedule,
          FlexSettingToggleCustomContracts,
          ...new TouchNavigation("flex", [ButtonKind.ConfirmYesButton, ButtonKind.SettingsNavRightButton]).schedule,
          FlexSettingToggleSequence,
        ];
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-tx-reject`, settingNav, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickBoth(undefined, false);
        await sim.clickRight();
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
      }

      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(StellarUserRefusedError);

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      const textToFind = dev.name.startsWith("nano") ? "Reject" : "Sign transaction?";
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-tx-reject`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax" || dev.name == "flex") {
        const settingNav = new TouchNavigation(dev.name, [ButtonKind.ApproveTapButton]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-tx-reject`, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)("reject fee bump tx ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.feeBumpTx();
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      // enable custom contracts and seqence number
      if (dev.name == "stax") {
        const settingNav = [
          ...new TouchNavigation("stax", [ButtonKind.InfoButton]).schedule,
          StaxSettingToggleCustomContracts,
          ...new TouchNavigation("stax", [ButtonKind.ConfirmYesButton]).schedule,
          StaxSettingToggleSequence,
        ];
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-fee-bump-tx-reject`, settingNav, true, false);
      } else if (dev.name == "flex") {
        const settingNav = [
          ...new TouchNavigation("flex", [ButtonKind.InfoButton]).schedule,
          FlexSettingToggleCustomContracts,
          ...new TouchNavigation("flex", [ButtonKind.ConfirmYesButton, ButtonKind.SettingsNavRightButton]).schedule,
          FlexSettingToggleSequence,
        ];
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-fee-bump-tx-reject`, settingNav, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickBoth(undefined, false);
        await sim.clickRight();
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
      }

      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(StellarUserRefusedError);

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);
      const textToFind = dev.name.startsWith("nano") ? "Reject" : "Sign transaction?";
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-fee-bump-tx-reject`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax" || dev.name == "flex") {
        const settingNav = new TouchNavigation(dev.name, [ButtonKind.ApproveTapButton]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-fee-bump-tx-reject`, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)("hide sequence tx ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.txNetworkPublic();
    const sim = new Zemu(dev.path);
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

  test.concurrent.each(models)("hide sequence fee bump tx ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.feeBumpTx();
    const sim = new Zemu(dev.path);
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

  test.concurrent.each(models)("custom contracts mode is not enabled ($dev.name)", async ({ dev, startText }) => {
    const tx = testCasesFunction.opInvokeHostFunctionScvalsCase0();
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      // TODO: Waiting for SDK update.
      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow();
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
      try {
        await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
        const transport = await sim.getTransport();
        const str = new Str(transport);

        // enable custom contracts
        if (dev.name == "stax") {
          const settingNav = [
            ...new TouchNavigation("stax", [ButtonKind.InfoButton]).schedule,
            StaxSettingToggleCustomContracts,
            ...new TouchNavigation("stax", [ButtonKind.ConfirmYesButton]).schedule
          ];
          await sim.navigate(".", `${dev.prefix.toLowerCase()}-soroban-auth-reject`, settingNav, true, false);
        } else if (dev.name == "flex") {
          const settingNav = [
            ...new TouchNavigation("flex", [ButtonKind.InfoButton]).schedule,
            FlexSettingToggleCustomContracts,
            ...new TouchNavigation("flex", [ButtonKind.ConfirmYesButton]).schedule
          ];
          await sim.navigate(".", `${dev.prefix.toLowerCase()}-soroban-auth-reject`, settingNav, true, false);
        } else {
          await sim.clickRight();
          await sim.clickBoth(undefined, false);
          await sim.clickBoth(undefined, false);
        }

        const result = str.signSorobanAuthorization("44'/148'/0'", hashIdPreimage.toXDR("raw"));
        const events = await sim.getEvents();
        await sim.waitForScreenChanges(events);
        if (!c.caseName.includes("Create") && (dev.name == "stax" || dev.name == "flex")) {
          const acceptRisk = new TouchNavigation(dev.name, [
            ButtonKind.ConfirmNoButton,
            ButtonKind.ConfirmYesButton,
          ]);
          await sim.navigate(".", `${dev.prefix.toLowerCase()}-${c.filePath}`, acceptRisk.schedule, true, false);
        }
        const textToFind = dev.name.startsWith("nano") ? /\bSign\b/ : /\bHold to\b/;
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

  test.concurrent.each(models)("reject soroban auth ($dev.name)", async ({ dev, startText }) => {
    const hashIdPreimage = testCasesFunction.sorobanAuthInvokeContract();
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText, approveAction: ButtonKind.RejectButton });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      // enable custom contracts
      if (dev.name == "stax") {
        const settingNav = [
          ...new TouchNavigation("stax", [ButtonKind.InfoButton]).schedule,
          StaxSettingToggleCustomContracts,
          ...new TouchNavigation("stax", [ButtonKind.ConfirmYesButton]).schedule
        ];
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-soroban-auth-reject`, settingNav, true, false);
      } else if (dev.name == "flex") {
        const settingNav = [
          ...new TouchNavigation("flex", [ButtonKind.InfoButton]).schedule,
          FlexSettingToggleCustomContracts,
          ...new TouchNavigation("flex", [ButtonKind.ConfirmYesButton]).schedule
        ];
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-soroban-auth-reject`, settingNav, true, false);
      } else {
        await sim.clickRight();
        await sim.clickBoth(undefined, false);
        await sim.clickBoth(undefined, false);
      }

      expect(() => str.signSorobanAuthorization("44'/148'/0'", hashIdPreimage.toXDR("raw"))).rejects.toThrow(StellarUserRefusedError);

      const events = await sim.getEvents();
      await sim.waitForScreenChanges(events);

      if (dev.name == "stax" || dev.name == "flex") {
        const acceptRisk = new TouchNavigation(dev.name, [
          ButtonKind.ConfirmNoButton,
          ButtonKind.ConfirmYesButton,
        ]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-soroban-auth-reject`, acceptRisk.schedule, true, false);
      }
      const textToFind = dev.name.startsWith("nano") ? "Reject" : "Sign Soroban Auth?";
      await sim.navigateAndCompareUntilText(
        ".",
        `${dev.prefix.toLowerCase()}-soroban-auth-reject`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax" || dev.name == "flex") {
        const settingNav = new TouchNavigation(dev.name, [ButtonKind.ApproveTapButton]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-soroban-auth-reject`, settingNav.schedule, true, false);
      }
    } finally {
      await sim.close();
    }
  });

  test.concurrent.each(models)("custom contracts mode is not enabled ($dev.name)", async ({ dev, startText }) => {
    const hashIdPreimage = testCasesFunction.sorobanAuthInvokeContract();
    const sim = new Zemu(dev.path);
    try {
      await sim.start({ ...defaultOptions, model: dev.name, startText: startText });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      // TODO: Waiting for SDK update.
      expect(() => str.signSorobanAuthorization("44'/148'/0'", hashIdPreimage.toXDR("raw"))).rejects.toThrow();
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

  test.concurrent.each(models)("reject tx ($dev.name)", async ({ dev, startText, plugin_path }) => {
    const tx = testCasesFunction.opInvokeHostFunctionTestPlugin();
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
        `${dev.prefix.toLowerCase()}-plugin-invoke-host-function-reject`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax" || dev.name == "flex") {
        const settingNav = new TouchNavigation(dev.name, [ButtonKind.ApproveTapButton]);
        await sim.navigate(".", `${dev.prefix.toLowerCase()}-plugin-invoke-host-function-reject`, settingNav.schedule, true, false);
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


  test.concurrent.each(models)("reject soroban auth ($dev.name)", async ({ dev, startText, plugin_path }) => {
    const hashIdPreimage = testCasesFunction.sorobanAuthInvokeContractTestPlugin();
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
        `${dev.prefix.toLowerCase()}-plugin-soroban-auth-reject`,
        textToFind,
        true,
        undefined,
        1000 * 60 * 60
      );
      if (dev.name == "stax" || dev.name == "flex") {
        const settingNav = new TouchNavigation(dev.name, [ButtonKind.ApproveTapButton]);
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

const StaxSettingToggleCustomContracts: INavElement = {
  type: ActionKind.Touch,
  button: {
    x: 350,
    y: 115,
    delay: 0.25,
    direction: 0
  }
}

const StaxSettingToggleHashSigning: INavElement = {
  type: ActionKind.Touch,
  button: {
    x: 350,
    y: 254,
    delay: 0.25,
    direction: 0
  }
}

const StaxSettingToggleSequence: INavElement = {
  type: ActionKind.Touch,
  button: {
    x: 350,
    y: 394,
    delay: 0.25,
    direction: 0
  }
}

const FlexSettingToggleCustomContracts: INavElement = {
  type: ActionKind.Touch,
  button: {
    x: 380,
    y: 123,
    delay: 0.25,
    direction: 0
  }
}

const FlexSettingToggleHashSigning: INavElement = {
  type: ActionKind.Touch,
  button: {
    x: 380,
    y: 263,
    delay: 0.25,
    direction: 0
  }
}

const FlexSettingToggleSequence: INavElement = {
  type: ActionKind.Touch,
  button: {
    x: 380,
    y: 123,
    delay: 0.25,
    direction: 0
  }
}
