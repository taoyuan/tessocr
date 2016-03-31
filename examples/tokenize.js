/**
 * Created by taoyuan on 16/3/28.
 */

var tessocr = require('..');
var fixtures = require('./fixtures');

var tess = tessocr.tess();
var fixture = fixtures[0];

console.time('tokenize');
tess.tokenize(fixture.image, {lang: fixture.lang, psm: 6, level: 2}, function (err, result) {
  console.log(result);
  console.timeEnd('tokenize');
});
