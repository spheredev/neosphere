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
exports = module.exports = Logger;
exports.__esModule = true;
exports.default = exports;

function Logger(fileName)
{
	var fullPath = FS.fullPath(fileName, '~/logFiles');
	if (fullPath.substr(0, 2) != fileName.substr(0, 2))  // SphereFS prefix?
		fullPath += '.log';

	this.utf8 = new TextEncoder();
	this.stream = new FileStream(fullPath, FileOp.Update);
	this.groups = [];

	var timestamp = new Date().toISOString();
	var logLine = "LOG FILE OPENED: " + timestamp;
	this.stream.write(this.utf8.encode("\n" + logLine + "\n"));
}

Logger.prototype.beginGroup = function beginGroup(text)
{
	text = text.toString();

	this.write("BEGIN: " + text);
	this.groups.push(text);
};

Logger.prototype.endGroup = function endGroup()
{
	var groupName = this.groups.pop();
	this.write("END: " + groupName);
};

Logger.prototype.write = function write(text)
{
	text = text.toString();

	var timestamp = new Date().toISOString();
	this.stream.write(this.utf8.encode(timestamp + " .. "));
	for (var i = 0; i < this.groups.length; ++i)
		this.stream.write(_encoder.encode("  "));
	this.stream.write(this.utf8.encode(text + "\n"));
};
