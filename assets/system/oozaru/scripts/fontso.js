/**
 *  Oozaru: Sphere for the Web
 *  Copyright (c) 2016-2024, Fat Cerberus
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

import { DataStream } from './data-stream.js';
import Fido from './fido.js';
import { Color, Shape, ShapeType, Texture } from './galileo.js';
import Game from './game.js';

var defaultFont;

export default
class Fontso
{
    static async initialize()
    {
		defaultFont = await Font.fromFile('#/default.rfn');
    }
}

export
class Font
{
	#fileName;
	#glyphAtlas;
	#glyphData = [];
	#lineHeight = 0;
	#maxWidth = 0;
	#numGlyphs = 0;
	#stride;

	static get Default()
	{
		return defaultFont;
	}

	static async fromFile(fileName)
	{
		const fontURL = Game.urlOf(fileName);
		const fileData = await Fido.fetchData(fontURL);
		const font = new Font(fileData);
		font.#fileName = Game.fullPath(fileName);
		return font;
	}

	constructor(...args)
	{
		if (typeof args[0] === 'string') {
			throw Error("'new Font' from filename is not supported");
		}
		else if (args[0] instanceof ArrayBuffer) {
			let dataStream = new DataStream(args[0]);
			let rfnHeader = dataStream.readStruct({
				signature: 'string/4',
				version:   'uint16-le',
				numGlyphs: 'uint16-le',
				reserved:  'reserve/248',
			});
			if (rfnHeader.signature !== '.rfn')
				throw Error(`Unable to load RFN font file`);
			if (rfnHeader.version < 2 || rfnHeader.version > 2)
				throw Error(`Unsupported RFN version '${rfnHeader.version}'`);
			if (rfnHeader.numGlyphs <= 0)
				throw Error(`Malformed RFN font (no glyphs)`);
			const numAcross = Math.ceil(Math.sqrt(rfnHeader.numGlyphs));
			this.#stride = 1.0 / numAcross;
			for (let i = 0; i < rfnHeader.numGlyphs; ++i) {
				let charInfo = dataStream.readStruct({
					width:    'uint16-le',
					height:   'uint16-le',
					reserved: 'reserve/28',
				});
				this.#lineHeight = Math.max(this.#lineHeight, charInfo.height);
				this.#maxWidth = Math.max(this.#maxWidth, charInfo.width);
				const pixelData = dataStream.readBytes(charInfo.width * charInfo.height * 4);
				this.#glyphData.push({
					width: charInfo.width,
					height: charInfo.height,
					u: i % numAcross / numAcross,
					v: 1.0 - Math.floor(i / numAcross) / numAcross,
					pixelData,
				});
			}
			this.#glyphAtlas = new Texture(numAcross * this.#maxWidth, numAcross * this.#lineHeight);
			this.#numGlyphs = rfnHeader.numGlyphs;
			for (let i = 0; i < this.#numGlyphs; ++i) {
				const glyph = this.#glyphData[i];
				const x = i % numAcross * this.#maxWidth;
				const y = Math.floor(i / numAcross) * this.#lineHeight;
				this.#glyphAtlas.upload(glyph.pixelData, x, y, glyph.width, glyph.height);
			}
		}
		else {
			throw RangeError("Invalid argument(s) passed to 'new Font'.");
		}
	}

	get fileName()
	{
		return this.#fileName;
	}

	get height()
	{
		return this.#lineHeight;
	}

	drawText(surface, x, y, text, color = Color.White, wrapWidth)
	{
		text = text.toString();
		const lines = wrapWidth !== undefined
			? this.wordWrap(text, wrapWidth)
			: [ text ];
		for (let i = 0, len = lines.length; i < len; ++i)
			this.#renderString(surface, x, y + i * this.#lineHeight, lines[i], color);
	}

	getTextSize(text, wrapWidth)
	{
		text = text.toString();
		if (wrapWidth !== undefined) {
			const lines = this.wordWrap(text, wrapWidth);
			return {
				width: wrapWidth,
				height: lines.length * this.#lineHeight,
			};
		}
		else {
			return {
				width: this.widthOf(text),
				height: this.#lineHeight,
			};
		}
	}

	heightOf(text, wrapWidth)
	{
		return this.getTextSize(text, wrapWidth).height;
	}

	widthOf(text)
	{
		text = text.toString();
		let cp;
		let ptr = 0;
		let width = 0;
		while ((cp = text.codePointAt(ptr++)) !== undefined) {
			if (cp > 0xFFFF)  // surrogate pair?
				++ptr;
			cp = toCP1252(cp);
			if (cp >= this.#numGlyphs)
				cp = 0x1A;
			width += this.#glyphData[cp].width;
		}
		return width;
	}

	wordWrap(text, wrapWidth)
	{
		text = text.toString();
		const lines = [];
		const codepoints = [];
		let currentLine = "";
		let lineWidth = 0;
		let lineFinished = false;
		let wordWidth = 0;
		let wordFinished = false;
		let cp;
		let ptr = 0;
		while ((cp = text.codePointAt(ptr++)) !== undefined) {
			if (cp > 0xFFFF)  // surrogate pair?
				++ptr;
			cp = toCP1252(cp);
			if (cp >= this.#numGlyphs)
				cp = 0x1A;
			const glyph = this.#glyphData[cp];
			switch (cp) {
				case 13: case 10:  // newline
					if (cp === 13 && text.codePointAt(ptr) == 10)
						++ptr;  // treat CRLF as a single newline
					lineFinished = true;
					break;
				case 8:  // tab
					codepoints.push(cp);
					wordWidth += this.#glyphData[32].width * 3;
					wordFinished = true;
					break;
				case 32:  // space
					codepoints.push(cp);
					wordWidth += glyph.width;
					wordFinished = true;
					break;
				default:
					codepoints.push(cp);
					wordWidth += glyph.width;
					break;
			}
			if (wordFinished || lineFinished) {
				currentLine += String.fromCodePoint(...codepoints);
				lineWidth += wordWidth;
				codepoints.length = 0;
				wordWidth = 0;
				wordFinished = false;
			}
			if (lineWidth + wordWidth > wrapWidth || lineFinished) {
				lines.push(currentLine);
				currentLine = "";
				lineWidth = 0;
				lineFinished = false;
			}
		}
		currentLine += String.fromCodePoint(...codepoints);
		if (currentLine !== "")
			lines.push(currentLine);
		return lines;
	}

	#renderString(surface, x, y, text, color)
	{
		x = Math.trunc(x);
        y = Math.trunc(y);
        if (text === "")
			return;  // empty string, nothing to render
		let cp;
		let ptr = 0;
		let xOffset = 0;
		const vertices = [];
		while ((cp = text.codePointAt(ptr++)) !== undefined) {
			if (cp > 0xFFFF)  // surrogate pair?
				++ptr;
			cp = toCP1252(cp);
			if (cp >= this.#numGlyphs)
				cp = 0x1A;
			const glyph = this.#glyphData[cp];
			const x1 = x + xOffset, x2 = x1 + glyph.width;
			const y1 = y, y2 = y1 + glyph.height;
			const u1 = glyph.u;
			const u2 = u1 + glyph.width / this.#maxWidth * this.#stride;
			const v1 = glyph.v;
			const v2 = v1 - glyph.height / this.#lineHeight * this.#stride;
			vertices.push(
				{ x: x1, y: y1, u: u1, v: v1, color },
				{ x: x2, y: y1, u: u2, v: v1, color },
				{ x: x1, y: y2, u: u1, v: v2, color },
				{ x: x2, y: y1, u: u2, v: v1, color },
				{ x: x1, y: y2, u: u1, v: v2, color },
				{ x: x2, y: y2, u: u2, v: v2, color },
			);
			xOffset += glyph.width;
		}
        Shape.drawImmediate(surface, ShapeType.Triangles, this.#glyphAtlas, vertices);
	}
}

function toCP1252(codepoint)
{
	return codepoint == 0x20AC ? 128
		: codepoint == 0x201A ? 130
		: codepoint == 0x0192 ? 131
		: codepoint == 0x201E ? 132
		: codepoint == 0x2026 ? 133
		: codepoint == 0x2020 ? 134
		: codepoint == 0x2021 ? 135
		: codepoint == 0x02C6 ? 136
		: codepoint == 0x2030 ? 137
		: codepoint == 0x0160 ? 138
		: codepoint == 0x2039 ? 139
		: codepoint == 0x0152 ? 140
		: codepoint == 0x017D ? 142
		: codepoint == 0x2018 ? 145
		: codepoint == 0x2019 ? 146
		: codepoint == 0x201C ? 147
		: codepoint == 0x201D ? 148
		: codepoint == 0x2022 ? 149
		: codepoint == 0x2013 ? 150
		: codepoint == 0x2014 ? 151
		: codepoint == 0x02DC ? 152
		: codepoint == 0x2122 ? 153
		: codepoint == 0x0161 ? 154
		: codepoint == 0x203A ? 155
		: codepoint == 0x0153 ? 156
		: codepoint == 0x017E ? 158
		: codepoint == 0x0178 ? 159
		: codepoint;
}
