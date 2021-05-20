var BSONExtModule = require('bindings')('bson').BSON;

var BSONJS = require('bson');

const BSON = new BSONExtModule([
  BSONJS.Binary,
  BSONJS.Code,
  BSONJS.DBRef,
  BSONJS.Decimal128,
  BSONJS.Double,
  BSONJS.Int32,
  BSONJS.Long,
  BSONJS.Map,
  BSONJS.MaxKey,
  BSONJS.MinKey,
  BSONJS.ObjectId,
  BSONJS.BSONRegExp,
  BSONJS.BSONSymbol,
  BSONJS.Timestamp
]);

module.exports = Object.create(null);

module.exports.BSON_INT32_MAX = 0x7fffffff;
module.exports.BSON_INT32_MIN = -0x80000000;

module.exports.BSON_INT64_MAX = BSONJS.Long.MAX_VALUE;
module.exports.BSON_INT64_MIN = BSONJS.Long.MIN_VALUE;

module.exports.JS_INT_MAX = Number.MAX_SAFE_INTEGER;
module.exports.JS_INT_MIN = Number.MIN_SAFE_INTEGER;

// Just add constants to the Native BSON parser
module.exports.BSON_BINARY_SUBTYPE_DEFAULT = 0;
module.exports.BSON_BINARY_SUBTYPE_FUNCTION = 1;
module.exports.BSON_BINARY_SUBTYPE_BYTE_ARRAY = 2;
module.exports.BSON_BINARY_SUBTYPE_UUID = 3;
module.exports.BSON_BINARY_SUBTYPE_UUID_NEW = 4;
module.exports.BSON_BINARY_SUBTYPE_MD5 = 5;
module.exports.BSON_BINARY_SUBTYPE_ENCRYPTED = 6;
module.exports.BSON_BINARY_SUBTYPE_USER_DEFINED = 128;

/** @type {import('bson').Binary} */
module.exports.Binary = BSONJS.Binary;
/** @type {import('bson').Code} */
module.exports.Code = BSONJS.Code;
/** @type {import('bson').DBRef} */
module.exports.DBRef = BSONJS.DBRef;
/** @type {import('bson').Decimal128} */
module.exports.Decimal128 = BSONJS.Decimal128;
/** @type {import('bson').Double} */
module.exports.Double = BSONJS.Double;
/** @type {import('bson').Int32} */
module.exports.Int32 = BSONJS.Int32;
/** @type {import('bson').Long} */
module.exports.Long = BSONJS.Long;
/** @type {import('bson').Map} */
module.exports.Map = BSONJS.Map;
/** @type {import('bson').MaxKey} */
module.exports.MaxKey = BSONJS.MaxKey;
/** @type {import('bson').MinKey} */
module.exports.MinKey = BSONJS.MinKey;
/** @type {import('bson').ObjectId} */
module.exports.ObjectId = BSONJS.ObjectId;
/** @type {import('bson').BSONRegExp} */
module.exports.BSONRegExp = BSONJS.BSONRegExp;
/** @type {import('bson').BSONSymbol} */
module.exports.BSONSymbol = BSONJS.BSONSymbol;
/** @type {import('bson').Timestamp} */
module.exports.Timestamp = BSONJS.Timestamp;

// special case for deprecated names
/** @type {import('bson').ObjectId} */
module.exports.ObjectID = BSONJS.ObjectId;

/** @type {import('bson').calculateObjectSize} */
module.exports.calculateObjectSize = BSON.calculateObjectSize.bind(BSON);
/** @type {import('bson').serialize} */
module.exports.serialize = BSON.serialize.bind(BSON);
/** @type {import('bson').serializeWithBufferAndIndex} */
module.exports.serializeWithBufferAndIndex = BSON.serializeWithBufferAndIndex.bind(BSON);
/** @type {import('bson').deserialize} */
module.exports.deserialize = BSON.deserialize.bind(BSON);
/** @type {import('bson').deserializeStream} */
module.exports.deserializeStream = BSON.deserializeStream.bind(BSON);

Object.freeze(module.exports);
