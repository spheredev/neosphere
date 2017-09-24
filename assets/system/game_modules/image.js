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
const from = require('from');

class Image
{
	constructor(fileName)
	{
		let fullPath = FS.fullPath(fileName, '@/images');
		fullPath = from([ '', '.png', '.jpg', '.bmp' ])
			.select(suffix => `${fullPath}${suffix}`)
			.first(fileName => FS.fileExists(fileName))
		if (fullPath === undefined)
			throw new Error(`couldn't find image '${fileName}'`);

		let texture = new Texture(fullPath);
		let shape = new Shape(ShapeType.TriStrip, texture,
			new VertexList([
				{ x: 0, y: 0, u: 0, v: 1 },
				{ x: 1, y: 0, u: 1, v: 1 },
				{ x: 0, y: 1, u: 0, v: 0 },
				{ x: 1, y: 1, u: 1, v: 0 },
			]));
		let tintShader = new Shader({
			fragment: '#/shaders/tintedImage.frag.glsl',
			vertex:   '#/shaders/tintedImage.vert.glsl'
		});

		this.model = new Model([ shape ], tintShader);
		this.texture = texture;
	}

	get height()
	{
		return this.texture.height;
	}

	get width()
	{
		return this.texture.width;
	}

	blitTo(surface, x, y, tintColor = Color.White)
	{
		this.model.transform = new Transform()
			.scale(this.texture.width, this.texture.height)
			.translate(x, y);
		this.model.setColorVector('tintColor', tintColor);
		this.model.draw(surface);
	}
}

// CommonJS
exports = module.exports = Image;
Object.assign(exports, {
	__esModule: true,
	default:    Image,
	Image:      Image,
});
