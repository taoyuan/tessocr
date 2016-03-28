/**
 * Created by taoyuan on 16/3/28.
 */

var tessocr = require('../..');
var fs = require('fs');
var path = require('path');

var image = process.argv[2] || path.join(__dirname, 'HelloWorld.jpg');

fs.readFile(image, function (err, data) {
  if (err) {
    throw err;
  }
  console.log(require('util').inspect(data));
  tessocr.ocr(data, function(err, result){
    if(err)
      throw err;
    console.log(result);
  });
});