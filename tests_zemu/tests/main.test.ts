import { DEFAULT_START_OPTIONS } from "@zondax/zemu";
import { APP_SEED, models } from "./common";
import * as testCasesFunction from 'tests-common'
import { Keypair } from 'stellar-base'
import Str from '@ledgerhq/hw-app-str'
import Zemu from './zemu'

beforeAll(async () => {
  await Zemu.checkAndPullImage();
});

jest.setTimeout(1000 * 60 * 60);

const defaultOptions = {
  ...DEFAULT_START_OPTIONS,
  logging: true,
  custom: `-s "${APP_SEED}"`,
  X11: false,
  startText: "is ready",
};

test.each(models)("can start and stop container ($name)", async (m) => {
  const sim = new Zemu(m.path);
  try {
    await sim.start({ ...defaultOptions, model: m.name });
  } finally {
    await sim.close();
  }
});

test.each(models)("app version ($name)", async (m) => {
  const sim = new Zemu(m.path);
  try {
    await sim.start({ ...defaultOptions, model: m.name });
    const transport = await sim.getTransport();
    const str = new Str(transport);
    const result = await str.getAppConfiguration();
    expect(result.version).toBe('4.0.0');
  } finally {
    await sim.close();
  }
});

describe('get public key', () => {
  test.each(models)("get public key without confirmation ($name)", async (m) => {
    const sim = new Zemu(m.path);
    try {
      await sim.start({ ...defaultOptions, model: m.name });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const result = await str.getPublicKey("44'/148'/0'", false, false);
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK")

      expect(result).toStrictEqual({
        publicKey: kp.publicKey(),
        raw: kp.rawPublicKey()
      })
    } finally {
      await sim.close();
    }
  });

  test.each(models)("get public key with confirmation - approve ($name)", async (m) => {
    const sim = new Zemu(m.path);
    try {
      await sim.start({ ...defaultOptions, model: m.name });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const result = str.getPublicKey("44'/148'/0'", false, true);
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK")

      await sim.waitScreenChange()
      await sim.navigateAndCompareUntilText(".", `${m.prefix.toLowerCase()}-public-key-approve`, 'Approve')

      expect(result).resolves.toStrictEqual({
        publicKey: kp.publicKey(),
        raw: kp.rawPublicKey()
      })
    } finally {
      await sim.close();
    }
  });

  test.each(models)("get public key with confirmation - reject ($name)", async (m) => {
    const sim = new Zemu(m.path);
    try {
      await sim.start({ ...defaultOptions, model: m.name });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      // TODO: Maybe we should throw a more specific exception in @ledgerhq/hw-app-str
      expect(() => str.getPublicKey("44'/148'/0'", false, true)).rejects.toThrow("Ledger device: Condition of use not satisfied (denied by the user?) (0x6985)");

      await sim.waitScreenChange()
      await sim.navigateAndCompareUntilText(".", `${m.prefix.toLowerCase()}-public-key-reject`, 'Reject')
    } finally {
      await sim.close();
    }
  });
})

describe('hash signing', () => {
  test.each(models)("hash signing mode is not enabled ($name)", async (m) => {
    const sim = new Zemu(m.path);
    try {
      await sim.start({ ...defaultOptions, model: m.name });
      const transport = await sim.getTransport();
      const str = new Str(transport);
      const hash = Buffer.from("3389e9f0f1a65f19736cacf544c2e825313e8447f569233bb8db39aa607c8889", "hex")
      expect(() => str.signHash("44'/148'/0'", hash)).rejects.toThrow(new Error("Hash signing not allowed. Have you enabled it in the app settings?"));
    } finally {
      await sim.close();
    }
  });

  test.each(models)("approve ($name)", async (m) => {
    const sim = new Zemu(m.path);
    try {
      await sim.start({ ...defaultOptions, model: m.name });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      // enable hash signing
      await sim.clickRight()
      await sim.clickBoth(undefined, false, 0)
      await sim.clickBoth(undefined, false, 0)

      const hash = Buffer.from("3389e9f0f1a65f19736cacf544c2e825313e8447f569233bb8db39aa607c8889", "hex")
      const result = str.signHash("44'/148'/0'", hash)
      await sim.waitScreenChange()
      await sim.navigateAndCompareUntilText(".", `${m.prefix.toLowerCase()}-hash-signing-approve`, 'Approve')
      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK")
      expect((await result).signature).toStrictEqual(kp.sign(hash));
    } finally {
      await sim.close();
    }
  });

  test.each(models)("reject ($name)", async (m) => {
    const sim = new Zemu(m.path);
    try {
      await sim.start({ ...defaultOptions, model: m.name });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      // enable hash signing
      await sim.clickRight()
      await sim.clickBoth(undefined, false, 0)
      await sim.clickBoth(undefined, false, 0)

      const hash = Buffer.from("3389e9f0f1a65f19736cacf544c2e825313e8447f569233bb8db39aa607c8889", "hex")
      expect(() => str.signHash("44'/148'/0'", hash)).rejects.toThrow(new Error("Transaction approval request was rejected"));

      await sim.waitScreenChange()
      await sim.navigateAndCompareUntilText(".", `${m.prefix.toLowerCase()}-hash-signing-reject`, 'Reject')
    } finally {
      await sim.close();
    }
  });
})

