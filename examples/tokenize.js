/**
 * Created by taoyuan on 16/3/28.
 */

var async = require('async');
var fs = require('fs');
var path = require('path');
var tessocr = require('..');

var tess = tessocr.tess();

var fixtures = [{
  lang: 'eng',
  image: path.join(__dirname, 'fixtures', 'eng.png')
}, {
  lang: 'chi_sim',
  image: path.join(__dirname, 'fixtures', 'chi_sim.png')
}, {
  lang: 'chs.pos.fast',
  image: path.join(__dirname, 'fixtures', 'receipt.png')
}];

var fixture = fixtures[2];

console.time('tokenize');
tess.tokenize(fixture.image, {lang: fixture.lang, psm: 6, level: 2, rects:[[0, 0, 600, 200], [0, 500, 600, 600]], ranges: [0, 1, 2]}, function (err, tokens) {
  console.log(tokens);
  console.timeEnd('tokenize');
  // async.map(tokens, function (token, cb) {
  //   tess.recognize(fixture.image, {lang: fixture.lang, psm: 6, rect: [token.x, token.y, token.w, token.h]}, cb);
  // }, function (err, results) {
  //   console.log(results);
  //   console.timeEnd('ocr');
  // });
});

function recognize(tokens) {
  console.time('recognize');
  async.map(tokens, function (token, cb) {
    tess.recognize(fixture.image, {lang: fixture.lang, psm: 6, rect: [token.x, token.y, token.w, token.h]}, cb);
  }, function (err, results) {
    console.log(results);
    console.timeEnd('recognize');
  });
}

//
// var tokens = [ { x: 358, y: 41, w: 745, h: 105, confidence: 79 },
//   { x: 367, y: 164, w: 746, h: 105, confidence: 79 },
//   { x: 348, y: 286, w: 765, h: 105, confidence: 79 },
//   { x: 358, y: 409, w: 755, h: 105, confidence: 79 },
//   { x: 358, y: 531, w: 755, h: 105, confidence: 79 } ];
//
// recognize(tokens);