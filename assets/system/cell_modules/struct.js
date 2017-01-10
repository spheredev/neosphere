/**
 *  miniRT struct CommonJS module
 *  (c) 2016 Fat Cerberus
**/

'use strict';
module.exports =
{
	Reader: Reader,
	Writer: Writer,
};

const assert = require('assert');
const from   = require('from');

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

		var bytes = m_stream.read(length);
		var string = m_decoder.decode(bytes);
		var nulIndex = string.indexOf('\0');
		if (nulIndex !== -1)
			return string.substring(0, nulIndex);
		else
			return string;
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
	function m_readStruct(desc)
	{
		_checkStructDescriptor(desc);

		var keys = Reflect.ownKeys(desc);
		var object = {};
		for (var i = 0; i < keys.length; ++i) {
			var key = keys[i];
			var fieldDesc = desc[key];
			var value;
			switch (fieldDesc.type) {
				case 'bool': value = m_readUint8() != 0; break;
				case 'float32be': value = m_readFloat32(); break;
				case 'float32le': value = m_readFloat32(true); break;
				case 'float64be': value = m_readFloat64(); break;
				case 'float64le': value = m_readFloat64(true); break;
				case 'int8': value = m_readInt8(); break;
				case 'int16be': value = m_readInt16(); break;
				case 'int16le': value = m_readInt16(true); break;
				case 'int32be': value = m_readInt32(); break;
				case 'int32le': value = m_readInt32(true); break;
				case 'uint8': value = m_readUint8(); break;
				case 'uint16be': value = m_readUint16(); break;
				case 'uint16le': value = m_readUint16(true); break;
				case 'uint32be': value = m_readUint32(); break;
				case 'uint32le': value = m_readUint32(true); break;
				case 'fstring': value = m_readString(fieldDesc.length); break;
				case 'lstr8': value = m_readString8(); break;
				case 'lstr16be': value = m_readString16(); break;
				case 'lstr16le': value = m_readString16(true); break;
				case 'lstr32be': value = m_readString32(); break;
				case 'lstr32le': value = m_readString32(true); break;
				case 'raw': value = m_stream.read(fieldDesc.size); break;
			}
			object[key] = value;
		}
		return object;
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
	function m_writeString(value, length)
	{
		assert.equal(typeof value, 'string');
		assert.equal(typeof length, 'number');

		var encoded = m_encoder.encode(value);
		var bytes = new Uint8Array(length);
		bytes.set(encoded.subarray(0, length));
		m_stream.write(bytes);
	}

	this.writeString8 = m_writeString8;
	function m_writeString8(value)
	{
		assert.equal(typeof value, 'string');

		var bytes = m_encoder.encode(value);
		m_writeUint8(bytes.length);
		m_stream.write(bytes);
	}

	this.writeString16 = m_writeString16;
	function m_writeString16(value, littleEndian)
	{
		assert.equal(typeof value, 'string');

		var bytes = m_encoder.encode(value);
		m_writeUint16(bytes.length, littleEndian);
		m_stream.write(bytes);
	}

	this.writeString32 = m_writeString32;
	function m_writeString32(value, littleEndian)
	{
		assert.equal(typeof value, 'string');

		var bytes = m_encoder.encode(value);
		m_writeUint32(bytes.length, littleEndian);
		m_stream.write(bytes);
	}

	this.writeStruct = m_writeStruct;
	function m_writeStruct(object, desc)
	{
		_checkStructDescriptor(desc);

		var keys = Reflect.ownKeys(desc);
		for (var i = 0; i < keys.length; ++i) {
			var key = keys[i];
			var value = key in object ? object[key]
				: desc[key].default
			switch (desc[key].type) {
				case 'bool': m_writeUint8(value ? 1 : 0); break;
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
				case 'fstring': m_writeString(value, desc[key].length); break;
				case 'lstr8': m_writeString8(value); break;
				case 'lstr16be': m_writeString16(value); break;
				case 'lstr16le': m_writeString16(value, true); break;
				case 'lstr32be': m_writeString32(value); break;
				case 'lstr32le': m_writeString32(value, true); break;
				case 'raw': m_stream.write(value); break;
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

function _checkStructDescriptor(desc)
{
	var types = [
		'float32be', 'float32le', 'float64be', 'float64le',
		'int8', 'int16be', 'int16le', 'int32be', 'int32le',
		'uint8', 'uint16be', 'uint16le', 'uint32be', 'uint32le',
		'fstring', 'lstr8', 'lstr16be', 'lstr16le', 'lstr32be', 'lstr32le',
		'bool', 'raw',
	];

	var attributes = {
		'fstring': [ 'length' ],
		'raw':     [ 'size' ],
	};

	var keys = Reflect.ownKeys(desc);
	for (var i = 0; i < keys.length; ++i) {
		var fieldDesc = desc[keys[i]];
		var fieldType = fieldDesc.type;
		if (!from.Array(types).any(function(v) { return fieldType === v; }))
			throw new TypeError("unrecognized field type '" + fieldType + "'");
		if (fieldType in attributes) {
			var haveAttributes = from.Array(attributes[fieldType])
				.all(function(x) { return x in fieldDesc; });
			if (!haveAttributes)
				throw new TypeError("missing attributes for " + fieldType);
		}
	}
}

function _readInt(stream, size, signed, littleEndian)
{
	// variable-size two's complement integer decoding algorithm jacked from
	// Node.js.  this allows us to read integer values up to 48 bits in size.

	assert(size >= 1 && size <= 6, "(u)int size out of range");

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
