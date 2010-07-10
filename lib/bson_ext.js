var binding = require('../build/default/binding'),
    common = require('./bson_common'),
    BSON = module.exports;

BSON.encode      = binding.encode;
BSON.decode      = binding.decode;
BSON.Binary      = common.Binary;
BSON.DBRef       = common.DBRef;
BSON.OrderedHash = common.OrderedHash;
BSON.Code        = binding.Code;
BSON.ObjectID    = binding.ObjectID;
BSON.Symbol      = binding.Symbol;
BSON.Timestamp   = binding.Timestamp;
BSON.MinKey      = binding.MinKey;
BSON.MaxKey      = binding.MaxKey;

BSON.ObjectID.createPk = function() {
  return new BSON.ObjectID();
};

BSON.ObjectID.prototype.toJSON = function() {
  return this.toHexString();
};

BSON.ObjectID.prototype.__defineGetter__("generationTime", function() {
  return this.toTimestamp() * 1000;
});

BSON.Binary.prototype.asBSON = function() {
  this.buffer.bsonType = this.sub_type;
  return this.buffer;
};