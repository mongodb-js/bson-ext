# BSON-EXT

_A BSON parser Node.JS native addon._

BSON is short for Bin­ary JSON and is the bin­ary-en­coded seri­al­iz­a­tion of JSON-like doc­u­ments. You can learn more about it in [the specification](http://bsonspec.org).

This module is a C++ addon for Node.js that implements the BSON parsing and serialization outside of the JavaScript environment in order to increase speed and memory efficiency for certain workloads. It uses the same types as the pure JavaScript implementation, which can be found at [mongodb/js-bson](https://github.com/mongodb/js-bson).

**NOTE: bson-ext version 4+ works with js-bson version 4+ and the mongodb driver version 4+.**

## Installation

```sh
npm install bson-ext
```

## Usage

A simple example of how to use BSON in `Node.js`:

```js
// Get BSON parser class
const BSON = require('bson-ext')
// Get the Long type
const Long = BSON.Long;

// Serialize document
const doc = { long: Long.fromNumber(100) }

// Serialize a document
const data = BSON.serialize(doc)
console.log('data:', data)

// Deserialize the resulting Buffer
var docRoundTrip = bson.deserialize(data)
console.log('docRoundTrip:', docRoundTrip)
```

## Compiling

To build a new version perform the following operation.

```sh
npm install
npm run build
```

## API

### BSON types

For all BSON types documentation, please refer to the documentation for the [MongoDB Node.js driver](http://mongodb.github.io/node-mongodb-native/4.0).

#### BSON.serialize

The BSON `serialize` method takes a JavaScript object and an optional options object and returns a Node.js Buffer.

```typescript
/**
 * The BSON library accepts plain javascript objects.
 * It serializes to BSON by iterating the keys
 */
interface Document {
    [key: string]: any;
}

interface SerializeOptions {
    /** the serializer will check if keys are valid. */
    checkKeys?: boolean;
    /** serialize the javascript functions **(default:false)**. */
    serializeFunctions?: boolean;
    /** serialize will not emit undefined fields **(default:true)** */
    ignoreUndefined?: boolean;
}

/**
 * Serialize a Javascript object.
 *
 * @param object - the Javascript object to serialize.
 * @returns Buffer object containing the serialized object.
 */
function serialize(object: Document, options?: SerializeOptions): Buffer;
```

#### BSON.serializeWithBufferAndIndex

The BSON `serializeWithBufferAndIndex` method takes an object, a target buffer instance and an optional options object and returns the end serialization index in the final buffer.

```typescript
/**
 * Serialize a Javascript object using a predefined Buffer and index into the buffer,
 * useful when pre-allocating the space for serialization.
 *
 * @param object - the Javascript object to serialize.
 * @param finalBuffer - the Buffer you pre-allocated to store the serialized BSON object.
 * @returns the index pointing to the last written byte in the buffer.
 */
function serializeWithBufferAndIndex(object: Document, finalBuffer: Buffer, options?: SerializeOptions): number;
```

#### BSON.calculateObjectSize

The BSON `calculateObjectSize` method takes a JavaScript object and an optional options object and returns the size of the BSON object.

```typescript
interface CalculateObjectSizeOptions {
    /** serialize the javascript functions **(default:false)**. */
    serializeFunctions?: boolean;
    /** serialize will not emit undefined fields **(default:true)** */
    ignoreUndefined?: boolean;
}


/**
 * Calculate the bson size for a passed in Javascript object.
 *
 * @param object - the Javascript object to calculate the BSON byte size for
 * @returns size of BSON object in bytes
 * @public
 */
function calculateObjectSize(object: Document, options?: CalculateObjectSizeOptions): number;
```

#### BSON.deserialize

The BSON `deserialize` method takes a Node.js Buffer and an optional options object and returns a deserialized JavaScript object.

```typescript
interface DeserializeOptions {
    /** evaluate functions in the BSON document scoped to the object deserialized. */
    evalFunctions?: boolean;
    /** cache evaluated functions for reuse. */
    cacheFunctions?: boolean;
    /** when deserializing a Long will fit it into a Number if it's smaller than 53 bits */
    promoteLongs?: boolean;
    /** when deserializing a Binary will return it as a node.js Buffer instance. */
    promoteBuffers?: boolean;
    /** when deserializing will promote BSON values to their Node.js closest equivalent types. */
    promoteValues?: boolean;
    /** allow to specify if there what fields we wish to return as unserialized raw buffer. */
    fieldsAsRaw?: Document;
    /** return BSON regular expressions as BSONRegExp instances. */
    bsonRegExp?: boolean;
    /** allows the buffer to be larger than the parsed BSON object */
    allowObjectSmallerThanBufferSize?: boolean;
    /** Offset into buffer to begin reading document from */
    index?: number;
}

/**
 * Deserialize data as BSON.
 *
 * @param buffer - the buffer containing the serialized set of BSON documents.
 * @returns returns the deserialized Javascript Object.
 * @public
 */
function deserialize(buffer: Buffer | ArrayBufferView | ArrayBuffer, options?: DeserializeOptions): Document;
```

#### BSON.deserializeStream

The BSON `deserializeStream` method takes a Node.js Buffer, `startIndex` and allow more control over deserialization of a Buffer containing concatenated BSON documents.

```typescript
/**
 * Deserialize stream data as BSON documents.
 *
 * @param data - the buffer containing the serialized set of BSON documents.
 * @param startIndex - the start index in the data Buffer where the deserialization is to start.
 * @param numberOfDocuments - number of documents to deserialize.
 * @param documents - an array where to store the deserialized documents.
 * @param docStartIndex - the index in the documents array from where to start inserting documents.
 * @param options - additional options used for the deserialization.
 * @returns next index in the buffer after deserialization **x** numbers of documents.
 * @public
 */
function deserializeStream(data: Buffer | ArrayBufferView | ArrayBuffer, startIndex: number, numberOfDocuments: number, documents: Document[], docStartIndex: number, options: DeserializeOptions): number;
```
