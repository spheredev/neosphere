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
class BinaryCoder
{
	get [Symbol.toStringTag]() { return 'BinaryCoder'; }

	constructor(stream, options = {})
	{
		options = Object.assign({
			littleEndian: false,
			nulTermination : false,
		}, options);
		this._littleEndian = options.littleEndian;
		this._nulTermination = options.nulTermination;
		this._stream = stream;
		this._textDec = new TextDecoder();
		this._textEnc = new TextEncoder();
	}

	readBytes(numBytes)
	{
		return new Uint8Array(this._stream.read(numBytes));
	}

	readFloat32(littleEndian = this._littleEndian)
	{
		const data = this._stream.read(4);
		const dv = new DataView(data);
		return dv.getFloat32(0, littleEndian);
	}

	readFloat64(littleEndian = this._littleEndian)
	{
		const data = this._stream.read(8);
		const dv = new DataView(data);
		return dv.getFloat64(0, littleEndian);
	}

	readInt8()
	{
		const data = this._stream.read(1);
		const dv = new DataView(data);
		return dv.getInt8(0);
	}

	readInt16(littleEndian = this._littleEndian)
	{
		const data = this._stream.read(2);
		const dv = new DataView(data);
		return dv.getInt16(0, littleEndian);
	}

	readInt32(littleEndian = this._littleEndian)
	{
		const data = this._stream.read(4);
		const dv = new DataView(data);
		return dv.getInt32(0, littleEndian);
	}

	readString(numBytes, nulTerminate = this._nulTermination)
	{
		const data = this._stream.read(numBytes);
		let text = this._textDec.decode(data);
		if (nulTerminate) {
			const nulIndex = text.indexOf('\0');
			if (nulIndex !== -1)
				text = text.slice(0, nulIndex);
		}
		return text;
	}

	readString8(nulTerminate = this._nulTermination)
	{
		const length = this.readUint8(this._stream);
		return this.readString(this._stream, length, nulTerminate);
	}

	readString16(nulTerminate = this._nulTermination, littleEndian = this._littleEndian)
	{
		const length = this.readUint16(this._stream, littleEndian);
		return this.readString(this._stream, length, nulTerminate);
	}

	readString32(nulTerminate = this._nulTermination, littleEndian = this._littleEndian)
	{
		const length = this.readUint32(this._stream, littleEndian);
		return this.readString(this._stream, length, nulTerminate);
	}

	readStruct(manifest)
	{
		const retval = {};
		for (const key of Object.keys(manifest)) {
			const matches = manifest[key].match(/(string(?:-n?z)?|raw)\/([0-9]*)/);
			const type = matches !== null ? matches[1] : manifest[key];
			const numBytes = matches !== null ? parseInt(matches[2], 10) : 0;
			const value = type === 'bool' ? this.readUint8(this._stream) !== 0
				: type === 'float32' ? this.readFloat32(this._stream)
				: type === 'float32-be' ? this.readFloat32(this._stream, false)
				: type === 'float32-le' ? this.readFloat32(this._stream, true)
				: type === 'float64' ? this.readFloat64(this._stream)
				: type === 'float64-be' ? this.readFloat64(this._stream, false)
				: type === 'float64-le' ? this.readFloat64(this._stream, true)
				: type === 'int8' ? this.readInt8(this._stream)
				: type === 'int16' ? this.readInt16(this._stream)
				: type === 'int16-be' ? this.readInt16(this._stream, false)
				: type === 'int16-le' ? this.readInt16(this._stream, true)
				: type === 'int32' ? this.readInt32(this._stream)
				: type === 'int32-be' ? this.readInt32(this._stream, false)
				: type === 'int32-le' ? this.readInt32(this._stream, true)
				: type === 'raw' ? this._stream.read(numBytes)
				: type === 'string' ? this.readString(this._stream, numBytes)
				: type === 'string-nz' ? this.readString(this._stream, numBytes, false)
				: type === 'string-z' ? this.readString(this._stream, numBytes, true)
				: type === 'string8' ? this.readString8(this._stream)
				: type === 'string8-nz' ? this.readString8(this._stream, false)
				: type === 'string8-z' ? this.readString8(this._stream, true)
				: type === 'string16' ? this.readString16(this._stream)
				: type === 'string16-be' ? this.readString16(this._stream, this._nulTermination, false)
				: type === 'string16-le' ? this.readString16(this._stream, this._nulTermination, true)
				: type === 'string16-nz' ? this.readString16(this._stream, false)
				: type === 'string16-nz-be' ? this.readString16(this._stream, false, false)
				: type === 'string16-nz-le' ? this.readString16(this._stream, false, true)
				: type === 'string16-z' ? this.readString16(this._stream, true)
				: type === 'string16-z-be' ? this.readString16(this._stream, true, false)
				: type === 'string16-z-le' ? this.readString16(this._stream, true, true)
				: type === 'string32' ? this.readString32(this._stream)
				: type === 'string32-be' ? this.readString32(this._stream, this._nulTermination, false)
				: type === 'string32-le' ? this.readString32(this._stream, this._nulTermination, true)
				: type === 'string32-nz' ? this.readString32(this._stream, false)
				: type === 'string32-nz-be' ? this.readString32(this._stream, false, false)
				: type === 'string32-nz-le' ? this.readString32(this._stream, false, true)
				: type === 'string32-z' ? this.readString32(this._stream, true)
				: type === 'string32-z-be' ? this.readString32(this._stream, true, false)
				: type === 'string32-z-le' ? this.readString32(this._stream, true, true)
				: type === 'uint8' ? this.readUint8(this._stream)
				: type === 'uint16' ? this.readUint16(this._stream)
				: type === 'uint16-be' ? this.readUint16(this._stream, false)
				: type === 'uint16-le' ? this.readUint16(this._stream, true)
				: type === 'uint32' ? this.readUint32(this._stream)
				: type === 'uint32-be' ? this.readUint32(this._stream, false)
				: type === 'uint32-le' ? this.readUint32(this._stream, true)
				: undefined;
			if (value === undefined)
				throw new RangeError(`Unknown element type '${type}'`);
			retval[key] = value;
		}
		return retval;
	}

