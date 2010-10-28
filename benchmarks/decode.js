var ext    = require('../lib/bson_ext'),
    pure   = require('../lib/bson_pure'),
    Buffer = require('buffer').Buffer,
    data   = ext.encode({
      'array'     : [1, 2, 3],
      'binary'    : function(){ var buf = new Buffer([1, 2, 3]); buf.bsonType = 2; return buf }(),
      'boolean'   : true,
      'code'      : new ext.Code('this.x == 3', {foo: 'bar'}),
      'date'      : new Date(),
      'dbref'     : new ext.DBRef('foo', new ext.ObjectID()),
      'document'  : {'a': 1, 'b': 2},
      'float'     : 33.3333,
      'int'       : 42,
      'null'      : null,
      'oid'       : new ext.ObjectID(),
      'regexp'    : /foobar/i,
      'string'    : 'hello',
      'undefined' : undefined
    });

console.log('Decoding test:');

var startAddon = new Date().getTime();

for (i=0; i<100000; i++) {
  ext.decode(data);
}

console.log("Addon completed in " + ((new Date().getTime() - startAddon)/1000) + " seconds.");

var startPure = new Date().getTime();

for (i=0; i<100000; i++) {
  pure.decode(data);
}

console.log("Pure completed in " + ((new Date().getTime() - startPure)/1000) + " seconds.");