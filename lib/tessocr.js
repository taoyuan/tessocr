'use strict';

var path = require('path');
var fs = require('fs');
// var binding = require('node-cmake')('tessocr');
var binary = require('node-pre-gyp');
var binding_path = binary.find(path.resolve(path.join(__dirname, '../package.json')));
var binding = require(binding_path);
var utils = require('./utils');

var TESSDATA = utils.findExists([
  '/usr/local/share/tessdata',
  '/usr/share/tesseract-ocr/tessdata'
]);

var DEFAULT_OPTIONS = {
  lang: 'eng',
  psm: 3,
  tessdata: TESSDATA
};

/**
 *
 * @returns {Tess}
 * @constructor
 */
function Tess() {
  if (!(this instanceof Tess)) {
    return new Tess();
  }
}

utils.merge(Tess, binding);

/**
 *
 * @param image
 * @param options
 * @param [options.level]
 *  0: RIL_BLOCK,     // Block of text/image/separator line.
 *  1: RIL_PARA,      // Paragraph within a block.
 *  2: RIL_TEXTLINE,  // Line within a paragraph.
 *  3: RIL_WORD,      // Word within a textline.
 *  4: RIL_SYMBOL     // Symbol/character within a word.
 * @param [options.textOnly]
 * @param cb
 */
Tess.prototype.tokenize = function (image, options, cb) {
  if (typeof options === 'function') {
    cb = options;
    options = cb;
  }
  if (typeof options === 'number') {
    options = {level: options};
  } else if (typeof options === 'boolean') {
    options = {textOnly: true};
  }
  options = options || {};
  options.language = options.language || options.lang || options.l;
  options = utils.merge({}, DEFAULT_OPTIONS, options);

  cb = cb || utils.noop;

  return binding.tokenize(image, options, cb);
};

/**
 *
 * @param {Object|String} image The image data or image filename
 * @param {Object|Function} [options] Options
 * @param {String} [options.language] Language
 * @param {String} [options.lang] Language
 * @param {String} [options.l] Language
 * @param {String} [options.tessdata] tessdata path
 * @param {Number} [options.psm] page segment mode
 * @param {Number} [options.prm] Page recognize mode
 * @param {Array} [options.rules] Rules for lines
 * @param {Array} [options.rects] Multi image rects
 * @param {Array} [options.rect] Image rect
 * @param {Function} [cb] Callback
 */
Tess.prototype.recognize = function (image, options, cb) {
  if (typeof options === 'function') {
    cb = options;
    options = cb;
  }
  options = options || {};
  options.language = options.lang || options.l || options.language;
  options = utils.merge({}, DEFAULT_OPTIONS, options);

  cb = cb || utils.noop;

  return binding.recognize(image, options, cb);
};

exports.Tess = exports.tess = Tess;

