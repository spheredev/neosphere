/**
 * minisphere Runtime 1.1b3 - (c) 2015 Fat Cerberus
 * A set of system scripts providing advanced, high-level functionality not
 * available in the engine itself.
 *
 * [mini/Scenes.js]
 * An advanced scene manager that allows you to coordinate complex sequences
 * using multiple timelines and cooperative threading. Based on Scenario.
**/

RequireSystemScript('mini/Core.js');
RequireSystemScript('mini/Threads.js');

mini.Scenes = mini.Scenes || {};

// IsSkippedFrame() is a minisphere feature and may not be available.
if (typeof IsSkippedFrame === 'undefined') {
	IsSkippedFrame = function() { return false; };
}

// mini.Scenelet()
// Registers a new miniscenes scenelet.
// Arguments:
//     name: The name of the command. This should be a valid JavaScript identifier (alphanumeric, no spaces).
//     code: An object defining the command's callback functions:
//           .start(scene, ...): Called when the command begins executing to initialize the state, or for
//                               instantaneous commands, perform the necessary action.
//           .update(scene):     Optional. A function to be called once per frame to update state data. If not
//                               provided, Scenes immediately moves on to the next command after calling start().
//                               This function should return true to keep the operation running, or false to
//                               terminate it.
//           .getInput(scene):   Optional. A function to be called once per frame while the command has the input
//                               focus to check for player input and update state data accordingly.
//           .render(scene):     Optional. A function to be called once per frame to perform any rendering
//                               related to the command (e.g. text boxes).
//           .finish(scene):     Optional. Called after command execution ends, just before Scenes executes
//                               the next instruction in the queue.
// Remarks:
//    It is safe to call this prior to runtime initialization.
mini.Scenelet = function(name, code)
{
	if (mini.Scene.prototype[name] != null)
		Abort("mini.Scenelet(): Scenelet identifier '" + name + "' is already in use", -1);
	mini.Scene.prototype[name] = function() {
		this.enqueue({
			arguments: arguments,
			start: code.start,
			getInput: code.getInput,
			update: code.update,
			render: code.render,
			finish: code.finish
		});
		return this;
	};
};

// initializer registration
// Initializes Scenes when the user calls mini.initialize().
mini.startup.add(mini.Scenes, function(params)
{
	Print("mini: Initializing mini.Scenes");
	
	this.activeScenes = [];
	this.screenMask = new Color(0, 0, 0, 0);
	var priority = 'scenePriority' in params
		? params.scenePriority : 0;
	mini.Threads.create(this, priority);
});

// mini.Scenes.render()
// Renders running Scenes.
mini.Scenes.render = function()
{
	if (IsSkippedFrame())
		return;
	if (this.screenMask.alpha > 0) {
		ApplyColorMask(this.screenMask);
	}
	for (var i = 0; i < this.activeScenes.length; ++i) {
		this.activeScenes[i].render();
	}
};

// mini.Scenes.update()
// Updates running Scenes for the next frame.
mini.Scenes.update = function()
{
	for (var i = 0; i < this.activeScenes.length; ++i) {
		var scene = this.activeScenes[i];
		scene.update();
		if (!scene.isRunning()) {
			if (scene.isLooping) {
				scene.run(false);
			}
			this.activeScenes.splice(i, 1);
			--i;
		}
	}
	mini.Scenes.hasUpdated = true;
	return true;
};

