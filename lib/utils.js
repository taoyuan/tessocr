var fs = require('fs');

exports.noop = noop;
function noop() {
}

/**
 * merge helper function to merge objects
 * @param  {Object} defaults
 * @return {Object}
 */
exports.merge = merge;
function merge(defaults) {
  defaults = defaults || {};

  var targets = Array.prototype.slice.call(arguments, 1);

  targets.forEach(function (target) {
    if (target && typeof target === 'object') {
      var keys = Object.keys(target);
      for (var i = 0; i < keys.length; i += 1) {
        if (target[keys[i]] !== undefined) {
          defaults[keys[i]] = target[keys[i]];
        }
      }
    }
  });

  return defaults;
}


exports.findExists = findExists;
function findExists(paths) {
  for (var i = 0; i < paths.length; i++) {
    if (fs.existsSync(paths[i])) {
      return paths[i];
    }
  }
}

exports.arrange = arrange;
function arrange(arr, size) {
  size = size || 1;
  var len = arr.length;
  var q = Math.floor(len / size);
  var r = len - size * q;

  var answer = [];
  for (var i = 0, a = 0; i < size && a < len; i++) {
    var c = a + q + (i < r ? 1 : 0);
    answer[i] = arr.slice(a, c);
    a = c;
  }
  return answer;
}