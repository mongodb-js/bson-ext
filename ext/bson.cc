//===========================================================================

#include <stdarg.h>
#include <cstdlib>
#include <cstring>
#include <string.h>
#include <stdlib.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#include <v8.h>

// this and the above block must be around the v8.h header otherwise
// v8 is not happy
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <node.h>
#include <node_version.h>
#include <node_buffer.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>
#include <errno.h>

#if defined(__sun) || defined(_AIX)
	#include <alloca.h>
#endif

#include "bson.h"

void die(const char *message) {
	if(errno) {
		perror(message);
	} else {
		printf("ERROR: %s\n", message);
	}

	exit(1);
}

//===========================================================================

// Equality Objects
static const char* LONG_CLASS_NAME = "Long";
static const char* OBJECT_ID_CLASS_NAME = "ObjectID";
static const char* BINARY_CLASS_NAME = "Binary";
static const char* CODE_CLASS_NAME = "Code";
static const char* DBREF_CLASS_NAME = "DBRef";
static const char* SYMBOL_CLASS_NAME = "Symbol";
static const char* DOUBLE_CLASS_NAME = "Double";
static const char* TIMESTAMP_CLASS_NAME = "Timestamp";
static const char* MIN_KEY_CLASS_NAME = "MinKey";
static const char* MAX_KEY_CLASS_NAME = "MaxKey";
static const char* REGEXP_CLASS_NAME = "BSONRegExp";

// Equality speed up comparison objects
static const char* BSONTYPE_PROPERTY_NAME = "_bsontype";
static const char* LONG_LOW_PROPERTY_NAME = "low_";
static const char* LONG_HIGH_PROPERTY_NAME = "high_";
static const char* OBJECT_ID_ID_PROPERTY_NAME = "id";
static const char* BINARY_POSITION_PROPERTY_NAME = "position";
static const char* BINARY_SUBTYPE_PROPERTY_NAME = "sub_type";
static const char* BINARY_BUFFER_PROPERTY_NAME = "buffer";
static const char* DOUBLE_VALUE_PROPERTY_NAME = "value";
static const char* SYMBOL_VALUE_PROPERTY_NAME = "value";

static const char* DBREF_REF_PROPERTY_NAME = "$ref";
static const char* DBREF_ID_REF_PROPERTY_NAME = "$id";
static const char* DBREF_DB_REF_PROPERTY_NAME = "$db";
static const char* DBREF_NAMESPACE_PROPERTY_NAME = "namespace";
static const char* DBREF_DB_PROPERTY_NAME = "db";
static const char* DBREF_OID_PROPERTY_NAME = "oid";
static const char* REGEX_PATTERN_PROPERTY_NAME = "pattern";
static const char* REGEX_OPTIONS_PROPERTY_NAME = "options";

static const char* CODE_CODE_PROPERTY_NAME = "code";
static const char* CODE_SCOPE_PROPERTY_NAME = "scope";
static const char* TO_BSON_PROPERTY_NAME = "toBSON";

void DataStream::WriteObjectId(const Local<Object>& object, const Local<String>& key)
{
	uint16_t buffer[12];
	Nan::MaybeLocal<v8::Value> obj = NanGet(object, key);

	if(node::Buffer::HasInstance(obj.ToLocalChecked())) {
		uint32_t length = (uint32_t)node::Buffer::Length(obj.ToLocalChecked());
		this->WriteData(node::Buffer::Data(obj.ToLocalChecked()), length);
	} else {
		NanGet(object, key)->ToString()->Write(buffer, 0, 12);

		for(uint32_t i = 0; i < 12; ++i)
		{
			*p++ = (char) buffer[i];
		}
	}
}

void ThrowAllocatedStringException(size_t allocationSize, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char* string = (char*) malloc(allocationSize);
	if(string == NULL) die("Failed to allocate ThrowAllocatedStringException");
	vsprintf(string, format, args);
	va_end(args);
	throw string;
}

void DataStream::CheckKey(const Local<String>& keyName)
{
	size_t keyLength = keyName->Utf8Length();
	if(keyLength == 0) return;

	// Allocate space for the key, do not need to zero terminate as WriteUtf8 does it
	char* keyStringBuffer = (char*) alloca(keyLength + 1);
	// Write the key to the allocated buffer
	keyName->WriteUtf8(keyStringBuffer);
	// Check for the zero terminator
	char* terminator = strchr(keyStringBuffer, 0x00);

	// If the location is not at the end of the string we've got an illegal 0x00 byte somewhere
	if(terminator != &keyStringBuffer[keyLength]) {
		ThrowAllocatedStringException(64+keyLength, "key %s must not contain null bytes", keyStringBuffer);
	}

	if(keyStringBuffer[0] == '$')
	{
		ThrowAllocatedStringException(64+keyLength, "key %s must not start with '$'", keyStringBuffer);
	}

	if(strchr(keyStringBuffer, '.') != NULL)
	{
		ThrowAllocatedStringException(64+keyLength, "key %s must not contain '.'", keyStringBuffer);
	}
}

template<typename T> void BSONSerializer<T>::SerializeDocument(const Local<Value>& value)
{
	void* documentSize = this->BeginWriteSize();
	Local<Object> object = bson->GetSerializeObject(value);

	// Get the object property names
  Local<Array> propertyNames = object->GetPropertyNames();

	// Length of the property
	int propertyLength = propertyNames->Length();
	for(int i = 0;  i < propertyLength; ++i)
	{
		const Local<String>& propertyName = NanGet(propertyNames, i)->ToString();
		if(checkKeys) this->CheckKey(propertyName);

		const Local<Value>& propertyValue = NanGet(object, propertyName);

		// We are not serializing the function
		if(!serializeFunctions && propertyValue->IsFunction()) {
			continue;
		}

		// We are ignoring undefined values
		if(ignoreUndefined && propertyValue->IsUndefined()) {
			continue;
		}

		// Serialize the value
		void* typeLocation = this->BeginWriteType();
		this->WriteString(propertyName);
		SerializeValue(typeLocation, propertyValue, false);
	}

	this->WriteByte(0);
	this->CommitSize(documentSize);
}

