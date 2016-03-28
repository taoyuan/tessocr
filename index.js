'use strict';

var bindings = require('node-cmake')('tessocr');

function noop() {
}

function Tessocr() {
  if (!(this instanceof Tessocr)) {
    return new Tessocr();
  }
  this.native = new bindings.Tessocr();
}

Tessocr.prototype.ocr = function (url, options, cb) {
  if (typeof options === 'function') {
    cb = options;
    options = cb;
  }

  options = options || {};
  cb = cb || noop;

  return this.native.ocr(url, options, cb);
};

module.exports = new Tessocr();

module.exports.Tessocr = Tessocr;
