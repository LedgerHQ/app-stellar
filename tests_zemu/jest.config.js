// For a detailed explanation regarding each configuration property, visit:
// https://jestjs.io/docs/en/configuration.html

module.exports = {
  preset: 'ts-jest',
  testEnvironment: 'node',
  transformIgnorePatterns: ['^.+\\.js$', '/node_modules/(?!axios/)'],
  moduleNameMapper: {
    '^axios$': require.resolve('axios'), // https://github.com/axios/axios/issues/5101
  },
  // globalSetup: "<rootDir>/tests/globalsetup.ts",
  // Stop immediatly when a test fail
  bail: false,
};