template<typename T> void BSONSerializer<T>::SerializeArray(const Local<Value>& value)
{
	void* documentSize = this->BeginWriteSize();

	Local<Array> array = Local<Array>::Cast(value->ToObject());
	uint32_t arrayLength = array->Length();

	for(uint32_t i = 0;  i < arrayLength; ++i)
	{
		void* typeLocation = this->BeginWriteType();
		this->WriteUInt32String(i);
		SerializeValue(typeLocation, NanGet(array, i), true);
	}

	this->WriteByte(0);
	this->CommitSize(documentSize);
}

// This is templated so that we can use this function to both count the number of bytes, and to serialize those bytes.
// The template approach eliminates almost all of the inspection of values unless they're required (eg. string lengths)
// and ensures that there is always consistency between bytes counted and bytes written by design.
template<typename T> void BSONSerializer<T>::SerializeValue(void* typeLocation, const Local<Value> constValue, bool isArray)
{
	Local<Value> value = constValue;

	// Check for toBSON function
	if(value->IsObject()) {
		Local<Object> object = value->ToObject();

		if(NanHas(object, "toBSON")) {
			const Local<Value>& toBSON = NanGet(object, "toBSON");
			if(!toBSON->IsFunction()) ThrowAllocatedStringException(64, "toBSON is not a function");
			value = Local<Function>::Cast(toBSON)->Call(object, 0, NULL);
		}
	}

	// Process all the values
	if(value->IsNumber())
	{
		double doubleValue = value->NumberValue();
		int intValue = (int) doubleValue;
		if(intValue == doubleValue)
		{
			this->CommitType(typeLocation, BSON_TYPE_INT);
			this->WriteInt32(intValue);
		}
		else
		{
			this->CommitType(typeLocation, BSON_TYPE_NUMBER);
			this->WriteDouble(doubleValue);
		}
	}
	else if(value->IsString())
	{
		this->CommitType(typeLocation, BSON_TYPE_STRING);
		this->WriteLengthPrefixedString(value->ToString());
	}
	else if(value->IsBoolean())
	{
		this->CommitType(typeLocation, BSON_TYPE_BOOLEAN);
		this->WriteBool(value);
	}
	else if(value->IsArray())
	{
		this->CommitType(typeLocation, BSON_TYPE_ARRAY);
		SerializeArray(value);
	}
	else if(value->IsDate())
	{
		this->CommitType(typeLocation, BSON_TYPE_DATE);
		this->WriteInt64(value);
	}
	else if(value->IsRegExp())
	{
		this->CommitType(typeLocation, BSON_TYPE_REGEXP);
		const Local<RegExp>& regExp = Local<RegExp>::Cast(value);

		this->WriteString(regExp->GetSource());

		int flags = regExp->GetFlags();
		if(flags & RegExp::kGlobal) this->WriteByte('s');
		if(flags & RegExp::kIgnoreCase) this->WriteByte('i');
		if(flags & RegExp::kMultiline) this->WriteByte('m');
		this->WriteByte(0);
	}
	else if(value->IsFunction())
	{
		this->CommitType(typeLocation, BSON_TYPE_CODE);
		this->WriteLengthPrefixedString(value->ToString());
	}
	else if(value->IsObject())
	{
		const Local<Object>& object = value->ToObject();
		if(NanHas(object, BSONTYPE_PROPERTY_NAME))
		{
			const Local<String>& constructorString = NanGet(object, BSONTYPE_PROPERTY_NAME)->ToString();

			if(NanStr(LONG_CLASS_NAME)->StrictEquals(constructorString))
			{
				this->CommitType(typeLocation, BSON_TYPE_LONG);
				this->WriteInt32(object, NanStr(LONG_LOW_PROPERTY_NAME));
				this->WriteInt32(object, NanStr(LONG_HIGH_PROPERTY_NAME));
			}
			else if(NanStr(TIMESTAMP_CLASS_NAME)->StrictEquals(constructorString))
			{
				this->CommitType(typeLocation, BSON_TYPE_TIMESTAMP);
				this->WriteInt32(object, NanStr(LONG_LOW_PROPERTY_NAME));
				this->WriteInt32(object, NanStr(LONG_HIGH_PROPERTY_NAME));
			}
			else if(NanStr(OBJECT_ID_CLASS_NAME)->StrictEquals(constructorString))
			{
				this->CommitType(typeLocation, BSON_TYPE_OID);
				this->WriteObjectId(object, NanStr(OBJECT_ID_ID_PROPERTY_NAME));
			}
			else if(NanStr(BINARY_CLASS_NAME)->StrictEquals(constructorString))
			{
				this->CommitType(typeLocation, BSON_TYPE_BINARY);

				uint32_t length = NanGet(object, BINARY_POSITION_PROPERTY_NAME)->Uint32Value();
				Local<Object> bufferObj = NanGet(object, BINARY_BUFFER_PROPERTY_NAME)->ToObject();

				this->WriteInt32(length);
				this->WriteByte(object, NanStr(BINARY_SUBTYPE_PROPERTY_NAME));	// write subtype
				// If type 0x02 write the array length aswell
				if(NanGet(object, BINARY_SUBTYPE_PROPERTY_NAME)->Int32Value() == 0x02) {
					this->WriteInt32(length);
				}
				// Write the actual data
				this->WriteData(node::Buffer::Data(bufferObj), length);
			}
			else if(NanStr(DOUBLE_CLASS_NAME)->StrictEquals(constructorString))
			{
				this->CommitType(typeLocation, BSON_TYPE_NUMBER);
				this->WriteDouble(object, NanStr(DOUBLE_VALUE_PROPERTY_NAME));
			}
			else if(NanStr(SYMBOL_CLASS_NAME)->StrictEquals(constructorString))
			{
				this->CommitType(typeLocation, BSON_TYPE_SYMBOL);
				this->WriteLengthPrefixedString(NanGet(object, SYMBOL_VALUE_PROPERTY_NAME)->ToString());
			}
			else if(NanStr(CODE_CLASS_NAME)->StrictEquals(constructorString))
			{
				const Local<String>& function = NanGet(object, CODE_CODE_PROPERTY_NAME)->ToString();
				const Local<Object>& scope = NanGet(object, CODE_SCOPE_PROPERTY_NAME)->ToObject();

				// For Node < 0.6.X use the GetPropertyNames
	      #if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION < 6
	        uint32_t propertyNameLength = scope->GetPropertyNames()->Length();
	      #else
	        uint32_t propertyNameLength = scope->GetOwnPropertyNames()->Length();
	      #endif

				if(propertyNameLength > 0)
				{
					this->CommitType(typeLocation, BSON_TYPE_CODE_W_SCOPE);
					void* codeWidthScopeSize = this->BeginWriteSize();
					this->WriteLengthPrefixedString(function->ToString());
					SerializeDocument(scope);
					this->CommitSize(codeWidthScopeSize);
				}
				else
				{
					this->CommitType(typeLocation, BSON_TYPE_CODE);
					this->WriteLengthPrefixedString(function->ToString());
				}
			}
			else if(NanStr(DBREF_CLASS_NAME)->StrictEquals(constructorString))
			{
				this->CommitType(typeLocation, BSON_TYPE_OBJECT);

				void* dbRefSize = this->BeginWriteSize();

				void* refType = this->BeginWriteType();
				this->WriteData("$ref", 5);
				SerializeValue(refType, NanGet(object, DBREF_NAMESPACE_PROPERTY_NAME), false);

				void* idType = this->BeginWriteType();
				this->WriteData("$id", 4);
				SerializeValue(idType, NanGet(object, DBREF_OID_PROPERTY_NAME), false);

				const Local<Value>& refDbValue = NanGet(object, DBREF_DB_PROPERTY_NAME);
				if(!refDbValue->IsUndefined())
				{
					void* dbType = this->BeginWriteType();
					this->WriteData("$db", 4);
					SerializeValue(dbType, refDbValue, false);
				}

				this->WriteByte(0);
				this->CommitSize(dbRefSize);
			}
			else if(NanStr(REGEXP_CLASS_NAME)->StrictEquals(constructorString))
			{
				this->CommitType(typeLocation, BSON_TYPE_REGEXP);
				// Get the pattern string
				Local<String> pattern = NanGet(object, REGEX_PATTERN_PROPERTY_NAME)->ToString();
				// Write the 0 terminated string
				this->WriteString(pattern);
				// Get the options string
				Local<String> options = NanGet(object, REGEX_OPTIONS_PROPERTY_NAME)->ToString();
				// Write the 0 terminated string
				this->WriteString(options);
			}
			else if(NanStr(MIN_KEY_CLASS_NAME)->StrictEquals(constructorString))
			{
				this->CommitType(typeLocation, BSON_TYPE_MIN_KEY);
			}
			else if(NanStr(MAX_KEY_CLASS_NAME)->StrictEquals(constructorString))
			{
				this->CommitType(typeLocation, BSON_TYPE_MAX_KEY);
			}
		}
		else if(node::Buffer::HasInstance(value))
		{
			this->CommitType(typeLocation, BSON_TYPE_BINARY);

	    #if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION < 3
       Local<Object> buffer = ObjectWrap::Unwrap<Buffer>(value->ToObject());
			 uint32_t length = object->length();
	    #else
			 uint32_t length = (uint32_t)node::Buffer::Length(value->ToObject());
	    #endif

			this->WriteInt32(length);
			this->WriteByte(0);
			this->WriteData(node::Buffer::Data(value->ToObject()), length);
		}
		else
		{
			this->CommitType(typeLocation, BSON_TYPE_OBJECT);
			SerializeDocument(value);
		}
	}
	else if(value->IsNull())
	{
		this->CommitType(typeLocation, BSON_TYPE_NULL);
	}
	else if(value->IsUndefined() && !isArray && !ignoreUndefined)
	{
		this->CommitType(typeLocation, BSON_TYPE_UNDEFINED);
	}
	else if(value->IsUndefined() && isArray && ignoreUndefined)
	{
		this->CommitType(typeLocation, BSON_TYPE_NULL);
	}
	else if(value->IsUndefined() && isArray && !ignoreUndefined)
	{
		this->CommitType(typeLocation, BSON_TYPE_UNDEFINED);
	}
}

