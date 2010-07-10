require('./common');

var bson = require('bson_ext'),
    Buffer = require('buffer').Buffer;

var comparisons = [
    [
        "Document (empty)", {}, "\x05\x00\x00\x00\x00"
    ],
    [
        "Document",
        {"javascript": "rocks", "mongodb": "rocks"},
        "\x2e\x00\x00\x00"          + // size
        "\x02javascript\x00"        + // key
        "\x06\x00\x00\x00rocks\x00" + // value
        "\x02mongodb\x00"           + // key
        "\x06\x00\x00\x00rocks\x00" + // value
        "\x00"
    ],
    [
        "Array document",
        ['foo', 'bar', 'baz'],
        "\x26\x00\x00\x00"        + // size
        "\x02"+"0\0"              + // item 0
        "\x04\x00\x00\x00foo\x00" +
        "\x02"+"1\0"              + // item 1
        "\x04\x00\x00\x00bar\x00" +
        "\x02"+"2\0"              + // item 2
        "\x04\x00\x00\x00baz\x00" +
        "\00"
    ],
    [
        "Float",
        {"hello": 4.20},
        "\x14\x00\x00\x00"                 + // size
        "\x01hello\x00"                    + // key
        "\xcd\xcc\xcc\xcc\xcc\xcc\x10\x40" +
        "\x00"
    ],
    [
        "String",
        {"hello": "world"},
        "\x16\x00\x00\x00\x02hello\x00\x06\x00\x00\x00world\x00\x00"
    ],
    [
        "Embedded document",
        {"great-old-ones": {"cthulhu": true}},
        "\x24\x00\x00\x00"     + // size
        "\x03great-old-ones\0" + // type, key
                                 // value:
        "\x0f\x00\x00\x00"     + //   size
        "\x08cthulhu\x00"      + //   type, key
        "\x01"                 + //   value
        "\x00"                 + //   eoo
        "\x00"
    ],
    [
        "Array of strings",
        {'mahbucket': [ 'foo', 'bar', 'baz' ]},
        "\x36\x00\x00\x00"        + // size
        "\x04mahbucket\0"         + // type, key
        "\x26\x00\x00\x00"        +
        "\x02"+"0\0"              + // item 0
        "\x04\x00\x00\x00foo\x00" +
        "\x02"+"1\0"              + // item 1
        "\x04\x00\x00\x00bar\x00" +
        "\x02"+"2\0"              + // item 2
        "\x04\x00\x00\x00baz\x00" +
        "\x00" +
        "\x00"
    ],
    [
        "Array of objects",
        {'mahbucket': [ {'foo': 'bar'} ]},
        "\x2a\x00\x00\x00"        + // size
        "\x04mahbucket\0"         + // type, key
        "\x1a\x00\x00\x00"        +
        "\x03"+"0\0"              + // item 0
        "\x12\x00\x00\x00"        + // size
        "\x02foo\x00"             + // key
        "\x04\x00\x00\x00bar\x00" + // value
        "\x00" +
        "\x00" +
        "\x00"
    ],
    [
        "Array of arrays",
        {'mahbucket': [ [ "foo", "bar" ], ["baz", "qux"] ]},
        "\x51\x00\x00\x00"        + // size
        "\x04mahbucket\0"         + // type, key

        "\x41\x00\x00\x00"        + // size of top level array

        "\x04"+"0\0"              + // first sub array

        "\x1b\x00\x00\x00"        + // size of inner array
        "\x02"+"0\0"              + // item 0
        "\x04\x00\x00\x00foo\x00" +
        "\x02"+"1\0"              + // item 1
        "\x04\x00\x00\x00bar\x00" +
        "\x00"                    +

        "\x04"+"1\0"              + // second sub array

        "\x1b\x00\x00\x00"        + // size of inner array
        "\x02"+"0\0"              + // item 0
        "\x04\x00\x00\x00baz\x00" +
        "\x02"+"1\0"              + // item 1
        "\x04\x00\x00\x00qux\x00" +
        "\x00" +

        "\x00" +
        "\x00"
    ],
    [
        "Binary (buffer)",
        {"bin": function() {var buf = new Buffer([1,2,3]); buf.bsonType = 0; return buf}()},
        "\x12\x00\x00\x00\x05bin\x00\x03\x00\x00\x00\x00\x01\x02\x03\x00"
    ],
    [
        "Binary type 2 (buffer)",
        {"bin": function() {var buf = new Buffer([1,2,3]); buf.bsonType = 2; return buf}()},
        "\x16\x00\x00\x00\x05bin\x00\x07\x00\x00\x00\x02\x03\x00\x00\x00\x01\x02\x03\x00"
    ],
    [
        "Undefined", {"foo": undefined}, "\x0a\x00\x00\x00\x06foo\x00\x00"
    ],
    [
        "ObjectID", {"id": new bson.ObjectID("123456789012345678901234")},
        "\x15\x00\x00\x00\x07id\x00\x12\x34\x56\x78\x90\x12\x34\x56\x78\x90\x12\x34\x00"
    ],
    [
        "True",
        {"hello": true},
        "\x0d\x00\x00\x00" + // size
        "\x08hello\x00"    + // key
        "\x01"             + // value
        "\x00"
    ],
    [
        "False",
        {"hello": false},
        "\x0d\x00\x00\x00" + // size
        "\x08hello\x00"    + // key
        "\x00"             + // value
        "\x00"
    ],
    [
        "Date", {"date": new Date(0)},
        "\x13\x00\x00\x00\x09date\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    ],
    [
        "Null", {"foo": null}, "\x0a\x00\x00\x00\x0afoo\x00\x00"
    ],
    [
        "Code",
        {"code": new bson.Code("this.x == 3")},
        "\x1b\x00\x00\x00\x0Dcode\x00\x0c\x00\x00\x00this.x == 3\x00\x00"
    ],
    [
        "Symbol",
        { "sym": new bson.Symbol("hello") },
        "\x14\x00\x00\x00\x0Esym\x00\x06\x00\x00\x00hello\x00\x00"
    ],
    [
        "Code with scope",
        {"code": new bson.Code("this.x == 3", {"foo": "bar"} )},
        "\x31\x00\x00\x00"                + // document size

        "\x0fcode\x00"                    + // key
        "\x26\x00\x00\x00"                + // code w scope size

        "\x0c\x00\x00\x00this.x == 3\x00" + // code string

        "\x12\x00\x00\x00"                + // scope size
        "\x02foo\x00"                     + // scope key
        "\x04\x00\x00\x00bar\x00"         + // scope string
        "\x00"                            + // end scope

        "\x00"                              // end document
    ],
    [
        "32-bit Integer",
        {"hello": 1},
        "\x10\x00\x00\x00"                 + // size
        "\x10hello\x00"                    + // key
        "\x01\x00\x00\x00" + // value
        "\x00"
    ],
    [
        "Timestamp",
        { "ts": new bson.Timestamp(1, 1) },
        "\x11\x00\x00\x00"                +  // size
        "\x11ts\x00"                      +  // key
        "\x01\x00\x00\x00"                +  // increment
        "\x01\x00\x00\x00"                +  // timestamp
        "\x00"
    ],
    [
        "64-bit Integer",
        {"hello": 2147483649},
        "\x14\x00\x00\x00"                 + // size
        "\x12hello\x00"                    + // key
        "\x01\x00\x00\x80\x00\x00\x00\x00" + // value
        "\x00"
    ],
    [
        "MinKey", {"min": bson.MinKey}, "\x0a\x00\x00\x00\xffmin\x00\x00"
    ],
    [
        "MaxKey", {"max": bson.MaxKey}, "\x0a\x00\x00\x00\x7fmax\x00\x00"
    ]
];

// Test decoding and encoding of objects
var i = 0;
comparisons.forEach(function (comp) {
    puts("Encode " + comp[0]);
    assert.deepEqual(bson.encode(comp[1]), comp[2]);
    puts("Decode " + comp[0]);
    assert.deepEqual(bson.decode(comp[2]), comp[1]);
});

var regobj = {"reg": /foobar/i},
    regbson = "\x13\x00\x00\x00\x0breg\x00foobar\x00i\x00\x00";

puts("Encode Regex");
assert.deepEqual(bson.encode(regobj), regbson);

puts("Decode Regex");
var regdecoded = bson.decode(regbson);
assert.strictEqual(regdecoded.source, regobj.source);
assert.strictEqual(regdecoded.global, regobj.global);
assert.strictEqual(regdecoded.ignoreCase, regobj.ignoreCase);
assert.strictEqual(regdecoded.multiline, regobj.multiline);

var dbrobj = {"dbref": new bson.DBRef('foo', new bson.ObjectID("123456789012345678901234"))},
    dbrbson =
        "\x30\x00\x00\x00"                                 + // size
        "\x03dbref\0"                                        + // type, key
                                                           // dbref:
        "\x24\x00\x00\x00"                                 + // size
        "\x02$ref\x00"                                     + // $ref
        "\x04\x00\x00\x00foo\x00"                          + // namespace
        "\x07$id\x00"                                      + // $id
        "\x12\x34\x56\x78\x90\x12\x34\x56\x78\x90\x12\x34" + // objectid
        "\x00"                                             +
        "\x00";

puts("Encode DBRef");
assert.deepEqual(bson.encode(dbrobj), dbrbson);

puts("Decode DBRef");
var dbrdecoded = bson.decode(dbrbson);
assert.deepEqual(dbrdecoded, dbrobj);