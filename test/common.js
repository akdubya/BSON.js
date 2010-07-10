var path = require('path');

exports = module.exports = global;

exports.testDir = path.dirname(__filename);
exports.fixturesDir = path.join(exports.testDir, "fixtures");
exports.libDir = path.join(exports.testDir, "../lib");

require.paths.push(exports.libDir);

var sys = require("sys");
for (var i in sys) exports[i] = sys[i];
exports.assert = require('assert');