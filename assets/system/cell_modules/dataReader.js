/**
 *  Sphere Runtime for Cellscripts
 *  Copyright (c) 2015-2017, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of miniSphere nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

'use strict';
exports = module.exports = DataReader;
exports.__esModule = true;
exports.default = exports;

const assert = require('assert');

function DataReader(stream)
{
	assert(this instanceof DataReader, "constructor requires 'new'");
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
