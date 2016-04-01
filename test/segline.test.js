'use strict';

var assert = require('chai').assert;
var Jimp = require('jimp');
var segline = require('../lib/segline');
var fixtures = require('../test/fixtures');

describe('segline', function () {


  it('should segline whole image', function (done) {
    Jimp.read(fixtures.hello_2.image, function (err, image) {
      var segments = segline(image.bitmap.data, image.bitmap.width, image.bitmap.height);
      // console.log(segments);
      assert.deepEqual([{line: 0, x: 0, y: 2, w: 128, h: 9}, {line: 1, x: 0, y: 52, w: 128, h: 9}], segments);
      done();
    });
  });

  it('should segline rect area', function (done) {
    Jimp.read(fixtures.hello_2.image, function (err, image) {
      var segments = segline(image.bitmap.data, image.bitmap.width, image.bitmap.height, {
        rect: [0, 0, 0, 20]
      });
      // console.log(segments);
      assert.deepEqual([{line: 0, x: 0, y: 2, w: 128, h: 9}], segments);
      done();
    });
  });

  it('should segline image with noises', function (done) {
    Jimp.read(fixtures.hello_1.image, function (err, image) {
      var segments = segline(image.bitmap.data, image.bitmap.width, image.bitmap.height);
      // console.log(segments);
      assert.deepEqual([{line: 0, x: 0, y: 208, w: 960, h: 136}], segments);
      done();
    });
  });

  it('should segline with ranges filter', function (done) {
    Jimp.read(fixtures.receipt_1.image, function (err, image) {
      var segments = segline(image.bitmap.data, image.bitmap.width, image.bitmap.height, {
        ranges: [
          [1, 2],
          [-10, -2]
        ]
      });
      // console.log(segments.length);
      // console.log(segments);
      assert.ok(segments);
      assert.lengthOf(segments, 4);
      assert.equal(segments[0].line, 1);
      assert.equal(segments[1].line, 2);
      assert.equal(segments[2].line, 21);
      assert.equal(segments[3].line, 22);
      done();
    });
  });
});
