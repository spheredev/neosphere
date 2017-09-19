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

class DataWriter
{
	constructor(stream)
	{
		assert('write' in stream, "stream used with DataWriter must be have a write() method");

		this.textEncoder = new TextEncoder();
		this.stream = stream;
	}

	writeFloat32(value, littleEndian)
	{
		assert.equal(typeof value, 'number');

		let dv = new DataView(new ArrayBuffer(4));
		dv.setFloat32(0, value, littleEndian);
		this.stream.write(dv.buffer);
	}

	writeFloat64(value, littleEndian)
	{
		assert.equal(typeof value, 'number');

		let dv = new DataView(new ArrayBuffer(8));
		dv.setFloat64(0, value, littleEndian);
		this.stream.write(dv.buffer);
	}

	writeInt8(value)
	{
		assert.equal(typeof value, 'number');

		let dv = new DataView(new ArrayBuffer(1));
		dv.setInt8(0, value);
		this.stream.write(dv.buffer);
	}

	writeInt16(value, littleEndian)
	{
		assert.equal(typeof value, 'number');

		let dv = new DataView(new ArrayBuffer(2));
		dv.setInt16(0, value, littleEndian);
		this.stream.write(dv.buffer);
	}

	writeInt32(value, littleEndian)
	{
		assert.equal(typeof value, 'number');

		let dv = new DataView(new ArrayBuffer(4));
		dv.setInt32(0, value, littleEndian);
		this.stream.write(dv.buffer);
	}

	writeStringRaw(value, length)
	{
		assert.equal(typeof value, 'string');
		assert.equal(typeof length, 'number');

		let encoded = this.textEncoder.encode(value);
		let bytes = new Uint8Array(length);
		bytes.set(encoded.subarray(0, length));
		this.stream.write(bytes);
	}

	writeString8(value)
	{
		assert.equal(typeof value, 'string');

		let bytes = this.textEncoder.encode(value);
		this.writeUint8(bytes.length);
		this.stream.write(bytes);
	}

	writeString16(value, littleEndian)
	{
		assert.equal(typeof value, 'string');

		let bytes = this.textEncoder.encode(value);
		this.writeUint16(bytes.length, littleEndian);
		this.stream.write(bytes);
	}

	writeString32(value, littleEndian)
	{
		assert.equal(typeof value, 'string');

		let bytes = this.textEncoder.encode(value);
		this.writeUint32(bytes.length, littleEndian);
		this.stream.write(bytes);
	}

	writeStruct(object, desc)
	{
		verifyStructDescriptor(desc);

		for (const key of Object.keys(desc)) {
			let value = key in object ? object[key] : desc[key].default
			switch (desc[key].type) {
				case 'bool': this.writeUint8(value ? 1 : 0); break;
				case 'float32be': this.writeFloat32(value); break;
				case 'float32le': this.writeFloat32(value, true); break;
				case 'float64be': this.writeFloat64(value); break;
				case 'float64le': this.writeFloat64(value, true); break;
				case 'int8': this.writeInt8(value); break;
				case 'int16be': this.writeInt16(value); break;
				case 'int16le': this.writeInt16(value, true); break;
				case 'int32be': this.writeInt32(value); break;
				case 'int32le': this.writeInt32(value, true); break;
				case 'uint8': this.writeUint8(value); break;
				case 'uint16be': this.writeUint16(value); break;
				case 'uint16le': this.writeUint16(value, true); break;
				case 'uint32be': this.writeUint32(value); break;
				case 'uint32le': this.writeUint32(value, true); break;
				case 'fstring': this.writeStringRaw(value, desc[key].length); break;
				case 'lstr8': this.writeString8(value); break;
				case 'lstr16be': this.writeString16(value); break;
				case 'lstr16le': this.writeString16(value, true); break;
				case 'lstr32be': this.writeString32(value); break;
				case 'lstr32le': this.writeString32(value, true); break;
				case 'raw': this.stream.write(value); break;
			}
		}
	}

	writeUint8(value)
	{
		assert.equal(typeof value, 'number');

		let dv = new DataView(new ArrayBuffer(1));
		dv.setUint8(0, value);
		this.stream.write(dv.buffer);
	}

	writeUint16(value, littleEndian)
	{
		assert.equal(typeof value, 'number');

		let dv = new DataView(new ArrayBuffer(2));
		dv.setUint16(0, value, littleEndian);
		this.stream.write(dv.buffer);
	}

	writeUint32(value, littleEndian)
	{
		assert.equal(typeof value, 'number');

		let dv = new DataView(new ArrayBuffer(4));
		dv.setUint32(0, value, littleEndian);
		this.stream.write(dv.buffer);
	}
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
exports = module.exports = DataWriter;
Object.assign(exports, {
	__esModule: true,
	DataWriter: DataWriter,
	default:    DataWriter,
});