// mini.Scene()
// Constructs a scene definition.
// Arguments:
//     isLooping: If true, the Scene loops endlessly until .stop() is called.
//                (default: false)
mini.Scene = function(isLooping)
{
	isLooping = isLooping !== undefined ? isLooping : false;
	
	this.activeThread = null;
	this.focusThreadStack = [];
	this.focusThread = null;
	this.forkedQueues = [];
	this.isLooping = isLooping;
	this.jumpsToFix = [];
	this.nextThreadID = 1;
	this.openBlockTypes = [];
	this.queueToFill = [];
	this.threads = [];
	this.vars = {};
	
	this.createThread = function(context, updater, renderer, inputHandler)
	{
		renderer = renderer !== undefined ? renderer : null;
		inputHandler = inputHandler !== undefined ? inputHandler : null;
		
		var threadObject = {
			id: this.nextThreadID,
			context: context,
			inputHandler: inputHandler,
			renderer: renderer,
			updater: updater
		};
		this.threads.push(threadObject);
		if (inputHandler != null) {
			this.focusThreadStack.push(this.focusThread);
			this.focusThread = threadObject.id;
		}
		++this.nextThreadID;
		return threadObject.id;
	};
	
	this.enqueue = function(command)
	{
		this.queueToFill.push(command);
	};
	
	this.forkUpdater = function(scene)
	{
		for (var i = 0; i < this.forkThreads.length; ++i) {
			if (!scene.isThreadRunning(this.forkThreads[i])) {
				this.forkThreads.splice(i, 1);
				--i;
			}
		}
		if (scene.isThreadRunning(this.currentCommandThread)) {
			return true;
		}
		if (this.counter >= this.instructions.length && this.forkThreads.length === 0) {
			return false;
		}
		if (this.counter < this.instructions.length) {
			var command = this.instructions[this.counter];
			++this.counter;
			var commandContext = {};
			if (command.start != null) {
				var parameters = [];
				parameters.push(scene);
				for (i = 0; i < command.arguments.length; ++i) {
					parameters.push(command.arguments[i]);
				}
				command.start.apply(commandContext, parameters);
			}
			if (command.update != null) {
				var updateShim = function(scene) {
					var isActive = command.update.call(this, scene);
					if (!isActive && command.finish != null) {
						command.finish.call(this, scene);
					}
					return isActive;
				};
				this.currentCommandThread = scene.createThread(commandContext, updateShim, command.render, command.getInput);
			} else if (command.finish != null) {
				command.finish.call(command.context, scene);
			}
		}
		return true;
	};
	
	this.goTo = function(commandID)
	{
		this.activeThread.context.counter = commandID;
	};
	
	this.isThreadRunning = function(id)
	{
		if (id == 0) {
			return false;
		}
		for (var i = 0; i < this.threads.length; ++i) {
			if (id == this.threads[i].id) {
				return true;
			}
		}
		return false;
	};
	
	this.killThread = function(id)
	{
		for (var i = 0; i < this.threads.length; ++i) {
			if (id == this.threads[i].id) {
				this.threads.splice(i, 1);
				--i;
			}
		}
	};
	
	this.render = function()
	{
		for (var i = 0; i < this.threads.length; ++i) {
			var renderer = this.threads[i].renderer;
			var context = this.threads[i].context;
			if (renderer != null) {
				renderer.call(context, this);
			}
		}
	};
	
	this.throwError = function(component, name, message)
	{
		Abort(component + " - error: " + name + "\n" + message, -1);
	};
	
	this.update = function()
	{
		for (var i = 0; i < this.threads.length; ++i) {
			this.activeThread = this.threads[i];
			var id = this.threads[i].id;
			var updater = this.threads[i].updater;
			var inputHandler = this.threads[i].inputHandler;
			var context = this.threads[i].context;
			if (updater == null) continue;
			if (this.focusThread == id) {
				inputHandler.call(context, this);
			}
			if (!updater.call(context, this)) {
				if (this.focusThread == id) {
					this.focusThread = this.focusThreadStack.pop();
				}
				this.threads.splice(i, 1);
				--i;
			}
		}
		this.activeThread = null;
	};
}

// mini.Scene:doIf()
// During scene execution, executes a block of commands only if a specified condition is met.
// Arguments:
//     conditional: A function to be called during scene execution to determine whether to run the following
//                  block. The function should return true to execute the block, or false to skip it. It
//                  will be called with 'this' set to the invoking scene.
mini.Scene.prototype.doIf = function(conditional)
{
	var jump = { ifFalse: null };
	this.jumpsToFix.push(jump);
	var command = {
		arguments: [],
		start: function(scene) {
			if (!conditional.call(scene)) {
				scene.goTo(jump.ifFalse);
			}
		}
	};
	this.enqueue(command);
	this.openBlockTypes.push('branch');
	return this;
};

