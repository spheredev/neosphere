/**
 *  Oozaru: Sphere for the Web
 *  Copyright (c) 2016-2025, Where'd She Go? LLC
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

import { DataStream } from './data-stream.js';
import Fido from './fido.js';

export
class Package
{
	#dataStream;
	#toc = {};

	static async fromFile(url)
	{
		const buffer = await Fido.fetchData(url);
		const dataStream = new DataStream(buffer);
		return new Package(dataStream);
	}

	constructor(dataStream)
	{
		const spkHeader = dataStream.readStruct({
			signature: 'string/4',
			version:   'uint16-le',
			numFiles:  'uint32-le',
			tocOffset: 'uint32-le',
			reserved:  'reserve/2',
		});
		if (spkHeader.signature != '.spk')
			throw RangeError("Not a valid Sphere package file");
		if (spkHeader.version !== 1)
			throw RangeError(`Unsupported SPK format version '${spkHeader.version}'`);
		dataStream.position = spkHeader.tocOffset;
		for (let i = 0; i < spkHeader.numFiles; ++i) {
			const entry = dataStream.readStruct({
				version:    'uint16-le',
				nameLength: 'uint16-le',
				byteOffset: 'uint32-le',
				fileSize:   'uint32-le',
				byteSize:   'uint32-le',
			});
			if (entry.version !== 1)
				throw RangeError(`Unsupported SPK file record version '${entry.version}'`);
			const pathName = dataStream.readString(entry.nameLength);
			this.#toc[pathName] = {
				byteOffset: entry.byteOffset,
				byteLength: entry.byteLength,
				fileSize: entry.fileSize,
			};
		}
		this.#dataStream = dataStream;
	}

	dataOf(pathName)
	{
		if (!(pathName in this.#toc))
			throw Error(`File not found in Sphere package '${pathName}'`);
		const fileRecord = this.#toc[pathName];
		if (fileRecord.data === undefined) {
			if (fileRecord.byteLength !== fileRecord.fileSize)
				throw RangeError(`Compressed packages are currently unsupported`);
			this.#dataStream.position = fileRecord.byteOffset;
			const compressedData = this.#dataStream.readBytes(fileRecord.byteLength);
			fileRecord.data = compressedData.buffer;
		}
		return fileRecord.data;
	}
}
