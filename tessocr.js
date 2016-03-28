'use strict';

var binding = require('node-cmake')('tessocr');

function noop() {
}

/**
 * 
 * @returns {Tess}
 * @constructor
 */
function Tess() {
  if (!(this instanceof Tess)) {
    return new Tess();
  }
  this.native = new binding.Tessocr();
}

/**
 *
 * @param {Buffer|String} image The image data or image filename
 * @param {Object|Function} [options] Options
 * @param {Function} [cb] Callback
 */
Tess.prototype.ocr = function (image, options, cb) {
  if (typeof options === 'function') {
    cb = options;
    options = cb;
  }

  options = options || {};
  cb = cb || noop;

  return this.native.ocr(image, options, cb);
};

exports.Tess = exports.tess = Tess;