	readUint8()
	{
		const data = this._stream.read(1);
		const dv = new DataView(data);
		return dv.getUint8(0);
	}

	readUint16(littleEndian = this._littleEndian)
	{
		const data = this._stream.read(2);
		const dv = new DataView(data);
		return dv.getUint16(0, littleEndian);
	}

	readUint32(littleEndian = this._littleEndian)
	{
		const data = this._stream.read(4);
		const dv = new DataView(data);
		return dv.getUint32(0, littleEndian);
	}

	writeFloat32(value, littleEndian = this._littleEndian)
	{
		const dv = new DataView(new ArrayBuffer(4));
		dv.setFloat32(0, value, littleEndian);
		this._stream.write(dv);
	}

	writeFloat64(value, littleEndian = this._littleEndian)
	{
		const dv = new DataView(new ArrayBuffer(8));
		dv.setFloat64(0, value, littleEndian);
		this._stream.write(dv);
	}

	writeInt8(value)
	{
		const dv = new DataView(new ArrayBuffer(1));
		dv.setInt8(0, value);
		this._stream.write(dv);
	}

	writeInt16(value, littleEndian = this._littleEndian)
	{
		const dv = new DataView(new ArrayBuffer(2));
		dv.setInt16(0, value, littleEndian);
		this._stream.write(dv);
	}

	writeInt32(value, littleEndian = this._littleEndian)
	{
		const dv = new DataView(new ArrayBuffer(4));
		dv.setInt32(0, value, littleEndian);
		this._stream.write(dv);
	}

	writeString(value, nulTerminate = this._nulTermination)
	{
		const bytes = this._textEnc.encode(value);
		const length = nulTerminate ? bytes.length + 1 : bytes.length;
		this._stream.write(bytes);
		if (nulTerminate)
			this.writeUint8(this._stream, 0);
		return length;
	}

	writeString8(value, nulTerminate = this._nulTermination)
	{
		const bytes = this._textEnc.encode(value);
		const length = nulTerminate ? bytes.length + 1 : bytes.length;
		this.writeUint8(this._stream, length);
		this._stream.write(bytes);
		if (nulTerminate)
			this.writeUint8(this._stream, 0);
	}

	writeString16(value, nulTerminate = this._nulTermination, littleEndian = this._littleEndian)
	{
		const bytes = this._textEnc.encode(value);
		const length = nulTerminate ? bytes.length + 1 : bytes.length;
		this.writeUint16(this._stream, length, littleEndian);
		this._stream.write(bytes);
		if (nulTerminate)
			this.writeUint8(this._stream, 0);
	}

	writeString32(value, nulTerminate = this._nulTermination, littleEndian = this._littleEndian)
	{
		const bytes = this._textEnc.encode(value);
		const length = nulTerminate ? bytes.length + 1 : bytes.length;
		this.writeUint32(this._stream, length, littleEndian);
		this._stream.write(bytes);
		if (nulTerminate)
			this.writeUint8(this._stream, 0);
	}

	writeUint8(value)
	{
		const dv = new DataView(new ArrayBuffer(1));
		dv.setUint8(0, value);
		this._stream.write(dv);
	}

	writeUint16(value, littleEndian = this._littleEndian)
	{
		const dv = new DataView(new ArrayBuffer(2));
		dv.setUint16(0, value, littleEndian);
		this._stream.write(dv);
	}

	writeUint32(value, littleEndian = this._littleEndian)
	{
		const dv = new DataView(new ArrayBuffer(4));
		dv.setUint32(0, value, littleEndian);
		this._stream.write(dv);
	}
}