// mini.Scene:doWhile()
// During scene execution, repeats a block of commands for as long as a specified condition is met.
// Arguments:
//     conditional: A function to be called at each iteration to determine whether to continue the
//                  loop. The function should return true to continue the loop, or false to
//                  stop it. It will be called with 'this' set to the invoking Scene object.
mini.Scene.prototype.doWhile = function(conditional)
{
	var jump = { loopStart: this.queueToFill.length, ifDone: null };
	this.jumpsToFix.push(jump);
	var command = {
		arguments: [],
		start: function(scene) {
			if (!conditional.call(scene)) {
				scene.goTo(jump.ifDone);
			}
		}
	};
	this.enqueue(command);
	this.openBlockTypes.push('loop');
	return this;
};

// mini.Scene:end()
// Marks the end of a block of commands.
mini.Scene.prototype.end = function()
{
	if (this.openBlockTypes.length == 0) {
		this.throwError("Scene:end()", "Malformed scene", "Mismatched end() - there are no blocks currently open.");
	}
	var blockType = this.openBlockTypes.pop();
	switch (blockType) {
		case 'fork':
			var command = {
				arguments: [ this.queueToFill ],
				start: function(scene, instructions) {
					var forkContext = {
						counter: 0,
						currentCommandThread: null,
						forkThreads: [],
						instructions: instructions
					};
					var thread = scene.createThread(forkContext, scene.forkUpdater);
					scene.activeThread.context.forkThreads.push(thread);
				}
			};
			this.queueToFill = this.forkedQueues.pop();
			this.enqueue(command);
			break;
		case 'branch':
			var jump = this.jumpsToFix.pop();
			jump.ifFalse = this.queueToFill.length;
			break;
		case 'loop':
			var command = {
				arguments: [],
				start: function(scene) {
					scene.goTo(jump.loopStart);
				}
			};
			this.enqueue(command);
			var jump = this.jumpsToFix.pop();
			jump.ifDone = this.queueToFill.length;
			break;
		default:
			this.throwError("Scene:end()", "Internal error", "The type of the open block is unknown.");
			break;
	}
	return this;
};

// mini.Scene:fork()
// During scene execution, forks the timeline, allowing a block to run simultaneously with
// the instructions after the block.
mini.Scene.prototype.fork = function()
{
	this.forkedQueues.push(this.queueToFill);
	this.queueToFill = [];
	this.openBlockTypes.push('fork');
	return this;
};

// mini.Scene:isRunning()
// Determines whether a scenario is still running.
// Returns:
//     true if the scenario is still executing commands; false otherwise.
mini.Scene.prototype.isRunning = function()
{
	return this.isThreadRunning(this.mainThread);
};

// mini.Scene:run()
// Runs the scene.
// Arguments:
//     waitUntilDone: Optional. If true, prevents .run() from returning until the scenario has finished executing.
//                    Otherwise, .run() returns immediately. (default: false)
mini.Scene.prototype.run = function(waitUntilDone)
{
	waitUntilDone = waitUntilDone !== undefined ? waitUntilDone : false;
	
	if (this.openBlockTypes.length > 0) {
		this.throwError("Scene:run()", "Malformed scene", "Caller attempted to run a scene with unclosed blocks.");
	}
	if (this.isLooping && waitUntilDone) {
		this.throwError("Scene:run()", "Invalid argument", "Caller attempted to wait on a looping scene. This would have created an infinite loop and has been prevented.");
	}
	
	if (this.isRunning()) {
		return;
	}
	var mainForkContext = {
		counter: 0,
		currentCommandThread: null,
		forkThreads: [],
		instructions: this.queueToFill
	};
	this.frameRate = IsMapEngineRunning() ? GetMapEngineFrameRate() : GetFrameRate();
	this.mainThread = this.createThread(mainForkContext, this.forkUpdater);
	mini.Scenes.activeScenes.push(this);
	if (waitUntilDone) {
		var currentFPS = GetFrameRate();
		if (IsMapEngineRunning()) {
			SetFrameRate(GetMapEngineFrameRate());
		}
		mini.Threads.join(mini.Threads.createEx(this, {
			update: this.isThreadRunning.bind(this, this.mainThread)
		}));
		SetFrameRate(currentFPS);
	}
	return this;
};

