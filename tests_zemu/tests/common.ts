import { DeviceModel } from '@zondax/zemu'

const Resolve = require('path').resolve

export const APP_SEED = 'other base behind follow wet put glad muscle unlock sell income october'

const APP_PATH_S = Resolve('./elfs/stellar_nanos.elf')
const APP_PATH_X = Resolve('./elfs/stellar_nanox.elf')
const APP_PATH_SP = Resolve('./elfs/stellar_nanosp.elf')

export const models: DeviceModel[] = [
  { name: 'nanos', prefix: 'S', path: APP_PATH_S },
  { name: 'nanox', prefix: 'X', path: APP_PATH_X },
  { name: 'nanosp', prefix: 'SP', path: APP_PATH_SP },
]
