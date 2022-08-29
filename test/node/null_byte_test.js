'use strict';

const expect = require('chai').expect;
const BSONLib = require('../../lib/index');
const createBSON = require('../utils');
const BSON = Object.assign({}, BSONLib, Object.getPrototypeOf(createBSON()));

describe('null byte handling in serializing', ()  => {
    it('should throw when null byte in BSON Field name within a root document', () => {
        expect(() => BSON.serialize({ 'a\x00b': 1 })).to.throw(/null bytes/);
    });

    it('should throw when null byte in BSON Field name within a sub-document', () => {
        expect(() => BSON.serialize({ a: { 'a\x00b': 1 } })).to.throw(/null bytes/);
    });

    it('should throw when null byte in Pattern for a regular expression', () => {
        // eslint-disable-next-line no-control-regex
        expect(() => BSON.serialize({ a: new RegExp('a\x00b') })).to.throw(/null bytes/);
        expect(() => BSON.serialize({ a: new BSON.BSONRegExp('a\x00b') })).to.throw(/null bytes/);
    });

    it('should throw when null byte in Flags/options for a regular expression', () => {
        expect(() => BSON.serialize({ a: new BSON.BSONRegExp('a', 'i\x00m') })).to.throw(/null bytes/);
    });
})
