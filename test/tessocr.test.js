'use strict';

var assert = require('chai').assert;
var fs = require('fs');
var path = require('path');
var tessocr = require('..');

var tess = tessocr.tess();

describe('tessocr', function () {

  it('should ocr image from file', function (done) {
    tess.ocr(path.join(__dirname, 'fixtures', 'HelloWorld.jpg'), function (err, result) {
      if (err) return done(err);
      assert.equal(result, 'hello, world\n\n');
      done();
    });
  });

  it('should ocr image from buffer', function (done) {
    fs.readFile(path.join(__dirname, 'fixtures', 'HelloWorld.jpg'), function (err, data) {
      if (err) return done(err);
      tess.ocr(data, function (err, result) {
        if (err) return done(err);
        assert.equal(result, 'hello, world\n\n');
        done();
      });
    });
  });
});
