/**
 *  Oozaru: Sphere for the Web
 *  Copyright (c) 2016-2025, Where'd She Go? LLC
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

import Fido from './fido.js';
import Game from './game.js';

export
const BlendOp =
{
	Default: 0,
	Add: 1,
	Average: 2,
	CopyAlpha: 3,
	CopyRGB: 4,
	Invert: 5,
	Multiply: 6,
	Replace: 7,
	Subtract: 8,
};

export
const ClipOp =
{
	Narrow: 'narrow',
	Override: 'override',
	Reset: 'reset',
}

export
const DepthOp =
{
	AlwaysPass: 0,
	Equal: 1,
	Greater: 2,
	GreaterOrEqual: 3,
	Less: 4,
	LessOrEqual: 5,
	NeverPass: 6,
	NotEqual: 7,
};

export
const ShapeType =
{
	Fan: 0,
	Lines: 1,
	LineLoop: 2,
	LineStrip: 3,
	Points: 4,
	Triangles: 5,
	TriStrip: 6,
};

var activeShader = null;
var activeSurface = null;
var backBuffer;
var defaultShader;
var flipShader;
var gl;
var immediateVL;

export default
class Galileo
{
	static async initialize(canvas)
	{
		const webGLContext = canvas.getContext('webgl', { alpha: false });
		if (webGLContext === null)
			throw Error(`Couldn't acquire a WebGL rendering context.`);
		gl = webGLContext;
		gl.clearColor(0.0, 0.0, 0.0, 1.0);
		gl.clearDepth(1.0);
		gl.blendEquation(gl.FUNC_ADD);
		gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
		gl.depthFunc(gl.ALWAYS);
		gl.enable(gl.BLEND);
		gl.enable(gl.DEPTH_TEST);
		gl.enable(gl.SCISSOR_TEST);

		defaultShader = await Shader.fromFiles({
			vertexFile: '#/default.vert.glsl',
			fragmentFile: '#/default.frag.glsl',
		});

		// TODO: see if there's a way to avoid needing a separate shader instance for flips.
		backBuffer = new Surface(canvas.width, canvas.height);
		flipShader = defaultShader.clone();

		Galileo.flip();
	}

	static draw(shapeType, vertexList, indexList, offset = 0, numVertices)
	{
		const drawMode = shapeType === ShapeType.Fan ? gl.TRIANGLE_FAN
			: shapeType === ShapeType.Lines ? gl.LINES
			: shapeType === ShapeType.LineLoop ? gl.LINE_LOOP
			: shapeType === ShapeType.LineStrip ? gl.LINE_STRIP
			: shapeType === ShapeType.Points ? gl.POINTS
			: shapeType === ShapeType.TriStrip ? gl.TRIANGLE_STRIP
			: gl.TRIANGLES;
		vertexList.activate();
		if (indexList != null) {
			if (numVertices === undefined)
				numVertices = indexList.length - offset;
			indexList.activate();
			gl.drawElements(drawMode, numVertices, gl.UNSIGNED_SHORT, offset);
		}
		else {
			if (numVertices === undefined)
				numVertices = vertexList.length - offset;
			gl.drawArrays(drawMode, offset, numVertices);
		}
	}

	static flip()
	{
		gl.bindFramebuffer(gl.FRAMEBUFFER, null);
		gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);
		gl.scissor(0, 0, gl.canvas.width, gl.canvas.height);
		gl.blendEquation(gl.FUNC_ADD);
		gl.blendFunc(gl.ONE, gl.ZERO);
		gl.depthFunc(gl.ALWAYS);
		flipShader.activate(true);
		backBuffer.useTexture(0);
		Galileo.draw(ShapeType.TriStrip, new VertexList([
			{ x: 0, y: 0, u: 0.0, v: 1.0 },
			{ x: gl.canvas.width, y: 0, u: 1.0, v: 1.0 },
			{ x: 0, y: gl.canvas.height, u: 0.0, v: 0.0 },
			{ x: gl.canvas.width, y: gl.canvas.height, u: 1.0, v: 0.0 },
		]));

		activeSurface = null;
		Surface.Screen.activate(defaultShader);
		Surface.Screen.clipTo(0, 0, Surface.Screen.width, Surface.Screen.height, ClipOp.Reset);
		gl.disable(gl.SCISSOR_TEST);
		gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
		gl.enable(gl.SCISSOR_TEST);
	}

	static rerez(width, height)
	{
		gl.canvas.width = width;
		gl.canvas.height = height;
		const oldBackBuffer = backBuffer;
		backBuffer = new Surface(width, height);
		flipShader.project(Transform.project2D(0, 0, width, height));
		if (activeSurface === oldBackBuffer) {
			gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);
			activeSurface = backBuffer;
		}
		if (width <= 400 && height <= 300) {
			gl.canvas.style.width = `${width * 2}px`;
			gl.canvas.style.height = `${height * 2}px`;
		}
		else {
			gl.canvas.style.width = `${width}px`;
			gl.canvas.style.height = `${height}px`;
		}
	}
}

export
class Color
{
	r;
	g;
	b;
	a;

	// note: predefined colors use `Color.fromRGBA()` rather than `new Color()` because this whole table was
	//       copied and pasted from the neoSphere codebase and I was too lazy to convert it.
	static get AliceBlue() { return Color.fromRGBA(240, 248, 255); }
	static get AntiqueWhite() { return Color.fromRGBA(250, 235, 215); }
	static get Aqua() { return Color.fromRGBA(0, 255, 255); }
	static get Aquamarine() { return Color.fromRGBA(127, 255, 212); }
	static get Azure() { return Color.fromRGBA(240, 255, 255); }
	static get Beige() { return Color.fromRGBA(245, 245, 220); }
	static get Bisque() { return Color.fromRGBA(255, 228, 196); }
	static get Black() { return Color.fromRGBA(0, 0, 0); }
	static get BlanchedAlmond() { return Color.fromRGBA(255, 235, 205); }
	static get Blue() { return Color.fromRGBA(0, 0, 255); }
	static get BlueViolet() { return Color.fromRGBA(138, 43, 226); }
	static get Brown() { return Color.fromRGBA(165, 42, 42); }
	static get BurlyWood() { return Color.fromRGBA(222, 184, 135); }
	static get CadetBlue() { return Color.fromRGBA(95, 158, 160); }
	static get Chartreuse() { return Color.fromRGBA(127, 255, 0); }
	static get Chocolate() { return Color.fromRGBA(210, 105, 30); }
	static get Coral() { return Color.fromRGBA(255, 127, 80); }
	static get CornflowerBlue() { return Color.fromRGBA(100, 149, 237); }
	static get Cornsilk() { return Color.fromRGBA(255, 248, 220); }
	static get CosmicLatte() { return Color.fromRGBA(255, 248, 231); }
	static get Crimson() { return Color.fromRGBA(220, 20, 60); }
	static get Cyan() { return Color.fromRGBA(0, 255, 255); }
	static get DarkBlue() { return Color.fromRGBA(0, 0, 139); }
	static get DarkCyan() { return Color.fromRGBA(0, 139, 139); }
	static get DarkGoldenrod() { return Color.fromRGBA(184, 134, 11); }
	static get DarkGray() { return Color.fromRGBA(169, 169, 169); }
	static get DarkGreen() { return Color.fromRGBA(0, 100, 0); }
	static get DarkKhaki() { return Color.fromRGBA(189, 183, 107); }
	static get DarkMagenta() { return Color.fromRGBA(139, 0, 139); }
	static get DarkOliveGreen() { return Color.fromRGBA(85, 107, 47); }
	static get DarkOrange() { return Color.fromRGBA(255, 140, 0); }
	static get DarkOrchid() { return Color.fromRGBA(153, 50, 204); }
	static get DarkRed() { return Color.fromRGBA(139, 0, 0); }
	static get DarkSalmon() { return Color.fromRGBA(233, 150, 122); }
	static get DarkSeaGreen() { return Color.fromRGBA(143, 188, 143); }
	static get DarkSlateBlue() { return Color.fromRGBA(72, 61, 139); }
	static get DarkSlateGray() { return Color.fromRGBA(47, 79, 79); }
	static get DarkTurquoise() { return Color.fromRGBA(0, 206, 209); }
	static get DarkViolet() { return Color.fromRGBA(148, 0, 211); }
	static get DeepPink() { return Color.fromRGBA(255, 20, 147); }
	static get DeepSkyBlue() { return Color.fromRGBA(0, 191, 255); }
	static get DimGray() { return Color.fromRGBA(105, 105, 105); }
	static get DodgerBlue() { return Color.fromRGBA(30, 144, 255); }
	static get EatyPink() { return Color.fromRGBA(253, 215, 228); }
	static get FireBrick() { return Color.fromRGBA(178, 34, 34); }
	static get FloralWhite() { return Color.fromRGBA(255, 250, 240); }
	static get ForestGreen() { return Color.fromRGBA(34, 139, 34); }
	static get Fuchsia() { return Color.fromRGBA(255, 0, 255); }
	static get Gainsboro() { return Color.fromRGBA(220, 220, 220); }
	static get GhostWhite() { return Color.fromRGBA(248, 248, 255); }
	static get Gold() { return Color.fromRGBA(255, 215, 0); }
	static get Goldenrod() { return Color.fromRGBA(218, 165, 32); }
	static get Gray() { return Color.fromRGBA(128, 128, 128); }
	static get Green() { return Color.fromRGBA(0, 128, 0); }
	static get GreenYellow() { return Color.fromRGBA(173, 255, 47); }
	static get Honeydew() { return Color.fromRGBA(240, 255, 240); }
	static get HotPink() { return Color.fromRGBA(255, 105, 180); }
	static get IndianRed() { return Color.fromRGBA(205, 92, 92); }
	static get Indigo() { return Color.fromRGBA(75, 0, 130); }
	static get Ivory() { return Color.fromRGBA(255, 255, 240); }
	static get Khaki() { return Color.fromRGBA(240, 230, 140); }
	static get Lavender() { return Color.fromRGBA(230, 230, 250); }
	static get LavenderBlush() { return Color.fromRGBA(255, 240, 245); }
	static get LawnGreen() { return Color.fromRGBA(124, 252, 0); }
	static get LemonChiffon() { return Color.fromRGBA(255, 250, 205); }
	static get LightBlue() { return Color.fromRGBA(173, 216, 230); }
	static get LightCoral() { return Color.fromRGBA(240, 128, 128); }
	static get LightCyan() { return Color.fromRGBA(224, 255, 255); }
	static get LightGoldenrodYellow() { return Color.fromRGBA(250, 250, 210); }
	static get LightGray() { return Color.fromRGBA(211, 211, 211); }
	static get LightGreen() { return Color.fromRGBA(144, 238, 144); }
	static get LightPink() { return Color.fromRGBA(255, 182, 193); }
	static get LightSalmon() { return Color.fromRGBA(255, 160, 122); }
	static get LightSeaGreen() { return Color.fromRGBA(32, 178, 170); }
	static get LightSkyBlue() { return Color.fromRGBA(135, 206, 250); }
	static get LightSlateGray() { return Color.fromRGBA(119, 136, 153); }
	static get LightSteelBlue() { return Color.fromRGBA(176, 196, 222); }
	static get LightYellow() { return Color.fromRGBA(255, 255, 224); }
	static get Lime() { return Color.fromRGBA(0, 255, 0); }
	static get LimeGreen() { return Color.fromRGBA(50, 205, 50); }
	static get Linen() { return Color.fromRGBA(250, 240, 230); }
	static get Magenta() { return Color.fromRGBA(255, 0, 255); }
	static get Maroon() { return Color.fromRGBA(128, 0, 0); }
	static get MediumAquamarine() { return Color.fromRGBA(102, 205, 170); }
	static get MediumBlue() { return Color.fromRGBA(0, 0, 205); }
	static get MediumOrchid() { return Color.fromRGBA(186, 85, 211); }
	static get MediumPurple() { return Color.fromRGBA(147, 112, 219); }
	static get MediumSeaGreen() { return Color.fromRGBA(60, 179, 113); }
	static get MediumSlateBlue() { return Color.fromRGBA(123, 104, 238); }
	static get MediumSpringGreen() { return Color.fromRGBA(0, 250, 154); }
	static get MediumTurquoise() { return Color.fromRGBA(72, 209, 204); }
	static get MediumVioletRed() { return Color.fromRGBA(199, 21, 133); }
	static get MidnightBlue() { return Color.fromRGBA(25, 25, 112); }
	static get MintCream() { return Color.fromRGBA(245, 255, 250); }
	static get MistyRose() { return Color.fromRGBA(255, 228, 225); }
	static get Moccasin() { return Color.fromRGBA(255, 228, 181); }
	static get NavajoWhite() { return Color.fromRGBA(255, 222, 173); }
	static get Navy() { return Color.fromRGBA(0, 0, 128); }
	static get OldLace() { return Color.fromRGBA(253, 245, 230); }
	static get Olive() { return Color.fromRGBA(128, 128, 0); }
	static get OliveDrab() { return Color.fromRGBA(107, 142, 35); }
	static get Orange() { return Color.fromRGBA(255, 165, 0); }
	static get OrangeRed() { return Color.fromRGBA(255, 69, 0); }
	static get Orchid() { return Color.fromRGBA(218, 112, 214); }
	static get PaleGoldenrod() { return Color.fromRGBA(238, 232, 170); }
	static get PaleGreen() { return Color.fromRGBA(152, 251, 152); }
	static get PaleTurquoise() { return Color.fromRGBA(175, 238, 238); }
	static get PaleVioletRed() { return Color.fromRGBA(219, 112, 147); }
	static get PapayaWhip() { return Color.fromRGBA(225, 239, 213); }
	static get PeachPuff() { return Color.fromRGBA(255, 218, 185); }
	static get Peru() { return Color.fromRGBA(205, 133, 63); }
	static get Pink() { return Color.fromRGBA(255, 192, 203); }
	static get Plum() { return Color.fromRGBA(221, 160, 221); }
	static get PowderBlue() { return Color.fromRGBA(176, 224, 230); }
	static get Purple() { return Color.fromRGBA(128, 0, 128); }
	static get PurwaBlue() { return Color.fromRGBA(155, 225, 255); }
	static get RebeccaPurple() { return Color.fromRGBA(102, 51, 153); }
	static get Red() { return Color.fromRGBA(255, 0, 0); }
	static get RosyBrown() { return Color.fromRGBA(188, 143, 143); }
	static get RoyalBlue() { return Color.fromRGBA(65, 105, 225); }
	static get SaddleBrown() { return Color.fromRGBA(139, 69, 19); }
	static get Salmon() { return Color.fromRGBA(250, 128, 114); }
	static get SandyBrown() { return Color.fromRGBA(244, 164, 96); }
	static get SeaGreen() { return Color.fromRGBA(46, 139, 87); }
	static get Seashell() { return Color.fromRGBA(255, 245, 238); }
	static get Sienna() { return Color.fromRGBA(160, 82, 45); }
	static get Silver() { return Color.fromRGBA(192, 192, 192); }
	static get SkyBlue() { return Color.fromRGBA(135, 206, 235); }
	static get SlateBlue() { return Color.fromRGBA(106, 90, 205); }
	static get SlateGray() { return Color.fromRGBA(112, 128, 144); }
	static get Snow() { return Color.fromRGBA(255, 250, 250); }
	static get SpringGreen() { return Color.fromRGBA(0, 255, 127); }
	static get StankyBean() { return Color.fromRGBA(197, 162, 171); }
	static get SteelBlue() { return Color.fromRGBA(70, 130, 180); }
	static get Tan() { return Color.fromRGBA(210, 180, 140); }
	static get Teal() { return Color.fromRGBA(0, 128, 128); }
	static get Thistle() { return Color.fromRGBA(216, 191, 216); }
	static get Tomato() { return Color.fromRGBA(255, 99, 71); }
	static get Transparent() { return Color.fromRGBA(0, 0, 0, 0); }
	static get Turquoise() { return Color.fromRGBA(64, 224, 208); }
	static get Violet() { return Color.fromRGBA(238, 130, 238); }
	static get Wheat() { return Color.fromRGBA(245, 222, 179); }
	static get White() { return Color.fromRGBA(255, 255, 255); }
	static get WhiteSmoke() { return Color.fromRGBA(245, 245, 245); }
	static get Yellow() { return Color.fromRGBA(255, 255, 0); }
	static get YellowGreen() { return Color.fromRGBA(154, 205, 50); }

	static fromRGBA(r, g, b, a = 255)
	{
		return new Color(r / 255.0, g / 255.0, b / 255.0, a / 255.0);
	}

	static is(x, y)
	{
		return x.r === y.r && x.g === y.g && x.b === y.b;
	}

	static mix(x, y, wx = 1.0, wy = 1.0)
	{
		const totalWeight = wx + wy;
		wx /= totalWeight;
		wy /= totalWeight;
		return new Color(
			x.r * wx + y.r * wy,
			x.g * wx + y.g * wy,
			x.b * wx + y.b * wy,
			x.a * wx + y.a * wy);
	}

	static of(name)
	{
		// parse 6-digit format (#rrggbb)
		let matched = name.match(/^#?([0-9a-f]{6})$/i);
		if (matched) {
			const m = matched[1];
			return new Color(
				parseInt(m.slice(0, 2), 16) / 255.0,
				parseInt(m.slice(2, 4), 16) / 255.0,
				parseInt(m.slice(4, 6), 16) / 255.0,
			);
		}

		// parse 8-digit format (#aarrggbb)
		matched = name.match(/^#?([0-9a-f]{8})$/i);
		if (matched) {
			const m = matched[1];
			return new Color(
				parseInt(m.slice(2, 4), 16) / 255.0,
				parseInt(m.slice(4, 6), 16) / 255.0,
				parseInt(m.slice(6, 8), 16) / 255.0,
				parseInt(m.slice(0, 2), 16) / 255.0,
			);
		}

		// see if `name` matches a predefined color (not case sensitive)
		const toMatch = name.toUpperCase();

		for (const colorName in Color) {
			if (colorName.toUpperCase() === toMatch) {
				try {
					let propValue = Color[colorName];
					if (propValue instanceof Color)
						return propValue;
				}
				catch {
					// *MUNCH*
				}
				break;
			}
		}

		// if we got here, none of the parsing attempts succeeded, so throw an error.
		throw RangeError(`Invalid color designation '${name}'`);
	}

	constructor(r, g, b, a = 1.0)
	{
		this.r = r;
		this.g = g;
		this.b = b;
		this.a = a;
	}

	get name()
	{
		throw Error(`The Color#name API is not implemented.`);
	}

	clone()
	{
		return new Color(this.r, this.g, this.b, this.a);
	}

	fadeTo(alphaFactor)
	{
		return new Color(this.r, this.g, this.b,
			this.a * alphaFactor);
	}

	toVector()
	{
		return [ this.r, this.g, this.b, this.a ];
	}
}

export
class IndexList
{
	#glBuffer = null;
	#length = 0;

	constructor(indices)
	{
		this.#glBuffer = gl.createBuffer();
		if (this.#glBuffer === null)
			throw Error(`The engine couldn't create a WebGL buffer object.`);
		this.upload(indices);
	}

	get length()
	{
		return this.#length;
	}

	activate()
	{
		gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.#glBuffer);
	}

	upload(indices)
	{
		const values = new Uint16Array(indices);
		gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.#glBuffer);
		gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, values, gl.STREAM_DRAW);
		this.#length = values.length;
	}
}

export
class Model
{
	#shapes;
	#shader;
	#transform;

	constructor(shapes, shader = Shader.Default)
	{
		this.#shapes = [ ...shapes ];
		this.#shader = shader;
		this.#transform = Transform.Identity;
	}

	get shader()
	{
		return this.#shader;
	}

	get transform()
	{
		return this.#transform;
	}

	set shader(value)
	{
		if (!(value instanceof Shader))
			throw TypeError(`Model#shader must be set to a 'Shader' object`);
		this.#shader = value;
	}

	set transform(value)
	{
		if (!(value instanceof Transform))
			throw TypeError(`Model#transform must be set to a 'Transform' object`);
		this.#transform = value;
	}

	draw(surface = Surface.Screen)
	{
		for (const shape of this.#shapes)
			shape.draw(surface, this.#transform, this.#shader);
	}
}

export
class Shader
{
	#fragmentShaderSource = "";
	#glFragmentShader;
	#glProgram;
	#glVertexShader;
	#modelViewMatrix = Transform.Identity;
	#projection = Transform.Identity;
	#uniformIDs = {};
	#vertexShaderSource = "";
	#valuesToSet = {};

	static get Default()
	{
		return defaultShader;
	}

	static async fromFiles(options)
	{
		const vertexShaderURL = Game.urlOf(options.vertexFile);
		const fragmentShaderURL = Game.urlOf(options.fragmentFile);
		const sources = await Promise.all([
			Fido.fetchText(vertexShaderURL),
			Fido.fetchText(fragmentShaderURL),
		]);
		return new Shader({
			vertexSource: sources[0],
			fragmentSource: sources[1],
		})
}

	constructor(options)
	{
		const program = gl.createProgram();
		const vertexShader = gl.createShader(gl.VERTEX_SHADER);
		const fragmentShader = gl.createShader(gl.FRAGMENT_SHADER);
		if (program === null || vertexShader === null || fragmentShader === null)
			throw Error(`Couldn't create a WebGL shader object`);
		this.#glProgram = program;
		this.#glVertexShader = vertexShader;
		this.#glFragmentShader = fragmentShader;

		if ('vertexFile' in options && options.vertexFile !== undefined) {
			throw Error(`'new Shader' from filenames is not supported`);
		}
		else if ('vertexSource' in options && options.vertexSource !== undefined) {
			this.compile(options.vertexSource, options.fragmentSource);
		}
		else {
			throw RangeError(`'new Shader()' called without either filenames or shader sources`);
		}
	}

	activate(useTexture)
	{
		if (activeShader !== this) {
			gl.useProgram(this.#glProgram);
			for (const name of Object.keys(this.#valuesToSet)) {
				const entry = this.#valuesToSet[name];
				let location = this.#uniformIDs[name];
				if (location === undefined) {
					location = gl.getUniformLocation(this.#glProgram, name);
					this.#uniformIDs[name] = location;
				}
				let size;
				switch (entry.type) {
					case 'boolean':
						gl.uniform1i(location, entry.value ? 1 : 0);
						break;
					case 'float':
						gl.uniform1f(location, entry.value);
						break;
					case 'floatArray':
						gl.uniform1fv(location, entry.value);
						break;
					case 'floatVector':
						size = entry.value.length;
						size === 4 ? gl.uniform4fv(location, entry.value)
							: size === 3 ? gl.uniform3fv(location, entry.value)
							: size === 2 ? gl.uniform2fv(location, entry.value)
							: gl.uniform1fv(location, entry.value);
						break;
					case 'int':
						gl.uniform1i(location, entry.value);
						break;
					case 'intArray':
						gl.uniform1iv(location, entry.value);
						break;
					case 'intVector':
						size = entry.value.length;
						size === 4 ? gl.uniform4iv(location, entry.value)
							: size === 3 ? gl.uniform3iv(location, entry.value)
							: size === 2 ? gl.uniform2iv(location, entry.value)
							: gl.uniform1iv(location, entry.value);
						break;
					case 'matrix':
						gl.uniformMatrix4fv(location, false, entry.value.values);
						break;
				}
			}
			this.#valuesToSet = {};
			activeShader = this;
		}
		this.setBoolean('al_use_tex', useTexture);
	}

	clone()
	{
		return new Shader({
			vertexSource: this.#vertexShaderSource,
			fragmentSource: this.#fragmentShaderSource,
		});
	}

	compile(vertexShaderSource, fragmentShaderSource)
	{
		// compile vertex and fragment shaders and check for errors
		gl.shaderSource(this.#glVertexShader, vertexShaderSource);
		gl.shaderSource(this.#glFragmentShader, fragmentShaderSource);
		gl.compileShader(this.#glVertexShader);
		if (!gl.getShaderParameter(this.#glVertexShader, gl.COMPILE_STATUS)) {
			const message = gl.getShaderInfoLog(this.#glVertexShader);
			throw Error(`Couldn't compile WebGL vertex shader.\n${message}`);
		}
		gl.compileShader(this.#glFragmentShader);
		if (!gl.getShaderParameter(this.#glFragmentShader, gl.COMPILE_STATUS)) {
			const message = gl.getShaderInfoLog(this.#glFragmentShader);
			throw Error(`Couldn't compile WebGL fragment shader.\n${message}`);
		}

		// link the individual shaders into a program, check for errors
		gl.attachShader(this.#glProgram, this.#glVertexShader);
		gl.attachShader(this.#glProgram, this.#glFragmentShader);
		gl.bindAttribLocation(this.#glProgram, 0, 'al_pos');
		gl.bindAttribLocation(this.#glProgram, 1, 'al_color');
		gl.bindAttribLocation(this.#glProgram, 2, 'al_texcoord');
		gl.linkProgram(this.#glProgram);
		if (!gl.getProgramParameter(this.#glProgram, gl.LINK_STATUS)) {
			const message = gl.getProgramInfoLog(this.#glProgram);
			throw Error(`Couldn't link WebGL shader program.\n${message}`);
		}

		this.#vertexShaderSource = vertexShaderSource;
		this.#fragmentShaderSource = fragmentShaderSource;
		this.#uniformIDs = {};

		const transformation = this.#modelViewMatrix.clone()
			.compose(this.#projection);
		this.setMatrix('al_projview_matrix', transformation);
		this.setInt('al_tex', 0);
	}

	project(matrix)
	{
		this.#projection = matrix.clone();
		let transformation = this.#modelViewMatrix.clone()
			.compose(this.#projection);
		this.setMatrix('al_projview_matrix', transformation);
	}

	setBoolean(name, value)
	{
		if (activeShader === this) {
			let location = this.#uniformIDs[name];
			if (location === undefined) {
				location = gl.getUniformLocation(this.#glProgram, name);
				this.#uniformIDs[name] = location;
			}
			gl.uniform1i(location, value ? 1 : 0);
		}
		else {
			this.#valuesToSet[name] = { type: 'boolean', value };
		}
	}

	setColorVector(name, color)
	{
		this.setFloatVector(name, color.toVector());
	}

	setFloat(name, value)
	{
		if (activeShader === this) {
			let location = this.#uniformIDs[name];
			if (location === undefined) {
				location = gl.getUniformLocation(this.#glProgram, name);
				this.#uniformIDs[name] = location;
			}
			gl.uniform1f(location, value);
		}
		else {
			this.#valuesToSet[name] = { type: 'float', value };
		}
	}

	setFloatArray(name, values)
	{
		if (activeShader === this) {
			let location = this.#uniformIDs[name];
			if (location === undefined) {
				location = gl.getUniformLocation(this.#glProgram, name);
				this.#uniformIDs[name] = location;
			}
			gl.uniform1fv(location, values);
		}
		else {
			this.#valuesToSet[name] = { type: 'floatArray', value: values };
		}
	}

	setFloatVector(name, values)
	{
		if (activeShader === this) {
			let location = this.#uniformIDs[name];
			if (location === undefined) {
				location = gl.getUniformLocation(this.#glProgram, name);
				this.#uniformIDs[name] = location;
			}
			const size = values.length;
			size === 4 ? gl.uniform4fv(location, values)
				: size === 3 ? gl.uniform3fv(location, values)
				: size === 2 ? gl.uniform2fv(location, values)
				: gl.uniform1fv(location, values);
		}
		else {
			this.#valuesToSet[name] = { type: 'floatVector', value: values };
		}
	}

	setInt(name, value)
	{
		if (activeShader === this) {
			let location = this.#uniformIDs[name];
			if (location === undefined) {
				location = gl.getUniformLocation(this.#glProgram, name);
				this.#uniformIDs[name] = location;
			}
			gl.uniform1i(location, value);
		}
		else {
			this.#valuesToSet[name] = { type: 'int', value };
		}
	}

	setIntArray(name, values)
	{
		if (activeShader === this) {
			let location = this.#uniformIDs[name];
			if (location === undefined) {
				location = gl.getUniformLocation(this.#glProgram, name);
				this.#uniformIDs[name] = location;
			}
			gl.uniform1iv(location, values);
		}
		else {
			this.#valuesToSet[name] = { type: 'intArray', value: values };
		}
	}

	setIntVector(name, values)
	{
		if (activeShader === this) {
			let location = this.#uniformIDs[name];
			if (location === undefined) {
				location = gl.getUniformLocation(this.#glProgram, name);
				this.#uniformIDs[name] = location;
			}
			const size = values.length;
			size === 4 ? gl.uniform4iv(location, values)
				: size === 3 ? gl.uniform3iv(location, values)
				: size === 2 ? gl.uniform2iv(location, values)
				: gl.uniform1iv(location, values);
		}
		else {
			this.#valuesToSet[name] = { type: 'intVector', value: values };
		}
	}

	setMatrix(name, value)
	{
		if (activeShader === this) {
			let location = this.#uniformIDs[name];
			if (location === undefined) {
				location = gl.getUniformLocation(this.#glProgram, name);
				this.#uniformIDs[name] = location;
			}
			gl.uniformMatrix4fv(location, false, value.values);
		}
		else {
			this.#valuesToSet[name] = { type: 'matrix', value };
		}
	}

	transform(matrix)
	{
		this.#modelViewMatrix = matrix.clone();
		let transformation = this.#modelViewMatrix.clone()
			.compose(this.#projection);
		this.setMatrix('al_projview_matrix', transformation);
	}
}

export
class Shape
{
	#indexList;
	#shapeType;
	#texture;
	#vertexList;

	static drawImmediate(surface, shapeType, arg1, arg2)
	{
		if (arg1 instanceof Texture || arg1 === null) {
			const texture = arg1;
			const vertices = arg2;
			surface.activate(defaultShader, texture);
			(immediateVL ??= new VertexList([]))
				.upload(vertices);
			Galileo.draw(shapeType, immediateVL);
		}
		else {
			const vertices = arg1;
			surface.activate(defaultShader);
			(immediateVL ??= new VertexList([]))
				.upload(vertices);
			Galileo.draw(shapeType, immediateVL);
		}
	}

	constructor(shapeType, arg1, arg2 = null, arg3 = null)
	{
		this.#shapeType = shapeType;
		if (arg2 instanceof VertexList) {
			if (!(arg1 instanceof Texture) && arg1 != undefined)
				throw Error(`Expected a Texture or 'null' as second argument of Shape constructor`);
			this.#vertexList = arg2;
			this.#indexList = arg3;
			this.#texture = arg1;
		}
		else {
			if (!(arg1 instanceof VertexList))
				throw Error(`Expected a VertexList or Texture as second argument of Shape constructor`);
			this.#vertexList = arg1;
			this.#indexList = arg2;
			this.#texture = null;
		}
	}

	get indexList()
	{
		return this.#indexList;
	}

	get texture()
	{
		return this.#texture;
	}

	get vertexList()
	{
		return this.#vertexList;
	}

	set indexList(value)
	{
		if (value !== null && !(value instanceof IndexList))
			throw TypeError(`Shape#indexList must be set to an IndexList object or 'null'.`);
		this.#indexList = value;
	}

	set texture(value)
	{
		if (value !== null && !(value instanceof Texture))
			throw TypeError(`Shape#texture must be set to a Texture object or 'null'.`);
		this.#texture = value;
	}

	set vertexList(value)
	{
		if (!(value instanceof VertexList))
			throw TypeError(`Shape#vertexList must be set to a VertexList object.`);
		this.#vertexList = value;
	}

	draw(surface = Surface.Screen, transform = Transform.Identity, shader = Shader.Default)
	{
		surface.activate(shader, this.#texture, transform);
		Galileo.draw(this.#shapeType, this.#vertexList, this.#indexList);
	}
}

export
class Texture
{
	#fileName;
	#glTexture;
	#height;
	#width;

	static async fromFile(fileName)
	{
		const imageURL = Game.urlOf(fileName);
		const image = await Fido.fetchImage(imageURL);
		const texture = new Texture(image);
		texture.#fileName = Game.fullPath(fileName);
		return texture;
	}

	constructor(...args)
	{
		const glTexture = gl.createTexture();
		if (glTexture === null)
			throw Error(`Engine couldn't create a WebGL texture object.`);
		this.#glTexture = glTexture;
		if (typeof args[0] === 'string') {
			throw Error(`'new Texture' from filename is not supported`);
		}
		else {
			const oldBinding = gl.getParameter(gl.TEXTURE_BINDING_2D);
			gl.bindTexture(gl.TEXTURE_2D, glTexture);
			gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
			if (args[0] instanceof HTMLImageElement) {
				const image = args[0];
				gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image);
				this.#width = args[0].width;
				this.#height = args[0].height;
			}
			else if (typeof args[0] === 'number' && typeof args[1] === 'number') {
				const width = args[0];
				const height = args[1];
				if (width < 1 || height < 1)
					throw RangeError("A texture cannot be less than one pixel in size.");
				if (args[2] instanceof ArrayBuffer || ArrayBuffer.isView(args[2])) {
					const buffer = args[2] instanceof ArrayBuffer ? args[2] : args[2].buffer;
					if (buffer.byteLength < width * height * 4)
						throw RangeError(`The provided buffer is too small to initialize a ${width}x${height} texture.`);
					gl.texImage2D(gl.TEXTURE_2D,
						0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE,
						new Uint8Array(buffer));
				}
				else {
					const pixels = new Uint32Array(width * height);
					if (args[2] !== undefined)
						pixels.fill((args[2].a * 255 << 24) + (args[2].b * 255 << 16) + (args[2].g * 255 << 8) + (args[2].r * 255));
					gl.texImage2D(gl.TEXTURE_2D,
						0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE,
						new Uint8Array(pixels.buffer));
				}
				this.#width = width;
				this.#height = height;
			}
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
			gl.bindTexture(gl.TEXTURE_2D, oldBinding);
		}
	}

	get fileName()
	{
		return this.#fileName;
	}

	get glHandle()
	{
		return this.#glTexture;
	}

	get height()
	{
		return this.#height;
	}

	get width()
	{
		return this.#width;
	}

	upload(content, x = 0, y = 0, width = this.width, height = this.height)
	{
		const pixelData = ArrayBuffer.isView(content)
			? new Uint8Array(content.buffer)
			: new Uint8Array(content);
		gl.bindTexture(gl.TEXTURE_2D, this.#glTexture);
		gl.texSubImage2D(gl.TEXTURE_2D, 0, x, this.#height - y - height, width, height, gl.RGBA, gl.UNSIGNED_BYTE, pixelData);
	}

	useTexture(textureUnit = 0)
	{
		gl.activeTexture(gl.TEXTURE0 + textureUnit);
		gl.bindTexture(gl.TEXTURE_2D, this.#glTexture);
	}
}

export
class Surface extends Texture
{
	#blendOp = BlendOp.Default;
	#clipRectangle;
	#clipStack = [];
	#depthOp = DepthOp.AlwaysPass;
	#frameBuffer;
	#projection;

	static get Screen()
	{
		return backBuffer;
	}

	static async fromFile(fileName)
	{
		throw Error(`'Surface.fromFile' is not supported`);
	}

	constructor(...args)
	{
		if (typeof args[0] === 'string')
			throw Error(`'new Surface' from filename is not supported`);

		super(...args);

		const frameBuffer = gl.createFramebuffer();
		const depthBuffer = gl.createRenderbuffer();
		if (frameBuffer === null || depthBuffer === null)
			throw Error(`Couldn't create a WebGL framebuffer object`);

		// in order to set up a new FBO we need to change the current framebuffer binding, so make sure
		// it gets changed back afterwards.
		const previousFBO = gl.getParameter(gl.FRAMEBUFFER_BINDING);
		gl.bindFramebuffer(gl.FRAMEBUFFER, frameBuffer);
		gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, this.glHandle, 0);
		gl.bindRenderbuffer(gl.RENDERBUFFER, depthBuffer);
		gl.renderbufferStorage(gl.RENDERBUFFER, gl.DEPTH_COMPONENT16, this.width, this.height);
		gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.RENDERBUFFER, depthBuffer);
		gl.bindFramebuffer(gl.FRAMEBUFFER, previousFBO);

		this.#frameBuffer = frameBuffer;
		this.#projection = Transform.project2D(0, 0, this.width, this.height);

		this.#computeClipping();
	}

	get blendOp()
	{
		return this.#blendOp;
	}

	get depthOp()
	{
		return this.#depthOp;
	}

	get transform()
	{
		return this.#projection;
	}

	set blendOp(value)
	{
		this.#blendOp = value;
		if (activeSurface === this)
			applyBlendOp(value);
	}

	set depthOp(value)
	{
		this.#depthOp = value;
		if (activeSurface === this)
			applyDepthOp(value);
	}

	set transform(value)
	{
		this.#projection = value;
	}

	activate(shader, texture = null, transform = Transform.Identity)
	{
		if (this !== activeSurface) {
			gl.bindFramebuffer(gl.FRAMEBUFFER, this.#frameBuffer);
			gl.viewport(0, 0, this.width, this.height);
			gl.scissor(
				this.#clipRectangle.x1,
				this.height - this.#clipRectangle.y2,
				this.#clipRectangle.x2 - this.#clipRectangle.x1,
				this.#clipRectangle.y2 - this.#clipRectangle.y1);
			applyBlendOp(this.#blendOp);
			applyDepthOp(this.#depthOp);
			activeSurface = this;
		}
		shader.activate(texture !== null);
		shader.project(this.#projection);
		shader.transform(transform);
		if (texture !== null) {
			texture.useTexture(0);
		}
		else {
			gl.activeTexture(gl.TEXTURE0 + 0);
			gl.bindTexture(gl.TEXTURE_2D, null);
		}
	}

	clipTo(x, y, width, height, clipOp = ClipOp.Narrow)
	{
		if (clipOp === ClipOp.Reset) {
			this.#clipStack.length = 0;
			clipOp = ClipOp.Override;
		}
		this.#clipStack.push({ clipOp, x1: x, y1: y, x2: x + width, y2: y + height });
		this.#computeClipping();
	}

	unclip()
	{
		if (this.#clipStack.length === 0)
			throw RangeError(`No clipping rectangle changes to undo`);
		this.#clipStack.pop();
		this.#computeClipping();
	}

	#computeClipping()
	{
		let x1 = 0;
		let y1 = 0;
		let x2 = this.width;
		let y2 = this.height;
		for (let i = 0, len = this.#clipStack.length; i < len; ++i) {
			const clip = this.#clipStack[i];
			switch (clip.clipOp) {
				case ClipOp.Narrow:
					x1 = Math.max(x1, clip.x1);
					y1 = Math.max(y1, clip.y1);
					x2 = Math.min(x2, clip.x2);
					y2 = Math.min(y2, clip.y2);
					break;
				case ClipOp.Override:
					x1 = clip.x1;
					y1 = clip.y1;
					x2 = clip.x2;
					y2 = clip.y2;
					break;
			}
		}
		if (this === activeSurface)
			gl.scissor(x1, this.height - y2, x2 - x1, y2 - y1);
		this.#clipRectangle = { x1, y1, x2, y2 };
	}
}

export
class Transform
{
	#subarrays;
	#values;

	static get Identity()
	{
		return new Transform([
			1.0, 0.0, 0.0, 0.0,
			0.0, 1.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 0.0, 1.0,
		]);
	}

	static project2D(left, top, right, bottom, near = -1.0, far = 1.0)
	{
		const deltaX = right - left;
		const deltaY = top - bottom;
		const deltaZ = far - near;

		const transform = Transform.Identity;
		const values = transform.#values;
		values[0] = 2.0 / deltaX;
		values[5] = 2.0 / deltaY;
		values[10] = 2.0 / deltaZ;
		values[15] = 1.0;
		values[12] = -(right + left) / deltaX;
		values[13] = -(top + bottom) / deltaY;
		values[14] = -(far + near) / deltaZ;
		return transform;
	}

	static project3D(fov, aspect, near, far)
	{
		const fh = Math.tan(fov * Math.PI / 360.0) * near;
		const fw = fh * aspect;

		const deltaX = fw - -fw;
		const deltaY = -fh - fh;
		const deltaZ = far - near;

		const transform = Transform.Identity;
		const values = transform.#values;
		values[0] = 2.0 * near / deltaX;
		values[5] = 2.0 * near / deltaY;
		values[8] = (fw + -fw) / deltaX;
		values[9] = (-fh + fh) / deltaY;
		values[10] = -(far + near) / deltaZ;
		values[11] = -1.0;
		values[14] = -2.0 * far * near / deltaZ;
		values[15] = 0.0;
		return transform;
	}

	static rotate(angle, vX, vY, vZ)
	{
		// normalize the rotation axis vector
		const norm = Math.sqrt(vX * vX + vY * vY + vZ * vZ);
		if (norm > 0.0) {
			vX = vX / norm;
			vY = vY / norm;
			vZ = vZ / norm;
		}

		// convert degrees to radians
		const theta = angle * Math.PI / 180.0;

		const cos = Math.cos(theta);
		const sin = Math.sin(theta);
		const siv = 1.0 - cos;

		const transform = Transform.Identity;
		const values = transform.#values;
		values[0] = (siv * vX * vX) + cos;
		values[1] = (siv * vX * vY) + (vZ * sin);
		values[2] = (siv * vX * vZ) - (vY * sin);
		values[4] = (siv * vX * vY) - (vZ * sin);
		values[5] = (siv * vY * vY) + cos;
		values[6] = (siv * vZ * vY) + (vX * sin);
		values[8] = (siv * vX * vZ) + (vY * sin);
		values[9] = (siv * vY * vZ) - (vX * sin);
		values[10] = (siv * vZ * vZ) + cos;
		return transform;
	}
	
	static scale(sX, sY, sZ = 1.0)
	{
		return new Transform([
			sX,  0.0, 0.0, 0.0,
			0.0, sY,  0.0, 0.0,
			0.0, 0.0, sZ,  0.0,
			0.0, 0.0, 0.0, 1.0,
		]);
	}
	
	static translate(tX, tY, tZ = 0.0)
	{
		return new Transform([
			1.0, 0.0, 0.0, 0.0,
			0.0, 1.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			tX,  tY,  tZ,  1.0,
		]);
	}

	constructor(values)
	{
		if (values !== undefined) {
			if (values.length !== 16)
				throw RangeError("new Transform() requires a 16-element array of numbers as input.");
			this.#values = new Float32Array(values);
		}
		else {
			// default configuration is an identity matrix
			this.#values = new Float32Array([
				1.0, 0.0, 0.0, 0.0,
				0.0, 1.0, 0.0, 0.0,
				0.0, 0.0, 1.0, 0.0,
				0.0, 0.0, 0.0, 1.0,
			]);
		}
	}

	get matrix()
	{
		if (this.#subarrays === undefined) {
			this.#subarrays = [
				this.#values.subarray(0, 4),
				this.#values.subarray(4, 8),
				this.#values.subarray(8, 12),
				this.#values.subarray(12, 16),
			];
		}
		return this.#subarrays;
	}
	
	get values()
	{
		return this.#values;
	}

	clone()
	{
		return new Transform(this.#values);
	}

	compose(other)
	{
		if (!(other instanceof Transform))
			throw TypeError("Transform#compose argument must be a Transform object");

		const m1 = this.#values;
		const m2 = other.#values;

		// multiply from the left (i.e. `other * this`).  this emulates the way Allegro's
		// `al_compose_transform()` function works--that is, transformations are logically applied in
		// the order they're specified, rather than reversed as in classic OpenGL.
		const a00  = m2[0], a01 = m2[1], a02 = m2[2], a03 = m2[3];
		const a10  = m2[4], a11 = m2[5], a12 = m2[6], a13 = m2[7];
		const a20  = m2[8], a21 = m2[9], a22 = m2[10], a23 = m2[11];
		const a30  = m2[12], a31 = m2[13], a32 = m2[14], a33 = m2[15];
		const b00 = m1[0], b01 = m1[1], b02 = m1[2], b03 = m1[3];
		const b10 = m1[4], b11 = m1[5], b12 = m1[6], b13 = m1[7];
		const b20 = m1[8], b21 = m1[9], b22 = m1[10], b23 = m1[11];
		const b30 = m1[12], b31 = m1[13], b32 = m1[14], b33 = m1[15];

		// multiply the matrices together.  funny story: I still don't understand how this
		// works.  but it does, so...
		m1[0] = b00 * a00 + b01 * a10 + b02 * a20 + b03 * a30;
		m1[1] = b00 * a01 + b01 * a11 + b02 * a21 + b03 * a31;
		m1[2] = b00 * a02 + b01 * a12 + b02 * a22 + b03 * a32;
		m1[3] = b00 * a03 + b01 * a13 + b02 * a23 + b03 * a33;
		m1[4] = b10 * a00 + b11 * a10 + b12 * a20 + b13 * a30;
		m1[5] = b10 * a01 + b11 * a11 + b12 * a21 + b13 * a31;
		m1[6] = b10 * a02 + b11 * a12 + b12 * a22 + b13 * a32;
		m1[7] = b10 * a03 + b11 * a13 + b12 * a23 + b13 * a33;
		m1[8] = b20 * a00 + b21 * a10 + b22 * a20 + b23 * a30;
		m1[9] = b20 * a01 + b21 * a11 + b22 * a21 + b23 * a31;
		m1[10] = b20 * a02 + b21 * a12 + b22 * a22 + b23 * a32;
		m1[11] = b20 * a03 + b21 * a13 + b22 * a23 + b23 * a33;
		m1[12] = b30 * a00 + b31 * a10 + b32 * a20 + b33 * a30;
		m1[13] = b30 * a01 + b31 * a11 + b32 * a21 + b33 * a31;
		m1[14] = b30 * a02 + b31 * a12 + b32 * a22 + b33 * a32;
		m1[15] = b30 * a03 + b31 * a13 + b32 * a23 + b33 * a33;

		return this;
	}

	identity()
	{
		this.#values.set([
			1.0, 0.0, 0.0, 0.0,
			0.0, 1.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 0.0, 1.0,
		]);
		return this;
	}

	project2D(left, top, right, bottom, near = -1.0, far = 1.0)
	{
		return this.compose(Transform.project2D(left, top, right, bottom, near, far));
	}

	project3D(fov, aspect, near, far)
	{
		return this.compose(Transform.project3D(fov, aspect, near, far));
	}

	rotate(angle, vX, vY, vZ)
	{
		return this.compose(Transform.rotate(angle, vX, vY, vZ));
	}

	scale(sX, sY, sZ = 1.0)
	{
		this.#values[0] *= sX;
		this.#values[4] *= sX;
		this.#values[8] *= sX;
		this.#values[12] *= sX;

		this.#values[1] *= sY;
		this.#values[5] *= sY;
		this.#values[9] *= sY;
		this.#values[13] *= sY;

		this.#values[2] *= sZ;
		this.#values[6] *= sZ;
		this.#values[10] *= sZ;
		this.#values[14] *= sZ;

		return this;
	}

	translate(tX, tY, tZ = 0.0)
	{
		this.#values[12] += tX;
		this.#values[13] += tY;
		this.#values[14] += tZ;
		return this;
	}
}

export
class VertexList
{
	#glBuffer = null;
	#length = 0;

	constructor(vertices)
	{
		this.#glBuffer = gl.createBuffer();
		if (this.#glBuffer === null)
			throw Error(`Engine couldn't create a WebGL buffer object.`);
		this.upload(vertices);
	}

	get length()
	{
		return this.#length;
	}

	activate()
	{
		gl.bindBuffer(gl.ARRAY_BUFFER, this.#glBuffer);
		gl.enableVertexAttribArray(0);
		gl.enableVertexAttribArray(1);
		gl.enableVertexAttribArray(2);
		gl.vertexAttribPointer(0, 4, gl.FLOAT, false, 40, 0);
		gl.vertexAttribPointer(1, 4, gl.FLOAT, false, 40, 16);
		gl.vertexAttribPointer(2, 2, gl.FLOAT, false, 40, 32);
	}

	upload(vertices)
	{
		const data = new Float32Array(10 * vertices.length);
		for (let i = 0, len = vertices.length; i < len; ++i) {
			const vertex = vertices[i];
			data[0 + i * 10] = vertex.x;
			data[1 + i * 10] = vertex.y;
			data[2 + i * 10] = vertex.z ?? 0.0;
			data[3 + i * 10] = 1.0;
			data[4 + i * 10] = vertex.color?.r ?? 1.0;
			data[5 + i * 10] = vertex.color?.g ?? 1.0;
			data[6 + i * 10] = vertex.color?.b ?? 1.0;
			data[7 + i * 10] = vertex.color?.a ?? 1.0;
			data[8 + i * 10] = vertex.u ?? 0.0;
			data[9 + i * 10] = vertex.v ?? 0.0;
		}
		gl.bindBuffer(gl.ARRAY_BUFFER, this.#glBuffer);
		gl.bufferData(gl.ARRAY_BUFFER, data, gl.STREAM_DRAW);
		this.#length = vertices.length;
	}
}

function applyBlendOp(blendOp)
{
	switch (blendOp) {
		case BlendOp.Default:
			gl.blendEquation(gl.FUNC_ADD);
			gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
			break;
		case BlendOp.Add:
			gl.blendEquation(gl.FUNC_ADD);
			gl.blendFunc(gl.ONE, gl.ONE);
			break;
		case BlendOp.Average:
			gl.blendEquation(gl.FUNC_ADD);
			gl.blendFunc(gl.CONSTANT_COLOR, gl.CONSTANT_COLOR);
			gl.blendColor(0.5, 0.5, 0.5, 0.5);
			break;
		case BlendOp.CopyAlpha:
			gl.blendEquation(gl.FUNC_ADD);
			gl.blendFuncSeparate(gl.ZERO, gl.ONE, gl.ONE, gl.ZERO);
			break;
		case BlendOp.CopyRGB:
			gl.blendEquation(gl.FUNC_ADD);
			gl.blendFuncSeparate(gl.ONE, gl.ZERO, gl.ZERO, gl.ONE);
			break;
		case BlendOp.Invert:
			gl.blendEquation(gl.FUNC_ADD);
			gl.blendFunc(gl.ZERO, gl.ONE_MINUS_SRC_COLOR);
			break;
		case BlendOp.Multiply:
			gl.blendEquation(gl.FUNC_ADD);
			gl.blendFunc(gl.DST_COLOR, gl.ZERO);
			break;
		case BlendOp.Replace:
			gl.blendEquation(gl.FUNC_ADD);
			gl.blendFunc(gl.ONE, gl.ZERO);
			break;
		case BlendOp.Subtract:
			gl.blendEquation(gl.FUNC_REVERSE_SUBTRACT);
			gl.blendFunc(gl.ONE, gl.ONE);
			break;
		default:
			// something went horribly wrong if we got here; just set the blender to output
			// nothing so the user can see something went awry.
			gl.blendEquation(gl.FUNC_ADD);
			gl.blendFunc(gl.ZERO, gl.ZERO);
	}
}

function applyDepthOp(depthOp)
{
	const depthFunc = depthOp === DepthOp.AlwaysPass ? gl.ALWAYS
		: depthOp === DepthOp.Equal ? gl.EQUAL
		: depthOp === DepthOp.Greater ? gl.GREATER
		: depthOp === DepthOp.GreaterOrEqual ? gl.GEQUAL
		: depthOp === DepthOp.Less ? gl.LESS
		: depthOp === DepthOp.LessOrEqual ? gl.LEQUAL
		: depthOp === DepthOp.NotEqual ? gl.NOTEQUAL
		: gl.NEVER;
	gl.depthFunc(depthFunc);
}