// Data points to start of element list, length is length of entire document including '\0' but excluding initial size
BSONDeserializer::BSONDeserializer(BSON* aBson, char* data, size_t length, bool bsonRegExp)
: bson(aBson),
  pStart(data),
  p(data),
  pEnd(data + length - 1),
	bsonRegExp(bsonRegExp)
{
	if(*pEnd != '\0') ThrowAllocatedStringException(64, "Missing end of document marker '\\0'");
}

BSONDeserializer::BSONDeserializer(BSONDeserializer& parentSerializer, size_t length, bool bsonRegExp)
: bson(parentSerializer.bson),
  pStart(parentSerializer.p),
  p(parentSerializer.p),
  pEnd(parentSerializer.p + length - 1),
	bsonRegExp(bsonRegExp)
{
	parentSerializer.p += length;
	if(pEnd > parentSerializer.pEnd) ThrowAllocatedStringException(64, "Child document exceeds parent's bounds");
	if(*pEnd != '\0') ThrowAllocatedStringException(64, "Missing end of document marker '\\0'");
}

Local<Value> BSONDeserializer::ReadCString() {
	char* start = p;
	while(*p++ && (p < pEnd)) { }
	if(p > pEnd) {
		return Nan::Null();
	}

	return Unmaybe(Nan::New<String>(start, (int32_t) (p-start-1) ));
}

int32_t BSONDeserializer::ReadRegexOptions() {
	int32_t options = 0;
	for(;;) {
		switch(*p++) {
			case '\0': return options;
			case 's': options |= RegExp::kGlobal; break;
			case 'i': options |= RegExp::kIgnoreCase; break;
			case 'm': options |= RegExp::kMultiline; break;
		}
	}
}

uint32_t BSONDeserializer::ReadIntegerString() {
	uint32_t value = 0;
	while(*p) {
		if(*p < '0' || *p > '9') ThrowAllocatedStringException(64, "Invalid key for array");
		value = value * 10 + *p++ - '0';
	}
	++p;
	return value;
}

