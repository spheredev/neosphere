/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2025, Where'd She Go? LLC
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
 *  * Neither the name of Spherical nor the names of its contributors may be
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
class DataStream
{
	static async fromFile(fileName)
	{
		const buffer = Sphere.APILevel >= 4
			? await File.load(fileName, DataType.Raw)
			: FS.readFile(fileName, DataType.Raw);
		return new DataStream(buffer);
	}

	constructor(buffer)
	{
		this.textDec = new TextDecoder();
		if (ArrayBuffer.isView(buffer))
			this.bytes = new Uint8Array(buffer.buffer);
		else
			this.bytes = new Uint8Array(buffer);
		this.view = new DataView(this.bytes.buffer);
		this.ptr = 0;
	}

	get atEOF()
	{
		return this.ptr >= this.bytes.length;
	}

	get bufferSize()
	{
		return this.bytes.length;
	}

	get position()
	{
		return this.ptr;
	}

	set position(value)
	{
		if (value > this.bytes.length)
			throw RangeError(`Stream position '${value}' is out of range`);
		this.ptr = value;
	}

	readBytes(numBytes)
	{
		if (this.ptr + numBytes > this.bytes.length)
			throw Error(`Unable to read ${numBytes} bytes from stream`);
		const bytes = this.bytes.slice(this.ptr, this.ptr + numBytes);
		this.ptr += numBytes;
		return bytes;
	}

	readFloat32(littleEndian = false)
	{
		if (this.ptr + 4 > this.bytes.length)
			throw Error(`Unable to read 32-bit float from stream`);
		const value = this.view.getFloat32(this.ptr, littleEndian);
		this.ptr += 4;
		return value;
	}

	readFloat64(littleEndian = false)
	{
		if (this.ptr + 8 > this.bytes.length)
			throw Error(`Unable to read 64-bit float from stream`);
		const value = this.view.getFloat64(this.ptr, littleEndian);
		this.ptr += 8;
		return value;
	}

	readInt8()
	{
		if (this.ptr + 1 > this.bytes.length)
			throw Error(`Unable to read 8-bit signed integer from stream`);
		return this.view.getInt8(this.ptr++);
	}

	readInt16(littleEndian = false)
	{
		if (this.ptr + 2 > this.bytes.length)
			throw Error(`Unable to read 16-bit signed integer from stream`);
		const value = this.view.getInt16(this.ptr, littleEndian);
		this.ptr += 2;
		return value;
	}

	readInt32(littleEndian = false)
	{
		if (this.ptr + 4 > this.bytes.length)
			throw Error(`Unable to read 32-bit signed integer from stream`);
		const value = this.view.getInt32(this.ptr, littleEndian);
		this.ptr += 4;
		return value;
	}

	readString(numBytes, stripNUL = false)
	{
		if (this.ptr + numBytes > this.bytes.length)
			throw Error(`Unable to read ${numBytes}-byte string from stream`);
		const slice = this.bytes.subarray(this.ptr, this.ptr + numBytes);
		let value = this.textDec.decode(slice);
		if (stripNUL && value.endsWith('\0'))
			value = value.slice(0, -1);
		this.ptr += numBytes;
		return value;
	}

	readLPStr8(stripNUL = false)
	{
		const length = this.readUint8();
		return this.readString(length, stripNUL);
	}

	readLPStr16(littleEndian = false, stripNUL = false)
	{
		const length = this.readUint16(littleEndian);
		return this.readString(length, stripNUL);
	}

	readLPStr32(littleEndian = false, stripNUL = false)
	{
		const length = this.readUint32(littleEndian);
		return this.readString(length, stripNUL);
	}

	readStruct(manifest)
	{
		let retval = {};
		for (const key of Object.keys(manifest)) {
			const matches = manifest[key].match(/(string|cstring|raw|nil)\/([0-9]*)/);
			const valueType = matches !== null ? matches[1] : manifest[key];
			const numBytes = matches !== null ? parseInt(matches[2], 10) : 0;
			switch (valueType) {
				case 'bool':
					retval[key] = this.readUint8() !== 0;
				case 'cstring':
					retval[key] = this.readString(numBytes, true);
					break;
				case 'cstr8': case 'cstr8-be': case 'cstr8-le':
					retval[key] = this.readLPStr8(true);
					break;
				case 'cstr16-be':
					retval[key] = this.readLPStr16(false, true);
					break;
				case 'cstr16-le':
					retval[key] = this.readLPStr16(true, true);
					break;
				case 'cstr32-be':
					retval[key] = this.readLPStr32(false, true);
					break;
				case 'cstr32-le':
					retval[key] = this.readLPStr32(true, true);
					break;
				case 'float32-be':
					retval[key] = this.readFloat32();
					break;
				case 'float32-le':
					retval[key] = this.readFloat32(true);
					break;
				case 'float64-be':
					retval[key] = this.readFloat64();
					break;
				case 'float64-le':
					retval[key] = this.readFloat64(true);
					break;
				case 'int8': case 'int8-be': case 'int8-le':
					retval[key] = this.readInt8();
					break;
				case 'int16-be':
					retval[key] = this.readInt16();
					break;
				case 'int16-le':
					retval[key] = this.readInt16(true);
					break;
				case 'int32-be':
					retval[key] = this.readInt32();
					break;
				case 'int32-le':
					retval[key] = this.readInt32(true);
					break;
				case 'lpstr8': case 'lpstr8-be': case 'lpstr8-le':
					retval[key] = this.readLPStr8();
					break;
				case 'lpstr16-be':
					retval[key] = this.readLPStr16();
					break;
				case 'lpstr16-le':
					retval[key] = this.readLPStr16(true);
					break;
				case 'lpstr32-be':
					retval[key] = this.readLPStr32();
					break;
				case 'lpstr32-le':
					retval[key] = this.readLPStr32(true);
					break;
				case 'nil':
					retval[key] = null;
					this.skipAhead(numBytes);
					break;
				case 'raw':
					retval[key] = this.readBytes(numBytes).buffer;
					break;
				case 'string':
					retval[key] = this.readString(numBytes);
					break;
				case 'uint8': case 'uint8-be': case 'uint8-le':
					retval[key] = this.readUint8();
					break;
				case 'uint16-be':
					retval[key] = this.readUint16();
					break;
				case 'uint16-le':
					retval[key] = this.readUint16(true);
					break;
				case 'uint32-be':
					retval[key] = this.readUint32();
					break;
				case 'uint32-le':
					retval[key] = this.readUint32(true);
					break;
				default:
					throw RangeError(`Unknown readStruct() value type '${valueType}'`);
			}
		}
		return retval;
	}

	readUint8()
	{
		if (this.ptr + 1 > this.bytes.length)
			throw Error(`Unable to read 8-bit unsigned integer from stream`);
		return this.view.getUint8(this.ptr++);
	}

	readUint16(littleEndian = false)
	{
		if (this.ptr + 2 > this.bytes.length)
			throw Error(`Unable to read 16-bit unsigned integer from stream`);
		const value = this.view.getUint16(this.ptr, littleEndian);
		this.ptr += 2;
		return value;
	}

	readUint32(littleEndian = false)
	{
		if (this.ptr + 4 > this.bytes.length)
			throw Error(`Unable to read 32-bit unsigned integer from stream`);
		const value = this.view.getUint32(this.ptr, littleEndian);
		this.ptr += 4;
		return value;
	}

	skipAhead(numBytes)
	{
		if (this.ptr + numBytes > this.bytes.length)
			throw Error(`Cannot read ${numBytes} bytes from stream`);
		this.ptr += numBytes;
	}
}