describe('transactions', () => {
  describe.each(getTestCases())('$caseName', (c) => {
    test.each(models)("device ($name)", async (m) => {
      const tx = c.txFunction();
      const sim = new Zemu(m.path);
      try {
        await sim.start({ ...defaultOptions, model: m.name });
        const transport = await sim.getTransport();
        const str = new Str(transport);

        // display sequence
        await sim.clickRight()
        await sim.clickBoth(undefined, false, 0)
        await sim.clickRight()
        await sim.clickBoth(undefined, false, 0)

        const result = str.signTransaction("44'/148'/0'", tx.signatureBase())
        await sim.waitScreenChange(1000 * 60 * 60)
        await sim.navigateAndCompareUntilText(".", `${m.prefix.toLowerCase()}-${c.filePath}`, 'Finalize', true, undefined, 1000 * 60 * 60)
        const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK")
        tx.sign(kp)
        expect((await result).signature).toStrictEqual(tx.signatures[0].signature());
      } finally {
        await sim.close();
      }
    });
  })

  test.each(models)("reject tx ($name)", async (m) => {
    const tx = testCasesFunction.txNetworkPublic()
    const sim = new Zemu(m.path);
    try {
      await sim.start({ ...defaultOptions, model: m.name });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      // display sequence
      await sim.clickRight()
      await sim.clickBoth(undefined, false, 0)
      await sim.clickRight()
      await sim.clickBoth(undefined, false, 0)

      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(new Error("Transaction approval request was rejected"));

      await sim.waitScreenChange(1000 * 60 * 60)
      await sim.navigateAndCompareUntilText(".", `${m.prefix.toLowerCase()}-tx-reject`, 'Cancel', true, undefined, 1000 * 60 * 60)
    } finally {
      await sim.close();
    }
  })

  test.each(models)("reject fee bump tx ($name)", async (m) => {
    const tx = testCasesFunction.feeBumpTx()
    const sim = new Zemu(m.path);
    try {
      await sim.start({ ...defaultOptions, model: m.name });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      // display sequence
      await sim.clickRight()
      await sim.clickBoth(undefined, false, 0)
      await sim.clickRight()
      await sim.clickBoth(undefined, false, 0)

      expect(() => str.signTransaction("44'/148'/0'", tx.signatureBase())).rejects.toThrow(new Error("Transaction approval request was rejected"));

      await sim.waitScreenChange(1000 * 60 * 60)
      await sim.navigateAndCompareUntilText(".", `${m.prefix.toLowerCase()}-fee-bump-tx-reject`, 'Cancel', true, undefined, 1000 * 60 * 60)
    } finally {
      await sim.close();
    }
  })

  test.each(models)("hide sequence tx ($name)", async (m) => {
    const tx = testCasesFunction.txNetworkPublic()
    const sim = new Zemu(m.path);
    try {
      await sim.start({ ...defaultOptions, model: m.name });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      const result = str.signTransaction("44'/148'/0'", tx.signatureBase())
      await sim.waitScreenChange(1000 * 60 * 60)
      await sim.navigateAndCompareUntilText(".", `${m.prefix.toLowerCase()}-tx-hide-sequence`, 'Finalize', true, undefined, 1000 * 60 * 60)

      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK")
      tx.sign(kp)
      expect((await result).signature).toStrictEqual(tx.signatures[0].signature());
    } finally {
      await sim.close();
    }
  })

  test.each(models)("hide sequence fee bump tx ($name)", async (m) => {
    const tx = testCasesFunction.feeBumpTx()
    const sim = new Zemu(m.path);
    try {
      await sim.start({ ...defaultOptions, model: m.name });
      const transport = await sim.getTransport();
      const str = new Str(transport);

      const result = str.signTransaction("44'/148'/0'", tx.signatureBase())
      await sim.waitScreenChange(1000 * 60 * 60)
      await sim.navigateAndCompareUntilText(".", `${m.prefix.toLowerCase()}-fee-bump-tx-hide-sequence`, 'Finalize', true, undefined, 1000 * 60 * 60)

      const kp = Keypair.fromSecret("SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK")
      tx.sign(kp)
      expect((await result).signature).toStrictEqual(tx.signatures[0].signature());
    } finally {
      await sim.close();
    }
  })
})

function camelToFilePath(str: string) {
  return str.replace(/([A-Z])/g, '-$1').toLowerCase();
}

function getTestCases() {
  const casesFunction = Object.keys(testCasesFunction);
  const cases = []
  for (const rawCase of casesFunction) {
    cases.push({
      caseName: rawCase,
      filePath: camelToFilePath(rawCase),
      txFunction: (testCasesFunction as any)[rawCase]  // dirty hack
    });
  }
  return cases;
}