Local<String> BSONDeserializer::ReadString() {
	uint32_t length = ReadUInt32();
	char* start = p;
	p += length;
	return Unmaybe(Nan::New<String>(start, length-1));
}

Local<Object> BSONDeserializer::ReadObjectId() {
	// Copy the data into a buffer
	Local<Object> buffer = Unmaybe(Nan::CopyBuffer(p, 12));
	// Move pointer
	p += 12;
	// Return the buffer
	return Unmaybe(buffer);
}

Local<Value> BSONDeserializer::DeserializeDocument(bool promoteLongs, bool promoteBuffers) {
	uint32_t length = ReadUInt32();
	if(length < 5) ThrowAllocatedStringException(64, "Bad BSON: Document is less than 5 bytes");

	BSONDeserializer documentDeserializer(*this, length-4, bsonRegExp);
	return documentDeserializer.DeserializeDocumentInternal(promoteLongs, promoteBuffers);
}

Local<Value> BSONDeserializer::DeserializeDocumentInternal(bool promoteLongs, bool promoteBuffers) {
	Local<Object> returnObject = Unmaybe(Nan::New<Object>());

	while(HasMoreData()) {
		BsonType type = (BsonType) ReadByte();
		const Local<Value>& name = ReadCString();
		if(name->IsNull()) ThrowAllocatedStringException(64, "Bad BSON Document: illegal CString");
		// name->Is
		const Local<Value>& value = DeserializeValue(type, promoteLongs, promoteBuffers);
		returnObject->Set(name, value);
	}

	if(p != pEnd) ThrowAllocatedStringException(64, "Bad BSON Document: Serialize consumed unexpected number of bytes");

	// From JavaScript:
	// if(object['$id'] != null) object = new DBRef(object['$ref'], object['$id'], object['$db']);
	if(NanHas(returnObject, DBREF_ID_REF_PROPERTY_NAME)) {
		Local<Value> argv[] = { NanGet(returnObject, DBREF_REF_PROPERTY_NAME), NanGet(returnObject, DBREF_ID_REF_PROPERTY_NAME), NanGet(returnObject, DBREF_DB_REF_PROPERTY_NAME) };
		Nan::MaybeLocal<Object> obj = Nan::NewInstance(Nan::New(bson->dbrefConstructor), 3, argv);
		return obj.ToLocalChecked();
	} else {
		return returnObject;
	}
}

Local<Value> BSONDeserializer::DeserializeArray(bool promoteLongs, bool promoteBuffers) {
	uint32_t length = ReadUInt32();
	if(length < 5) ThrowAllocatedStringException(64, "Bad BSON: Array Document is less than 5 bytes");

	BSONDeserializer documentDeserializer(*this, length-4, bsonRegExp);
	return documentDeserializer.DeserializeArrayInternal(promoteLongs, promoteBuffers);
}

Local<Value> BSONDeserializer::DeserializeArrayInternal(bool promoteLongs, bool promoteBuffers) {
	Local<Array> returnArray = Unmaybe(Nan::New<Array>());

	while(HasMoreData()) {
		BsonType type = (BsonType) ReadByte();
		uint32_t index = ReadIntegerString();
		const Local<Value>& value = DeserializeValue(type, promoteLongs, promoteBuffers);
		returnArray->Set(index, value);
	}

	if(p != pEnd) ThrowAllocatedStringException(64, "Bad BSON Array: Serialize consumed unexpected number of bytes");
	return returnArray;
}

