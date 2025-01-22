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

export
const Key =
{
	Alt: 0,
	AltGr: 1,
	Apostrophe: 2,
	Backslash: 3,
	Backspace: 4,
	CapsLock: 5,
	CloseBrace: 6,
	Comma: 7,
	Delete: 8,
	Down: 9,
	End: 10,
	Enter: 11,
	Equals: 12,
	Escape: 13,
	F1: 14,
	F2: 15,
	F3: 16,
	F4: 17,
	F5: 18,
	F6: 19,
	F7: 20,
	F8: 21,
	F9: 22,
	F10: 23,
	F11: 24,
	F12: 25,
	Home: 26,
	Hyphen: 27,
	Insert: 28,
	LCtrl: 29,
	LShift: 30,
	Left: 31,
	NumLock: 32,
	OpenBrace: 33,
	PageDown: 34,
	PageUp: 35,
	Period: 36,
	RCtrl: 37,
	RShift: 38,
	Right: 39,
	ScrollLock: 40,
	Semicolon: 41,
	Slash: 42,
	Space: 43,
	Tab: 44,
	Tilde: 45,
	Up: 46,
	A: 47,
	B: 48,
	C: 49,
	D: 50,
	E: 51,
	F: 52,
	G: 53,
	H: 54,
	I: 55,
	J: 56,
	K: 57,
	L: 58,
	M: 59,
	N: 60,
	O: 61,
	P: 62,
	Q: 63,
	R: 64,
	S: 65,
	T: 66,
	U: 67,
	V: 68,
	W: 69,
	X: 70,
	Y: 71,
	Z: 72,
	D1: 73,
	D2: 74,
	D3: 75,
	D4: 76,
	D5: 77,
	D6: 78,
	D7: 79,
	D8: 80,
	D9: 81,
	D0: 82,
	NumPad1: 83,
	NumPad2: 84,
	NumPad3: 85,
	NumPad4: 86,
	NumPad5: 87,
	NumPad6: 88,
	NumPad7: 89,
	NumPad8: 90,
	NumPad9: 91,
	NumPad0: 92,
	NumPadEnter: 93,
	Add: 94,
	Decimal: 95,
	Divide: 96,
	Multiply: 97,
	Subtract: 98,
};

export
const MouseKey =
{
	Left: 0,
	Middle: 1,
	Right: 2,
	Back: 3,
	Forward: 4,
	WheelUp: 5,
	WheelDown: 6,
};

var buttonStates = {};
var keyQueue = [];
var keyStates = { '': false };
var lastMouseX = undefined;
var lastMouseY = undefined;
var mouseQueue = [];
var nullJoystick;

