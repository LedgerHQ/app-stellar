const path = require("path");
const fs = require("fs");
const testCasesFunction = require('tests-common')

function getTestCases() {
    const casesFunction = Object.keys(testCasesFunction);
    const cases = []
    for (const rawCase of casesFunction) {
        cases.push({
            caseName: rawCase,
            txFunction: testCasesFunction[rawCase]
        });
    }
    return cases;
}

function main() {
    const projectDir = path.dirname(path.dirname(__filename))
    const args = process.argv.slice(2);
    const type = args[0];
    console.log(`type: ${type}`);
    switch (type) {
        case "unit":
            dir = path.join(projectDir, "tests_unit", "testcases")
            break
        case "fuzz":
            dir = path.join(projectDir, "fuzz", "testcases")
            break
        default:
            throw new Error("Unknown type: " + type)
    }
    console.log("Generating test cases...");
    for (const testCase of getTestCases()) {
        const outputPath = path.join(dir, `${testCase.caseName}.raw`)
        const buf = testCase.txFunction().signatureBase()
        console.log(outputPath)
        fs.writeFile(outputPath, buf, (err) => {
            if (err) {
                console.log(`Failed to write to ${testCase.caseName}`)
                console.log(err)
                process.exit(1)
            }
        });
    }
    console.log("Finished.")
}
main()