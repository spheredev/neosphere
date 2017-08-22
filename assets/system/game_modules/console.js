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

'use strict';
const from   = require('from'),
      Logger = require('logger'),
      Prim   = require('prim'),
      Scene  = require('scene');

exports.__esModule = true;
exports.default = exports;

var activationKey = Key.Tilde,
    buffer        = [],
    bufferSize    = 1000,
    commands      = [],
    cursorColor   = Color.Gold,
    entry         = "",
    font          = Font.Default,
    logger        = null,
    keyboard      = Keyboard.Default,
    mouse         = Mouse.Default,
    nextLine      = 0,
    numLines      = 0,
    prompt        = "$",
    visible       = { yes: false, fade: 0.0, line: 0.0 },
    wasKeyDown    = false;

numLines = Math.floor((screen.height - 32) / font.height);
if ('saveID' in Sphere.Game)
    logger = new Logger('consoleOutput');
new Scene()
    .doWhile(function() { return true; })
        .tween(cursorColor, 0.25 * screen.frameRate, 'easeInSine', { a: 1.0 })
        .tween(cursorColor, 0.25 * screen.frameRate, 'easeOutSine', { a: 0.5 })
    .end()
    .run();
log(Sphere.Game.name + " Debug Console");
log("Sphere v" + Sphere.Version + " API lv. " + Sphere.APILevel + " (" + Sphere.Platform + ")");
log("");

Object.defineProperty(exports, 'visible',
{
    enumerable: true, configurable: true,
    get: function() { return visible.yes; },
    set: function(value) { value ? _show() : _hide(); },
});

exports.initialize = initialize;
function initialize(options)
{
    options = options !== undefined ? options : {};

    activationKey = options.hotKey !== undefined ? options.hotKey : Key.Tilde;
    Dispatch.onRender(_render, Infinity);
    Dispatch.onUpdate(function() {
        _getInput();
        _update();
    }, Infinity);
}

exports.defineObject = defineObject;
function defineObject(name, that, methods)
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
    if (buffer[lineInBuffer] == ">")
        buffer[lineInBuffer] = "";
    ++nextLine;
    visible.line = 0.0;
    SSj.log(buffer[lineInBuffer].substr(1));
    if (logger !== null)
        logger.write(buffer[lineInBuffer]);
}

exports.undefineObject = undefineObject;
function undefineObject(name)
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
        log("unrecognized object name '" + entity + "'");
        return;
    }
    if (tokens.length < 2) {
        log("missing instruction for '" + entity + "'");
        return;
    }
    if (!from.Array(commands)
        .where(function(c) { return entity == c.entity; })
        .any(function(c) { return instruction == c.instruction; }))
    {
        log("instruction '" + instruction + "' not valid for '" + entity + "'");
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
            try {
                desc.method.apply(desc.that, tokens.slice(2));
            }
            catch(e) {
                log("caught error '" + e.toString() + "'");
            }
        });
    });
}

function _getInput()
{
    if (!wasKeyDown && keyboard.isPressed(activationKey)) {
        if (!visible.yes)
            _show();
        else
            _hide();
    }
    wasKeyDown = keyboard.isPressed(activationKey);
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
        if (keycode === activationKey)
            return;
        switch (keycode) {
            case Key.Enter:
                log("player entered command '" + entry + "'");
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
    Prim.drawSolidRectangle(screen, 0, boxY, screen.width, 22, Color.Black.fade(visible.fade * 0.875));
    var promptWidth = font.getTextSize(prompt + " ").width;
    font.drawText(screen, 6, 6 + boxY, prompt, Color.Black.fade(visible.fade * 0.75));
    font.drawText(screen, 5, 5 + boxY, prompt, Color.Gray.fade(visible.fade * 0.75));
    font.drawText(screen, 6 + promptWidth, 6 + boxY, entry, Color.Black.fade(visible.fade * 0.75));
    font.drawText(screen, 5 + promptWidth, 5 + boxY, entry, Color.Gold.fade(visible.fade * 0.75));
    font.drawText(screen, 5 + promptWidth + font.getTextSize(entry).width, 5 + boxY, "_", cursorColor);

    // ...then the console output
    var boxHeight = numLines * font.height + 10;
    var boxY = screen.height - boxHeight * visible.fade;
    Prim.drawSolidRectangle(screen, 0, boxY, screen.width, boxHeight, Color.Black.fade(visible.fade * 0.75));
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