// mini.Scene:stop()
// Immediately halts execution of the scene. Has no effect if the scene isn't running.
// Remarks:
//     After calling this method, calling run() afterwards will start the scene over from the
//     beginning.
mini.Scene.prototype.stop = function()
{
	this.killThread(this.mainThread);
};

// .synchronize() scenelet
// During a scene, suspends the current timeline until all its forks have finished executing.
mini.Scene.prototype.synchronize = function()
{
	var command = {};
	command.arguments = [];
	command.start = function(scene) {
		this.forkThreads = scene.activeThread.context.forkThreads;
	};
	command.update = function(scene) {
		return this.forkThreads.length != 0;
	};
	this.enqueue(command);
	return this;
};

// .call() scenelet
// Calls a function during scene execution.
// Arguments:
//     method: The function to be called.
// Remarks:
//     Any additional arguments provided beyond the 'method' argument will be passed
//     to the specified function when it is called.
mini.Scenelet('call',
{
	start: function(scene, method /*...*/) {
		method.apply(null, [].slice.call(arguments, 2));
	}
});

// .facePerson() scenelet
// Changes the facing direction of a map entity.
// Arguments:
//     person:    The name of the entity whose direction to change.
//     direction: The name of the new direction.
mini.Scenelet('facePerson',
{
	start: function(scene, person, direction) {
		var faceCommand;
		switch (direction.toLowerCase()) {
			case "n": case "north":
				faceCommand = COMMAND_FACE_NORTH;
				break;
			case "ne": case "northeast":
				faceCommand = COMMAND_FACE_NORTHEAST;
				break;
			case "e": case "east":
				faceCommand = COMMAND_FACE_EAST;
				break;
			case "se": case "southeast":
				faceCommand = COMMAND_FACE_SOUTHEAST;
				break;
			case "s": case "south":
				faceCommand = COMMAND_FACE_SOUTH;
				break;
			case "sw": case "southwest":
				faceCommand = COMMAND_FACE_SOUTHWEST;
				break;
			case "w": case "west":
				faceCommand = COMMAND_FACE_WEST;
				break;
			case "nw": case "northwest":
				faceCommand = COMMAND_FACE_NORTHWEST;
				break;
			default:
				faceCommand = COMMAND_WAIT;
		}
		QueuePersonCommand(person, faceCommand, false);
	}
});

// .fadeTo() scenelet
// Fades the screen mask to a specified color.
// Arguments:
//     color:    The new screen mask color.
//     duration: The length of the fading operation, in seconds.
mini.Scenelet('fadeTo',
{
	start: function(scene, color, duration) {
		duration = duration !== undefined ? duration : 0.25;
		
		this.fader = new mini.Scene()
			.tween(mini.Scenes.screenMask, duration, 'linear', color)
			.run();
	},
	update: function(scene) {
		return this.fader.isRunning();
	}
});

// .focusOnPerson() scenelet
// Pans the camera to a point centered over a specified map entity.
// Arguments:
//     person:   The name of the entity to focus on.
//     duration: Optional. The length of the panning operation, in seconds.
//               (default: 0.25)
mini.Scenelet('focusOnPerson',
{
	start: function(scene, person, duration) {
		duration = duration !== undefined ? duration : 0.25;
		
		this.pan = new mini.Scene()
			.panTo(GetPersonX(person), GetPersonY(person), duration)
			.run();
	},
	update: function(scene) {
		return this.pan.isRunning();
	}
});

