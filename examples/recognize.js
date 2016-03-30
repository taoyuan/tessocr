/**
 * Created by taoyuan on 16/3/28.
 */

var tessocr = require('..');
var fs = require('fs');
var path = require('path');

var tess = tessocr.tess();

// var image = process.argv[2] || path.join(__dirname, 'fixtures', 'piao.png');
var image1 = path.join(__dirname, 'fixtures', 'chi_sim.png');
var image2 = path.join(__dirname, 'fixtures', 'piao.png');

// tess.recognize(image1, {lang: 'chi_sim', psm: 6, prm: 1}, function (err, result) {
// tess.recognize(image2, {lang: 'chs.pos.fast', psm: 6, prm: 1}, function (err, result) {
tess.recognize(image2, {lang: 'chi_sim', psm: 6, prm: 1}, function (err, result) {
  if (err) throw err;
  if (result) console.log(result);
  // tess.recognize(image2, function (err, result) {
  //   if (err) throw err;
  //   if (result) console.log(result);
  // });
});