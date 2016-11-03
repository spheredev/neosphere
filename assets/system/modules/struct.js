/**
 *  miniRT struct CommonJS module
 *  (c) 2016 Fat Cerberus
**/

'use strict';
module.exports =
{
	Enum:   Enum,
	Reader: Reader,
	Writer: Writer,
};

const assert = require('assert');
const link   = require('link');

function Reader(stream)
{
	assert(this instanceof Reader, "constructor requires 'new'");
	assert('read' in stream, "not a readable stream");

	var m_decoder = new TextDecoder('utf-8');
	var m_stream = stream;

	this.readFloat32 = m_readFloat32;
	function m_readFloat32(littleEndian)
	{
		var view = new DataView(m_stream.read(4));
		value = view.getFloat32(0, littleEndian);
	}

	this.readFloat64 = m_readFloat64;
	function m_readFloat64(littleEndian)
	{
		var view = new DataView(m_stream.read(8));
		value = view.getFloat64(0, littleEndian);
	}

	this.readInt8 = m_readInt8
	function m_readInt8()
	{
		return _readInt(m_stream, 1, true);
	}

	this.readInt16 = m_readInt16
	function m_readInt16(littleEndian)
	{
		assert.equal(typeof littleEndian, 'boolean');

		return _readInt(m_stream, 2, true, littleEndian);
	}

	this.readInt32 = m_readInt32
	function m_readInt32(littleEndian)
	{
		assert.equal(typeof littleEndian, 'boolean');

		return _readInt(m_stream, 4, true, littleEndian);
	}

	this.readString = m_readString;
	function m_readString(length)
	{
		assert.equal(typeof length, 'number');

		var data = m_stream.read(length);
		return m_decoder.decode(data);
	}

	this.readString8 = m_readString8;
	function m_readString8()
	{
		var length = _readInt(m_stream, 1, false);
		return m_readString(length);
	}

	this.readString16 = m_readString16;
	function m_readString16(littleEndian)
	{
		assert.equal(typeof littleEndian, 'boolean');

		var length = _readInt(m_stream, 2, false, littleEndian);
		return m_readString(length);
	}

	this.readString32 = m_readString32;
	function m_readString32(littleEndian)
	{
		assert.equal(typeof littleEndian, 'boolean');

		var length = _readInt(m_stream, 4, false, littleEndian);
		return m_readString(length);
	}

	this.readStruct = m_readStruct;
	function m_readStruct(schema)
	{
		assert(Array.isArray(schema), "array expected");

		return _readObject(m_stream, schema, null);
	}

	this.readUint8 = m_readUint8
	function m_readUint8()
	{
		return _readInt(m_stream, 1, false);
	}

	this.readUint16 = m_readUint16
	function m_readUint16(littleEndian)
	{
		assert.equal(typeof littleEndian, 'boolean');

		return _readInt(m_stream, 2, false, littleEndian);
	}

	this.readUint32 = m_readUint32
	function m_readUint32(littleEndian)
	{
		assert.equal(typeof littleEndian, 'boolean');

		return _readInt(m_stream, 4, false, littleEndian);
	}
}

