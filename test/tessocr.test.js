'use strict';

var assert = require('chai').assert;
var fs = require('fs');
var tessocr = require('..');
var fixtures = require('../test/fixtures');

var tess = tessocr.tess();

describe('tessocr', function () {
  this.timeout(10000);

  it('should recognize image from file', function (done) {
    tess.recognize(fixtures.hello_1.image, function (err, result) {
      if (err) return done(err);
      assert.equal(result, 'hello, world');
      done();
    });
  });

  it('should recognize image from buffer', function (done) {
    fs.readFile(fixtures.hello_1.image, function (err, data) {
      if (err) return done(err);
      tess.recognize(data, function (err, result) {
        if (err) return done(err);
        assert.equal(result, 'hello, world');
        done();
      });
    });
  });

  it('should ocr image with segline', function (done) {
    tess.ocr(fixtures.hello_1.image, {
      segline: true
    }, function (err, result) {
      if (err) return done(err);
      assert.equal(result, 'hello, world');
      done();
    });
  });

  it('should ocr image with rects', function (done) {
    var fixture = fixtures.receipt_1;
    tess.ocr(fixture.image, {
      tessdata: fixtures.tessdata,
      language: fixture.lang,
      rects: [
        [0, 50, 600, 40],
        [0, 90, 600, 40]
      ]
    }, function (err, result) {
      if (err) return done(err);
      assert.include(result, '日期');
      assert.include(result, '单号');
      done();
    });
  });

  it('should ocr image with segline and rects', function (done) {
    var fixture = fixtures.receipt_1;
    tess.ocr(fixture.image, {
      tessdata: fixtures.tessdata,
      language: fixture.lang,
      rects: [
        [0, 50, 600, 80],
        [0, 130, 600, 80]
      ]
    }, function (err, result) {
      if (err) return done(err);
      assert.include(result, '日期');
      assert.include(result, '单号');
      assert.include(result, '店铺名称');
      assert.include(result, '颜色');
      done();
    });
  });
});
