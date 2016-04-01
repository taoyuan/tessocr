'use strict';

var assert = require('chai').assert;
var path = require('path');
var Jimp = require('jimp');
var segline = require('../lib/segline');
var fixtures = require('../test/fixtures');

describe('segline', function () {
  it('should segment whole image', function (done) {
    Jimp.read(fixtures.hello_2.image, function (err, image) {
      var segments = segline(image.bitmap.data, image.bitmap.width, image.bitmap.height);
      // console.log(segments);
      assert.deepEqual([{x: 0, y: 2, w: 128, h: 9}, {x: 0, y: 52, w: 128, h: 9}], segments);
      done();
    });
  });

  it('should segment rect area', function (done) {
    Jimp.read(fixtures.hello_2.image, function (err, image) {
      var segments = segline(image.bitmap.data, image.bitmap.width, image.bitmap.height, {
        rect: [0, 0, 0, 20]
      });
      // console.log(segments);
      assert.deepEqual([{x: 0, y: 2, w: 128, h: 9}], segments);
      done();
    });
  });
});
