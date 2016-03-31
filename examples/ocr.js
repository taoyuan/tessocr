/**
 * Created by taoyuan on 16/3/28.
 */

var tessocr = require('..');
var fixtures = require('./fixtures');

var tess = tessocr.tess();
// var fixture = fixtures[0];
// var fixture = fixtures[1];
var fixture = fixtures[2];

console.time('tokenize');
console.time('all');
tess.tokenize(fixture.image, {lang: fixture.lang, psm: 6, level: 2}, function (err, rects) {
  console.log(rects);
  console.timeEnd('tokenize');
  console.time('ocr');
  tess.ocr(fixture.image, {lang: fixture.lang, psm: 6, jobs: 4, rects: rects}, function (err, result) {
    if (err) throw err;
    if (result) console.log(result);
    console.timeEnd('ocr');
    console.timeEnd('all');
  });
});

