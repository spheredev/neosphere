/**
 *  Sphere Runtime for Sphere games
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
class Logger
{
	get [Symbol.toStringTag]() { return 'Logger'; }

	constructor(fileName)
	{
		let fullPath = FS.fullPath(fileName, '~/logFiles');
		if (fullPath.substr(0, 2) !== fileName.substr(0, 2))  // SphereFS prefix?
			fullPath += '.log';

		this._textEncoder = new TextEncoder();
		this._stream = new FileStream(fullPath, FileOp.Update);
		this._groups = [];

		let timestamp = new Date().toISOString();
		let logLine = `LOG FILE OPENED: ${timestamp}`;
		this._stream.write(this._textEncoder.encode(`\n${logLine}\n`));
	}

	beginGroup(text)
	{
		text = text.toString();
		this.write(`BEGIN: ${text}`);
		this._groups.push(text);
	}

	endGroup()
	{
		let groupName = this._groups.pop();
		this.write(`END: ${groupName}`);
	}

	write(text)
	{
		text = text.toString();
		let timestamp = new Date().toISOString();
		this._stream.write(this._textEncoder.encode(`${timestamp} .. `));
		for (let i = 0; i < this._groups.length; ++i)
			this._stream.write(this._textEncoder.encode("  "));
		this._stream.write(this._textEncoder.encode(`${text}\n`));
	}
}