Local<Value> BSONDeserializer::DeserializeValue(BsonType type, bool promoteLongs, bool promoteBuffers)
{
	switch(type)
	{
	case BSON_TYPE_STRING:
		return ReadString();

	case BSON_TYPE_INT:
		return Nan::New<Integer>(ReadInt32());

	case BSON_TYPE_NUMBER:
		return Nan::New<Number>(ReadDouble());

	case BSON_TYPE_NULL:
		return Nan::Null();

	case BSON_TYPE_UNDEFINED:
		return Nan::Undefined();

	case BSON_TYPE_TIMESTAMP: {
			int32_t lowBits = ReadInt32();
			int32_t highBits = ReadInt32();
			Local<Value> argv[] = { Nan::New<Int32>(lowBits), Nan::New<Int32>(highBits) };
			Nan::MaybeLocal<Object> obj = Nan::NewInstance(Nan::New(bson->timestampConstructor), 2, argv);
			return obj.ToLocalChecked();
		}

	case BSON_TYPE_BOOLEAN:
		return (ReadByte() != 0) ? Nan::True() : Nan::False();

	case BSON_TYPE_REGEXP: {
			const Local<Value>& regex = ReadCString();
			if(regex->IsNull()) ThrowAllocatedStringException(64, "Bad BSON Document: illegal CString");

			if (bsonRegExp) {
				const Local<Value>& options = ReadCString();
				Local<Value> argv[] = { regex, options };
				Nan::MaybeLocal<Object> obj = Nan::NewInstance(Nan::New(bson->regexpConstructor), 2, argv);
				return obj.ToLocalChecked();
			} else {
				int32_t options = ReadRegexOptions();
				return Unmaybe(Nan::New<RegExp>(regex->ToString(), (RegExp::Flags) options));
			}
		}

	case BSON_TYPE_CODE: {
			const Local<Value>& code = ReadString();
			const Local<Value>& scope = Unmaybe(Nan::New<Object>());
			Local<Value> argv[] = { code, scope };
			Nan::MaybeLocal<Object> obj = Nan::NewInstance(Nan::New(bson->codeConstructor), 2, argv);
			return obj.ToLocalChecked();
		}

	case BSON_TYPE_CODE_W_SCOPE: {
			ReadUInt32();
			const Local<Value>& code = ReadString();
			const Local<Value>& scope = DeserializeDocument(promoteLongs, promoteBuffers);
			Local<Value> argv[] = { code, scope->ToObject() };
			Nan::MaybeLocal<Object> obj = Nan::NewInstance(Nan::New(bson->codeConstructor), 2, argv);
			return obj.ToLocalChecked();
		}

	case BSON_TYPE_OID: {
			Local<Value> argv[] = { ReadObjectId() };
			Nan::MaybeLocal<Object> obj = Nan::NewInstance(Nan::New(bson->objectIDConstructor), 1, argv);
			return obj.ToLocalChecked();
		}

	case BSON_TYPE_BINARY: {
			uint32_t length = ReadUInt32();
			uint32_t subType = ReadByte();
			if(subType == 0x02) {
				length = ReadInt32();
			}

			Local<Object> buffer = Unmaybe(Nan::CopyBuffer(p, length));
			p += length;

			if (promoteBuffers) {
				return buffer;
			} else {
				Local<Value> argv[] = { buffer, Nan::New<Uint32>(subType) };
				Nan::MaybeLocal<Object> obj = Nan::NewInstance(Nan::New(bson->binaryConstructor), 2, argv);
				return obj.ToLocalChecked();
			}
		}

	case BSON_TYPE_LONG: {
			// Read 32 bit integers
			int32_t lowBits = (int32_t) ReadInt32();
			int32_t highBits = (int32_t) ReadInt32();

			// Promote long is enabled
			if(promoteLongs) {
				// If value is < 2^53 and >-2^53
				if((highBits < 0x200000 || (highBits == 0x200000 && lowBits == 0)) && highBits >= -0x200000) {
					// Adjust the pointer and read as 64 bit value
					p -= 8;
					// Read the 64 bit value
					int64_t finalValue = (int64_t) ReadInt64();
					// JS has no int64 type.
					return Nan::New<Number>(static_cast<double>(finalValue));
				}
			}

			// Decode the Long value
			Local<Value> argv[] = { Nan::New<Int32>(lowBits), Nan::New<Int32>(highBits) };
			Nan::MaybeLocal<Object> obj = Nan::NewInstance(Nan::New(bson->longConstructor), 2, argv);
			return obj.ToLocalChecked();
		}

	case BSON_TYPE_DATE:
		return Unmaybe(Nan::New<Date>((double) ReadInt64()));

	case BSON_TYPE_ARRAY:
		return DeserializeArray(promoteLongs, promoteBuffers);

	case BSON_TYPE_OBJECT:
		return DeserializeDocument(promoteLongs, promoteBuffers);

	case BSON_TYPE_SYMBOL: {
			const Local<String>& string = ReadString();
			Local<Value> argv[] = { string };
			Nan::MaybeLocal<Object> obj = Nan::NewInstance(Nan::New(bson->symbolConstructor), 1, argv);
			return obj.ToLocalChecked();
		}

	case BSON_TYPE_MIN_KEY: {
		Nan::MaybeLocal<Object> obj = Nan::NewInstance(Nan::New(bson->minKeyConstructor));
		return obj.ToLocalChecked();
	}

	case BSON_TYPE_MAX_KEY: {
		Nan::MaybeLocal<Object> obj = Nan::NewInstance(Nan::New(bson->maxKeyConstructor));
		return obj.ToLocalChecked();
	}

	default:
		ThrowAllocatedStringException(64, "Unhandled BSON Type: %d", type);
	}

	return Nan::Null();
}

// statics
Persistent<FunctionTemplate> BSON::constructor_template;

BSON::BSON() : ObjectWrap()
{
}

BSON::~BSON()
{
	Nan::HandleScope scope;
	// dispose persistent handles
	buffer.Reset();
	longConstructor.Reset();
	objectIDConstructor.Reset();
	binaryConstructor.Reset();
	codeConstructor.Reset();
	dbrefConstructor.Reset();
	symbolConstructor.Reset();
	doubleConstructor.Reset();
	timestampConstructor.Reset();
	minKeyConstructor.Reset();
	maxKeyConstructor.Reset();
}

void BSON::Initialize(v8::Local<v8::Object> target) {
	// Grab the scope of the call from Node
	Nan::HandleScope scope;
	// Define a new function template
	Local<FunctionTemplate> t = Nan::New<FunctionTemplate>(New);
	t->InstanceTemplate()->SetInternalFieldCount(1);
	t->SetClassName(NanStr("BSON"));

	// Instance methods
	Nan::SetPrototypeMethod(t, "calculateObjectSize", CalculateObjectSize);
	Nan::SetPrototypeMethod(t, "serialize", BSONSerialize);
	Nan::SetPrototypeMethod(t, "serializeWithBufferAndIndex", SerializeWithBufferAndIndex);
	Nan::SetPrototypeMethod(t, "deserialize", BSONDeserialize);
	Nan::SetPrototypeMethod(t, "deserializeStream", BSONDeserializeStream);

	constructor_template.Reset(t);

	target->Set(NanStr("BSON"), t->GetFunction());
}

