'use strict';

var assert = require('chai').assert;
var utils = require('../lib/utils');

describe('utils', function () {

  it('#arrange', function () {
    var arranged = utils.arrange([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], 4);
    assert.deepEqual(arranged, [ [ 1, 2, 3 ], [ 4, 5, 6 ], [ 7, 8 ], [ 9, 10 ] ]);
  });
});
