import Zemu from "@zondax/zemu";
import axios from "axios";

function getRandomInt(min: number, max: number): number {
  min = Math.ceil(min);
  max = Math.floor(max);
  return Math.floor(Math.random() * (max - min + 1)) + min;
}

export default class StellarZemu extends Zemu {
  private stellarSpeculosApiPort?: number;

  constructor(
    elfPath: string,
    libElfs: { [key: string]: string } = {},
    host: string = "127.0.0.1",
    desiredTransportPort?: number
  ) {
    const desiredSpeculosApiPort = getRandomInt(3000, 65535);  // TODO: Check if it has been used
    super(elfPath, libElfs, host, desiredTransportPort, desiredSpeculosApiPort);
    this.stellarSpeculosApiPort = desiredSpeculosApiPort;
  }

  async click(
    endpoint: string,
    filename?: string,
    waitForScreenUpdate?: boolean,
    waitBeforeSnapshot?: number
  ) {
    let previousScreen;
    if (waitForScreenUpdate) {
      previousScreen = await this.snapshot();
    }
    const bothClickUrl =
      "http://localhost:" + this.stellarSpeculosApiPort?.toString() + endpoint;
    const payload = { action: "press-and-release" };
    await axios.post(bothClickUrl, payload);
    this.log(`Click ${endpoint} -> ${filename}`);

    // Wait and poll Speculos until the application screen gets updated
    if (waitForScreenUpdate) {
      let watchdog = 5000;
      let currentScreen = await this.snapshot();
      while (currentScreen.data.equals(previousScreen.data)) {
        this.log("sleep");
        await Zemu.delay(100);
        watchdog -= 100;
        if (watchdog <= 0) throw "Timeout waiting for screen update";
        currentScreen = await this.snapshot();
      }
    } else {
      // A minimum delay is required
      Zemu.delay(100)
    }
    // In GitHub Action, it is often encountered that the screen is only refreshed halfway 
    // before being snapshotted, which will cause the test to fail, so we made this change, 
    // which can greatly reduce the occurrence of this situation.
    if (waitBeforeSnapshot) {
      await Zemu.delay(waitBeforeSnapshot);
    }
    return this.snapshot(filename);
  }

  async clickBoth(filename?: string, waitForScreenUpdate = true, waitBeforeSnapshot = 1000) {
    return this.click('/button/both', filename, waitForScreenUpdate, waitBeforeSnapshot);
  }
}
