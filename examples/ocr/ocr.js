/**
 * Created by taoyuan on 16/3/28.
 */

var tessocr = require('../..');
var fs = require('fs');
var path = require('path');

var tess = tessocr.tess();

console.log(tess);

var image = process.argv[2] || path.join(__dirname, 'HelloWorld.jpg');

tess.ocr(image, function (err, result) {
  if (err) throw err;
  console.log(result);
});