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
exports = module.exports = Image;
exports.__esModule = true;
exports.default = exports;

const from = require('from');

function Image(fileName)
{
	var fullPath = FS.fullPath(fileName, '@/images');
	fullPath = from([ '', 'png', 'jpg', 'bmp' ])
		.select(function(it) { return fullPath + '.' + it; })
		.first(function(it) { return FS.fileExists(it); })
	if (fullPath === undefined)
		throw new Error("couldn't find image '" + fileName + "'");
	this.texture = new Texture(fullPath);
	var shape = new Shape(ShapeType.TriStrip, this.texture,
		new VertexList([
			{ x: 0, y: 0, u: 0, v: 1 },
			{ x: 1, y: 0, u: 1, v: 1 },
			{ x: 0, y: 1, u: 0, v: 0 },
			{ x: 1, y: 1, u: 1, v: 0 },
		]));
	var tintShader = new Shader({
		fragment: '#/shaders/tintedImage.frag.glsl',
		vertex:   '#/shaders/tintedImage.vert.glsl',
	});
	this.model = new Model([ shape ], tintShader);
	
	Object.defineProperty(this, 'width', {
		writable: false, enumerable: false, configurable: true,
		value: this.texture.width,
	});
	Object.defineProperty(this, 'height', {
		writable: false, enumerable: false, configurable: true,
		value: this.texture.height,
	});
}

Image.prototype.blitTo = function blitTo(surface, x, y, tintColor)
{
	tintColor = tintColor || Color.White;

	this.model.transform = new Transform()
		.scale(this.texture.width, this.texture.height)
		.translate(x, y);
	this.model.setColorVector('tintColor', tintColor);
	this.model.draw(surface);
}
