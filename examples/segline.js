/**
 * Created by taoyuan on 16/3/28.
 */

var tessocr = require('..');
var fixtures = require('../test/fixtures');

var tess = tessocr.tess();
var fixture = fixtures.receipt_1;

console.time('segment');
tess.segline(fixture.image, function (err, rects) {
  if (err) throw err;
  console.log(rects.length);
  console.log(rects);
  console.timeEnd('segment');
  console.time('recognize');

  tess.ocr(fixture.image, {
    tessdata: fixtures.tessdata,
    language: fixture.lang,
    psm: 6,
    rects: rects
  }, function (err, result) {
    if (err) throw err;
    console.log(result);
    console.timeEnd('recognize');
  });
});