'use strict';

var Buffer = require('buffer').Buffer,
  fs = require('fs'),
  expect = require('chai').expect,
  BSON = require('../..'),
  Code = BSON.Code,
  BSONRegExp = BSON.BSONRegExp,
  Binary = BSON.Binary,
  Timestamp = BSON.Timestamp,
  Long = BSON.Long,
  ObjectID = BSON.ObjectID,
  ObjectId = BSON.ObjectID,
  Symbol = BSON.Symbol,
  DBRef = BSON.DBRef,
  Decimal128 = BSON.Decimal128,
  Int32 = BSON.Int32,
  Double = BSON.Double,
  MinKey = BSON.MinKey,
  MaxKey = BSON.MaxKey,
  BinaryParser = require('../binary_parser').BinaryParser,
  vm = require('vm');

var createBSON = require('../utils');

var assertBuffersEqual = function(done, buffer1, buffer2) {
  if (buffer1.length !== buffer2.length) {
    done('Buffers do not have the same length', buffer1, buffer2);
  }

  for (var i = 0; i < buffer1.length; i++) {
    expect(buffer1[i]).to.equal(buffer2[i]);
  }
};

/**
 * Module for parsing an ISO 8601 formatted string into a Date object.
 */
var ISODate = function(string) {
  var match;

  if (typeof string.getTime === 'function') return string;
  else if (
    (match = string.match(
      /^(\d{4})(-(\d{2})(-(\d{2})(T(\d{2}):(\d{2})(:(\d{2})(\.(\d+))?)?(Z|((\+|-)(\d{2}):(\d{2}))))?)?)?$/
    ))
  ) {
    var date = new Date();
    date.setUTCFullYear(Number(match[1]));
    date.setUTCMonth(Number(match[3]) - 1 || 0);
    date.setUTCDate(Number(match[5]) || 0);
    date.setUTCHours(Number(match[7]) || 0);
    date.setUTCMinutes(Number(match[8]) || 0);
    date.setUTCSeconds(Number(match[10]) || 0);
    date.setUTCMilliseconds(Number('.' + match[12]) * 1000 || 0);

    if (match[13] && match[13] !== 'Z') {
      var h = Number(match[16]) || 0,
        m = Number(match[17]) || 0;

      h *= 3600000;
      m *= 60000;

      var offset = h + m;
      if (match[15] === '+') offset = -offset;

      date = new Date(date.valueOf() + offset);
    }

    return date;
  } else throw new Error('Invalid ISO 8601 date given.', __filename);
};

describe('BSON', function() {
  /**
   * @ignore
   */
  it('Should correctly serialize a given javascript object using a BSON instance with more than 16MB', function(done) {
    var expectedSize = 20*1024*1024;
    // Create a simple object
    var doc = { large: Buffer.alloc(expectedSize) };
    // Create a BSON parser instance
    var bson = createBSON();
    bson.setInternalBufferSize(expectedSize+17);
    // Serialize the object to the buffer, checking keys and not serializing functions
    var buffer = bson.serialize(doc);
    // check
    expect(buffer.length).to.equal(expectedSize+17);

    done();
  });

  /**
   * @ignore
   */
  it('Should correctly serializeWithBufferAndIndex a given javascript object using a BSON instance with more than 16MB', function(
    done
  ) {
    var expectedSize = 20*1024*1024;
    // Create a simple object
    var doc = { large: Buffer.alloc(expectedSize) };
    // Create a BSON parser instance
    var bson = createBSON();
    // Calculate the size of the document, no function serialization
    var size = bson.calculateObjectSize(doc);
    // Allocate a buffer
    var buffer = new Buffer(size);
    bson.setInternalBufferSize(size);
    // Serialize the object to the buffer, checking keys and not serializing functions
    var index = bson.serializeWithBufferAndIndex(doc, buffer);
    // check
    expect(size).to.equal(expectedSize+17);
    expect(index).to.equal(expectedSize+16);

    done();
  });
});
