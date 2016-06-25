/**
 *  miniRT terminal CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
module.exports =
{
	isVisible:  isVisible,
	hide:       hide,
	log:        log,
	register:   register,
	show:       show,
	unregister: unregister,
};

const link	  = require('link');
const prim	  = require('prim');
const scenes  = require('scenes');
const threads = require('threads');

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
new scenes.Scene()
	.doWhile(function() { return true; })
		.tween(cursorColor, 0.25, 'easeInSine', { alpha: 255 })
		.tween(cursorColor, 0.25, 'easeOutSine', { alpha: 64 })
	.end()
	.run();
threads.create({
	update:	  update,
	render:	  render,
	getInput: getInput,
}, Infinity);

log(engine.game.name + " Console");
log(engine.name + " " + engine.version + " (G" + engine.apiVersion + ", API v" + engine.apiLevel + ")");
log("");

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
	if (!link(commands)
		.pluck('entity')
		.contains(entity))
	{
		log("Entity name '" + entity + "' not recognized");
		return;
	}
	if (tokens.length < 2) {
		log("No instruction provided for '" + entity + "'");
		return;
	}
	if (!link(commands)
		.filterBy('entity', entity)
		.pluck('instruction')
		.contains(instruction))
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
	link(commands)
		.filterBy('instruction', instruction)
		.filterBy('entity', entity)
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
	if (!wasKeyDown && kb.isPressed(Key.Tab)) {
		if (!isVisible())
			show();
		else
			hide();
	}
	wasKeyDown = kb.isPressed(Key.Tab);
	if (isVisible()) {
		if (kb.isPressed(Key.PageUp)) {
			visible.line = Math.min(visible.line + 0.5, buffer.length - numLines);
		} else if (kb.isPressed(Key.PageDown)) {
			visible.line = Math.max(visible.line - 0.5, 0);
		}
		var keycode = kb.getKey();
		switch (keycode) {
			case Key.Enter:
				log("Command entered: '" + entry + "'");
				executeCommand(entry);
				entry = "";
				break;
			case Key.Backspace:
				entry = entry.slice(0, -1);
				break;
			case Key.Home:
				var newLine = buffer.length - numLines;
				new scenes.Scene()
					.tween(visible, 0.125, 'easeOut', { line: newLine })
					.run();
				break;
			case Key.End:
				new scenes.Scene()
					.tween(visible, 0.125, 'easeOut', { line: 0.0 })
					.run();
				break;
			case Key.Tab: break;
			case Key.None: break;
			default:
				var isShifted = kb.isPressed(Key.LShift) || kb.isPressed(Key.RShift);
				var ch = kb.keyString(keycode, isShifted);
				ch = kb.capsLock ? ch.toUpperCase() : ch;
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
	var promptWidth = font.getStringWidth(prompt + " ");
	font.drawText(screen, 6, 6 + boxY, prompt, Color.Black.fade(visible.fade * 0.75));
	font.drawText(screen, 5, 5 + boxY, prompt, Color.Gray.fade(visible.fade * 0.75));
	font.drawText(screen, 6 + promptWidth, 6 + boxY, entry, Color.Black.fade(visible.fade * 0.75));
	font.drawText(screen, 5 + promptWidth, 5 + boxY, entry, Color.Gold.fade(visible.fade * 0.75));
	font.drawText(screen, 5 + promptWidth + font.getStringWidth(entry), 5 + boxY, "_", cursorColor);

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

function isVisible()
{
	return visible.yes;
}

function hide()
{
	new scenes.Scene()
		.tween(visible, 0.25, 'easeInQuad', { fade: 0.0 })
		.call(function() { visible.yes = false; entry = ""; })
		.run();
}

function log(/*...*/)
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

function register(name, that, methods)
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

function show()
{
	new scenes.Scene()
		.tween(visible, 0.25, 'easeOutQuad', { fade: 1.0 })
		.call(function() { visible.yes = true; })
		.run();
}

function unregister(name)
{
	commands = link(commands)
		.where(function(command) { return command.entity != name; })
		.toArray();
}
