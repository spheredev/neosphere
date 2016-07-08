'use strict';
module.exports = ObstructionMap;

function ObstructionMap()
{
	this.testLine = _testLine;
	this.testRect = _testRect;
	
	var _lines = [];
	
	function _addLine(line)
	{
		_lines.push(line);
	}
	
	function _testLine(line)
	{
		for (var i = 0; i < _lines.length; ++i) {
			if (doLinesIntersect(line, _lines[i]))
				return true;
		}
		return false;
	}
	
	function _testRect(rect)
	{
		return false;
	}
}

function doLinesIntersect(a, b)
{
	var q = (a.y1 - b.y1) * (b.x2 - b.x1) - (a.x1 - b.x1) * (b.y2 - b.y1);
	var d = (a.x2 - a.x1) * (b.y2 - b.y1) - (a.y2 - a.y1) * (b.x2 - b.x1);
	if (d == 0)
		return false;
	var r = q / d;
	q = (a.y1 - b.y1) * (a.x2 - a.x1) - (a.x1 - b.x1) * (a.y2 - a.y1);
	var s = q / d;
	return !(r < 0 || r > 1 || s < 0 || s > 1);
};
