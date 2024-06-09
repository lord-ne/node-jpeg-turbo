const { readDCT, readDCTSync } = require("..");
const { readFileSync } = require("fs");
const path = require("path");
const zlib = require('node:zlib');

const sampleJpeg1 = readFileSync(path.join(__dirname, "github_logo.jpg"));

// DCT components extracted using an external library
const expectedJpeg1 = JSON.parse(zlib.unzipSync(readFileSync(path.join(__dirname, "github_logo.json.gz"))))

const arr_eq = (nd_arr, nested_arr, index_location = []) => {
  if (!Array.isArray(nested_arr)) {
    return nd_arr == nested_arr
  }
  for (let i = 0; i < nested_arr.length; i++) {
    let new_index_location = index_location.concat([i])

    if (!Array.isArray(nested_arr[i])) {
      rtn = nd_arr.get(i) == nested_arr[i]
      if (!rtn) {
        console.log("Element is not equal at index ("
          + new_index_location.toString()
          + "). Left value was " + nd_arr.get(i)
          + ", right value was " + nested_arr[i])
      }
      return rtn;
    }

    if (!arr_eq(nd_arr.pick(i), nested_arr[i], new_index_location)) {
      return false
    }
  }
  return true
}

const assert_output_is_expected = (out, expected) => {
  expect(arr_eq(out.Y.data, expected.Y.data)).toBeTruthy()
  expect(out.Y.qt_no).toEqual(expected.Y.qt_no)

  expect(arr_eq(out.Cb.data, expected.Cb.data)).toBeTruthy()
  expect(out.Cb.qt_no).toEqual(expected.Cb.qt_no)

  expect(arr_eq(out.Cr.data, expected.Cr.data)).toBeTruthy()
  expect(out.Cr.qt_no).toEqual(expected.Cr.qt_no)

  if (Object.hasOwnProperty(out, "K")) {
    expect(arr_eq(out.K.data, expected.K.data)).toBeTruthy()
    expect(out.K.qt_no).toEqual(expected.K.qt_no)
  }

  expect(out.qts.length).toEqual(expected.qts.length)
  for (let i = 0; i < out.qts.length; i++ ) {
    expect(arr_eq(out.qts[i], expected.qts[i])).toBeTruthy()
  }
}

describe("read_dct", () => {

  test("test sync nominal", () => {
    out = readDCTSync(sampleJpeg1)
    assert_output_is_expected(out, expectedJpeg1)
  });

  test("test async nominal", async () => {
    out = await readDCT(sampleJpeg1)
    assert_output_is_expected(out, expectedJpeg1)
  });

  test("check readDCTSync dest buffer length", () => {
    // Needed space for this three 70x70x8x8 int16 arrays of components,
    // and up to four 8x8 uint16 quantization tables
    const neededSpace = (3*70*70*8*8*2)+(4*8*8*2)

    assert_output_is_expected(readDCTSync(sampleJpeg1, Buffer.alloc(9999999)), expectedJpeg1)
    assert_output_is_expected(readDCTSync(sampleJpeg1, Buffer.alloc(neededSpace)), expectedJpeg1)

    expect(() => readDCTSync(sampleJpeg1, Buffer.alloc(0))).toThrow('Insufficient output buffer');
    expect(() => readDCTSync(sampleJpeg1, Buffer.alloc(10))).toThrow('Insufficient output buffer');
    expect(() => readDCTSync(sampleJpeg1, Buffer.alloc(1000))).toThrow('Insufficient output buffer');
    expect(() => readDCTSync(sampleJpeg1, Buffer.alloc(neededSpace - 1))).toThrow('Insufficient output buffer');
  });
});