export default
class InputEngine
{
	static initialize(canvas)
	{
		canvas.addEventListener('contextmenu', (e) => {
			e.preventDefault();
		});

		canvas.addEventListener('keydown', (e) => {
			e.preventDefault();
			keyStates[e.code] = true;
			switch (e.code) {
				case 'ArrowLeft': keyQueue.push(Key.Left); break;
				case 'ArrowRight': keyQueue.push(Key.Right); break;
				case 'ArrowDown': keyQueue.push(Key.Down); break;
				case 'ArrowUp': keyQueue.push(Key.Up); break;
				case 'Backquote': keyQueue.push(Key.Tilde); break;
				case 'Backslash': keyQueue.push(Key.Backslash); break;
				case 'Backspace': keyQueue.push(Key.Backspace); break;
				case 'BracketLeft': keyQueue.push(Key.OpenBrace); break;
				case 'BracketRight': keyQueue.push(Key.CloseBrace); break;
				case 'Comma': keyQueue.push(Key.Comma); break;
				case 'Delete': keyQueue.push(Key.Delete); break;
				case 'Digit0': keyQueue.push(Key.D0); break;
				case 'Digit1': keyQueue.push(Key.D1); break;
				case 'Digit2': keyQueue.push(Key.D2); break;
				case 'Digit3': keyQueue.push(Key.D3); break;
				case 'Digit4': keyQueue.push(Key.D4); break;
				case 'Digit5': keyQueue.push(Key.D5); break;
				case 'Digit6': keyQueue.push(Key.D6); break;
				case 'Digit7': keyQueue.push(Key.D7); break;
				case 'Digit8': keyQueue.push(Key.D8); break;
				case 'Digit9': keyQueue.push(Key.D9); break;
				case 'End': keyQueue.push(Key.End); break;
				case 'Enter': keyQueue.push(Key.Enter); break;
				case 'Equal': keyQueue.push(Key.Equals); break;
				case 'Escape': keyQueue.push(Key.Escape); break;
				case 'F1': keyQueue.push(Key.F1); break;
				case 'F2': keyQueue.push(Key.F2); break;
				case 'F3': keyQueue.push(Key.F3); break;
				case 'F4': keyQueue.push(Key.F4); break;
				case 'F5': keyQueue.push(Key.F5); break;
				case 'F6': keyQueue.push(Key.F6); break;
				case 'F7': keyQueue.push(Key.F7); break;
				case 'F8': keyQueue.push(Key.F8); break;
				case 'F9': keyQueue.push(Key.F9); break;
				case 'F10': keyQueue.push(Key.F10); break;
				case 'F11': keyQueue.push(Key.F11); break;
				case 'F12': keyQueue.push(Key.F12); break;
				case 'Home': keyQueue.push(Key.Home); break;
				case 'Insert': keyQueue.push(Key.Insert); break;
				case 'KeyA': keyQueue.push(Key.A); break;
				case 'KeyB': keyQueue.push(Key.B); break;
				case 'KeyC': keyQueue.push(Key.C); break;
				case 'KeyD': keyQueue.push(Key.D); break;
				case 'KeyE': keyQueue.push(Key.E); break;
				case 'KeyF': keyQueue.push(Key.F); break;
				case 'KeyG': keyQueue.push(Key.G); break;
				case 'KeyH': keyQueue.push(Key.H); break;
				case 'KeyI': keyQueue.push(Key.I); break;
				case 'KeyJ': keyQueue.push(Key.J); break;
				case 'KeyK': keyQueue.push(Key.K); break;
				case 'KeyL': keyQueue.push(Key.L); break;
				case 'KeyM': keyQueue.push(Key.M); break;
				case 'KeyN': keyQueue.push(Key.N); break;
				case 'KeyO': keyQueue.push(Key.O); break;
				case 'KeyP': keyQueue.push(Key.P); break;
				case 'KeyQ': keyQueue.push(Key.Q); break;
				case 'KeyR': keyQueue.push(Key.R); break;
				case 'KeyS': keyQueue.push(Key.S); break;
				case 'KeyT': keyQueue.push(Key.T); break;
				case 'KeyU': keyQueue.push(Key.U); break;
				case 'KeyV': keyQueue.push(Key.V); break;
				case 'KeyW': keyQueue.push(Key.W); break;
				case 'KeyX': keyQueue.push(Key.X); break;
				case 'KeyY': keyQueue.push(Key.Y); break;
				case 'KeyZ': keyQueue.push(Key.Z); break;
				case 'Minus': keyQueue.push(Key.Hyphen); break;
				case 'Numpad0': keyQueue.push(Key.NumPad0); break;
				case 'Numpad1': keyQueue.push(Key.NumPad1); break;
				case 'Numpad2': keyQueue.push(Key.NumPad2); break;
				case 'Numpad3': keyQueue.push(Key.NumPad3); break;
				case 'Numpad4': keyQueue.push(Key.NumPad4); break;
				case 'Numpad5': keyQueue.push(Key.NumPad5); break;
				case 'Numpad6': keyQueue.push(Key.NumPad6); break;
				case 'Numpad7': keyQueue.push(Key.NumPad7); break;
				case 'Numpad8': keyQueue.push(Key.NumPad8); break;
				case 'Numpad9': keyQueue.push(Key.NumPad9); break;
				case 'NumpadAdd': keyQueue.push(Key.Add); break;
				case 'NumpadDecimal': keyQueue.push(Key.Decimal); break;
				case 'NumpadDivide': keyQueue.push(Key.Divide); break;
				case 'NumpadEnter': keyQueue.push(Key.NumPadEnter); break;
				case 'NumpadMultiply': keyQueue.push(Key.Multiply); break;
				case 'NumpadSubtract': keyQueue.push(Key.Subtract); break;
				case 'PageDown': keyQueue.push(Key.PageDown); break;
				case 'PageUp': keyQueue.push(Key.PageUp); break;
				case 'Period': keyQueue.push(Key.Period); break;
				case 'Quote': keyQueue.push(Key.Apostrophe); break;
				case 'Semicolon': keyQueue.push(Key.Semicolon); break;
				case 'Slash': keyQueue.push(Key.Slash); break;
				case 'Space': keyQueue.push(Key.Space); break;
				case 'Tab': keyQueue.push(Key.Tab); break;
			}
		});
		canvas.addEventListener('keyup', e => {
			e.preventDefault();
			keyStates[e.code] = false;
		});

		canvas.addEventListener('mousemove', (e) => {
			e.preventDefault();
			lastMouseX = e.offsetX;
			lastMouseY = e.offsetY;
		});
		canvas.addEventListener('mouseout', (e) => {
			e.preventDefault();
			lastMouseX = undefined;
			lastMouseY = undefined;
		});
		canvas.addEventListener('mousedown', (e) => {
			e.preventDefault();
			canvas.focus();
			const key = e.button === 1 ? MouseKey.Middle
				: e.button === 2 ? MouseKey.Right
				: e.button === 3 ? MouseKey.Back
				: e.button === 4 ? MouseKey.Forward
				: MouseKey.Left;
			buttonStates[key] = true;
		});
		canvas.addEventListener('mouseup', (e) => {
			e.preventDefault();
			const key = e.button === 1 ? MouseKey.Middle
				: e.button === 2 ? MouseKey.Right
				: e.button === 3 ? MouseKey.Back
				: e.button === 4 ? MouseKey.Forward
				: MouseKey.Left;
			buttonStates[key] = false;
			mouseQueue.push({
				key,
				x: e.offsetX,
				y: e.offsetY,
			});
		});
		canvas.addEventListener('wheel', (e) => {
			e.preventDefault();
			const key = e.deltaY < 0.0 ? MouseKey.WheelUp
				: MouseKey.WheelDown;
			mouseQueue.push({
				key,
				delta: Math.abs(e.deltaY),
				x: e.offsetX,
				y: e.offsetY,
			});
		});

		nullJoystick = new Joystick();
	}
}

