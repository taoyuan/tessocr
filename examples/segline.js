/**
 * Created by taoyuan on 16/3/28.
 */

var tessocr = require('..');
var fixtures = require('./fixtures');

var tess = tessocr.tess();
// var fixture = fixtures[0];
// var fixture = fixtures[1];
var fixture = fixtures[2];

console.time('segment');
tess.segline(fixture.image, function (err, result) {
  if (err) throw err;
  console.log(result);
  console.timeEnd('segment');
});