// .followPerson() scenelet
// Pans to and attaches the camera to a specified map entity.
// Arguments:
//     person: The name of the entity to follow.
mini.Scenelet('followPerson',
{
	start: function(scene, person) {
		this.person = person;
		this.pan = new mini.Scene()
			.focusOnPerson(person)
			.run();
	},
	update: function(scene) {
		return this.pan.isRunning();
	},
	finish: function(scene) {
		AttachCamera(this.person);
	}
});

// .hidePerson() scenelet
// Hides a map entity and prevents it from obstructing other entities.
// Arguments:
//     person: The name of the entity to hide.
mini.Scenelet('hidePerson',
{
	start: function(scene, person) {
		SetPersonVisible(person, false);
		IgnorePersonObstructions(person, true);
	}
});

// .killPerson() scenelet
// Destroys a map entity.
// Arguments:
//     person: The name of the entity to destroy.
mini.Scenelet('killPerson',
{
	start: function(scene, person) {
		DestroyPerson(person);
	}
});

// .marquee() scenelet
// Shows a scrolling marquee with the specified text. Useful for announcing boss battles.
// Arguments:
//     text:            The text to display.
//     backgroundColor: The background color of the marquee.
//     color:           The text color.
mini.Scenelet('marquee',
{
	start: function(scene, text, backgroundColor, color) {
		if (backgroundColor === undefined) { backgroundColor = new Color(0, 0, 0, 255); }
		if (color === undefined) { color = new Color(255, 255, 255, 255); }
		
		this.text = text;
		this.color = color;
		this.background = backgroundColor;
		this.font = GetSystemFont();
		this.windowSize = GetScreenWidth() + this.font.getStringWidth(this.text);
		this.height = this.font.getHeight() + 10;
		this.textHeight = this.font.getHeight();
		this.fadeness = 0.0;
		this.scroll = 0.0;
		this.animation = new mini.Scene()
			.tween(this, 0.25, 'linear', { fadeness: 1.0 })
			.tween(this, 1.0, 'easeOutExpo', { scroll: 0.5 })
			.tween(this, 1.0, 'easeInExpo', { scroll: 1.0 })
			.tween(this, 0.25, 'linear', { fadeness: 0.0 })
			.run();
	},
	render: function(scene) {
		var boxHeight = this.height * this.fadeness;
		var boxY = GetScreenHeight() / 2 - boxHeight / 2;
		var textX = GetScreenWidth() - this.scroll * this.windowSize;
		var textY = boxY + boxHeight / 2 - this.textHeight / 2;
		Rectangle(0, boxY, GetScreenWidth(), boxHeight, this.background);
		this.font.setColorMask(new Color(0, 0, 0, this.color.alpha));
		this.font.drawText(textX + 1, textY + 1, this.text);
		this.font.setColorMask(this.color);
		this.font.drawText(textX, textY, this.text);
	},
	update: function(scene) {
		return this.animation.isRunning();
	}
});

