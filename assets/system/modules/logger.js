/***
 *   miniRT logger CommonJS module
  *  (c) 2015-2016 Fat Cerberus
***/

'use strict';

exports.Log = Log;
function Log(fileName)
{
	var _file = fs.open(fileName, 'at');
	var _groups = [];
	var timestamp = new Date().toISOString();
	_file.writeString("\nLOG FILE OPENED: " + timestamp + "\n");
	
	this.beginGroup = beginGroup;
	function beginGroup(text)
	{
		write("BEGIN: " + text.toString());
		_groups.push(text.toString());
	}
	
	this.endGroup = endGroup;
	function endGroup()
	{
		var groupName = _groups.pop();
		write("END: " + groupName);
	}
	
	this.write = write;
	function write(text)
	{
		var timestamp = new Date().toISOString();
		var line = text.toString() + "\n";
		_file.writeString(timestamp + " .. ");
		for (var i = 0; i < _groups.length; ++i)
			_file.writeString("  ");
		_file.writeString(text.toString() + "\n");
	}
}
