var ext    = require('../lib/bson_ext'),
    pure   = require('../lib/bson_pure'),
    sys    = require('sys'),
    Buffer = require('buffer').Buffer,
    data   = {
      'array'     : [1, 2, 3],
      'binary'    : function(){ var buf = new Buffer([1, 2, 3]); buf.bsonType = 2; return buf }(),
      'boolean'   : true,
      'custom'    : function(){ var obj = {}; obj.asBSON = function(){ return 69 }; return obj }(),
      'code'      : new ext.Code('this.x == 3', {foo: 'bar'}),
      'date'      : new Date(),
      'dbref'     : new ext.DBRef('foo', new ext.ObjectID()),
      'document'  : {'a': 1, 'b': 2},
      'float'     : 33.3333,
      'int'       : 42,
      'minkey'    : ext.MinKey,
      'maxkey'    : ext.MaxKey,
      'null'      : null,
      'oid'       : new ext.ObjectID(),
      'ordered'   : function(){ var obj = new ext.OrderedHash; obj.add('foo', 'bar'); obj.add('1', 'baz'); return obj }(),
      'regexp'    : /foobar/i,
      'string'    : 'hello',
      'symbol'    : new ext.Symbol('hello'),
      'timestamp' : new ext.Timestamp(1,1),
      'undefined' : undefined
    };

sys.puts('Encoding test:');

var startAddon = new Date().getTime();

for (i=0; i<100000; i++) {
  ext.encode(data);
}

sys.puts("Addon completed in " + ((new Date().getTime() - startAddon)/1000) + " seconds.");

data.binary = new pure.Binary([1,2,3]);
data.oid = new pure.ObjectID();
data.code = new pure.Code('this.x == 3', {foo: 'bar'});

var startPure = new Date().getTime();

for (i=0; i<100000; i++) {
  pure.encode(data);
}

sys.puts("Pure completed in " + ((new Date().getTime() - startPure)/1000) + " seconds.");