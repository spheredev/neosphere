/**
 *	miniRT/prim CommonJS module
 *	allows pre-rendering of expensive-to-draw primitives
 *	(c) 2015-2016 Fat Cerberus
**/

if (typeof exports === 'undefined') {
	throw new TypeError("script must be loaded with require()");
}

module.exports =
{
    Circle: Circle,
    Text:   Text,
};

const cos = Math.cos;
const sin = Math.sin;

Circle.prototype = Object.create(Shape.prototype);
Circle.prototype.constructor = Circle;
function Circle(x, y, radius, color, color2)
{
	if (arguments.length < 5)
		color2 = color;

	var numSegments = Math.min(radius, 126);
	var vertices = [ { x: x, y: y, u: 0.5, v: 0.5, color: color } ];
	var pi2 = 2 * Math.PI;
	for (var i = 0; i < numSegments; ++i) {
		var phi = pi2 * i / numSegments;
		var c = cos(phi), s = sin(phi);
		vertices.push({
			x: x + c * radius,
			y: y - s * radius,
			u: (c + 1.0) / 2.0,
			v: (s + 1.0) / 2.0,
			color: color2,
		});
	}
	vertices.push({
		x: x + radius,  // cos(0) = 1.0
		y: y,           // sin(0) = 0.0
		u: 1.0, v: 0.5,
		color: color2,
	});
	
	var shape = new Shape(vertices, null, SHAPE_TRI_FAN);
	Object.setPrototypeOf(shape, Circle.prototype);
	return shape;
}

Text.prototype = Object.create(Shape.prototype);
Text.prototype.constructor = Text;
function Text(text, options)
{
	options = options != null ? options : {};

	var font = 'font' in options ? options.font : Font.Default;
	var color = 'color' in options ? options.color : new Color(255, 255, 255, 255);
	var shadow = 'shadow' in options ? options.shadow : 0;

	var shadowColor = new Color(0, 0, 0, color.alpha);
	var width = font.getStringWidth(text) + Math.abs(shadow);
	var height = font.height + Math.abs(shadow);
	var surface = new Surface(width, height);
	var lastColorMask = font.getColorMask();
	var offsetX = 0;
	var offsetY = 0;
	if (shadow > 0) {
		font.setColorMask(shadowColor);
		surface.drawText(font, shadow, shadow, text);
		font.setColorMask(color);
		surface.drawText(font, 0, 0, text);
	}
	else if (shadow < 0) {
		font.setColorMask(shadowColor);
		surface.drawText(font, 0, 0, text);
		font.setColorMask(color);
		surface.drawText(font, -shadow, -shadow, text);
		offsetX = shadow;
		offsetY = shadow;
	}
	else {
		font.setColorMask(color);
		surface.drawText(font, 0, 0, text);
	}
	font.setColorMask(lastColorMask);

	var image = surface.createImage();
	width = image.width;
	height = image.height;
	var shape = new Shape([
		{ x: 0, y: 0, u: 0.0, v: 1.0 },
		{ x: width, y: 0, u: 1.0, v: 1.0 },
		{ x: 0, y: height + 1, u: 0.0, v: 0.0 },
		{ x: width, y: height, u: 1.0, v: 0.0 },
	], image);
	Object.setPrototypeOf(shape, Text.prototype);
	return shape;
}
