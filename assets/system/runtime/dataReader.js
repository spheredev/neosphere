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
const assert = require('assert'),
      from   = require('from');

class DataReader
{
	constructor(stream)
	{
		assert('read' in stream, "stream used with DataReader must be have a read() method");

		this.textDecoder = new TextDecoder('utf-8');
		this.stream = stream;
	}

	readFloat32(littleEndian)
	{
		let view = new DataView(this.stream.read(4));
		return view.getFloat32(0, littleEndian);
	}

	readFloat64(littleEndian)
	{
		let view = new DataView(this.stream.read(8));
		return view.getFloat64(0, littleEndian);
	}

	readInt8()
	{
		return readInteger(this.stream, 1, true);
	}

	readInt16(littleEndian)
	{
		assert.equal(typeof littleEndian, 'boolean');

		return readInteger(this.stream, 2, true, littleEndian);
	}

	readInt32(littleEndian)
	{
		assert.equal(typeof littleEndian, 'boolean');

		return readInteger(this.stream, 4, true, littleEndian);
	}

	readStringRaw(length)
	{
		assert.equal(typeof length, 'number');

		let bytes = this.stream.read(length);
		let string = this.textDecoder.decode(bytes);
		let nulIndex = string.indexOf('\0');
		if (nulIndex !== -1)
			return string.substring(0, nulIndex);
		else
			return string;
	}

	readString8()
	{
		let length = readInteger(this.stream, 1, false);
		return this.readStringRaw(length);
	}

	readString16(littleEndian)
	{
		assert.equal(typeof littleEndian, 'boolean');

		let length = readInteger(this.stream, 2, false, littleEndian);
		return this.readStringRaw(length);
	}

	readString32(littleEndian)
	{
		assert.equal(typeof littleEndian, 'boolean');

		let length = readInteger(this.stream, 4, false, littleEndian);
		return this.readStringRaw(length);
	}

	readStruct(desc)
	{
		verifyStructDescriptor(desc);

		let object = {};
		for (const key of Object.keys(desc)) {
			let fieldDesc = desc[key];
			let value;
			switch (fieldDesc.type) {
				case 'bool': value = this.readUint8() != 0; break;
				case 'float32be': value = this.readFloat32(); break;
				case 'float32le': value = this.readFloat32(true); break;
				case 'float64be': value = this.readFloat64(); break;
				case 'float64le': value = this.readFloat64(true); break;
				case 'int8': value = this.readInt8(); break;
				case 'int16be': value = this.readInt16(); break;
				case 'int16le': value = this.readInt16(true); break;
				case 'int32be': value = this.readInt32(); break;
				case 'int32le': value = this.readInt32(true); break;
				case 'uint8': value = this.readUint8(); break;
				case 'uint16be': value = this.readUint16(); break;
				case 'uint16le': value = this.readUint16(true); break;
				case 'uint32be': value = this.readUint32(); break;
				case 'uint32le': value = this.readUint32(true); break;
				case 'fstring': value = this.readStringRaw(fieldDesc.length); break;
				case 'lstr8': value = this.readString8(); break;
				case 'lstr16be': value = this.readString16(); break;
				case 'lstr16le': value = this.readString16(true); break;
				case 'lstr32be': value = this.readString32(); break;
				case 'lstr32le': value = this.readString32(true); break;
				case 'raw': value = this.stream.read(fieldDesc.size); break;
			}
			object[key] = value;
		}
		return object;
	}

	readUint8()
	{
		return readInteger(this.stream, 1, false);
	}

	readUint16(littleEndian)
	{
		assert.equal(typeof littleEndian, 'boolean');

		return readInteger(this.stream, 2, false, littleEndian);
	}

	readUint32(littleEndian)
	{
		assert.equal(typeof littleEndian, 'boolean');

		return readInteger(this.stream, 4, false, littleEndian);
	}
}

function readInteger(stream, size, signed, littleEndian)
{
	// variable-size two's complement integer decoding algorithm jacked from
	// Node.js.  this allows us to read integer values up to 48 bits in size.

	assert(size >= 1 && size <= 6, `(u)int size ${size} out of range`);

	let bytes = new Uint8Array(stream.read(size));
	let mul = 1;
	let value;
	if (littleEndian) {
		let ptr = 0;
		value = bytes[ptr];
		while (++ptr < size && (mul *= 0x100))
			value += bytes[ptr] * mul;
	}
	else {
		let ptr = size - 1;
		value = bytes[ptr];
		while (ptr > 0 && (mul *= 0x100))
			value += bytes[--ptr] * mul;
	}
	if (signed && value > mul * 0x80)
		value -= Math.pow(2, 8 * size);
	return value;
}

function verifyStructDescriptor(desc)
{
	const FieldTypes = [
		'float32be', 'float32le', 'float64be', 'float64le',
		'int8', 'int16be', 'int16le', 'int32be', 'int32le',
		'uint8', 'uint16be', 'uint16le', 'uint32be', 'uint32le',
		'fstring', 'lstr8', 'lstr16be', 'lstr16le', 'lstr32be', 'lstr32le',
		'bool', 'raw',
	];
	const Attributes = {
		'fstring': [ 'length' ],
		'raw':     [ 'size' ],
	};

	for (const key of Object.keys(desc)) {
		let fieldDesc = desc[key];
		let fieldType = fieldDesc.type;
		if (!from.Array(FieldTypes).any(it => it === fieldType))
			throw new TypeError(`invalid field type '${fieldType}'`);
		if (fieldType in Attributes) {
			let haveAttributes = from.Array(Attributes[fieldType])
				.all(it => it in fieldDesc);
			if (!haveAttributes)
				throw new TypeError(`missing attributes for '${fieldType}'`);
		}
	}
}

// CommonJS
exports = module.exports = DataReader;
Object.assign(exports, {
	__esModule: true,
	DataReader: DataReader,
	default:    DataReader,
});
