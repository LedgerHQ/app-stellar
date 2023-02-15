import { IDeviceModel } from '@zondax/zemu'

const Resolve = require('path').resolve

export const APP_SEED = 'other base behind follow wet put glad muscle unlock sell income october'

const APP_PATH_S = Resolve('./elfs/stellar_nanos.elf')
const APP_PATH_X = Resolve('./elfs/stellar_nanox.elf')
const APP_PATH_SP = Resolve('./elfs/stellar_nanosp.elf')
const APP_PATH_STAX = Resolve('./elfs/stellar_stax.elf')

const NANO_START_TEXT = "is ready"
const STAX_START_TEXT = "Go to Ledger Live"

export const models: {dev:IDeviceModel,startText:string}[] = [
    {dev:{ name : 'stax', prefix: 'stax' , path: APP_PATH_STAX}, startText: STAX_START_TEXT},
    {dev:{ name : 'nanos', prefix: 'S' , path: APP_PATH_S}, startText: NANO_START_TEXT},
    {dev:{ name : 'nanox', prefix: 'X' , path: APP_PATH_X}, startText: NANO_START_TEXT},
    {dev:{ name : 'nanosp', prefix: 'SP' , path: APP_PATH_SP}, startText: NANO_START_TEXT}
]
