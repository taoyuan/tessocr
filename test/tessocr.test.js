'use strict';

var assert = require('chai').assert;
var fs = require('fs');
var path = require('path');
var tessocr = require('..');

describe('tessocr', function () {
  it('should ocr hello world image!', function (done) {
    fs.readFile(path.join(__dirname, 'fixtures', 'HelloWorld.jpg'), function (err, data) {
      if (err) return done(err);
      tessocr.ocr(data, function(err, result){
        if (err) return done(err);
        assert.equal(result, 'hello, world\n\n');
        done();
      });
    });
  });
});
