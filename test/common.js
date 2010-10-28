var path = require('path');

exports = module.exports = global;

exports.testDir = path.dirname(__filename);
exports.fixturesDir = path.join(exports.testDir, "fixtures");
exports.libDir = path.join(exports.testDir, "../lib");

require.paths.push(exports.libDir);

var util = require('util');
for (var i in util) exports[i] = util[i];
exports.assert = require('assert');