// Create a new instance of BSON and passing it the existing context
NAN_METHOD(BSON::New) {
	Nan::HandleScope scope;

	// Var maximum bson size
	uint32_t maxBSONSize = MAX_BSON_SIZE;

	// Check that we have an array
	if(info.Length() >= 1 && info[0]->IsArray()) {
		// Cast the array to a local reference
		Local<Array> array = Local<Array>::Cast(info[0]);

		// If we have an options object we can set the maximum bson size to enforce
		if(info.Length() == 2 && info[1]->IsObject()) {
			Local<Object> options = info[1]->ToObject();

			// Do we have a value set
			if(NanHas(options, "maxBSONSize") && NanGet(options, "maxBSONSize")->IsNumber()) {
				maxBSONSize = (size_t)NanGet(options, "maxBSONSize")->Int32Value();
			}
		}

		if(array->Length() > 0) {
			// Create a bson object instance and return it
			BSON *bson = new BSON();
			bson->maxBSONSize = maxBSONSize;

			// Allocate a new Buffer
			bson->buffer.Reset(Unmaybe(Nan::NewBuffer(sizeof(char) * maxBSONSize)));

			// Defined the classmask
			uint32_t foundClassesMask = 0;

			// Iterate over all entries to save the instantiate functions
			for(uint32_t i = 0; i < array->Length(); i++) {
				// Let's get a reference to the function
				Local<Function> func = Local<Function>::Cast(NanGet(array, i));
				Local<String> functionName = func->GetName()->ToString();

				// Save the functions making them persistant handles (they don't get collected)
				if(functionName->StrictEquals(NanStr(LONG_CLASS_NAME))) {
					bson->longConstructor.Reset(func);
					foundClassesMask |= 1;
				} else if(functionName->StrictEquals(NanStr(OBJECT_ID_CLASS_NAME))) {
					bson->objectIDConstructor.Reset(func);
					foundClassesMask |= 2;
				} else if(functionName->StrictEquals(NanStr(BINARY_CLASS_NAME))) {
					bson->binaryConstructor.Reset(func);
					foundClassesMask |= 4;
				} else if(functionName->StrictEquals(NanStr(CODE_CLASS_NAME))) {
					bson->codeConstructor.Reset(func);
					foundClassesMask |= 8;
				} else if(functionName->StrictEquals(NanStr(DBREF_CLASS_NAME))) {
					bson->dbrefConstructor.Reset(func);
					foundClassesMask |= 0x10;
				} else if(functionName->StrictEquals(NanStr(SYMBOL_CLASS_NAME))) {
					bson->symbolConstructor.Reset(func);
					foundClassesMask |= 0x20;
				} else if(functionName->StrictEquals(NanStr(DOUBLE_CLASS_NAME))) {
					bson->doubleConstructor.Reset(func);
					foundClassesMask |= 0x40;
				} else if(functionName->StrictEquals(NanStr(TIMESTAMP_CLASS_NAME))) {
					bson->timestampConstructor.Reset(func);
					foundClassesMask |= 0x80;
				} else if(functionName->StrictEquals(NanStr(MIN_KEY_CLASS_NAME))) {
					bson->minKeyConstructor.Reset(func);
					foundClassesMask |= 0x100;
				} else if(functionName->StrictEquals(NanStr(MAX_KEY_CLASS_NAME))) {
					bson->maxKeyConstructor.Reset(func);
					foundClassesMask |= 0x200;
				} else if(functionName->StrictEquals(NanStr(REGEXP_CLASS_NAME))) {
					bson->regexpConstructor.Reset(func);
					foundClassesMask |= 0x400;
				}
			}

			// Check if we have the right number of constructors otherwise throw an error
			if(foundClassesMask != 0x7ff) {
				delete bson;
				return Nan::ThrowError("Missing function constructor for either [Long/ObjectID/Binary/Code/DbRef/Symbol/Double/Timestamp/MinKey/MaxKey/BSONRegExp]");
			} else {
				bson->Wrap(info.This());
				info.GetReturnValue().Set(info.This());
			}
		} else {
			return Nan::ThrowError("No types passed in");
		}
	} else {
		return Nan::ThrowTypeError("First argument passed in must be an array of types");
	}
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------

NAN_METHOD(BSON::BSONDeserialize) {
	Nan::HandleScope scope;

	// Fail if the first argument is not a string or a buffer
	if(info.Length() > 1 && !info[0]->IsString() && !node::Buffer::HasInstance(info[0]))
		return Nan::ThrowError("First Argument must be a Buffer or String.");

	// Promote longs
	bool promoteLongs = true;
	bool promoteBuffers = false;
	bool bsonRegExp = false;

	// If we have an options object
	if(info.Length() == 2 && info[1]->IsObject()) {
		Local<Object> options = info[1]->ToObject();

		// Check if we have the promoteLongs variable
		if(NanHas(options, "promoteLongs")) {
			if(NanGet(options, "promoteLongs")->IsBoolean()) {
				promoteLongs = NanGet(options, "promoteLongs")->ToBoolean()->Value();
			} else {
				return Nan::ThrowError("promoteLongs argument must be a boolean");
			}
		}

		// Check if we have the promoteLongs variable
		if(NanHas(options, "promoteBuffers")) {
			if(NanGet(options, "promoteBuffers")->IsBoolean()) {
				promoteBuffers = NanGet(options, "promoteBuffers")->ToBoolean()->Value();
			} else {
				return Nan::ThrowError("promoteBuffers argument must be a boolean");
			}
		}

		// Check if we have the promoteLongs variable
		if(NanHas(options, "bsonRegExp")) {
			if(NanGet(options, "bsonRegExp")->IsBoolean()) {
				bsonRegExp = NanGet(options, "bsonRegExp")->ToBoolean()->Value();
			} else {
				return Nan::ThrowError("bsonRegExp argument must be a boolean");
			}
		}
	}

	// Define pointer to data
	Local<Object> obj = info[0]->ToObject();

	// Unpack the BSON parser instance
	BSON *bson = ObjectWrap::Unwrap<BSON>(info.This());

	// If we passed in a buffer, let's unpack it, otherwise let's unpack the string
	if(node::Buffer::HasInstance(obj)) {
#if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION < 3
		Local<Object> buffer = ObjectWrap::Unwrap<Buffer>(obj);
		char* data = buffer->data();
		size_t length = buffer->length();
#else
		char* data = node::Buffer::Data(obj);
		size_t length = node::Buffer::Length(obj);
#endif

		// Validate that we have at least 5 bytes
		if(length < 5) return Nan::ThrowError("corrupt bson message < 5 bytes long");

		try {
			BSONDeserializer deserializer(bson, data, length, bsonRegExp);
			info.GetReturnValue().Set(deserializer.DeserializeDocument(promoteLongs, promoteBuffers));
		} catch(char* exception) {
			Local<String> error = NanStr(exception);
			free(exception);
			return Nan::ThrowError(error);
		}
	} else {
		// The length of the data for this encoding
		ssize_t len = Nan::DecodeBytes(info[0]);

		// Validate that we have at least 5 bytes
		if(len < 5) return Nan::ThrowError("corrupt bson message < 5 bytes long");

		// Let's define the buffer size
		char* data = (char *)malloc(len);
		if(data == NULL) die("Failed to allocate char buffer for BSON serialization");
		Nan::DecodeWrite(data, len, info[0]);

		try {
			BSONDeserializer deserializer(bson, data, len, bsonRegExp);
			// deserializer.promoteLongs = promoteLongs;
			Local<Value> result = deserializer.DeserializeDocument(promoteLongs, promoteBuffers);
			free(data);
			info.GetReturnValue().Set(result);

		} catch(char* exception) {
			Local<String> error = NanStr(exception);
			free(exception);
			free(data);
			return Nan::ThrowError(error);
		}
	}
}

Local<Object> BSON::GetSerializeObject(const Local<Value>& argValue)
{
	Local<Object> object = argValue->ToObject();

	if(NanHas(object, TO_BSON_PROPERTY_NAME)) {
		const Local<Value>& toBSON = NanGet(object, TO_BSON_PROPERTY_NAME);
		if(!toBSON->IsFunction()) ThrowAllocatedStringException(64, "toBSON is not a function");

		Local<Value> result = Local<Function>::Cast(toBSON)->Call(object, 0, NULL);
		if(!result->IsObject()) ThrowAllocatedStringException(64, "toBSON function did not return an object");
		return result->ToObject();
	} else {
		return object;
	}
}

NAN_METHOD(BSON::BSONSerialize) {
	Nan::HandleScope scope;

	// Default values
	bool checkKeys = false;
	bool serializeFunctions = false;
	bool ignoreUndefined = true;

	// Make sure we have an object as the first argument
	if(info.Length() >= 1 && !info[0]->IsObject()) {
		return Nan::ThrowError("First argument must be a javascript object");
	}

	// Unpack all the options for the parser
	if(info.Length() >= 2 && info[1]->IsObject()) {
		Local<Object> options = info[1]->ToObject();

		// Check if we have the checkKeys variable
		if(NanHas(options, "checkKeys")) {
			if(NanGet(options, "checkKeys")->IsBoolean()) {
				checkKeys = NanGet(options, "checkKeys")->ToBoolean()->Value();
			} else {
				return Nan::ThrowError("checkKeys argument must be a boolean");
			}
		}

		// Check if we have the serializeFunctions variable
		if(NanHas(options, "serializeFunctions")) {
			if(NanGet(options, "serializeFunctions")->IsBoolean()) {
				serializeFunctions = NanGet(options, "serializeFunctions")->ToBoolean()->Value();
			} else {
				return Nan::ThrowError("serializeFunctions argument must be a boolean");
			}
		}

		// Check if we have the serializeFunctions variable
		if(NanHas(options, "ignoreUndefined")) {
			if(NanGet(options, "ignoreUndefined")->IsBoolean()) {
				ignoreUndefined = NanGet(options, "ignoreUndefined")->ToBoolean()->Value();
			} else {
				return Nan::ThrowError("ignoreUndefined argument must be a boolean");
			}
		}
	}

	// Unpack the BSON parser instance
	BSON *bson = ObjectWrap::Unwrap<BSON>(info.This());

	// Calculate the total size of the document in binary form to ensure we only allocate memory once
	// With serialize function
	// bool serializeFunctions = (info.Length() >= 4) && info[3]->BooleanValue();
	char *final = NULL;
	char *serialized_object = NULL;
	size_t object_size;

	try {
		Local<Object> object = bson->GetSerializeObject(info[0]);
		// Get a reference to the buffer *char pointer
		serialized_object = node::Buffer::Data(Nan::New(bson->buffer));

		// Check if we have a boolean value
		BSONSerializer<DataStream> data(bson, checkKeys, serializeFunctions, ignoreUndefined, serialized_object);
		data.SerializeDocument(object);

		// Get the object size
		object_size = data.GetSerializeSize();
		// Copy the correct size
		final = (char *)malloc(sizeof(char) * object_size);
		// Copy to the final
		memcpy(final, serialized_object, object_size);
		// Assign pointer
		serialized_object = final;
	} catch(char *err_msg) {
		free(final);
		Local<String> error = NanStr(err_msg);
		free(err_msg);
		return Nan::ThrowError(error);
	}

	// NewBuffer takes ownership on the memory, so no need to free the final allocation
	Local<Object> buffer = Unmaybe(Nan::NewBuffer(serialized_object, (uint32_t)object_size));
	info.GetReturnValue().Set(buffer);
}

NAN_METHOD(BSON::CalculateObjectSize) {
	Nan::HandleScope scope;

	// Default values
	bool serializeFunctions = false;
	bool ignoreUndefined = true;
	bool checkKeys = false;

	// Make sure we have an object as the first argument
	if(info.Length() >= 1 && !info[0]->IsObject()) {
		return Nan::ThrowError("First argument must be a javascript object");
	}

	// Unpack all the options for the parser
	if(info.Length() >= 2 && info[1]->IsObject()) {
		Local<Object> options = info[1]->ToObject();

		// Check if we have the checkKeys variable
		if(NanHas(options, "checkKeys")) {
			if(NanGet(options, "checkKeys")->IsBoolean()) {
				checkKeys = NanGet(options, "checkKeys")->ToBoolean()->Value();
			} else {
				return Nan::ThrowError("checkKeys argument must be a boolean");
			}
		}

		// Check if we have the serializeFunctions variable
		if(NanHas(options, "serializeFunctions")) {
			if(NanGet(options, "serializeFunctions")->IsBoolean()) {
				serializeFunctions = NanGet(options, "serializeFunctions")->ToBoolean()->Value();
			} else {
				return Nan::ThrowError("serializeFunctions argument must be a boolean");
			}
		}

		// Check if we have the serializeFunctions variable
		if(NanHas(options, "ignoreUndefined")) {
			if(NanGet(options, "ignoreUndefined")->IsBoolean()) {
				ignoreUndefined = NanGet(options, "ignoreUndefined")->ToBoolean()->Value();
			} else {
				return Nan::ThrowError("ignoreUndefined argument must be a boolean");
			}
		}
	}

	// Unpack the BSON parser instance
	BSON *bson = ObjectWrap::Unwrap<BSON>(info.This());
	BSONSerializer<CountStream> countSerializer(bson, checkKeys, serializeFunctions, ignoreUndefined);
	countSerializer.SerializeDocument(info[0]);

	// Return the object size
	info.GetReturnValue().Set(Nan::New<Uint32>((uint32_t) countSerializer.GetSerializeSize()));
}

NAN_METHOD(BSON::SerializeWithBufferAndIndex) {
	Nan::HandleScope scope;

	// Default values
	bool checkKeys = false;
	bool serializeFunctions = false;
	uint32_t index = 0;
	bool ignoreUndefined = true;
	// Object size
	size_t object_size;

	// Make sure we have an object as the first argument
	if(info.Length() >= 1 && !info[0]->IsObject()) {
		return Nan::ThrowError("First argument must be a javascript object");
	}

	// Make sure we have an object as the first argument
	if(info.Length() >= 2 && !node::Buffer::HasInstance(info[1])) {
		return Nan::ThrowError("Second argument must be a Buffer instance");
	}

	// Unpack all the options for the parser
	if(info.Length() >= 3 && info[2]->IsObject()) {
		Local<Object> options = info[2]->ToObject();

		// Check if we have the checkKeys variable
		if(NanHas(options, "checkKeys")) {
			if(NanGet(options, "checkKeys")->IsBoolean()) {
				checkKeys = NanGet(options, "checkKeys")->ToBoolean()->Value();
			} else {
				return Nan::ThrowError("checkKeys argument must be a boolean");
			}
		}

		// Check if we have the serializeFunctions variable
		if(NanHas(options, "serializeFunctions")) {
			if(NanGet(options, "serializeFunctions")->IsBoolean()) {
				serializeFunctions = NanGet(options, "serializeFunctions")->ToBoolean()->Value();
			} else {
				return Nan::ThrowError("serializeFunctions argument must be a boolean");
			}
		}

		// Check if we have the serializeFunctions variable
		if(NanHas(options, "ignoreUndefined")) {
			if(NanGet(options, "ignoreUndefined")->IsBoolean()) {
				ignoreUndefined = NanGet(options, "ignoreUndefined")->ToBoolean()->Value();
			} else {
				return Nan::ThrowError("ignoreUndefined argument must be a boolean");
			}
		}
	}

	try {
		BSON *bson = ObjectWrap::Unwrap<BSON>(info.This());

		// printf("== SerializeWithBufferAndIndex\n");
		// printf("\tcheckKeys=%s\n", checkKeys ? "true" : "false");
		// printf("\tserializeFunctions=%s\n", serializeFunctions ? "true" : "false");
		// printf("\tignoreUndefined=%s\n", ignoreUndefined ? "true" : "false");

		Local<Object> obj = info[1]->ToObject();
		char* data = node::Buffer::Data(obj);
		size_t length = node::Buffer::Length(obj);

		BSONSerializer<DataStream> dataSerializer(bson, checkKeys, serializeFunctions, ignoreUndefined, data+index);
		dataSerializer.SerializeDocument(bson->GetSerializeObject(info[0]));
		object_size = dataSerializer.GetSerializeSize();

		if(object_size + index > length) return Nan::ThrowError("Serious error - overflowed buffer!!");
	} catch(char *exception) {
		Local<String> error = NanStr(exception);
		free(exception);
		return Nan::ThrowError(error);
	}

	info.GetReturnValue().Set(Nan::New<Uint32>((uint32_t) (index + object_size - 1)));
}

NAN_METHOD(BSON::BSONDeserializeStream) {
	Nan::HandleScope scope;

	// At least 3 arguments required
	if(info.Length() < 5) return Nan::ThrowError("Arguments required (Buffer(data), Number(index in data), Number(number of documents to deserialize), Array(results), Number(index in the array), Object(optional))");

	// If the number of argumets equals 3
	if(info.Length() >= 5) {
		if(!node::Buffer::HasInstance(info[0])) return Nan::ThrowError("First argument must be Buffer instance");
		if(!info[1]->IsUint32()) return Nan::ThrowError("Second argument must be a positive index number");
		if(!info[2]->IsUint32()) return Nan::ThrowError("Third argument must be a positive number of documents to deserialize");
		if(!info[3]->IsArray()) return Nan::ThrowError("Fourth argument must be an array the size of documents to deserialize");
		if(!info[4]->IsUint32()) return Nan::ThrowError("Sixth argument must be a positive index number");
	}

	// If we have 4 arguments
	if(info.Length() == 6 && !info[5]->IsObject()) return Nan::ThrowError("Fifth argument must be an object with options");

	// Define pointer to data
	Local<Object> obj = info[0]->ToObject();
	uint32_t numberOfDocuments = info[2]->Uint32Value();
	uint32_t index = info[1]->Uint32Value();
	uint32_t resultIndex = info[4]->Uint32Value();
	bool promoteLongs = true;
	bool promoteBuffers = false;
	bool bsonRegExp = false;

	// Check for the value promoteLongs in the options object
	if(info.Length() == 6) {
		Local<Object> options = info[5]->ToObject();

		// Check if we have the promoteLong variable
		if(NanHas(options, "promoteLongs")) {
			promoteLongs = NanGet(options, "promoteLongs")->ToBoolean()->Value();
		}
	}

	// Unpack the BSON parser instance
	BSON *bson = ObjectWrap::Unwrap<BSON>(info.This());

	// Unpack the buffer variable
#if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION < 3
	Local<Object> buffer = ObjectWrap::Unwrap<Buffer>(obj);
	char* data = buffer->data();
	size_t length = buffer->length();
#else
	char* data = node::Buffer::Data(obj);
	size_t length = node::Buffer::Length(obj);
#endif

	// Fetch the documents
	Local<Object> documents = info[3]->ToObject();

	BSONDeserializer deserializer(bson, data+index, length-index, bsonRegExp);
	for(uint32_t i = 0; i < numberOfDocuments; i++) {
		try {
			documents->Set(i + resultIndex, deserializer.DeserializeDocument(promoteLongs, promoteBuffers));
		} catch (char* exception) {
		  Local<String> error = NanStr(exception);
			free(exception);
			return Nan::ThrowError(error);
		}
	}

	// Return new index of parsing
	info.GetReturnValue().Set(Nan::New<Uint32>((uint32_t) (index + deserializer.GetSerializeSize())));
}

// Exporting function
extern "C" void init(Local<Object> target) {
	Nan::HandleScope scope;
	BSON::Initialize(target);
}

NODE_MODULE(bson, BSON::Initialize);