// .movePerson() scenelet
// Instructs a map entity to move a specified distance.
// Arguments:
//     person:    The person to move.
//     direction: The direction in which to move the entity.
//     distance:  The distance the entity should move.
//     speed:     The number of pixels per frame the entity should move.
//     faceFirst: Optional. If this is false, the entity will move without changing its facing
//                direction. (default: true)
mini.Scenelet('movePerson',
{
	start: function(scene, person, direction, distance, speed, faceFirst) {
		faceFirst = faceFirst !== undefined ? faceFirst : true;
		
		if (!isNaN(speed)) {
			speedVector = [ speed, speed ];
		} else {
			speedVector = speed;
		}
		this.person = person;
		this.oldSpeedVector = [ GetPersonSpeedX(person), GetPersonSpeedY(person) ];
		if (speedVector != null) {
			SetPersonSpeedXY(this.person, speedVector[0], speedVector[1]);
		} else {
			speedVector = this.oldSpeedVector;
		}
		var xMovement;
		var yMovement;
		var faceCommand;
		var stepCount;
		switch (direction) {
			case "n": case "north":
				faceCommand = COMMAND_FACE_NORTH;
				xMovement = COMMAND_WAIT;
				yMovement = COMMAND_MOVE_NORTH;
				stepCount = distance / speedVector[1];
				break;
			case "e": case "east":
				faceCommand = COMMAND_FACE_EAST;
				xMovement = COMMAND_MOVE_EAST;
				yMovement = COMMAND_WAIT;
				stepCount = distance / speedVector[0];
				break;
			case "s": case "south":
				faceCommand = COMMAND_FACE_SOUTH;
				xMovement = COMMAND_WAIT;
				yMovement = COMMAND_MOVE_SOUTH;
				stepCount = distance / speedVector[1];
				break;
			case "w": case "west":
				faceCommand = COMMAND_FACE_WEST;
				xMovement = COMMAND_MOVE_WEST;
				yMovement = COMMAND_WAIT;
				stepCount = distance / speedVector[0];
				break;
			default:
				faceCommand = COMMAND_WAIT;
				xMovement = COMMAND_WAIT;
				yMovement = COMMAND_WAIT;
				stepCount = 0;
		}
		if (faceFirst) {
			QueuePersonCommand(this.person, faceCommand, true);
		}
		for (iStep = 0; iStep < stepCount; ++iStep) {
			QueuePersonCommand(this.person, xMovement, true);
			QueuePersonCommand(this.person, yMovement, true);
			QueuePersonCommand(this.person, COMMAND_WAIT, false);
		}
		return true;
	},
	update: function(scene) {
		return !IsCommandQueueEmpty(this.person);
	},
	finish: function(scene) {
		SetPersonSpeedXY(this.person, this.oldSpeedVector[0], this.oldSpeedVector[1]);
	}
});

// .panTo() scenelet
// Pans the map camera to center on a specified location on the map.
// Arguments:
//     x:        The X coordinate of the location to pan to.
//     y:        The Y coordinate of the location to pan to.
//     duration: Optional. The length of the panning operation, in seconds. (default: 0.25)
mini.Scenelet('panTo',
{
	start: function(scene, x, y, duration) {
		duration = duration !== undefined ? duration : 0.25;
		
		DetachCamera();
		var targetXY = {
			cameraX: x,
			cameraY: y
		};
		this.cameraX = GetCameraX();
		this.cameraY = GetCameraY();
		this.pan = new mini.Scene()
			.tween(this, duration, 'easeOutQuad', targetXY)
			.run();
	},
	update: function(scene) {
		SetCameraX(this.cameraX);
		SetCameraY(this.cameraY);
		return this.pan.isRunning();
	}
});

// .pause() scenelet
// Delays execution of the current timeline for a specified amount of time.
// Arguments:
//     duration: The length of the delay, in seconds.
mini.Scenelet('pause',
{
	start: function(scene, duration) {
		this.duration = duration;
		this.elapsed = 0;
	},
	update: function(scene) {
		this.elapsed += 1.0 / scene.frameRate;
		return this.elapsed < this.duration;
	}
});

// .playSound() scenelet
// Plays a sound from a file.
//     fileName: The name of the file to play.
mini.Scenelet('playSound',
{
	start: function(scene, fileName) {
		this.sound = new Sound(fileName);
		this.sound.play(false);
		return true;
	},
	update: function(scene) {
		return this.sound.isPlaying();
	}
});

// .showPerson() scenelet
// Makes a map entity visible and enables obstruction.
// Arguments:
//     person: The name of the entity to show.
mini.Scenelet('showPerson',
{
	start: function(scene, person) {
		SetPersonVisible(person, true);
		IgnorePersonObstructions(person, false);
	}
});

