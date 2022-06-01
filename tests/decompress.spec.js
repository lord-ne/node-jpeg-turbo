const { decompressSync, decompress, SAMP_444, FORMAT_BGR, FORMAT_BGRA, FORMAT_GRAY } = require("..");
const { readFileSync } = require("fs");
const path = require("path");


const sampleJpeg1 = readFileSync(path.join(__dirname, "github_logo.jpg"));
const sampleJpeg1Pixels = 560 * 560;

describe("decompress", () => {
  test("check decompressSync parameters", () => {
    const okOptions = {
      format: FORMAT_BGR
    };
    decompressSync(sampleJpeg1, okOptions);
    decompressSync(sampleJpeg1, Buffer.alloc(sampleJpeg1Pixels * 3), okOptions);

    expect(() => decompressSync()).toThrow('Invalid source buffer');
    expect(() => decompressSync(null, okOptions)).toThrow('Invalid source buffer');
    expect(() => decompressSync(undefined, okOptions)).toThrow('Invalid source buffer');
    expect(() => decompressSync({}, okOptions)).toThrow('Invalid source buffer');
  });

  test("check decompressSync options", () => {
    const okOptions = {
      format: FORMAT_BGR
    };
    expect(() =>
      decompressSync(sampleJpeg1, {})
    ).toThrow('Invalid format');
    decompressSync(sampleJpeg1, okOptions);

    // Format
    expect(() =>
      decompressSync(sampleJpeg1, {
        format: -1
      })
    ).toThrow('Invalid output format');
    expect(() =>
      decompressSync(sampleJpeg1, {
        format: 50
      })
    ).toThrow('Invalid output format');
  });

  test("check decompressSync dest buffer length", () => {
    const options = {
      format: FORMAT_BGRA
    };
    decompressSync(sampleJpeg1, Buffer.alloc(10000000), options);
    decompressSync(sampleJpeg1, Buffer.alloc(sampleJpeg1Pixels * 4), options);
    decompressSync(sampleJpeg1, Buffer.alloc(sampleJpeg1Pixels * 3), { format: FORMAT_BGR });

    expect(() => decompressSync(sampleJpeg1, Buffer.alloc(0), options)).toThrow('Invalid destination buffer');
    expect(() => decompressSync(sampleJpeg1, Buffer.alloc(10), options)).toThrow('Insufficient output buffer');
    expect(() => decompressSync(sampleJpeg1, Buffer.alloc(1000), options)).toThrow('Insufficient output buffer');
    expect(() => decompressSync(sampleJpeg1, Buffer.alloc(sampleJpeg1Pixels * 3), options)).toThrow('Insufficient output buffer');
  });

  test("check libjpeg errors throw", async () => {
    const source = Buffer.alloc(800);
    const options = {
      format: FORMAT_GRAY
    };
    expect(() => compressSync(source, options)).toThrow();
    expect(async () => await compress2(source, options).rejects).toBeTruthy();
  });

  test("check result length", async () => {
    const target = sampleJpeg1Pixels * 3;
    const dest = Buffer.alloc(target);
    const options = {
      width: 20,
      height: 10,
      format: FORMAT_BGR
    };
    const res1 = decompressSync(sampleJpeg1, dest, options);
    expect(res1.data.length).toEqual(target);
    expect(res1.width).toEqual(560);
    expect(res1.height).toEqual(560);
    expect(res1.format).toEqual(options.format);
    expect(res1.size).toEqual(target);

    const res2 = await decompress(sampleJpeg1, dest, options);
    expect(res2.data.length).toEqual(target);
    expect({...res2, data: null}).toEqual({...res1, data: null});
    expect(res2.data.toString('base64')).toEqual(res1.data.toString('base64'));

    const res3 = decompressSync(sampleJpeg1, options);
    expect(res3.data.length).toEqual(target);

    const res4 = await decompress(sampleJpeg1, options);
    expect(res4.data.length).toEqual(target);

    // Overfeed the dest buffer
    const res5 = decompressSync(sampleJpeg1, Buffer.alloc(target * 2), options);
    expect(res5.size).toEqual(target);
    expect(res5.data.length).toEqual(target);
  });
});
