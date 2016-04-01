var path = require('path');

module.exports = {
  tessdata: path.join(__dirname, 'fixtures', 'tessdata'),
  hello_1: {
    lang: 'eng',
    image: path.join(__dirname, 'fixtures', 'hello_1.jpg')
  },
  hello_2: {
    lang: 'eng',
    image: path.join(__dirname, 'fixtures', 'hello_2.png')
  },
  eng: {
    lang: 'eng',
    image: path.join(__dirname, 'fixtures', 'eng.png')
  },
  chi_sim: {
    lang: 'chi_sim',
    image: path.join(__dirname, 'fixtures', 'chi_sim.png')
  },
  receipt_1: {
    lang: 'pos.chs.fast',
    image: path.join(__dirname, 'fixtures', 'receipt_1.png')
  },
  receipt_2: {
    lang: 'pos.chs.fast',
    image: path.join(__dirname, 'fixtures', 'receipt_2.png')
  }
};