// .tween() scenelet
// Smoothly adjusts numeric properties of an object over a period of time.
// Arguments:
//    object:     The object containing the properties to be tweened.
//    duration:   The length of the tweening operation, in seconds.
//    easingType: The name of the easing function to use, e.g. 'linear' or 'easeOutQuad'.
//    endValues:  An object specifying the properties to tween and their final values.
mini.Scenelet('tween',
{
	start: function(scene, object, duration, easingType, endValues) {
		this.easers = {
			linear: function(t, b, c, d) {
				return c * t / d + b;
			},
			easeInQuad: function(t, b, c, d) {
				return c*(t/=d)*t + b;
			},
			easeOutQuad: function(t, b, c, d) {
				return -c *(t/=d)*(t-2) + b;
			},
			easeInOutQuad: function(t, b, c, d) {
				if ((t/=d/2) < 1) return c/2*t*t + b;
				return -c/2 * ((--t)*(t-2) - 1) + b;
			},
			easeInCubic: function(t, b, c, d) {
				return c*(t/=d)*t*t + b;
			},
			easeOutCubic: function(t, b, c, d) {
				return c*((t=t/d-1)*t*t + 1) + b;
			},
			easeInOutCubic: function(t, b, c, d) {
				if ((t/=d/2) < 1) return c/2*t*t*t + b;
				return c/2*((t-=2)*t*t + 2) + b;
			},
			easeInQuart: function(t, b, c, d) {
				return c*(t/=d)*t*t*t + b;
			},
			easeOutQuart: function(t, b, c, d) {
				return -c * ((t=t/d-1)*t*t*t - 1) + b;
			},
			easeInOutQuart: function(t, b, c, d) {
				if ((t/=d/2) < 1) return c/2*t*t*t*t + b;
				return -c/2 * ((t-=2)*t*t*t - 2) + b;
			},
			easeInQuint: function(t, b, c, d) {
				return c*(t/=d)*t*t*t*t + b;
			},
			easeOutQuint: function(t, b, c, d) {
				return c*((t=t/d-1)*t*t*t*t + 1) + b;
			},
			easeInOutQuint: function(t, b, c, d) {
				if ((t/=d/2) < 1) return c/2*t*t*t*t*t + b;
				return c/2*((t-=2)*t*t*t*t + 2) + b;
			},
			easeInSine: function(t, b, c, d) {
				return -c * Math.cos(t/d * (Math.PI/2)) + c + b;
			},
			easeOutSine: function(t, b, c, d) {
				return c * Math.sin(t/d * (Math.PI/2)) + b;
			},
			easeInOutSine: function(t, b, c, d) {
				return -c/2 * (Math.cos(Math.PI*t/d) - 1) + b;
			},
			easeInExpo: function(t, b, c, d) {
				return (t==0) ? b : c * Math.pow(2, 10 * (t/d - 1)) + b;
			},
			easeOutExpo: function(t, b, c, d) {
				return (t==d) ? b+c : c * (-Math.pow(2, -10 * t/d) + 1) + b;
			},
			easeInOutExpo: function(t, b, c, d) {
				if (t==0) return b;
				if (t==d) return b+c;
				if ((t/=d/2) < 1) return c/2 * Math.pow(2, 10 * (t - 1)) + b;
				return c/2 * (-Math.pow(2, -10 * --t) + 2) + b;
			},
			easeInCirc: function(t, b, c, d) {
				return -c * (Math.sqrt(1 - (t/=d)*t) - 1) + b;
			},
			easeOutCirc: function(t, b, c, d) {
				return c * Math.sqrt(1 - (t=t/d-1)*t) + b;
			},
			easeInOutCirc: function(t, b, c, d) {
				if ((t/=d/2) < 1) return -c/2 * (Math.sqrt(1 - t*t) - 1) + b;
				return c/2 * (Math.sqrt(1 - (t-=2)*t) + 1) + b;
			},
			easeInElastic: function(t, b, c, d) {
				var s=1.70158;var p=0;var a=c;
				if (t==0) return b;  if ((t/=d)==1) return b+c;  if (!p) p=d*.3;
				if (a < Math.abs(c)) { a=c; var s=p/4; }
				else var s = p/(2*Math.PI) * Math.asin (c/a);
				return -(a*Math.pow(2,10*(t-=1)) * Math.sin( (t*d-s)*(2*Math.PI)/p )) + b;
			},
			easeOutElastic: function(t, b, c, d) {
				var s=1.70158;var p=0;var a=c;
				if (t==0) return b;  if ((t/=d)==1) return b+c;  if (!p) p=d*.3;
				if (a < Math.abs(c)) { a=c; var s=p/4; }
				else var s = p/(2*Math.PI) * Math.asin (c/a);
				return a*Math.pow(2,-10*t) * Math.sin( (t*d-s)*(2*Math.PI)/p ) + c + b;
			},
			easeInOutElastic: function(t, b, c, d) {
				var s=1.70158;var p=0;var a=c;
				if (t==0) return b;  if ((t/=d/2)==2) return b+c;  if (!p) p=d*(.3*1.5);
				if (a < Math.abs(c)) { a=c; var s=p/4; }
				else var s = p/(2*Math.PI) * Math.asin (c/a);
				if (t < 1) return -.5*(a*Math.pow(2,10*(t-=1)) * Math.sin( (t*d-s)*(2*Math.PI)/p )) + b;
				return a*Math.pow(2,-10*(t-=1)) * Math.sin( (t*d-s)*(2*Math.PI)/p )*.5 + c + b;
			},
			easeInBack: function(t, b, c, d, s) {
				if (s == undefined) s = 1.70158;
				return c*(t/=d)*t*((s+1)*t - s) + b;
			},
			easeOutBack: function(t, b, c, d, s) {
				if (s == undefined) s = 1.70158;
				return c*((t=t/d-1)*t*((s+1)*t + s) + 1) + b;
			},
			easeInOutBack: function(t, b, c, d, s) {
				if (s == undefined) s = 1.70158; 
				if ((t/=d/2) < 1) return c/2*(t*t*(((s*=(1.525))+1)*t - s)) + b;
				return c/2*((t-=2)*t*(((s*=(1.525))+1)*t + s) + 2) + b;
			},
			easeInBounce: function(t, b, c, d) {
				return c - this.easeOutBounce(d-t, 0, c, d) + b;
			},
			easeOutBounce: function(t, b, c, d) {
				if ((t/=d) < (1/2.75)) {
					return c*(7.5625*t*t) + b;
				} else if (t < (2/2.75)) {
					return c*(7.5625*(t-=(1.5/2.75))*t + .75) + b;
				} else if (t < (2.5/2.75)) {
					return c*(7.5625*(t-=(2.25/2.75))*t + .9375) + b;
				} else {
					return c*(7.5625*(t-=(2.625/2.75))*t + .984375) + b;
				}
			},
			easeInOutBounce: function(t, b, c, d) {
				if (t < d/2) return this.easeInBounce(t*2, 0, c, d) * .5 + b;
				return this.easeOutBounce(t*2-d, 0, c, d) * .5 + c*.5 + b;
			}
		};
		this.change = {};
		this.duration = duration;
		this.elapsed = 0.0;
		this.object = object;
		this.startValues = {};
		this.type = easingType in this.easers ? easingType : 'linear';
		var isChanged = false;
		for (var p in endValues) {
			this.change[p] = endValues[p] - object[p];
			this.startValues[p] = object[p];
			isChanged = isChanged || this.change[p] != 0;
		}
		var specialPropertyNames = [
			'red', 'green', 'blue', 'alpha'
		];
		for (var i = 0; i < specialPropertyNames.length; ++i) {
			var p = specialPropertyNames[i];
			if (!(p in this.change) && p in endValues) {
				this.change[p] = endValues[p] - object[p];
				this.startValues[p] = object[p];
				isChanged = isChanged || this.change[p] != 0;
			}
		}
		if (!isChanged) {
			this.elapsed = this.duration;
		}
	},
	update: function(scene) {
		this.elapsed += 1.0 / scene.frameRate;
		if (this.elapsed < this.duration) {
			for (var p in this.change) {
				this.object[p] = this.easers[this.type](this.elapsed, this.startValues[p], this.change[p], this.duration);
			}
			return true;
		} else {
			return false;
		}
	},
	finish: function(scene) {
		for (var p in this.change) {
			this.object[p] = this.startValues[p] + this.change[p];
		}
	}
});
