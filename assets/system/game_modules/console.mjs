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

import from   from 'from';
import Logger from 'logger';
import Prim   from 'prim';
import Scene  from 'scene';
import Thread from 'thread';

export default
class Console extends Thread
{
	constructor(options = {})
	{
		options = Object.assign({
			hotKey:   Key.Tilde,
			prompt:   "$",
			priority: Infinity,
		}, options);

		super(options);

		this.activationKey = options.hotKey;
		this.buffer = [];
		this.bufferSize = 1000;
		this.commands = [];
		this.cursorColor = Color.Gold;
		this.entry = "";
		this.font = Font.Default;
		this.logger = null;
		this.keyboard = Keyboard.Default;
		this.mouse = Mouse.Default;
		this.nextLine = 0;
		this.numLines = Math.floor((screen.height - 32) / this.font.height);
		this.prompt = options.prompt;
		this.view = { visible: false, fade: 0.0, line: 0.0 };
		this.wasKeyDown = false;

		this.start();
	}

	get visible()
	{
		return this.view.visible;
	}

	set visible(value)
	{
		if (value)
			showConsole(this);
		else
			hideConsole(this);
	}

	defineObject(name, that, methods)
	{
		for (const instruction in methods) {
			this.commands.push({
				entity: name,
				instruction: instruction,
				that: that,
				method: methods[instruction]
			});
		}
	}

	log(...texts)
	{
		let lineInBuffer = this.nextLine % this.bufferSize;
		this.buffer[lineInBuffer] = texts[0];
		for (let i = 1; i < texts.length; ++i) {
			this.buffer[lineInBuffer] += ` >>${texts[i]}`;
		}
		++this.nextLine;
		this.view.line = 0.0;
		SSj.log(this.buffer[lineInBuffer]);

		// if we have a logger, write the line to the log file
		if (this.logger !== null)
			this.logger.write(this.buffer[lineInBuffer]);
	}

	start()
	{
		if (this.running)
			throw new Error("the Console has already been started");

		new Scene()
			.doWhile(() => true)
				.tween(this.cursorColor, 0.25 * screen.frameRate, 'easeInSine', { a: 1.0 })
				.tween(this.cursorColor, 0.25 * screen.frameRate, 'easeOutSine', { a: 0.5 })
			.end()
			.run();

		// create a log file only if the game has a save ID defined
		if ('saveID' in Sphere.Game)
			this.logger = new Logger('consoleOutput');

		this.log(`initializing the Sphere Runtime Console`);
		this.log(`  ${Sphere.Game.name} by ${Sphere.Game.author}`);
		this.log(`  Sphere v${Sphere.Version} API L${Sphere.APILevel} (${Sphere.Platform})`);
		this.log("");

		super.start();
	}

	undefineObject(name)
	{
		from.Array(this.commands)
			.where(it => it.entity == name)
			.remove();
	}

	on_inputCheck()
	{
		if (this.view.visible) {
			let mouseEvent = this.mouse.getEvent();
			let wheelUp = mouseEvent !== null && mouseEvent.key == MouseKey.WheelUp;
			let wheelDown = mouseEvent !== null && mouseEvent.key == MouseKey.WheelDown;
			let speed = (wheelUp || wheelDown) ? 1.0 : 0.5;
			if (this.keyboard.isPressed(Key.PageUp) || wheelUp) {
				this.view.line = Math.min(this.view.line + speed, this.buffer.length - this.numLines);
			} else if (this.keyboard.isPressed(Key.PageDown) || wheelDown) {
				this.view.line = Math.max(this.view.line - speed, 0);
			}
			let keycode = this.keyboard.getKey();
			let fps = screen.frameRate;
			if (keycode === this.activationKey)
				return;
			switch (keycode) {
				case Key.Enter:
					this.log(`executing command line '${this.entry}'`);
					executeCommand(this, this.entry);
					this.entry = "";
					break;
				case Key.Backspace:
					this.entry = this.entry.slice(0, -1);
					break;
				case Key.Home:
					let newLine = this.buffer.length - this.numLines;
					new Scene()
						.tween(this.view, 0.125 * fps, 'easeOut', { line: newLine })
						.run();
					break;
				case Key.End:
					new Scene()
						.tween(this.view, 0.125 * fps, 'easeOut', { line: 0.0 })
						.run();
					break;
				case Key.Tab:
				case null:
					break;
				default:
					let isShifted = this.keyboard.isPressed(Key.LShift) || this.keyboard.isPressed(Key.RShift);
					let ch = this.keyboard.getChar(keycode, isShifted);
					ch = this.keyboard.capsLock ? ch.toUpperCase() : ch;
					this.entry += ch;
			}
		}
	}

