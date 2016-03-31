/**
 * Created by taoyuan on 16/3/28.
 */

var tessocr = require('..');
var fs = require('fs');
var path = require('path');

console.log(tessocr.tess);

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

var fixture = fixtures[1];

// tess.recognize(fixture.image, {lang: fixture.lang, psm: 6}, function (err, result) {
tess.recognize(fixture.image, {lang: fixture.lang, psm: 6, prm: 1, ranges: [0, 1, 2]}, function (err, result) {
// tess.recognize(fixture.image, {lang: fixture.lang, psm: 6, prm: 1, ranges: [[0, 2]]}, function (err, result) {
  if (err) throw err;
  if (result) console.log(result);
});