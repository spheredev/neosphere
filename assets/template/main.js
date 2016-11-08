const assert = require('assert');
const link   = require('link');
const prim   = require('prim');
const random = require('random');

// initialize the game
screen.frameRate = 60;  // note: 60fps is the default.
Dispatch.onUpdate(doUpdate);
Dispatch.onRender(doRender);

// when you're ready to start programming, delete this comment block and the
// two lines below.  but first, click Debug and see what happens!
var message = "this game has not been implemented";
throw new Error(message);

function doUpdate()
{
	// put code here to update your game every frame, for example moving
	// sprites and updating animations.
	
}

function doRender()
{
	// put code here to draw a frame.  don't do anything other than rendering
	// here, as renders can be skipped and thus are not guaranteed to match the
	// frame rate.
	prim.rect(screen, 0, 0, screen.width, screen.height, Color.DodgerBlue);
}
