require('./common');

var bson = require('bson_ext');

puts("Encode bad args");
assert.throws(function() { bson.encode() });
assert.throws(function() { bson.encode(null) });
assert.throws(function() { bson.encode(undefined) });
assert.throws(function() { bson.encode('value') });
assert.throws(function() { bson.encode(true) });

puts("Decode bad args");
assert.throws(function() { bson.decode() });
assert.throws(function() { bson.decode(null) });
assert.throws(function() { bson.decode(undefined) });
assert.throws(function() { bson.decode([]) });
assert.throws(function() { bson.decode({}) });

puts("Decode reported doc size > actual size");
assert.doesNotThrow(function() {
  bson.decode(
      "\x99\x00\x00\x00"           +
      "\x10hello\x00"              +
      "\x01\x00\x00\x00"           +
      "\x00")
    });

puts("Decode reported doc size < actual size");
assert.doesNotThrow(function() {
  bson.decode(
      "\x00\x00\x00\x00"           +
      "\x10hello\x00"              +
      "\x01\x00\x00\x00"           +
      "\x00") 
    });

puts("Decode nonexistent type");
assert.throws(function() { 
  bson.decode(
      "\x00\x00\x00\x00"           +
      "\x69hello\x00"              +
      "\x01\x00\x00\x00"           +
      "\x00")
});

puts("Decode wrong type");
assert.throws(function() {
  bson.decode(
      "\x10\x00\x00\x00"           +
      "\x0fhello\x00"              +
      "\x01\x00\x00\x00"           +
      "\x00")
});

puts("Decode partial");
assert.throws(function() {
  bson.decode(
    "\x10\x00\x00\x00"             +
    "\x10hello")
});