export
class Joystick
{
	static get P1() { return nullJoystick; }
	static get P2() { return nullJoystick; }
	static get P3() { return nullJoystick; }
	static get P4() { return nullJoystick; }

	static getDevices()
	{
		return [];
	}

	constructor()
	{
	}

	get name()
	{
		return "Null Device";
	}

	get numAxes()
	{
		return Infinity;
	}

	get numButtons()
	{
		return Infinity;
	}

	getPosition(axisID)
	{
		return 0.0;
	}

	isPressed(buttonID)
	{
		return false;
	}
}

export
class Keyboard
{
	static get Default()
	{
		return this;
	}

	static get capsLock()
	{
		return false;
	}

	static get numLock()
	{
		return false;
	}

	static get scrollLock()
	{
		return false;
	}

	static charOf(key, shifted = false)
	{
		return key === Key.Space ? " "
			: key === Key.Apostrophe ? shifted ? "\"" : "'"
			: key === Key.Backslash ? shifted ? "|" : "\\"
			: key === Key.Comma ? shifted ? "<" : ","
			: key === Key.CloseBrace ? shifted ? "}" : "]"
			: key === Key.Equals ? shifted ? "+" : "="
			: key === Key.Hyphen ? shifted ? "_" : "-"
			: key === Key.OpenBrace ? shifted ? "{" : "["
			: key === Key.Period ? shifted ? ">" : "."
			: key === Key.Semicolon ? shifted ? ":" : ";"
			: key === Key.Slash ? shifted ? "?" : "/"
			: key === Key.Tab ? "\t"
			: key === Key.Tilde ? shifted ? "~" : "`"
			: key === Key.D0 ? shifted ? ")" : "0"
			: key === Key.D1 ? shifted ? "!" : "1"
			: key === Key.D2 ? shifted ? "@" : "2"
			: key === Key.D3 ? shifted ? "#" : "3"
			: key === Key.D4 ? shifted ? "$" : "4"
			: key === Key.D5 ? shifted ? "%" : "5"
			: key === Key.D6 ? shifted ? "^" : "6"
			: key === Key.D7 ? shifted ? "&" : "7"
			: key === Key.D8 ? shifted ? "*" : "8"
			: key === Key.D9 ? shifted ? "(" : "9"
			: key === Key.A ? shifted ? "A" : "a"
			: key === Key.B ? shifted ? "B" : "b"
			: key === Key.C ? shifted ? "C" : "c"
			: key === Key.D ? shifted ? "D" : "d"
			: key === Key.E ? shifted ? "E" : "e"
			: key === Key.F ? shifted ? "F" : "f"
			: key === Key.G ? shifted ? "G" : "g"
			: key === Key.H ? shifted ? "H" : "h"
			: key === Key.I ? shifted ? "I" : "i"
			: key === Key.J ? shifted ? "J" : "j"
			: key === Key.K ? shifted ? "K" : "k"
			: key === Key.L ? shifted ? "L" : "l"
			: key === Key.M ? shifted ? "M" : "m"
			: key === Key.N ? shifted ? "N" : "n"
			: key === Key.O ? shifted ? "O" : "o"
			: key === Key.P ? shifted ? "P" : "p"
			: key === Key.Q ? shifted ? "Q" : "q"
			: key === Key.R ? shifted ? "R" : "r"
			: key === Key.S ? shifted ? "S" : "s"
			: key === Key.T ? shifted ? "T" : "t"
			: key === Key.U ? shifted ? "U" : "u"
			: key === Key.V ? shifted ? "V" : "v"
			: key === Key.W ? shifted ? "W" : "w"
			: key === Key.X ? shifted ? "X" : "x"
			: key === Key.Y ? shifted ? "Y" : "y"
			: key === Key.Z ? shifted ? "Z" : "z"
			: "";
	}

