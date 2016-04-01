var _ = require('lodash');
var utils = require('./utils');

module.exports = function segline(data, width, height, options) {
  options = utils.merge({
    threshold: 0x7F,
    minHeight: 4,
    spacing: 4 // line spacing, default 4px
  }, options);

  var channels = options.channels = options.channels || 4;

  var rect = options.rect;
  if (Array.isArray(rect)) {
    if (rect.length === 2) {
      rect = {x: rect[0], y: rect[1], w: 0, h: 0};
    } else if (rect.length === 4) {
      rect = {x: rect[0], y: rect[1], w: rect[2], h: rect[3]};
    }
  }
  if (!rect) {
    rect = {x: 0, y: 0, w: width, h: height}
  }

  var x = rect.x;
  var y = rect.y;
  var w = rect.w;
  var h = rect.h;

  if (w < 0) {
    x += w;
    w = Math.abs(w);
  }
  if (x < 0) x += width;

  if (h < 0) {
    y += h;
    h = Math.abs(h);
  }
  if (y < 0) y += height;

  if (x < 0) {
    w += x;
    x = 0;
  }

  if (x < 0) {
    h += y;
    y = 0;
  }

  if (x >= width || y >= height) {
    x = y = w = h = 0;
  }

  if (x + w > width) {
    w = width - x;
  }

  if (y + h > height) {
    h = height - y;
  }

  if (w <= 0) w = width - x;
  if (h <= 0) h = height - x;

  var r = x + w;
  var b = y + h;

  // normalize ranges
  var ranges = Ranges(options.ranges);


  // get marks for lines
  var marks = [], _x, _y, pos, i;
  for (_y = y, i = 0; _y < b; _y++, i++) {
    marks[i] = 0;
    for (_x = x; _x < r; _x++) {
      pos = (_x + _y * width) * channels;
      if (isDot(data, pos, options)) {
        marks[i] = 1;
        break;
      }
    }
  }

  var result = [];

  var dot = null, space = null, segment;
  for (i = 0; i < marks.length; i++) {
    if (marks[i] && dot === null) {
      dot = i;
      space = null;
    }
    if (!marks[i] && space === null) {
      space = i;
    }
    if (dot !== null && space !== null && (i - space >= options.spacing || i === marks.length - 1)) {
      segment = {line: result.length, x: x, y: y + dot, w: w, h: space - dot};
      if (segment.h >= options.minHeight) {
        result.push(segment);
      }
      dot = null;
    }
  }

  return _.filter(result, ranges.filter(result.length));
};

function isDot(data, pos, options) {
  var channels = options.channels;
  var threshold = options.threshold || 0x7F;
  var reverse = options.reverse;

  var result = false;
  if (channels === 1) {
    result = data[pos] <= threshold; // dark for 1
  } else if (channels >= 3) {
    result = data[pos] <= threshold || data[pos + 1] <= threshold || data[pos + 2] <= threshold;
  }
  if (channels === 4) {
    result = result && !!data[pos + 3]; // false for transparent
  }

  return reverse ? !result : result;
}

function Ranges(ranges) {
  var rules = [];
  if (Array.isArray(ranges)) {
    _.forEach(ranges, function (range) {
      if (typeof range === 'number') {
        rules.push({from: range, count: 1});
      } else if (Array.isArray(range) && range.length > 0) {
        if (range.length === 1) {
          rules.push({from: range[0], count: 1});
        } else {
          if (range[1] < 0) {
            rules.push({from: range[0] + range[1], count: Math.abs(range[1])});
          } else {
            rules.push({from: range[0], count: range[1] || 1});
          }
        }
      } else if (range && typeof range === 'object') {
        if (range.count < 0) {
          rules.push({from: range.from + range.count, count: Math.abs(range.count)});
        } else {
          rules.push({from: range.from, count: range.count || 1});
        }

      }
    });
  }

  function filter(count) {
    var runtime = _.map(rules, function (rule) {
      var from = rule.from < 0 ? rule.from + count : rule.from;
      var end = from + rule.count;
      return {from: from, end: end};
    });

    return function (segment) {
      if (!rules || rules.length === 0) {
        return true;
      }

      var line = segment.line;
      if (!count || line >= count || line < 0) {
        return false;
      }

      return !!_.find(runtime, function (rule) {
        return line >= rule.from && line < rule.end;
      });
    }
  }

  return {filter: filter};
}