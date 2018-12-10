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
class DataStream extends DataView
{
	get [Symbol.toStringTag]() { return 'DataStream'; }

	constructor(...args)
	{
		super(...args);
		this.bytes = new Uint8Array(this.buffer);
		this.ptr = 0;
		this.textDec = new TextDecoder();
		this.textEnc = new TextEncoder();
	}

	get canExtend()
	{
		return false;
	}

	get position()
	{
		return this.ptr;
	}
	set position(value)
	{
		if (value < 0 || value > this.bytes.length)
			throw new RangeError(`Attempt to advance beyond end of buffer`);
		this.ptr = value;
	}

	get size()
	{
		return this.byteLength;
	}

	read(numBytes)
	{
		const ptr = this.position;
		this.position += numBytes;
		return this.bytes.buffer.slice(ptr, ptr + numBytes);
	}

	readBytes(numBytes)
	{
		return new Uint8Array(this.read(numBytes));
	}

	readFloat32(littleEndian = false)
	{
		const ptr = this.position;
		this.position += 4;
		return this.getFloat32(ptr, littleEndian);
	}

	readFloat64(littleEndian = false)
	{
		const ptr = this.position;
		this.position += 8;
		return this.getFloat64(ptr, littleEndian);
	}

	readInt8()
	{
		const ptr = this.position;
		this.position += 1;
		return this.getInt8(ptr);
	}

	readInt16(littleEndian = false)
	{
		const ptr = this.position;
		this.position += 2;
		return this.getInt16(ptr, littleEndian);
	}

	readInt32(littleEndian = false)
	{
		const ptr = this.position;
		this.position += 4;
		return this.getInt32(ptr, littleEndian);
	}

	readString(numBytes)
	{
		const ptr = this.position;
		this.position += numBytes;
		const slice = this.bytes.subarray(ptr, ptr + numBytes);
		return this.textDec.decode(slice);
	}

	readString8()
	{
		const length = this.readUint8();
		return this.readString(length);
	}

	readString16(littleEndian = false)
	{
		const length = this.readUint16(littleEndian);
		return this.readString(length);
	}

	readString32(littleEndian = false)
	{
		const length = this.readUint32(littleEndian);
		return this.readString(length);
	}

	readStruct(manifest)
	{
		const retval = {};
		for (const key of Object.keys(manifest)) {
			const matches = manifest[key].match(/(string|pad)\/([0-9]*)/);
			const valueType = matches !== null ? matches[1] : manifest[key];
			const numBytes = matches !== null ? parseInt(matches[2], 10) : 0;
			switch (valueType) {
				case 'bool':
					retval[key] = this.readUint8() !== 0;
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
				case 'int8':
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
				case 'pad':
					retval[key] = null;
					this.position += numBytes;
					break;
				case 'string':
					retval[key] = this.readString(numBytes);
					break;
				case 'string8':
					retval[key] = this.readString8();
					break;
				case 'string16-be':
					retval[key] = this.readString16();
					break;
				case 'string16-le':
					retval[key] = this.readString16(true);
					break;
				case 'string32-be':
					retval[key] = this.readString32();
					break;
				case 'string32-le':
					retval[key] = this.readString32(true);
					break;
				case 'uint8':
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
					throw new RangeError(`Unknown element type '${valueType}'`);
			}
		}
		return retval;
	}

	readUint8()
	{
		const ptr = this.position;
		this.position += 1;
		return this.getUint8(ptr);
	}

	readUint16(littleEndian = false)
	{
		const ptr = this.position;
		this.position += 2;
		return this.getUint16(ptr, littleEndian);
	}

	readUint32(littleEndian = false)
	{
		const ptr = this.position;
		this.position += 4;
		return this.getUint32(ptr, littleEndian);
	}

	write(data)
	{
		const buffer = ArrayBuffer.isView(data) ? data.buffer : data;
		const payload = new Uint8Array(buffer);
		const ptr = this.position;
		this.position += buffer.byteLength;
		this.bytes.set(payload, ptr);
	}

	writeFloat32(value, littleEndian = false)
	{
		const ptr = this.position;
		this.position += 4;
		this.setFloat32(ptr, value, littleEndian);
	}

	writeFloat64(value, littleEndian = false)
	{
		const ptr = this.position;
		this.position += 8;
		this.setFloat64(ptr, value, littleEndian);
	}

	writeInt8(value)
	{
		const ptr = this.position;
		this.position += 1;
		this.setInt8(ptr, value);
	}

	writeInt16(value, littleEndian = false)
	{
		const ptr = this.position;
		this.position += 2;
		this.setInt16(ptr, value, littleEndian);
	}

	writeInt32(value, littleEndian = false)
	{
		const ptr = this.position;
		this.position += 4;
		this.setInt32(ptr, value, littleEndian);
	}

	writeString(value)
	{
		const bytes = this.textEnc.encode(value);
		this.writeBytes(bytes);
	}

	writeString8(value)
	{
		const bytes = this.textEnc.encode(value);
		this.writeUint8(bytes.length);
		this.writeBytes(bytes);
	}

	writeString16(value, littleEndian = false)
	{
		const bytes = this.textEnc.encode(value);
		this.writeUint16(bytes.length, littleEndian);
		this.writeBytes(bytes);
	}

	writeString32(value, littleEndian = false)
	{
		const bytes = this.textEnc.encode(value);
		this.writeUint32(bytes.length, littleEndian);
		this.writeBytes(bytes);
	}

	writeUint8(value)
	{
		const ptr = this.position;
		this.position += 1;
		this.setUint8(ptr, value);
	}

	writeUint16(value, littleEndian = false)
	{
		const ptr = this.position;
		this.position += 2;
		this.setUint16(ptr, value, littleEndian);
	}

	writeUint32(value, littleEndian = false)
	{
		const ptr = this.position;
		this.position += 4;
		this.setUint32(ptr, value, littleEndian);
	}
}