	on_render()
	{
		if (this.view.fade <= 0.0)
			return;

		// draw the command prompt...
		let promptWidth = this.font.getTextSize(`${this.prompt} `).width;
		let boxY = -22 * (1.0 - this.view.fade);
		Prim.drawSolidRectangle(screen, 0, boxY, screen.width, 22, Color.Black.fadeTo(this.view.fade * 0.875));
		this.font.drawText(screen, 6, 6 + boxY, this.prompt, Color.Black.fadeTo(this.view.fade * 0.75));
		this.font.drawText(screen, 5, 5 + boxY, this.prompt, Color.Gray.fadeTo(this.view.fade * 0.75));
		this.font.drawText(screen, 6 + promptWidth, 6 + boxY, this.entry, Color.Black.fadeTo(this.view.fade * 0.75));
		this.font.drawText(screen, 5 + promptWidth, 5 + boxY, this.entry, Color.Gold.fadeTo(this.view.fade * 0.75));
		this.font.drawText(screen, 5 + promptWidth + this.font.getTextSize(this.entry).width, 5 + boxY, "_", this.cursorColor);

		// ...then the console output
		let boxHeight = this.numLines * this.font.height + 10;
		boxY = screen.height - boxHeight * this.view.fade;
		Prim.drawSolidRectangle(screen, 0, boxY, screen.width, boxHeight, Color.Black.fadeTo(this.view.fade * 0.75));
		screen.clipTo(5, boxY + 5, screen.width - 10, boxHeight - 10);
		for (let i = -1; i < this.numLines + 1; ++i) {
			let lineToDraw = (this.nextLine - this.numLines) + i - Math.floor(this.view.line);
			let lineInBuffer = lineToDraw % this.bufferSize;
			if (lineToDraw >= 0 && this.buffer[lineInBuffer] != null) {
				let y = boxY + 5 + i * this.font.height;
				y += (this.view.line - Math.floor(this.view.line)) * this.font.height;
				this.font.drawText(screen, 6, y + 1, this.buffer[lineInBuffer], Color.Black.fadeTo(this.view.fade * 0.75));
				this.font.drawText(screen, 5, y, this.buffer[lineInBuffer], Color.White.fadeTo(this.view.fade * 0.75));
			}
		}
		screen.clipTo(0, 0, screen.width, screen.height);
	}

	on_update()
	{
		if (!this.wasKeyDown && this.keyboard.isPressed(this.activationKey)) {
			if (!this.view.visible)
				showConsole(this);
			else
				hideConsole(this);
		}
		this.wasKeyDown = this.keyboard.isPressed(this.activationKey);

		if (this.view.fade <= 0.0)
			this.view.line = 0.0;
	}
}

function executeCommand(console, command)
{
	// tokenize the command string
	let tokens = command.match(/'.*?'|".*?"|\S+/g);
	if (tokens == null)
		return;
	for (let i = 0; i < tokens.length; ++i) {
		tokens[i] = tokens[i].replace(/'(.*)'/, "$1");
		tokens[i] = tokens[i].replace(/"(.*)"/, "$1");
	}
	let objectName = tokens[0];
	let instruction = tokens[1];

	// check that the instruction is valid
	if (!from.Array(console.commands)
		.any(it => it.entity == objectName))
	{
		console.log(`unrecognized object name '${objectName}'`);
		return;
	}
	if (tokens.length < 2) {
		console.log(`missing instruction for '${objectName}'`);
		return;
	}
	if (!from.Array(console.commands)
		.where(it => it.entity == objectName)
		.any(it => it.instruction == instruction))
	{
		console.log(`instruction '${instruction}' not valid for '${objectName}'`);
		return;
	}

	// parse arguments
	for (let i = 2; i < tokens.length; ++i) {
		let maybeNumber = parseFloat(tokens[i]);
		tokens[i] = !isNaN(maybeNumber) ? maybeNumber : tokens[i];
	}

	// execute the command
	let matches = from.Array(console.commands)
		.where(it => it.entity == objectName)
		.where(it => it.instruction == instruction)
	for (const command of matches) {
		Dispatch.now(() => {
			try {
				command.method.apply(command.that, tokens.slice(2));
			}
			catch (e) {
				console.log(`caught JS error '${e.toString()}'`);
			}
		});
	}
}

function hideConsole(console)
{
	console.yieldInput();
	let fps = screen.frameRate;
	new Scene()
		.tween(console.view, 0.25 * fps, 'easeInQuad', { fade: 0.0 })
		.call(() => { console.view.visible = false; console.entry = ""; })
		.run();
}

function showConsole(console)
{
	console.takeInput();
	let fps = screen.frameRate;
	new Scene()
		.tween(console.view, 0.25 * fps, 'easeOutQuad', { fade: 1.0 })
		.call(() => console.view.visible = true)
		.run();
}
