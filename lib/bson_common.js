var BSON = exports,
    Buffer = require('buffer').Buffer;

// BSON MAX VALUES
BSON.BSON_INT32_MAX = 2147483648;
BSON.BSON_INT32_MIN = -2147483648;

// BSON DATA TYPES
BSON.BSON_DATA_NUMBER = 1;
BSON.BSON_DATA_STRING = 2;
BSON.BSON_DATA_OBJECT = 3;
BSON.BSON_DATA_ARRAY = 4;
BSON.BSON_DATA_BINARY = 5;
BSON.BSON_DATA_OID = 7;
BSON.BSON_DATA_BOOLEAN = 8;
BSON.BSON_DATA_DATE = 9;
BSON.BSON_DATA_NULL = 10;
BSON.BSON_DATA_REGEXP = 11;
BSON.BSON_DATA_SYMBOL = 14;
BSON.BSON_DATA_CODE_W_SCOPE = 15;
BSON.BSON_DATA_INT = 16;
BSON.BSON_DATA_TIMESTAMP = 17;
BSON.BSON_DATA_LONG = 18;

// BSON BINARY DATA TYPES
BSON.BSON_BINARY_SUBTYPE_FUNCTION = 1;
BSON.BSON_BINARY_SUBTYPE_BYTE_ARRAY = 2;
BSON.BSON_BINARY_SUBTYPE_UUID = 3;
BSON.BSON_BINARY_SUBTYPE_MD5 = 4;
BSON.BSON_BINARY_SUBTYPE_USER_DEFINED = 128;

BSON.DBRef = function DBRef(ref, id, db) {
  this["$ref"] = ref;
  this["$id"] = id;
  if (db) this["$db"] = db;
}

BSON.DBRef.prototype.__defineGetter__('ref', function() {
  return this["$ref"];
});

BSON.DBRef.prototype.__defineGetter__('id', function() {
  return this["$id"];
});

BSON.DBRef.prototype.__defineGetter__('db', function() {
  return this["$db"];
});

BSON.OrderedHash = function OrderedHash(arguments) {
  this.ordered_keys = [];
  this.values = {};
  var index = 0;

  for(var argument in arguments) {
    var value = arguments[argument];
    this.values[argument] = value;
    this.ordered_keys[index++] = argument;
  }
};

// Functions to add values
BSON.OrderedHash.prototype.add = function(key, value) {
  if(this.values[key] == null) {
    this.ordered_keys[this.ordered_keys.length] = key;
  }

  this.values[key] = value;
  return this;
};

BSON.OrderedHash.prototype.remove = function(key) {
  var new_ordered_keys = [];
  // Remove all non_needed keys
  for(var i = 0; i < this.ordered_keys.length; i++) {
    if(!(this.ordered_keys[i] == key)) {
      new_ordered_keys[new_ordered_keys.length] = this.ordered_keys[i];
    }
  }
  // Assign the new arrays
  this.ordered_keys = new_ordered_keys;
  // Remove this reference to this
  delete this.values[key];
  return this;
};

BSON.OrderedHash.prototype.unorderedHash = function() {
  var hash = {};
  for(var i = 0; i < this.ordered_keys.length; i++) {
    hash[this.ordered_keys[i]] = this.values[this.ordered_keys[i]];
  }
  return hash;
};

// Fetch the keys for the hash
BSON.OrderedHash.prototype.keys = function() {
  return this.ordered_keys;
};

BSON.OrderedHash.prototype.get = function(key) {
  return this.values[key];
};

BSON.OrderedHash.prototype.length = function(){
 return this.keys().length;
};

BSON.OrderedHash.prototype.toArray = function() {
  var result = {},
    keys = this.keys();

  for (var key in keys)
    result[key] = this.values[key];

  return result;
};

BSON.Binary = function Binary(buffer) {
  this.sub_type = BSON.BSON_BINARY_SUBTYPE_BYTE_ARRAY;
  // Create a default sized binary in case non is passed in and prepare the size
  if (buffer) {
    if (buffer instanceof Buffer) {
      this.buffer = buffer;
    } else if (Array.isArray(buffer)) {
      this.buffer = new Buffer(buffer);
    } else {
      this.buffer = new Buffer(buffer, arguments[1]);
    }
    this.position = buffer.length;
  } else {
    this.buffer = new Buffer(Binary.BUFFER_SIZE);
    this.position = 0;
  }
};

BSON.Binary.BUFFER_SIZE = 256;

BSON.Binary.prototype.put = function(byte_value) {
  if(this.buffer.length > this.position) {
    this.buffer[this.position++] = byte_value.charCodeAt(0);
  } else {
    // Create additional overflow buffer
    var buffer = new Buffer(Binary.BUFFER_SIZE + this.buffer.length);
    // Combine the two buffers together
    this.buffer.copy(buffer, 0, 0, this.buffer.length);
    this.buffer = buffer;
    this.buffer[this.position++] = byte_value.charCodeAt(0);
  }
};

BSON.Binary.prototype.write = function(string, offset) {
  offset = offset ? offset : this.position;
  // If the buffer is to small let's extend the buffer
  if(this.buffer.length < offset + string.length) {
    var buffer = new Buffer(this.buffer.length + string.length);
    this.buffer.copy(buffer, 0, 0, this.buffer.length);
    // Assign the new buffer
    this.buffer = buffer;
  }
  // Write the content to the buffer
  this.buffer.write(string, 'binary', offset);
  this.position = offset + string.length;
};

BSON.Binary.prototype.read = function(position, length) {
  length = length && length > 0 ? length : this.position;
  // sys.puts(this.buffer.toString('binary', position, position + length))

  // Let's extract the string we want to read
  return this.buffer.toString('binary', position, position + length);
};

BSON.Binary.prototype.value = function() {
  // sys.puts("=============================================================================== START")
  // BinaryParser.pprint(this.buffer.toString('binary', 0, this.position))
  // sys.puts("=============================================================================== END")

  return this.buffer.toString('binary', 0, this.position);
};

BSON.Binary.prototype.length = function() {
  return this.position;
};