function Writer(stream)
{
	assert(this instanceof Writer, "constructor requires 'new'");
	assert('write' in stream, "not a writable stream");

	var m_encoder = new TextEncoder();
	var m_stream = stream;

	this.writeFloat32 = m_writeFloat32;
	function m_writeFloat32(value, littleEndian)
	{
		assert.equal(typeof value, 'number');

		var dv = new DataView(new ArrayBuffer(4));
		dv.setFloat32(0, value, littleEndian);
		m_stream.write(dv);
	}

	this.writeFloat64 = m_writeFloat64;
	function m_writeFloat64(value, littleEndian)
	{
		assert.equal(typeof value, 'number');

		var dv = new DataView(new ArrayBuffer(8));
		dv.setFloat64(0, value, littleEndian);
		m_stream.write(dv);
	}

	this.writeInt8 = m_writeInt8;
	function m_writeInt8(value)
	{
		assert.equal(typeof value, 'number');

		var dv = new DataView(new ArrayBuffer(1));
		dv.setInt8(0, value);
		m_stream.write(dv);
	}

	this.writeInt16 = m_writeInt16;
	function m_writeInt16(value, littleEndian)
	{
		assert.equal(typeof value, 'number');

		var dv = new DataView(new ArrayBuffer(2));
		dv.setInt16(0, value, littleEndian);
		m_stream.write(dv);
	}

	this.writeInt32 = m_writeInt32;
	function m_writeInt32(value, littleEndian)
	{
		assert.equal(typeof value, 'number');

		var dv = new DataView(new ArrayBuffer(4));
		dv.setInt32(0, value, littleEndian);
		m_stream.write(dv);
	}

	this.writeString = m_writeString;
	function m_writeString(string)
	{
		assert.equal(typeof value, 'string');

		var bytes = m_encoder.encode(string);
		m_stream.write(dv);
	}

	this.writeString8 = m_writeString8;
	function m_writeString8(string)
	{
		assert.equal(typeof value, 'string');

		var bytes = m_encoder.encode(string);
		m_writeUint8(bytes.length);
		m_stream.write(bytes);
	}

	this.writeString16 = m_writeString16;
	function m_writeString16(string, littleEndian)
	{
		assert.equal(typeof value, 'string');

		var bytes = m_encoder.encode(string);
		m_writeUint16(bytes.length, littleEndian);
		m_stream.write(bytes);
	}

	this.writeString32 = m_writeString32;
	function m_writeString32(string, littleEndian)
	{
		assert.equal(typeof value, 'string');

		var bytes = m_encoder.encode(string);
		m_writeUint32(bytes.length, littleEndian);
		m_stream.write(bytes);
	}

	this.writeStruct = m_writeStruct;
	function m_writeStruct(object, desc)
	{
		var keys = Reflect.ownKeys(desc);
		for (var i = 0; i < keys.length; ++i) {
			var value = 'key' in object ? object[key]
				: desc[key].default
			switch (desc[key].type) {
				case 'float32be': m_writeFloat32(value); break;
				case 'float32le': m_writeFloat32(value, true); break;
				case 'float64be': m_writeFloat64(value); break;
				case 'float64le': m_writeFloat64(value, true); break;
				case 'int8': m_writeInt8(value); break;
				case 'int16be': m_writeInt16(value); break;
				case 'int16le': m_writeInt16(value, true); break;
				case 'int32be': m_writeInt32(value); break;
				case 'int32le': m_writeInt32(value, true); break;
				case 'uint8': m_writeUint8(value); break;
				case 'uint16be': m_writeUint16(value); break;
				case 'uint16le': m_writeUint16(value, true); break;
				case 'uint32be': m_writeUint32(value); break;
				case 'uint32le': m_writeUint32(value, true); break;
				case 'string': m_writeString(value); break;
				case 'lstr8': m_writeString8(value); break;
				case 'lstr16be': m_writeString16(value); break;
				case 'lstr16le': m_writeString16(value, true); break;
				case 'lstr32be': m_writeString32(value); break;
				case 'lstr32le': m_writeString32(value, true); break;
				default:
					assert(false, "malformed struct descriptor");
			}
		}
	};
	
	this.writeUint8 = m_writeUint8;
	function m_writeUint8(value)
	{
		assert.equal(typeof value, 'number');

		var dv = new DataView(new ArrayBuffer(1));
		dv.setUint8(0, value);
		m_stream.write(dv);
	}

	this.writeUint16 = m_writeUint16;
	function m_writeUint16(value, littleEndian)
	{
		assert.equal(typeof value, 'number');

		var dv = new DataView(new ArrayBuffer(2));
		dv.setUint16(0, value, littleEndian);
		m_stream.write(dv);
	}

	this.writeUint32 = m_writeUint32;
	function m_writeUint32(value, littleEndian)
	{
		assert.equal(typeof value, 'number');

		var dv = new DataView(new ArrayBuffer(4));
		dv.setUint32(0, value, littleEndian);
		m_stream.write(dv);
	}
}

function Enum(memberNames)
{
	if (!(this instanceof Enum))
		return new Enum(memberNames);

	this.max = memberNames.length;
	for (var i = 0; i < memberNames.length; ++i) {
		this[memberNames[i]] = i;
		this[i] = memberNames[i];
	}
}

function _fixup(value, data)
{
	// this fixes up backreferences in the schema, for example '@numPigs'.
	// because files are loaded sequentially, references can only be to
	// earlier fields.

	if (typeof value === 'function')
		return value.call(data);
	if (typeof value === 'string' && value[0] == '@')
		return data[value.substr(1)];
	else
		return value;
}

