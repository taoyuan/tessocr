'use strict';

// var binding = require('node-cmake')('tessocr');
var path = require('path');
var binary = require('node-pre-gyp');
var binding_path = binary.find(path.resolve(path.join(__dirname, '../package.json')));
var binding = require(binding_path);

var _ = require('lodash');
var os = require('os');
var async = require('async');
var Jimp = require("jimp");
var segline = require('./segline');

var utils = require('./utils');

var TESSDATA = utils.findExists([
  '/usr/local/share/tessdata',
  '/usr/share/tesseract-ocr/tessdata'
]);

var DEFAULT_OPTIONS = {
  language: 'eng',
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

Tess.prototype.segline = function (image, options, cb) {
  if (typeof options === 'function') {
    cb = options;
    options = null;
  } else if (typeof options === 'number') {
    options = {threshold: options}
  }
  options = utils.merge(options);

  if (image && image.data && image.width && image.height) {
    cb(null, segline(image.data, image.width, image.height, options));
  } else {
    Jimp.read(image, function (err, image) {
      cb(null, segline(image.bitmap.data, image.bitmap.width, image.bitmap.height, options));
    });
  }
};

/**
 *
 * @param image
 * @param {Object|Number|Boolean} [options]
 * @param {String} [options.language] Language
 * @param {String} [options.language] Language
 * @param {String} [options.l] Language
 * @param {String} [options.tessdata] tessdata path
 * @param {Number} [options.psm] page segment mode
 * @param {Array} [options.rects] Multi image rects
 * @param {Number} [options.level]
 *  0: RIL_BLOCK,     // Block of text/image/separator line.
 *  1: RIL_PARA,      // Paragraph within a block.
 *  2: RIL_TEXTLINE,  // Line within a paragraph.
 *  3: RIL_WORD,      // Word within a textline.
 *  4: RIL_SYMBOL     // Symbol/character within a word.
 * @param {Boolean} [options.textOnly]
 * @param cb function (err, result)
 *
 * Example for result
 * {
 *   dimensions: [ 1486, 668 ],   // image width and height
 *   boxes: [
 *     { x: 38, y: 0, w: 1448, h: 114, confidence: 77 },
 *     { x: 99, y: 108, w: 1387, h: 156, confidence: 77 },
 *     { x: 34, y: 185, w: 1437, h: 88, confidence: 77 },
 *     { x: 101, y: 256, w: 1356, h: 81, confidence: 77 },
 *     { x: 31, y: 336, w: 1395, h: 74, confidence: 77 },
 *     { x: 99, y: 412, w: 1337, h: 83, confidence: 77 },
 *     { x: 28, y: 487, w: 1417, h: 82, confidence: 77 },
 *     { x: 96, y: 563, w: 1306, h: 84, confidence: 77 }
 *   ]
 * }
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

  options = parseOptions(options);
  cb = cb || utils.noop;

  return binding.tokenize(image, options, cb);
};

/**
 *
 * @param {Object|String} image The image data or image filename
 * @param {Object|Function} [options] Options
 * @param {String} [options.language] Language
 * @param {String} [options.tessdata] tessdata path
 * @param {Number} [options.psm] page segment mode
 * @param {Array} [options.rects] Multi image rects
 * @param {Function} [cb] Callback
 */
Tess.prototype.recognize = function (image, options, cb) {
  if (typeof options === 'function') {
    cb = options;
    options = cb;
  }
  options = parseOptions(options);
  cb = cb || utils.noop;

  return binding.recognize(image, options, function (err, result) {
    if (err) return cb(err);
    cb(null, _.trim(result));
  });
};

/**
 *
 * @param {Object|String} image The image data or image filename
 * @param {Object|Function} [options] Options
 * @param {String} [options.language] Language
 * @param {String} [options.tessdata] tessdata path
 * @param {Number} [options.psm] Set page segment mode
 * @param {Array} [options.rects] The rects of image to ocr
 * @param {Number} [options.jobs] Jobs to run recognize. It is work only when has rects or with segline.
 * @param {Boolean|Object} [options.segline] Using segline to segment image in rects
 * @param {Function} [cb] Callback
 */
Tess.prototype.ocr = function (image, options, cb) {
  if (typeof options === 'function') {
    cb = options;
    options = cb;
  }
  options = parseOptions(options);

  var that = this;

  var rects = options.rects;
  var sopts = options.segline;

  var bitmap;
  if (image && typeof image === 'object' && image.toBuffer && image.getPixel) {
    bitmap = image;
    image = image.toBuffer();
  }

  if (sopts) {
    if (typeof sopts !== 'object') {
      sopts = {};
    }
    rects = sopts.rects || rects;

    if (bitmap) {
      seglineAndRecognize(bitmap, bitmap.bytesPerPixel, done);
    } else if (typeof image === 'string' || Buffer.isBuffer(image)) {
      Jimp.read(image, function (err, image) {
        seglineAndRecognize(image.bitmap, done);
      });
    } else {
      throw new Error('Invalid image');
    }
  } else {
    recognize(rects, done);
  }

  function seglineAndRecognize(bitmap, channels, done) {
    if (typeof channels === 'function') {
      done = channels;
      channels = null;
    }
    sopts.channels = channels;
    if (rects) {
      rects = _.flatten(_.map(rects, function (rect) {
        sopts.rect = rect;
        return segline(bitmap.data, bitmap.width, bitmap.height, sopts);
      }));
    } else {
      rects = segline(bitmap.data, bitmap.width, bitmap.height, sopts);
    }

    recognize(rects, done);
  }

  function recognize(rects, done) {
    if (!rects) {
      return that.recognize(image, options, cb);
    }

    var jobs = utils.arrange(rects, options.jobs || os.cpus().length);
    async.map(jobs, function (rects, cb) {
      options.rects = normalize(rects);
      that.recognize(image, options, cb);
    }, done);
  }

  function done(err, result) {
    if (err) return cb(err);
    result = Array.isArray(result) ? result.join('\n') : result;
    cb(null, result);
  }
};

function parseOptions(options) {
  options = options || {};
  options.language = options.language || options.lang;
  if (options.rect && !options.rects) {
    options.rects = [options.rect];
  }
  return utils.merge({}, DEFAULT_OPTIONS, options);
}

function normalize(rects) {
  return rects.map(function (rect) {
    if (Array.isArray(rect)) return rect;
    if (rect && typeof rect === 'object') {
      return [rect.x, rect.y, rect.w, rect.h];
    }
  })
}

exports.Tess = exports.tess = Tess;

