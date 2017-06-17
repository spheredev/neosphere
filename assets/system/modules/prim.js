/**
 *	miniRT prim CommonJS module
 *	(c) 2015-2017 Fat Cerberus
**/

'use strict';
exports.__esModule = true;
exports.default = exports;

exports.blit = blit;
function blit(surface, x, y, texture, mask)
{
	mask = mask || Color.White;

	var x1 = x;
	var y1 = y;
	var x2 = x1 + texture.width;
	var y2 = y1 + texture.height;
	var shape = new Shape([
		{ x: x1, y: y1, u: 0.0, v: 1.0, color: mask },
		{ x: x2, y: y1, u: 1.0, v: 1.0, color: mask },
		{ x: x1, y: y2, u: 0.0, v: 0.0, color: mask },
		{ x: x2, y: y2, u: 1.0, v: 0.0, color: mask },
	], texture, ShapeType.TriStrip);
	shape.draw(surface);
}

exports.circle = circle;
function circle(surface, x, y, radius, color, color2)
{
	ellipse(surface, x, y, radius, radius, color, color2);
}

exports.ellipse = ellipse;
function ellipse(surface, x, y, rx, ry, color, color2)
{
	color2 = color2 || color;

	var numSegments = Math.ceil(10 * Math.sqrt((rx + ry) / 2.0));
	var vlist = [ { x: x, y: y, color: color } ];
	var pi2 = 2 * Math.PI;
	var cos = Math.cos;
	var sin = Math.sin;
	for (var i = 0; i < numSegments; ++i) {
		var phi = pi2 * i / numSegments;
		var c = cos(phi);
		var s = sin(phi);
		vlist[i + 1] = {
			x: x + c * rx,
			y: y - s * ry,
			color: color2
		};
	}
	vlist[numSegments + 1] = {
		x: x + rx,  // cos(0) = 1.0
		y: y,       // sin(0) = 0.0
		color: color2
	};

	var shape = new Shape(vlist, null, ShapeType.Fan);
	shape.draw(surface);
}

exports.fill = fill;
function fill(surface, color)
{
	rect(surface, 0, 0, surface.width, surface.height, color);
}

exports.line = line;
function line(surface, x1, y1, x2, y2, thickness, color1, color2)
{
	color2 = color2 || color1;

	var shape;
	var xSize = x2 - x1;
	var ySize = y2 - y1;
	var length = Math.sqrt(xSize*xSize + ySize*ySize);
	if (length == 0.0)
		return;
	var tx = 0.5 * thickness * (y2 - y1) / length;
	var ty = 0.5 * thickness * -(x2 - x1) / length;
	shape = new Shape([
		{ x: x1 + tx, y: y1 + ty, color: color1 },
		{ x: x1 - tx, y: y1 - ty, color: color1 },
		{ x: x2 - tx, y: y2 - ty, color: color2 },
		{ x: x2 + tx, y: y2 + ty, color: color2 },
	], null, ShapeType.Fan);
	shape.draw(surface);
}

exports.lineCircle = lineCircle;
function lineCircle(surface, x, y, radius, color)
{
	lineEllipse(surface, x, y, radius, radius, color);
}

exports.lineEllipse = lineEllipse;
function lineEllipse(surface, x, y, rx, ry, color)
{
	var numSegments = Math.ceil(10 * Math.sqrt((rx + ry) / 2.0));
	var vlist = [];
	var pi2 = 2 * Math.PI;
	var cos = Math.cos;
	var sin = Math.sin;
	for (var i = 0; i < numSegments - 1; ++i) {
		var phi = pi2 * i / numSegments;
		var c = cos(phi);
		var s = sin(phi);
		vlist.push({
			x: x + c * rx,
			y: y - s * ry,
			color: color
		});
	}
	var shape = new Shape(vlist, null, ShapeType.LineLoop)
	shape.draw(surface);
}

exports.lineRect = lineRect;
function lineRect(surface, x, y, width, height, thickness, color)
{
	var t = 0.5 * thickness;
	var x1 = x + t;
	var y1 = y + t;
	var x2 = x1 + width - thickness;
	var y2 = y1 + height - thickness;
	var shape = new Shape([
		{ x: x1 - t, y: y1 - t, color: color },
		{ x: x1 + t, y: y1 + t, color: color },
		{ x: x2 + t, y: y1 - t, color: color },
		{ x: x2 - t, y: y1 + t, color: color },
		{ x: x2 + t, y: y2 + t, color: color },
		{ x: x2 - t, y: y2 - t, color: color },
		{ x: x1 - t, y: y2 + t, color: color },
		{ x: x1 + t, y: y2 - t, color: color },
		{ x: x1 - t, y: y1 - t, color: color },
		{ x: x1 + t, y: y1 + t, color: color },
	], null, ShapeType.TriStrip);
	shape.draw(surface);
}

exports.point = point;
function point(surface, x, y, color)
{
	prim.rect(surface, x, y, 1, 1, color);
}

exports.rect = rect;
function rect(surface, x, y, width, height, color_ul, color_ur, color_lr, color_ll)
{
	color_ur = color_ur || color_ul;
	color_lr = color_lr || color_ul;
	color_ll = color_ll || color_ul;

	var shape = new Shape([
		{ x: x, y: y, color: color_ul },
		{ x: x + width, y: y, color: color_ur },
		{ x: x, y: y + height, color: color_ll },
		{ x: x + width, y: y + height, color: color_lr },
	], null, ShapeType.TriStrip);
	shape.draw(surface);
}

exports.triangle = triangle;
function triangle(surface, x1, y1, x2, y2, x3, y3, color1, color2, color3)
{
	color2 = color2 || color1;
	color3 = color3 || color1;

	var shape = new Shape([
		{ x: x1, y: y1, color: color1 },
		{ x: x2, y: y2, color: color2 },
		{ x: x3, y: y3, color: color3 },
	], null, ShapeType.Triangles);
}
