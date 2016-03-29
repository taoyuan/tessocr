'use strict';

// var path = require('path');
// var binary = require('node-pre-gyp');
// var binding_path = binary.find(path.resolve(path.join(__dirname,'./package.json')));
// var binding = require(binding_path);

var binding = require('node-cmake')('tessocr');
var fs = require('fs');

function noop() {
}

var DEFAULT_TESSDATA = '/usr/local/share/tessdata';
if (!fs.existsSync(DEFAULT_TESSDATA)) {
  DEFAULT_TESSDATA = '/usr/share/tesseract-ocr/tessdata';
  if (!fs.existsSync(DEFAULT_TESSDATA)) {
    console.warn('[WARN] No default tessdata found from:');
    console.warn('    - /usr/local/share/tessdata');
    console.warn('    - /usr/share/tesseract-ocr/tessdata');
  }
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
 * @param {Object|String} image The image data or image filename
 * @param {Object|Function} [options] Options
 * @param {Function} [cb] Callback
 */
Tess.prototype.ocr = function (image, options, cb) {
  if (typeof options === 'function') {
    cb = options;
    options = cb;
  }

  options = merge({
    lang: 'eng',
    psm: 3,
    tessdata: DEFAULT_TESSDATA
  }, options);
  cb = cb || noop;

  return this.native.ocr(image, options, cb);
};

exports.Tess = exports.tess = Tess;

/**
 * merge helper function to merge objects
 * @param  {Object} defaults
 * @param  {Object} options
 * @return {Object}
 */
function merge(defaults, options) {
  defaults = defaults || {};
  if (options && typeof options === 'object') {
    var i = 0,
      keys = Object.keys(options);

    for (i = 0; i < keys.length; i += 1) {
      if (options[keys[i]] !== undefined) {
        defaults[keys[i]] = options[keys[i]];
      }
    }
  }
  return defaults;
}