function _readField(stream, fieldType, data)
{
	var dataSize = _fixup(fieldType.size, data);
	var itemCount = _fixup(fieldType.count, data);

	// read a value or object from the stream
	var value;
	switch (fieldType.type) {
		case 'switch':
			var control = data[fieldType.field];
			var checkType = null;
			for (var i = 0; i < fieldType.cases.length; ++i) {
				var checkType = fieldType.cases[i];
				if (('value' in checkType && typeof control == typeof checkType.value && control === checkType.value)
					|| ('range' in checkType && typeof control == 'number' && control >= checkType.range[0] && control <= checkType.range[1]))
				{
					break;
				}
				checkType = null;
			}
			if (checkType != null)
				value = _readField(stream, checkType, data);
			else
				throw new Error("binary field `" + fieldType.field + "` is invalid");
			break;
		case 'array':
			value = [];
			for (var i = 0; i < itemCount; ++i) {
				value[i] = _readField(stream, fieldType.subtype, data);
			}
			break;
		case 'bool':
			var view = new DataView(stream.read(1));
			value = view.getUint8(0) != 0;
			break;
		case 'double':
			var view = new DataView(stream.read(8));
			value = view.getFloat64(0);
			break;
		case 'doubleLE':
			var view = new DataView(stream.read(8));
			value = view.getFloat64(0, true);
			break;
		case 'float':
			var view = new DataView(stream.read(4));
			value = view.getFloat32(0);
			break;
		case 'floatLE':
			var view = new DataView(stream.read(4));
			value = view.getFloat32(0, true);
			break;
		case 'fstring':
			var bytes = stream.read(dataSize);
			value = new TextDecoder('utf-8').decode(bytes);
			break;
		case 'image':
			var width = _fixup(fieldType.width, data);
			var height = _fixup(fieldType.height, data);
			if (typeof width != 'number' || typeof height != 'number')
				throw new Error("missing or invalid width/height for `image` field");
			var pixelData = stream.read(width * height * 4);
			value = new Image(width, height, pixelData);
			break;
		case 'int':
			value = _readInt(stream, dataSize, true, false);
			break;
		case 'intLE':
			value = _readInt(stream, dataSize, true, true);
			break;
		case 'object':
			value = _readObject(stream, fieldType.schema, data);
			break;
		case 'pstring':
			var length = _readInt(stream, dataSize, false, false);
			var bytes = stream.read(length);
			value = new TextDecoder('utf-8').decode(bytes);
			break;
		case 'pstringLE':
			var length = _readInt(stream, dataSize, false, true);
			var bytes = stream.read(length);
			value = new TextDecoder('utf-8').decode(bytes);
			break;
		case 'raw':
			value = stream.read(dataSize);
			break;
		case 'reserved':
			stream.read(dataSize);
			break;
		case 'uint':
			value = _readInt(stream, dataSize, false, false);
			break;
		case 'uintLE':
			value = _readInt(stream, dataSize, false, true);
			break;
		default:
			throw new TypeError("unknown field type `" + fieldType.type + "` in schema");
	}

	// verify the value we got against the schema
	var isValidValue = true;
	if (typeof fieldType.regex === 'string' && typeof value === 'string') {
		var regex = new RegExp(fieldType.regex);
		if (!regex.test(value))
			isValidValue = false;
	}
	else if (Array.isArray(fieldType.values)) {
		if (!link(fieldType.values).contains(value))
			isValidValue = false;
	}
	else if (Array.isArray(fieldType.range) && fieldType.range.length == 2
		&& typeof fieldType.range[0] === typeof value
		&& typeof fieldType.range[1] === typeof value)
	{
		if (value < fieldType.range[0] || value > fieldType.range[1])
			isValidValue = false;
	}

	// if the value passes muster, return it.  otherwise, throw.
	if (!isValidValue) {
		throw new Error("binary field `" + fieldType.id + "` is invalid");
	}
	return value;
}

function _readInt(stream, size, signed, littleEndian)
{
	// variable-size two's complement integer decoding algorithm borrowed from
	// Node.js.  this allows us to read integer values up to 48 bits in size.

	assert(size >= 1 && size <= 6, "(u)int field size is [1-6]");

	var bytes = new Uint8Array(stream.read(size));
	var mul = 1;
	var value;
	if (littleEndian) {
		var ptr = 0;
		value = bytes[ptr];
		while (++ptr < size && (mul *= 0x100))
			value += bytes[ptr] * mul;
	}
	else {
		var ptr = size - 1;
		value = bytes[ptr];
		while (ptr > 0 && (mul *= 0x100))
			value += bytes[--ptr] * mul;
	}
	if (signed && value > mul * 0x80)
		value -= Math.pow(2, 8 * size);
	return value;
}

function _readObject(stream, schema, parentObj)
{
	var object = Object.create(parentObj);
	for (var i = 0; i < schema.length; ++i) {
		var fieldType = schema[i];
		var value = _readField(stream, fieldType, object);
		if ('id' in fieldType) {
			object[fieldType.id] = value;
		}
	}
	return object;
}
