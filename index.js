const binding = require("pkg-prebuilds")(
  __dirname,
  require("./binding-options")
);
const ndarray = require("ndarray");

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

// Convenience wrapper for converting to an ndarry.
module.exports.readDCTSync = function (a) {
  var out = binding.readDCTSync(a);
  for (let str of ["Y", "Cb", "Cr", "K"]) {
    if (str in out) {
      out[str] = {
        data: ndarray(out[str].data, [out[str].height, out[str].width, 8, 8]),
        qt_no: out[str].qt_no
      }
    }
  }

  out.qts = out.qts.map((tbl) => ndarray(tbl, [8, 8]));

  return out;
};