	static clearQueue()
	{
		keyQueue.length = 0;
	}

	static getKey()
	{
		return keyQueue.pop() ?? null;
	}

	static isPressed(key)
	{
		const keySpec = key === Key.Tilde ? 'Backquote'
			: key === Key.D0 ? 'Digit0'
			: key === Key.D1 ? 'Digit1'
			: key === Key.D2 ? 'Digit2'
			: key === Key.D3 ? 'Digit3'
			: key === Key.D4 ? 'Digit4'
			: key === Key.D5 ? 'Digit5'
			: key === Key.D6 ? 'Digit6'
			: key === Key.D7 ? 'Digit7'
			: key === Key.D8 ? 'Digit8'
			: key === Key.D9 ? 'Digit9'
			: key === Key.A ? 'KeyA'
			: key === Key.B ? 'KeyB'
			: key === Key.C ? 'KeyC'
			: key === Key.D ? 'KeyD'
			: key === Key.E ? 'KeyE'
			: key === Key.F ? 'KeyF'
			: key === Key.G ? 'KeyG'
			: key === Key.H ? 'KeyH'
			: key === Key.I ? 'KeyI'
			: key === Key.J ? 'KeyJ'
			: key === Key.K ? 'KeyK'
			: key === Key.L ? 'KeyL'
			: key === Key.M ? 'KeyM'
			: key === Key.N ? 'KeyN'
			: key === Key.O ? 'KeyO'
			: key === Key.P ? 'KeyP'
			: key === Key.Q ? 'KeyQ'
			: key === Key.R ? 'KeyR'
			: key === Key.S ? 'KeyS'
			: key === Key.T ? 'KeyT'
			: key === Key.U ? 'KeyU'
			: key === Key.V ? 'KeyV'
			: key === Key.W ? 'KeyW'
			: key === Key.X ? 'KeyX'
			: key === Key.Y ? 'KeyY'
			: key === Key.Z ? 'KeyZ'
			: key === Key.PageDown ? 'PageDown'
			: key === Key.PageUp ? 'PageUp'
			: key === Key.LShift ? 'ShiftLeft'
			: key === Key.LCtrl ? 'ControlLeft'
			: key === Key.Alt ? 'AltLeft'
			: key === Key.RShift ? 'ShiftRight'
			: key === Key.RCtrl ? 'ControlRight'
			: key === Key.AltGr ? 'AltRight'
			: '';
		return keyStates[keySpec];
	}
}

export
class Mouse
{
	static get Default()
	{
		return this;
	}

	static get position()
	{
		return [ lastMouseX, lastMouseY ];
	}

	static get x()
	{
		return lastMouseX;
	}

	static get y()
	{
		return lastMouseY;
	}

	static clearQueue()
	{
		mouseQueue.length = 0;
	}

	static getEvent()
	{
		return mouseQueue.pop() ?? { key: null };
	}

	static isPressed(key)
	{
		return buttonStates[key] ?? false;
	}
}
