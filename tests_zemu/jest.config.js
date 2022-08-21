// For a detailed explanation regarding each configuration property, visit:
// https://jestjs.io/docs/en/configuration.html

module.exports = {
  preset: 'ts-jest',
  testEnvironment: 'node',
  transformIgnorePatterns: ['^.+\\.js$'],
  // globalSetup: "<rootDir>/tests/globalsetup.ts",
  // Stop immediatly when a test fail
  bail: false,
};