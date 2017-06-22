/*
 *  <title of your new Sphere game goes here>
 *  (c) <year> <whoever made the game>
 */

import { Music, Prim, Thread } from 'sphere-runtime';

export default
class MyGame extends Thread
{
	constructor()
	{
		/* put code in here to initialize your game on startup--setting up
		   data, loading resources, etc.  use `this` to associate things with
		   the Game object, or `global` to make global variables that are
		   accessible game-wide. */

		// construct the parent class by calling its constructor (in our case,
		// Thread).  any class with an `extends` clause must do this in its
		// constructor.
		super();

		// initialize data for our game session
		this.image = new Texture('images/justSaiyan.png');
		this.x = 0;
		this.y = 0;
		this.xVel = 1;
		this.yVel = 1;

		// avoid boredom by playing some background music!
		Music.play('music/vegeta.ogg');
	}

	on_update()
	{
		/* put code in here to update your game every frame, for example moving
		   sprites and updating animations. */

		this.x += this.xVel;
		this.y += this.yVel;
		if (this.x <= 0) {
			this.x = 0;
			this.xVel = 1;
		}
		else if (this.x >= screen.width - this.image.width) {
			this.x = screen.width - this.image.width;
			this.xVel = -1;
		}
		if (this.y <= 0) {
			this.y = 0;
			this.yVel = 1;
		}
		else if (this.y >= screen.height - this.image.height) {
			this.y = screen.height - this.image.height;
			this.yVel = -1;
		}
	}

	on_render()
	{
		/* put code in here to draw a frame.  don't do anything other than
		   rendering-related things here, as renders can be skipped and thus
		   are not guaranteed to match the frame rate. */

		Prim.rect(screen, 0, 0, screen.width, screen.height, Color.DodgerBlue);
		Prim.ellipse(screen,
			screen.width / 2, screen.height / 2,
			screen.width / 4, screen.height / 4,
			Color.Chartreuse, Color.DarkGreen);
		Prim.lineEllipse(screen,
			screen.width / 2, screen.height / 2,
			screen.width / 4, screen.height / 4,
			Color.Black);

		Prim.blit(screen, this.x, this.y, this.image);
		Prim.lineRect(screen, this.x, this.y, this.image.width, this.image.height, 2, Color.Black);
	}
}
