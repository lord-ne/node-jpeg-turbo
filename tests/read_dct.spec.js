const { readDCTSync, SAMP_444, FORMAT_BGR, FORMAT_BGRA, FORMAT_GRAY } = require("..");
const { readFileSync } = require("fs");
const path = require("path");


const sampleJpeg1 = readFileSync(path.join(__dirname, "github_logo.jpg"));
const sampleJpeg1Pixels = 560 * 560;

describe("read_dct", () => {

  test("test nominal", () => {
    out = readDCTSync(sampleJpeg1)
    fail()
    expect(out).toEqual({})
  });
});
