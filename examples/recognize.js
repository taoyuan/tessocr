/**
 * Created by taoyuan on 16/3/28.
 */

var tessocr = require('..');
var fixtures = require('../test/fixtures');

var tess = tessocr.tess();
var fixture = fixtures.receipt_1;

console.time('recognize');
tess.recognize(fixture.image, {
  tessdata: fixtures.tessdata,
  language: fixture.lang, psm: 6
}, function (err, result) {
  if (err) throw err;
  console.log(result);
  console.timeEnd('recognize');
});