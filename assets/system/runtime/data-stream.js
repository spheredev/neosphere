/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2018, Fat Cerberus
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

export default
class DataStream extends FileStream
{
	get [Symbol.toStringTag]() { return 'DataStream'; }

	static async open(fileName, fileOp)
	{
		let fs = await FileStream.open(fileName, fileOp);
		Object.setPrototypeOf(fs, this.prototype);
		fs._textDecoder = new TextDecoder('utf-8');
		fs._textEncoder = new TextEncoder();
		return fs;
	}

	constructor(fileName, fileOp)
	{
		super(fileName, fileOp);

		this._textDecoder = new TextDecoder('utf-8');
		this._textEncoder = new TextEncoder();
	}

	readFloat32(littleEndian)
	{
		let view = new DataView(this.read(4));
		return view.getFloat32(0, littleEndian);
	}

	readFloat64(littleEndian)
	{
		let view = new DataView(this.read(8));
		return view.getFloat64(0, littleEndian);
	}

	readInt8()
	{
		return readInteger(this, 1, true);
	}

	readInt16(littleEndian)
	{
		return readInteger(this, 2, true, littleEndian);
	}

	readInt32(littleEndian)
	{
		return readInteger(this, 4, true, littleEndian);
	}

	readStringRaw(length)
	{
		let bytes = this.read(length);
		let string = this._textDecoder.decode(bytes);
		let nulIndex = string.indexOf('\0');
		if (nulIndex !== -1)
			return string.substring(0, nulIndex);
		else
			return string;
	}

	readString8()
	{
		let length = readInteger(this, 1, false);
		return this.readStringRaw(length);
	}

	readString16(littleEndian)
	{
		let length = readInteger(this, 2, false, littleEndian);
		return this.readStringRaw(length);
	}

	readString32(littleEndian)
	{
		let length = readInteger(this, 4, false, littleEndian);
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
				case 'bool': value = this.readUint8() !== 0; break;
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
				case 'raw': value = this.read(fieldDesc.size); break;
			}
			object[key] = value;
		}
		return object;
	}

	readUint8()
	{
		return readInteger(this, 1, false);
	}

	readUint16(littleEndian)
	{
		return readInteger(this, 2, false, littleEndian);
	}

	readUint32(littleEndian)
	{
		return readInteger(this, 4, false, littleEndian);
	}

	writeFloat32(value, littleEndian)
	{
		let dv = new DataView(new ArrayBuffer(4));
		dv.setFloat32(0, value, littleEndian);
		this.write(dv.buffer);
	}

	writeFloat64(value, littleEndian)
	{
		let dv = new DataView(new ArrayBuffer(8));
		dv.setFloat64(0, value, littleEndian);
		this.write(dv.buffer);
	}

	writeInt8(value)
	{
		let dv = new DataView(new ArrayBuffer(1));
		dv.setInt8(0, value);
		this.write(dv.buffer);
	}

	writeInt16(value, littleEndian)
	{
		let dv = new DataView(new ArrayBuffer(2));
		dv.setInt16(0, value, littleEndian);
		this.write(dv.buffer);
	}

	writeInt32(value, littleEndian)
	{
		let dv = new DataView(new ArrayBuffer(4));
		dv.setInt32(0, value, littleEndian);
		this.write(dv.buffer);
	}

	writeStringRaw(value, length)
	{
		let encoded = this._textEncoder.encode(value);
		let bytes = new Uint8Array(length);
		bytes.set(encoded.subarray(0, length));
		this.write(bytes);
	}

	writeString8(value)
	{
		let bytes = this._textEncoder.encode(value);
		this.writeUint8(bytes.length);
		this.write(bytes);
	}

	writeString16(value, littleEndian)
	{
		let bytes = this._textEncoder.encode(value);
		this.writeUint16(bytes.length, littleEndian);
		this.write(bytes);
	}

	writeString32(value, littleEndian)
	{
		let bytes = this._textEncoder.encode(value);
		this.writeUint32(bytes.length, littleEndian);
		this.write(bytes);
	}

	writeStruct(object, desc)
	{
		verifyStructDescriptor(desc);

		for (const key of Object.keys(desc)) {
			let value = key in object ? object[key] : desc[key].default;
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
				case 'raw': this.write(value); break;
			}
		}
	}

	writeUint8(value)
	{
		let dv = new DataView(new ArrayBuffer(1));
		dv.setUint8(0, value);
		this.write(dv.buffer);
	}

	writeUint16(value, littleEndian)
	{
		let dv = new DataView(new ArrayBuffer(2));
		dv.setUint16(0, value, littleEndian);
		this.write(dv.buffer);
	}

	writeUint32(value, littleEndian)
	{
		let dv = new DataView(new ArrayBuffer(4));
		dv.setUint32(0, value, littleEndian);
		this.write(dv.buffer);
	}
}

function readInteger(stream, size, signed, littleEndian)
{
	// variable-size two's complement integer decoding algorithm jacked from
	// Node.js.  this allows us to read integer values up to 48 bits in size.

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
		if (!FieldTypes.some(it => fieldType === it))
			throw new TypeError(`Invalid field type '${fieldType}'`);
		if (fieldType in Attributes) {
			if (!Attributes[fieldType].every(it => it in fieldDesc))
				throw new TypeError(`Missing attributes for '${fieldType}'`);
		}
	}
}
