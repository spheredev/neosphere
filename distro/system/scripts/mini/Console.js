/**
 * minisphere Runtime 1.1b4 - (c) 2015 Fat Cerberus
 * A set of system scripts providing advanced, high-level functionality not
 * available in the engine itself.
 *
 * [mini/Console.js]
 * An easy-to-use output console which optionally logs output to disk.
**/

RequireSystemScript('mini/Core.js');
RequireSystemScript('mini/Threads.js');

// mini.Console
// Encapsulates the console.
mini.Console = new (function()
{
	this.fadeness = 0.0;
	this.font = GetSystemFont();
	this.isVisible = false;
	this.lineOffset = 0.0;
	this.log = null;
	this.nextLine = 0;
	this.numLines = 0;
	this.thread = null;
	this.wasKeyDown = false;
})();

// mini.Console.initialize() method
// Initializes the console.
// Parameters:
//     consoleLines:  The number of lines to display at a time. If not provided, the line count
//                    will be determined dynamically based on the game resolution.
//     consoleBuffer: The maximum number of lines kept in the line buffer. (default: 1000)
//     logFile:       Path, relative to <game_dir>/logs, of the file to write console output
//                    to. Can be null, in which case no log file is created. (default: null)
mini.onStartUp.add(mini.Console, function(params)
{
	Print("mini: Initializing miniconsole");
	
	var numLines = 'consoleLines' in params ? params.consoleLines
		: Math.floor((GetScreenHeight() - 10) * 0.66 / this.font.getHeight());
	var bufferSize = 'consoleBuffer' in params ? params.consoleBuffer : 1000;
	var filename = 'logFile' in params ? params.logFile : null;
	
	if (typeof filename === 'string')
		this.log = OpenLog(params.logFile);
	else
		this.log = null;
	this.numLines = numLines;
	this.buffer = [];
	this.bufferSize = bufferSize;
	this.commands = [];
	this.thread = mini.Threads.create(this, 101);
	this.writeLine("minisphere Runtime 1.1b4 (miniconsole)");
	this.writeLine("Sphere " + GetVersionString());
	this.writeLine("");
	this.writeLine("Initialized miniconsole");
});

// .isOpen() method
// Determines whether the console is currently displayed or not.
// Returns:
//     true if the console is open, false otherwise.
mini.Console.isOpen = function()
{
	return this.isVisible;
}

// .append() method
// Appends additional output text to the last line in the console.
// Arguments:
//     text: The text to append.
mini.Console.append = function(text)
{
	if (this.nextLine == 0) {
		mini.Console.writeLine(text);
		return;
	}
	var lineInBuffer = (this.nextLine - 1) % this.bufferSize;
	this.buffer[lineInBuffer] += " >>" + text;
	this.lineOffset = 0.0;
};

// .getInput() method
// Checks for input and updates the console accordingly.
mini.Console.getInput = function()
{
	if (!IsKeyPressed(KEY_TAB)) this.wasKeyDown = false;
	if (!this.wasKeyDown && IsKeyPressed(KEY_TAB)) {
		if (!this.isOpen())
			this.show();
		else
			this.hide();
		this.wasKeyDown = true;
	}
	if (this.isOpen()) {
		var wheelKey = GetNumMouseWheelEvents() > 0 ? GetMouseWheelEvent() : null;
		var speed = wheelKey != null ? 1.0 : 0.5;
		if (IsKeyPressed(KEY_PAGEUP) || wheelKey == MOUSE_WHEEL_UP) {
			this.lineOffset = Math.min(this.lineOffset + speed, this.buffer.length - this.numLines);
		} else if (IsKeyPressed(KEY_PAGEDOWN) || wheelKey == MOUSE_WHEEL_DOWN) {
			this.lineOffset = Math.max(this.lineOffset - speed, 0);
		}
	}
};

// .hide() method
// Hides the console.
mini.Console.hide = function()
{
	new mini.Scene()
		.tween(this, 0.5, 'easeInBack', { fadeness: 0.0 })
		.call(function() { this.isVisible = false; }.bind(this))
		.run();
};

// .registerEntity() method
// Registers a named entity with the console.
// Arguments:
//     handle:  The name of the entity. Ideally, this should not contain spaces.
//     methods: An associative array of functions, keyed by name, defining the valid operations
//              for this entity.
mini.Console.registerEntity = function(handle, methods)
{
	this.commands[handle] = clone(methods);
};

// .render() method
// Renders the console in its current state.
mini.Console.render = function() {
	if (this.fadeness <= 0.0)
		return;
	var boxHeight = this.numLines * this.font.getHeight() + 10;
	var boxY = GetScreenHeight() - boxHeight * this.fadeness //-boxHeight * (1.0 - this.fadeness);
	Rectangle(0, boxY, GetScreenWidth(), boxHeight, CreateColor(0, 0, 0, this.fadeness * 192));
	var oldClip = GetClippingRectangle();
	SetClippingRectangle(5, boxY + 5, GetScreenWidth() - 10, boxHeight - 10);
	for (var i = -1; i < this.numLines + 1; ++i) {
		var lineToDraw = (this.nextLine - this.numLines) + i - Math.floor(this.lineOffset);
		var lineInBuffer = lineToDraw % this.bufferSize;
		if (lineToDraw >= 0 && this.buffer[lineInBuffer] != null) {
			var y = boxY + 5 + i * this.font.getHeight();
			y += (this.lineOffset - Math.floor(this.lineOffset)) * this.font.getHeight();
			this.font.setColorMask(CreateColor(0, 0, 0, this.fadeness * 192));
			this.font.drawText(6, y + 1, this.buffer[lineInBuffer]);
			this.font.setColorMask(CreateColor(255, 255, 255, this.fadeness * 192));
			this.font.drawText(5, y, this.buffer[lineInBuffer]);
		}
	}
	SetClippingRectangle(oldClip.x, oldClip.y, oldClip.width, oldClip.height);
};

// .show() method
// Shows the console window.
mini.Console.show = function()
{
	new mini.Scene()
		.tween(this, 0.5, 'easeOutBack', { fadeness: 1.0 })
		.call(function() { this.isVisible = true; }.bind(this))
		.run();
}

// .update() method
// Updates the console's internal state for the next frame.
mini.Console.update = function() {
	if (this.fadeness <= 0.0) {
		this.lineOffset = 0.0;
	}
	return true;
};

// .writeLine() method
// Writes a line of text to the console.
mini.Console.writeLine = function(text)
{
	if (this.log !== null && this.nextLine > 0) {
		var lineInBuffer = (this.nextLine - 1) % this.bufferSize;
		this.log.write(this.buffer[lineInBuffer]);
	}
	var lineInBuffer = this.nextLine % this.bufferSize;
	this.buffer[lineInBuffer] = ">" + text;
	++this.nextLine;
	this.lineOffset = 0.0;
};
