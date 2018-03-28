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

import from from 'from';

const white = Color.White;
let color = null;

let imageShader = new Shader({
	fragmentFile: '#/shaders/image.frag.glsl',
	vertexFile:   '#/shaders/image.vert.glsl',
});

export default
class Image
{
	constructor(fileName)
	{
		let fullPath = FS.fullPath(fileName, '@/images');
		fullPath = from.array([ '', '.png', '.jpg', '.bmp' ])
			.select(suffix => `${fullPath}${suffix}`)
			.first(fileName => FS.fileExists(fileName));
		if (fullPath === undefined)
			throw new Error(`couldn't find image '${fileName}'`);

		let texture = new Texture(fullPath);
		let shape = new Shape(ShapeType.TriStrip, texture,
			new VertexList([
				{ x: 0,             y: 0,              u: 0, v: 1 },
				{ x: texture.width, y: 0,              u: 1, v: 1 },
				{ x: 0,             y: texture.height, u: 0, v: 0 },
				{ x: texture.width, y: texture.height, u: 1, v: 0 },
			]));

		this._x = 0;
		this._y = 0;

		this.model = new Model([ shape ], imageShader);
		this.transform = new Transform();
		this.model.transform = this.transform;
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

	blitTo(surface, x, y, tintColor = white)
	{
		if (color !== tintColor) {
			imageShader.setColorVector('tintColor', tintColor);
			color = tintColor;
		}
		if (this._x !== x || this._y !== y) {
			this.transform.translate(x - this._x, y - this._y);
			this._x = x;
			this._y = y;
		}
		this.model.draw(surface);
	}
}
