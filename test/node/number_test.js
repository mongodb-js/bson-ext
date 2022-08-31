'use strict';

const expect = require('chai').expect;

const BSON = require('../..');

const BSON_DOUBLE_TYPE = 0x01;
const BSON_INT32_TYPE = 0x10;

const INT32_MAX = 2147483647;
const INT32_MIN = -2147483648;

const nanPayloadBuffer = Buffer.from('120000000000F87F', 'hex');
const nanPayloadDV = new DataView(
  nanPayloadBuffer.buffer,
  nanPayloadBuffer.byteOffset,
  nanPayloadBuffer.byteLength
);
const nanPayloadDouble = nanPayloadDV.getFloat64(0, true);

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

  it.skip('should preserve -0.0 through round trip -- TODO(NODE-4335)', () => {
    // TODO(NODE-4335): -0 should be serialized as double
    const document = { a: -0.0 };
    const bytes = BSON.serialize(document);

    expect(bytes[4]).to.equal(BSON_DOUBLE_TYPE);

    const returnedDocument = BSON.deserialize(bytes);
    expect(returnedDocument).to.deep.equal(document);
    expect(Object.is(returnedDocument.a, -0.0)).to.be.true;
  });

  it('converts -0.0 to an int32 -- TODO(NODE-4335)', () => {
    // TODO(NODE-4335): -0 should be serialized as double
    // This test is demonstrating the behavior of -0 being serialized as an int32 something we do NOT want to unintentionally change, but may want to change in the future, which the above ticket serves to track.
    const document = { a: -0.0 };
    const bytes = BSON.serialize(document);

    expect(bytes[4]).to.equal(BSON_INT32_TYPE);

    const returnedDocument = BSON.deserialize(bytes);
    expect(returnedDocument).to.have.property('a', 0);
    expect(Object.is(returnedDocument.a, -0.0)).to.be.false;
  });

  it('should preserve NaN through round trip', () => {
    const document = { a: NaN };
    const bytes = BSON.serialize(document);

    expect(bytes[4]).to.equal(BSON_DOUBLE_TYPE);

    const returnedDocument = BSON.deserialize(bytes);
    expect(returnedDocument).to.deep.equal(document);
  });

  it('should preserve NaN with payload through round trip', () => {
    const document = { a: nanPayloadDouble };
    const bytes = BSON.serialize(document);

    expect(bytes[4]).to.equal(BSON_DOUBLE_TYPE);
    expect(bytes.subarray(7, 15)).to.deep.equal(nanPayloadBuffer);

    const returnedDocument = BSON.deserialize(bytes);
    expect(returnedDocument).to.deep.equal(document);
  });

  it('should preserve Infinity through round trip', () => {
    const document = { a: Infinity };
    const bytes = BSON.serialize(document);

    expect(bytes[4]).to.equal(BSON_DOUBLE_TYPE);

    const returnedDocument = BSON.deserialize(bytes);
    expect(returnedDocument).to.deep.equal(document);
  });

  it('should preserve -Infinity through round trip', () => {
    const document = { a: -Infinity };
    const bytes = BSON.serialize(document);

    expect(bytes[4]).to.equal(BSON_DOUBLE_TYPE);

    const returnedDocument = BSON.deserialize(bytes);
    expect(returnedDocument).to.deep.equal(document);
  });
});
