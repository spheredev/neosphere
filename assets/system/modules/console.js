/**
 *  Sphere v2 Runtime: Console module
 *  (c) 2015-2017 Fat Cerberus
**/

'use strict';
exports.__esModule = true;
exports.default = exports;

const from   = require('from'),
      Logger = require('logger'),
      Prim   = require('prim'),
      Scene  = require('scene');

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
var logger = 'logPath' in Sphere.Game
    ? new Logger(Sphere.Game.logPath)
    : null;
new Scene()
	.doWhile(function() { return true; })
		.tween(cursorColor, 0.25 * fps, 'easeInSine', { alpha: 255 })
		.tween(cursorColor, 0.25 * fps, 'easeOutSine', { alpha: 64 })
	.end()
	.run();
Dispatch.onRender(_render, Infinity);
Dispatch.onUpdate(function() {
	_getInput();
	_update();
}, Infinity);

log(Sphere.Game.name + " Command Console");
log(Sphere.Platform + " " + Sphere.Version + " - Sphere v" + Sphere.APIVersion + " L" + Sphere.APILevel + " API");
log("");

Object.defineProperty(exports, 'visible',
{
	enumerable: true, configurable: true,
	get: function() { return visible.yes; },
	set: function(value) { value ? _show() : _hide(); },
});

exports.define = define;
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

exports.log = log;
function log(/*...*/)
{
	var lineInBuffer = nextLine % bufferSize;
	buffer[lineInBuffer] = ">" + arguments[0];
	for (var i = 1; i < arguments.length; ++i) {
		buffer[lineInBuffer] += " >>" + arguments[i];
	}
	++nextLine;
	visible.line = 0.0;
	if (logger !== null)
        logger.write(buffer[lineInBuffer]);
}

exports.undefine = undefine;
function undefine(name)
{
	from.Array(commands)
		.where(function(c) { return c.entity == name; })
		.remove();
}

function _executeCommand(command)
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
		log("Entity name '" + entity + "' not recognized");
		return;
	}
	if (tokens.length < 2) {
		log("No instruction provided for '" + entity + "'");
		return;
	}
	if (!from.Array(commands)
		.where(function(c) { return entity == c.entity; })
		.any(function(c) { return instruction == c.instruction; }))
	{
		log("Instruction '" + instruction + "' not valid for '" + entity + "'");
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
		Dispatch.now(function() {
			desc.method.apply(desc.that, tokens.slice(2));
		});
	});
}

function _getInput()
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
				log("Command entered: '" + entry + "'");
				_executeCommand(entry);
				entry = "";
				break;
			case Key.Backspace:
				entry = entry.slice(0, -1);
				break;
			case Key.Home:
				var newLine = buffer.length - numLines;
				new Scene()
					.tween(visible, 0.125 * fps, 'easeOut', { line: newLine })
					.run();
				break;
			case Key.End:
				new Scene()
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

function _hide()
{
	var fps = screen.frameRate;
	new Scene()
		.tween(visible, 0.25 * fps, 'easeInQuad', { fade: 0.0 })
		.call(function() { visible.yes = false; entry = ""; })
		.run();
}

function _render()
{
	if (visible.fade <= 0.0)
		return;

	// draw the command prompt...
	var boxY = -22 * (1.0 - visible.fade);
	Prim.rect(screen, 0, boxY, screen.width, 22, Color.Black.fade(visible.fade * 0.875));
	var promptWidth = font.getTextSize(prompt + " ").width;
	font.drawText(screen, 6, 6 + boxY, prompt, Color.Black.fade(visible.fade * 0.75));
	font.drawText(screen, 5, 5 + boxY, prompt, Color.Gray.fade(visible.fade * 0.75));
	font.drawText(screen, 6 + promptWidth, 6 + boxY, entry, Color.Black.fade(visible.fade * 0.75));
	font.drawText(screen, 5 + promptWidth, 5 + boxY, entry, Color.Gold.fade(visible.fade * 0.75));
	font.drawText(screen, 5 + promptWidth + font.getTextSize(entry).width, 5 + boxY, "_", cursorColor);

	// ...then the console output
	var boxHeight = numLines * font.height + 10;
	var boxY = screen.height - boxHeight * visible.fade;
	Prim.rect(screen, 0, boxY, screen.width, boxHeight, Color.Black.fade(visible.fade * 0.75));
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

function _show()
{
	var fps = screen.frameRate;
	new Scene()
		.tween(visible, 0.25 * fps, 'easeOutQuad', { fade: 1.0 })
		.call(function() { visible.yes = true; })
		.run();
}

function _update()
{
	if (visible.fade <= 0.0)
		visible.line = 0.0;
	return true;
}
