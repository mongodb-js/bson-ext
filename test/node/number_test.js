'use strict';

const expect = require('chai').expect;

const BSON = require('../..');

const BSON_DOUBLE_TYPE = 0x01;
const BSON_INT32_TYPE = 0x10;

const INT32_MAX = 2147483647;
const INT32_MIN = -2147483648;

describe('serializing javascript numbers', () => {
  it('should serialize a number that exceeds int32.max as a double', () => {
    const document = { a: INT32_MAX + 1 };
    const bytes = BSON.serialize(document);

    expect(bytes[4]).to.equal(BSON_DOUBLE_TYPE);

    const returnedDocument = BSON.deserialize(bytes);
    expect(returnedDocument).to.deep.equal(document);
  });

  it('should serialize a number that negatively exceeds int32.min as a double', () => {
    const document = { a: INT32_MIN - 1 };
    const bytes = BSON.serialize(document);

    expect(bytes[4]).to.equal(BSON_DOUBLE_TYPE);

    const returnedDocument = BSON.deserialize(bytes);
    expect(returnedDocument).to.deep.equal(document);
  });

  it('should serialize a number that is exactly int32.max as int32', () => {
    const document = { a: INT32_MAX };
    const bytes = BSON.serialize(document);

    expect(bytes[4]).to.equal(BSON_INT32_TYPE);

    const returnedDocument = BSON.deserialize(bytes);
    expect(returnedDocument).to.deep.equal(document);
  });

  it('should serialize a number that is exactly int32.min as int32', () => {
    const document = { a: INT32_MIN };
    const bytes = BSON.serialize(document);

    expect(bytes[4]).to.equal(BSON_INT32_TYPE);

    const returnedDocument = BSON.deserialize(bytes);
    expect(returnedDocument).to.deep.equal(document);
  });

  it('should serialize a number that has a fractional component as double', () => {
    const document = { a: 2.3 };
    const bytes = BSON.serialize(document);

    expect(bytes[4]).to.equal(BSON_DOUBLE_TYPE);

    const returnedDocument = BSON.deserialize(bytes);
    expect(returnedDocument).to.deep.equal(document);
  });
});
