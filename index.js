var BSON = require('./ext').BSON;
var jsBson = require('bson');

// BSON MAX VALUES
BSON.BSON_INT32_MAX = 0x7FFFFFFF;
BSON.BSON_INT32_MIN = -0x80000000;

BSON.BSON_INT64_MAX = Math.pow(2, 63) - 1;
BSON.BSON_INT64_MIN = -Math.pow(2, 63);

// JS MAX PRECISE VALUES
BSON.JS_INT_MAX = 0x20000000000000;  // Any integer up to 2^53 can be precisely represented by a double.
BSON.JS_INT_MIN = -0x20000000000000;  // Any integer down to -2^53 can be precisely represented by a double.

// Decorate BSON with types from js-bson
[
  'Binary',
  'Code',
  'DBRef',
  'Decimal128',
  'Double',
  'Int32',
  'Long',
  'Map',
  'MaxKey',
  'MinKey',
  'ObjectId',
  'BSONRegExp',
  'Symbol',
  'Timestamp'
].forEach(function(type) {
  BSON[type] = jsBson[type];
});

// special case for deprecated names
BSON.ObjectID = BSON.ObjectId;

// Return the BSON
module.exports = BSON;
