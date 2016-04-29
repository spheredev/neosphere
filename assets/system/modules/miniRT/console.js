/**
 *  miniRT/console CommonJS module
 *  an easy-to-use text console for games with optional logging
 *  (c) 2015-2016 Fat Cerberus
**/

if (typeof exports === 'undefined')
{
	throw new TypeError("script must be loaded with require()");
}

const link    = require('link');
const scenes  = require('./scenes');
const threads = require('./threads');

var console =
module.exports = (function() {
	var game = GetGameManifest();

	var font = GetSystemFont();
	var nextLine = 0;
	var numLines = 0;
	var visible = { yes: false, fade: 0.0, line: 0.0 };
	var wasKeyDown = false;

	var numLines = Math.floor((GetScreenHeight() - 32) / font.getHeight());
	var logFileName = 'logPath' in game ? game.logPath : null;
	var bufferSize = 1000;
	var prompt = "$";

	var logger = typeof logFileName === 'string'
		? new Logger(logFileName) : null;
	var buffer = [];
	var commands = [];
	var entry = "";
	var cursorColor = new Color(255, 255, 128, 255);
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
	}, 100);

	log(game.name + " [miniRT]");
	log("Sphere " + GetVersionString());
	log("");

	register('keymap', this, {
		'set': function(playerKey, keyName, playerID) {
			if (arguments.length < 2)
				return log("keymap set: 2+ arguments expected");
			playerID = playerID !== undefined ? playerID : 1;
			if (playerID < 1 || playerID > 4)
				return log("keymap set: Player ID out of range (" + playerID + ")");
			var keyConst, playerKeyConst;
			if ((keyConst = global["KEY_" + keyName.toUpperCase()]) == undefined)
				return log("keymap set: Invalid key name `" + keyName.toUpperCase() + "`");
			if ((playerKeyConst = global["PLAYER_KEY_" + playerKey.toUpperCase()]) == undefined)
				return log("keymap set: Unknown player key `" + keyName.toUpperCase() + "`");
			SetPlayerKey(playerID - 1, playerKeyConst, keyConst);
			log("PLAYER_KEY_" + playerKey.toUpperCase() + " mapped to KEY_" + keyName.toUpperCase());
		}
	});

	return {
		isOpen:     isOpen,
		append:     append,
		close:      close,
		log:        log,
		open:       open,
		register:   register,
		unregister: unregister,
	};

	function executeCommand(command)
	{
		// NOTES:
		//    * Command format is `<entity_name> <instruction> <arg_1> ... <arg_n>`
		//      e.g.: `cow eat kitties 100`
		//    * Quoted text (single or double quotes) is treated as a single token.
		//    * Numeric arguments are converted to actual JS numbers before being passed to an
		//      instruction method.

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
			threads.createEx(desc, {
				update: function() {
					this.method.apply(this.that, tokens.slice(2));
				}
			});
		});
	};

	function getInput()
	{
		var consoleKey = GetPlayerKey(PLAYER_1, PLAYER_KEY_MENU);
		if (!wasKeyDown && IsKeyPressed(consoleKey)) {
			if (!isOpen())
				open();
			else
				close();
		}
		wasKeyDown = IsKeyPressed(consoleKey);
		if (isOpen()) {
			var wheelKey = GetNumMouseWheelEvents() > 0 ? GetMouseWheelEvent() : null;
			var speed = wheelKey != null ? 1.0 : 0.5;
			if (IsKeyPressed(KEY_PAGEUP) || wheelKey == MOUSE_WHEEL_UP) {
				visible.line = Math.min(visible.line + speed, buffer.length - numLines);
			} else if (IsKeyPressed(KEY_PAGEDOWN) || wheelKey == MOUSE_WHEEL_DOWN) {
				visible.line = Math.max(visible.line - speed, 0);
			}
			var keycode = AreKeysLeft() ? GetKey() : null;
			switch (keycode) {
				case KEY_ENTER:
					log("Command entered: '" + entry + "'");
					executeCommand(entry);
					entry = "";
					break;
				case KEY_BACKSPACE:
					entry = entry.slice(0, -1);
					break;
				case KEY_HOME:
					var newLine = buffer.length - numLines;
					new scenes.Scene()
						.tween(visible, 0.125, 'easeOut', { line: newLine })
						.run();
					break;
				case KEY_END:
					new scenes.Scene()
						.tween(visible, 0.125, 'easeOut', { line: 0.0 })
						.run();
					break;
				case KEY_TAB: break;
				case null: break;
				default:
					var ch = GetKeyString(keycode, IsKeyPressed(KEY_SHIFT));
					ch = GetToggleState(KEY_CAPSLOCK) ? ch.toUpperCase() : ch;
					entry += ch;
			}
		}
	};

	function render()
	{
		if (visible.fade <= 0.0)
			return;

		// draw the command prompt...
		var boxY = -22 * (1.0 - visible.fade);
		Rectangle(0, boxY, GetScreenWidth(), 22, new Color(0, 0, 0, visible.fade * 224));
		var promptWidth = font.getStringWidth(prompt + " ");
		font.setColorMask(new Color(0, 0, 0, visible.fade * 192));
		font.drawText(6, 6 + boxY, prompt);
		font.setColorMask(new Color(128, 128, 128, visible.fade * 192));
		font.drawText(5, 5 + boxY, prompt);
		font.setColorMask(new Color(0, 0, 0, visible.fade * 192));
		font.drawText(6 + promptWidth, 6 + boxY, entry);
		font.setColorMask(new Color(255, 255, 128, visible.fade * 192));
		font.drawText(5 + promptWidth, 5 + boxY, entry);
		font.setColorMask(cursorColor);
		font.drawText(5 + promptWidth + font.getStringWidth(entry), 5 + boxY, "_");

		// ...then the console output
		var boxHeight = numLines * font.getHeight() + 10;
		var boxY = GetScreenHeight() - boxHeight * visible.fade;
		Rectangle(0, boxY, GetScreenWidth(), boxHeight, new Color(0, 0, 0, visible.fade * 192));
		var oldClip = GetClippingRectangle();
		SetClippingRectangle(5, boxY + 5, GetScreenWidth() - 10, boxHeight - 10);
		for (var i = -1; i < numLines + 1; ++i) {
			var lineToDraw = (nextLine - numLines) + i - Math.floor(visible.line);
			var lineInBuffer = lineToDraw % bufferSize;
			if (lineToDraw >= 0 && buffer[lineInBuffer] != null) {
				var y = boxY + 5 + i * font.getHeight();
				y += (visible.line - Math.floor(visible.line)) * font.getHeight();
				font.setColorMask(new Color(0, 0, 0, visible.fade * 192));
				font.drawText(6, y + 1, buffer[lineInBuffer]);
				font.setColorMask(new Color(255, 255, 255, visible.fade * 192));
				font.drawText(5, y, buffer[lineInBuffer]);
			}
		}
		SetClippingRectangle(oldClip.x, oldClip.y, oldClip.width, oldClip.height);
	};

	function update()
	{
		if (visible.fade <= 0.0) {
			visible.line = 0.0;
		}
		return true;
	};

	// console.isOpen()
	// determine whether the console is currently displayed or not.
	// returns:
	//     true if the console is open, false otherwise.
	function isOpen()
	{
		return visible.yes;
	}

	// console.append()
	// append additional output text to the last line in the console.
	// arguments:
	//     text: the text to append.
	function append(text)
	{
		if (nextLine == 0) {
			log(text);
			return;
		}
		var lineInBuffer = (nextLine - 1) % bufferSize;
		buffer[lineInBuffer] += " >>" + text;
		visible.line = 0.0;
	};

	// console.close()
	// close the console, hiding it from view.
	function close()
	{
		new scenes.Scene()
			.tween(visible, 0.25, 'easeInQuad', { fade: 0.0 })
			.call(function() { visible.yes = false; entry = ""; })
			.run();
	};

	// console.log()
	// write a line of text to the console.
	function log(text)
	{
		if (nextLine > 0) {
			var lineText = buffer[(nextLine - 1) % bufferSize];
			DebugPrint(lineText.substr(1));
			if (log !== null)
				logger.write(lineText);
		}
		var lineInBuffer = nextLine % bufferSize;
		buffer[lineInBuffer] = ">" + text;
		++nextLine;
		visible.line = 0.0;
	};

	// console.open()
	// open the console, making it visible to the player.
	function open()
	{
		new scenes.Scene()
			.tween(visible, 0.25, 'easeOutQuad', { fade: 1.0 })
			.call(function() { visible.yes = true; })
			.run();
	}

	// console.register()
	// register a named entity with the console.
	// arguments:
	//     name:    the name of the entity.	 this should not contain spaces.
	//     that:    the value which will be bound to `this` when one of the entity's methods is executed.
	//     methods: an associative array of functions, keyed by name, defining the valid operations
	//              for this entity.  one-word names are recommended and as with the entity name,
	//              should not contain spaces.
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
	};

	// console.unregister()
	// unregister all commands for a previously-registered entity.
	// arguments:
	//     name: the name of the entity as passed to console.register().
	function unregister(name)
	{
		commands = link(commands)
			.where(function(command) { return command.entity != name; })
			.toArray();
	};
})();
