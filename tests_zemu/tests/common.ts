import { IDeviceModel } from "@zondax/zemu";

const Resolve = require("path").resolve;

export const APP_SEED = "other base behind follow wet put glad muscle unlock sell income october";

const APP_PATH_X = Resolve("../build/nanox/bin/app.elf");
const APP_PATH_SP = Resolve("../build/nanos2/bin/app.elf");
const APP_PATH_STAX = Resolve("../build/stax/bin/app.elf");
const APP_PATH_FLEX = Resolve("../build/flex/bin/app.elf");

const PLUGIN_PATH_X = Resolve("../build/nanox/bin/plugin.elf");
const PLUGIN_PATH_SP = Resolve("../build/nanos2/bin/plugin.elf");
const PLUGIN_PATH_STAX = Resolve("../build/stax/bin/plugin.elf");
const PLUGIN_PATH_FLEX = Resolve("../build/flex/bin/plugin.elf");

const NANO_START_TEXT = "is ready";
const STAX_FLEX_START_TEXT = "This app enables";

export const models: { dev: IDeviceModel; startText: string, plugin_path: string }[] = [
  { dev: { name: "stax", prefix: "stax", path: APP_PATH_STAX }, startText: STAX_FLEX_START_TEXT, plugin_path: PLUGIN_PATH_STAX },
  { dev: { name: "flex", prefix: "flex", path: APP_PATH_FLEX }, startText: STAX_FLEX_START_TEXT, plugin_path: PLUGIN_PATH_FLEX },
  { dev: { name: "nanox", prefix: "X", path: APP_PATH_X }, startText: NANO_START_TEXT, plugin_path: PLUGIN_PATH_X },
  { dev: { name: "nanosp", prefix: "SP", path: APP_PATH_SP }, startText: NANO_START_TEXT, plugin_path: PLUGIN_PATH_SP },
];