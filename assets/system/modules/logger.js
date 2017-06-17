/***
 *   miniRT logger CommonJS module
  *  (c) 2015-2017 Fat Cerberus
***/

'use strict';
exports = module.exports = Logger;
exports.__esModule = true;
exports.default = exports;

function Logger(fileName)
{
	this.utf8 = new TextEncoder();
	this.stream = new FileStream(fileName, FileOp.Update);
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
