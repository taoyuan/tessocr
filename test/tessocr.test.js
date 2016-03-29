'use strict';

var assert = require('chai').assert;
var fs = require('fs');
var path = require('path');
var tessocr = require('..');

var tess = tessocr.tess();

var imageFile = path.join(__dirname, 'fixtures', 'hello_world.jpg');

describe('tessocr', function () {

  it('should ocr image from file', function (done) {
    tess.ocr(imageFile, function (err, result) {
      if (err) return done(err);
      assert.equal(result, 'hello, world\n\n');
      console.log(result);
      done();
    });
  });

  it('should ocr image from buffer', function (done) {
    fs.readFile(imageFile, function (err, data) {
      if (err) return done(err);
      tess.ocr(data, function (err, result) {
        if (err) return done(err);
        assert.equal(result, 'hello, world\n\n');
        done();
      });
    });
  });
});
