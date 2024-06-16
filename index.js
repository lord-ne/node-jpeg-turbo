const binding = require("pkg-prebuilds")(
  __dirname,
  require("./binding-options")
);
const ndarray = require("ndarray");
const assert = require('node:assert/strict');

// Copy exports so that we can customize them on the JS side without
// overwriting the binding itself.
Object.keys(binding).forEach(function (key) {
  module.exports[key] = binding[key];
});

// Convenience wrapper for Buffer slicing.
module.exports.compressSync = function (buffer, optionalOutBuffer, options) {
  var out = binding.compressSync(buffer, optionalOutBuffer, options);
  return out.data.slice(0, out.size);
};

// Convenience wrapper for Buffer slicing.
module.exports.compress = function (a, b, c) {
  return binding.compress(a, b, c).then((out) => {
    return out.data.slice(0, out.size);
  });
};

// Convenience wrapper for Buffer slicing.
module.exports.decompressSync = function (buffer, optionalOutBuffer, options) {
  var out = binding.decompressSync(buffer, optionalOutBuffer, options);
  out.data = out.data.slice(0, out.size);
  return out;
};

// Convenience wrapper for Buffer slicing.
module.exports.decompress = function (a, b, c) {
  return binding.decompress(a, b, c).then((out) => {
    out.data = out.data.slice(0, out.size);
    return out;
  });
};

// Helper for converting the output of readDCT and readDCTSync
function readDCTOutputTransformer(initial) {
  var final = {};

  for (let str of ["Y", "Cb", "Cr", "K"]) {
    if (str in initial) {
      final[str] = {
        data: ndarray(
          new Int16Array(
            initial.buffer.buffer,
            initial[str].data_offset_bytes,
            initial[str].data_length_elements),
          [initial[str].height, initial[str].width, 8, 8]),
        qt_no: initial[str].qt_no
      }
    }
  }

  final.qts = initial.qts.map((qt) => {
    if (!qt) {
      return null;
    }

    return ndarray(
      new Uint16Array(
        initial.buffer.buffer,
        qt.data_offset_bytes,
        qt.data_length_elements),
      [8, 8])
  });

  return final;
}

// Convenience wrapper for extracting buffers.
module.exports.readDCTSync = function (a, b) {
  return readDCTOutputTransformer(binding.readDCTSync(a, b));
};

// Convenience wrapper for extracting buffers.
module.exports.readDCT = function (a, b) {
  return binding.readDCT(a, b).then(readDCTOutputTransformer);
};

// Helper for converting the input of writeDCT and writeDCTSync
function writeDCTInputTransformer(initial) {
  var final = {};

  for (let str of ["Y", "Cb", "Cr", "K"]) {
    if (str in initial) {
      let shape = initial[str].data.shape
      assert.equal(shape.length, 4, "Error: Passed in component arrays must have shape HxWx8x8")
      assert.equal(shape[2], 8, "Error: Passed in component arrays must have shape HxWx8x8")
      assert.equal(shape[3], 8, "Error: Passed in component arrays must have shape HxWx8x8")
      assert.equal(initial[str].data.stride, [shape[0] * shape[1] * 64, shape[0] * 64, 64, 8],
        "Error: Passed in component arrays must have C-order stride");

      final[str] = {
        data: initial[str].data.data,
        data_offset_elements: initial[str].data.offset,
        height: initial[str].data.shape[0],
        width: initial[str].data.shape[1],
        qt_no: initial[str].qt_no
      }
    }
  }

  final.qts = initial.qts.map((qt) => {
    if (!qt) {
      return null
    }

    assert.equal(qt.shape, [8, 8], "Error: Passed in qt arrays must have shape 8x8")
    assert.equal(initial[str].data.stride, [64, 8], "Error: Passed in qt arrays must have C-order stride")
    return {
      data: qt.data,
      data_offset_elements: qt.offset,
    }
  });

  return final;
}

// Convenience wrapper for extracting buffers.
module.exports.writeDCTSync = function (a, b, c) {
  return binding.writeDCTSync(a, writeDCTInputTransformer(b), c);
};

// Convenience wrapper for extracting buffers.
module.exports.writeDCT = function (a, b, c) {
  return binding.writeDCT(a, writeDCTInputTransformer(b), c);
};

