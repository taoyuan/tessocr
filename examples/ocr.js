/**
 * Created by taoyuan on 16/3/28.
 */

var async = require('async');
var tessocr = require('..');
var fixtures = require('../test/fixtures');

var tess = tessocr.tess();
var fixture = fixtures.receipt_1;

async.series([
  function (cb) {
    console.time('ocr-segline-rects');
    tess.ocr(fixture.image, {
      tessdata: fixtures.tessdata,
      language: fixture.lang, psm: 6,
      segline: true,
      rects: [
        [0, 0, 600, 180],
        [0, -760, 600, 120]
      ]
    }, function (err, result) {
      if (err) throw err;
      if (result) console.log(result);
      // console.timeEnd('ocr');
      console.timeEnd('ocr-segline-rects');
      console.log('------------------------------------------------------------');
      cb();
    });
  },
  function (cb) {
    console.time('ocr-segline-all');
    tess.ocr(fixture.image, {
      tessdata: fixtures.tessdata,
      language: fixture.lang,
      psm: 6,
      segline: true
    }, function (err, result) {
      if (err) throw err;
      if (result) console.log(result);
      // console.timeEnd('ocr');
      console.timeEnd('ocr-segline-all');
      console.log('------------------------------------------------------------');
      cb();
    });
  },
  function (cb) {
    console.time('ocr-all');
    tess.ocr(fixture.image, {
      tessdata: fixtures.tessdata,
      language: fixture.lang,
      psm: 6
    }, function (err, result) {
      if (err) throw err;
      if (result) console.log(result);
      // console.timeEnd('ocr');
      console.timeEnd('ocr-all');
      console.log('------------------------------------------------------------');
      cb();
    });
  }
], function () {

});



