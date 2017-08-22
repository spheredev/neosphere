/**
 *  Sphere Runtime for Sphere games
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
exports = module.exports = DataWriter;
exports.__esModule = true;
exports.default = exports;

const assert = require('assert'),
      from = require('from');

function DataWriter(stream)
{
	assert(this instanceof DataWriter, "constructor requires 'new'");
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
