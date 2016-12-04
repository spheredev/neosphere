/**
 *  miniRT term CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
module.exports =
{
	get visible() { return visible.yes; },
	set visible(value) { value ? _show() : _hide(); },
	define:    define,
	print:     print,
	undefine:  undefine,
};

const from    = require('from');
const prim    = require('prim');
const scenes  = require('scenes');
const threads = require('threads');

var keyboard = Keyboard.Default;
var mouse = Mouse.Default;
var font = Font.Default;
var nextLine = 0;
var numLines = 0;
var visible = { yes: false, fade: 0.0, line: 0.0 };
var wasKeyDown = false;

var numLines = Math.floor((screen.height - 32) / font.height);
var bufferSize = 1000;
var prompt = "$";

var buffer = [];
var commands = [];
var entry = "";
var cursorColor = Color.Gold;
var fps = screen.frameRate;
new scenes.Scene()
	.doWhile(function() { return true; })
		.tween(cursorColor, 0.25 * fps, 'easeInSine', { alpha: 255 })
		.tween(cursorColor, 0.25 * fps, 'easeOutSine', { alpha: 64 })
	.end()
	.run();
threads.create({
	update:	  update,
	render:	  render,
	getInput: getInput,
}, Infinity);

print(system.game.name + " miniRT Console");
print(system.name + " " + system.version + " - Sphere v" + system.apiVersion + " L" + system.apiLevel + " API");
print("");

function executeCommand(command)
{
	// tokenize the command string
	var tokens = command.match(/'.*?'|".*?"|\S+/g);
	if (tokens == null) return;
	for (var i = 0; i < tokens.length; ++i) {
		tokens[i] = tokens[i].replace(/'(.*)'/, "$1");
		tokens[i] = tokens[i].replace(/"(.*)"/, "$1");
	}
	var entity = tokens[0];
	var instruction = tokens[1];

	// check that the instruction is valid
	if (!from.Array(commands)
		.any(function(c) { return entity == c.entity; }))
	{
		print("Entity name '" + entity + "' not recognized");
		return;
	}
	if (tokens.length < 2) {
		print("No instruction provided for '" + entity + "'");
		return;
	}
	if (!from.Array(commands)
		.where(function(c) { return entity == c.entity; })
		.any(function(c) { return instruction == c.instruction; }))
	{
		print("Instruction '" + instruction + "' not valid for '" + entity + "'");
		return;
	}

	// parse arguments
	for (var i = 2; i < tokens.length; ++i) {
		var maybeNumber = parseFloat(tokens[i]);
		tokens[i] = !isNaN(maybeNumber) ? maybeNumber : tokens[i];
	}

	// execute the command
	from.Array(commands)
		.where(function(c) { return entity == c.entity; })
		.where(function(c) { return instruction == c.instruction; })
		.each(function(desc)
	{
		threads.create({
			update: function() {
				desc.method.apply(desc.that, tokens.slice(2));
			}
		});
	});
}

function getInput()
{
	if (!wasKeyDown && keyboard.isPressed(Key.Tilde)) {
		if (!visible.yes)
			_show();
		else
			_hide();
	}
	wasKeyDown = keyboard.isPressed(Key.Tilde);
	if (visible.yes) {
		var mouseEvent = mouse.getEvent();
		var wheelUp = mouseEvent !== null && mouseEvent.key == MouseKey.WheelUp;
		var wheelDown = mouseEvent !== null && mouseEvent.key == MouseKey.WheelDown;
		var speed = (wheelUp || wheelDown) ? 1.0 : 0.5;
		if (keyboard.isPressed(Key.PageUp) || wheelUp) {
			visible.line = Math.min(visible.line + speed, buffer.length - numLines);
		} else if (keyboard.isPressed(Key.PageDown) || wheelDown) {
			visible.line = Math.max(visible.line - speed, 0);
		}
		var keycode = keyboard.getKey();
		var fps = screen.frameRate;
		switch (keycode) {
			case Key.Enter:
				print("Command entered: '" + entry + "'");
				executeCommand(entry);
				entry = "";
				break;
			case Key.Backspace:
				entry = entry.slice(0, -1);
				break;
			case Key.Home:
				var newLine = buffer.length - numLines;
				new scenes.Scene()
					.tween(visible, 0.125 * fps, 'easeOut', { line: newLine })
					.run();
				break;
			case Key.End:
				new scenes.Scene()
					.tween(visible, 0.125 * fps, 'easeOut', { line: 0.0 })
					.run();
				break;
			case Key.Tab:
			case Key.Tilde:
			case null:
                break;
			default:
				var isShifted = keyboard.isPressed(Key.LShift) || keyboard.isPressed(Key.RShift);
				var ch = keyboard.getChar(keycode, isShifted);
				ch = keyboard.capsLock ? ch.toUpperCase() : ch;
				entry += ch;
		}
	}
}

function render()
{
	if (visible.fade <= 0.0)
		return;

	// draw the command prompt...
	var boxY = -22 * (1.0 - visible.fade);
	prim.rect(screen, 0, boxY, screen.width, 22, Color.Black.fade(visible.fade * 0.875));
	var promptWidth = font.getTextSize(prompt + " ").width;
	font.drawText(screen, 6, 6 + boxY, prompt, Color.Black.fade(visible.fade * 0.75));
	font.drawText(screen, 5, 5 + boxY, prompt, Color.Gray.fade(visible.fade * 0.75));
	font.drawText(screen, 6 + promptWidth, 6 + boxY, entry, Color.Black.fade(visible.fade * 0.75));
	font.drawText(screen, 5 + promptWidth, 5 + boxY, entry, Color.Gold.fade(visible.fade * 0.75));
	font.drawText(screen, 5 + promptWidth + font.getTextSize(entry).width, 5 + boxY, "_", cursorColor);

	// ...then the console output
	var boxHeight = numLines * font.height + 10;
	var boxY = screen.height - boxHeight * visible.fade;
	prim.rect(screen, 0, boxY, screen.width, boxHeight, Color.Black.fade(visible.fade * 0.75));
	screen.clipTo(5, boxY + 5, screen.width - 10, boxHeight - 10);
	for (var i = -1; i < numLines + 1; ++i) {
		var lineToDraw = (nextLine - numLines) + i - Math.floor(visible.line);
		var lineInBuffer = lineToDraw % bufferSize;
		if (lineToDraw >= 0 && buffer[lineInBuffer] != null) {
			var y = boxY + 5 + i * font.height;
			y += (visible.line - Math.floor(visible.line)) * font.height;
			font.drawText(screen, 6, y + 1, buffer[lineInBuffer], Color.Black.fade(visible.fade * 0.75));
			font.drawText(screen, 5, y, buffer[lineInBuffer], Color.White.fade(visible.fade * 0.75));
		}
	}
	screen.clipTo(0, 0, screen.width, screen.height);
}

function update()
{
	if (visible.fade <= 0.0) {
		visible.line = 0.0;
	}
	return true;
}

function print(/*...*/)
{
	var lineInBuffer = nextLine % bufferSize;
	buffer[lineInBuffer] = ">" + arguments[0];
	for (var i = 1; i < arguments.length; ++i) {
		buffer[lineInBuffer] += " >>" + arguments[1];
	}
	++nextLine;
	visible.line = 0.0;
	console.log(buffer[lineInBuffer]);
}

function define(name, that, methods)
{
	for (var instruction in methods) {
		commands.push({
			entity: name,
			instruction: instruction,
			that: that,
			method: methods[instruction]
		});
	}
}

function undefine(name)
{
	from.Array(commands)
		.where(function(c) { return c.entity == name; })
		.remove();
}

function _hide()
{
	var fps = screen.frameRate;
	new scenes.Scene()
		.tween(visible, 0.25 * fps, 'easeInQuad', { fade: 0.0 })
		.call(function() { visible.yes = false; entry = ""; })
		.run();
}

function _show()
{
	var fps = screen.frameRate;
	new scenes.Scene()
		.tween(visible, 0.25 * fps, 'easeOutQuad', { fade: 1.0 })
		.call(function() { visible.yes = true; })
		.run();
}
