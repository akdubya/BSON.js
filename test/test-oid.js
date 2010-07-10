require('./common');

var bson = require('bson_ext');

puts("ObjectID from hex string");
var oid_hex = "123456789012345678901234";
assert.equal(new bson.ObjectID(oid_hex).toString(), oid_hex);

puts("ObjectID generate")
assert.doesNotThrow(function() { new bson.ObjectID() });

puts("ObjectID utf8 string");
assert.equal(new bson.ObjectID(oid_hex).toUtf8String(), "\x12\x34\x56\x78\x90\x12\x34\x56\x78\x90\x12\x34");

puts("ObjectID timestamp")
var o = new bson.ObjectID();
assert.equal(o.toTimestamp(), Math.floor(new Date().valueOf()/1000));

puts("ObjectID generationTime")
assert.equal(o.generationTime, Math.floor(new Date().valueOf()/1000)*1000);