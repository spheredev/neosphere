/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2022, Fat Cerberus
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

export default
class Prim extends null
{
	static blit(surface, x, y, texture, mask)
	{
		Prim.blitSection(surface, x, y, texture, 0, 0, texture.width, texture.height, mask);
	}

	static blitSection(surface, x, y, texture, sx, sy, width, height, mask = Color.White)
	{
		let x1 = x;
		let y1 = y;
		let x2 = x1 + width;
		let y2 = y1 + height;
		let u1 = sx / texture.width;
		let v1 = 1.0 - sy / texture.height;
		let u2 = (sx + width) / texture.width;
		let v2 = 1.0 - (sy + height) / texture.height;
		drawTexturedShape(surface, ShapeType.TriStrip, texture, [
			{ x: x1, y: y1, u: u1, v: v1, color: mask },
			{ x: x2, y: y1, u: u2, v: v1, color: mask },
			{ x: x1, y: y2, u: u1, v: v2, color: mask },
			{ x: x2, y: y2, u: u2, v: v2, color: mask },
		]);
	}

	static drawCircle(surface, x, y, radius, color)
	{
		Prim.drawEllipse(surface, x, y, radius, radius, color);
	}

	static drawEllipse(surface, x, y, rx, ry, color)
	{
		let numSegments = Math.ceil(10 * Math.sqrt((rx + ry) / 2.0));
		let vertices = [];
		let tau = 2 * Math.PI;
		let cos = Math.cos;
		let sin = Math.sin;
		for (let i = 0; i < numSegments - 1; ++i) {
			let phi = tau * i / numSegments;
			let c = cos(phi);
			let s = sin(phi);
			vertices.push({
				x: x + c * rx,
				y: y - s * ry,
				color: color,
			});
		}
		drawShape(surface, ShapeType.LineLoop, vertices);
	}

	static drawSolidCircle(surface, x, y, radius, color, color2)
	{
		Prim.drawSolidEllipse(surface, x, y, radius, radius, color, color2);
	}

	static drawSolidEllipse(surface, x, y, rx, ry, color, color2)
	{
		color2 = color2 || color;

		let numSegments = Math.ceil(10 * Math.sqrt((rx + ry) / 2.0));
		let vertices = [ { x: x, y: y, color: color } ];
		let tau = 2 * Math.PI;
		let cos = Math.cos;
		let sin = Math.sin;
		for (let i = 0; i < numSegments; ++i) {
			let phi = tau * i / numSegments;
			let c = cos(phi);
			let s = sin(phi);
			vertices[i + 1] = {
				x: x + c * rx,
				y: y - s * ry,
				color: color2,
			};
		}
		vertices[numSegments + 1] = {
			x: x + rx,  // cos(0) = 1.0
			y: y,       // sin(0) = 0.0
			color: color2,
		};

		drawShape(surface, ShapeType.Fan, vertices);
	}

	static drawSolidRectangle(surface, x, y, width, height, color_ul, color_ur, color_lr, color_ll)
	{
		color_ur = color_ur || color_ul;
		color_lr = color_lr || color_ul;
		color_ll = color_ll || color_ul;

		drawShape(surface, ShapeType.TriStrip, [
			{ x: x, y: y, color: color_ul },
			{ x: x + width, y: y, color: color_ur },
			{ x: x, y: y + height, color: color_ll },
			{ x: x + width, y: y + height, color: color_lr },
		]);
	}

	static drawSolidTriangle(surface, x1, y1, x2, y2, x3, y3, color1, color2, color3)
	{
		color2 = color2 || color1;
		color3 = color3 || color1;

		drawShape(surface, ShapeType.Triangles, [
			{ x: x1, y: y1, color: color1 },
			{ x: x2, y: y2, color: color2 },
			{ x: x3, y: y3, color: color3 },
		]);
	}

	static drawLine(surface, x1, y1, x2, y2, thickness, color1, color2)
	{
		color2 = color2 || color1;

		let xSize = x2 - x1;
		let ySize = y2 - y1;
		let length = Math.sqrt(xSize * xSize + ySize * ySize);
		if (length === 0.0)
			return;
		let tx = 0.5 * thickness * (y2 - y1) / length;
		let ty = 0.5 * thickness * -(x2 - x1) / length;
		drawShape(surface, ShapeType.Fan, [
			{ x: x1 + tx, y: y1 + ty, color: color1 },
			{ x: x1 - tx, y: y1 - ty, color: color1 },
			{ x: x2 - tx, y: y2 - ty, color: color2 },
			{ x: x2 + tx, y: y2 + ty, color: color2 },
		]);
	}

	static drawPoint(surface, x, y, color)
	{
		Prim.drawSolidRectangle(surface, x, y, 1, 1, color);
	}

	static drawRectangle(surface, x, y, width, height, thickness, color)
	{
		let t = 0.5 * thickness;
		let x1 = x + t;
		let y1 = y + t;
		let x2 = x1 + width - thickness;
		let y2 = y1 + height - thickness;
		drawShape(surface, ShapeType.TriStrip, [
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
		]);
	}

	static fill(surface, color)
	{
		Prim.drawSolidRectangle(surface, 0, 0, surface.width, surface.height, color);
	}
}

function drawShape(surface, type, vertices)
{
	if (Sphere.APILevel >= 2) {
		Shape.drawImmediate(surface, type, vertices);
	}
	else {
		let vertexList = new VertexList(vertices);
		let shape = new Shape(type, vertexList);
		shape.draw(surface);
	}
}

function drawTexturedShape(surface, type, texture, vertices)
{
	if (Sphere.APILevel >= 2) {
		Shape.drawImmediate(surface, type, texture, vertices);
	}
	else {
		let vertexList = new VertexList(vertices);
		let shape = new Shape(type, texture, vertexList);
		shape.draw(surface);
	}
}
