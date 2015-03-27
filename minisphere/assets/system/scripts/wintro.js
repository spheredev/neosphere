/*
wintro.js by August T. Bigelow (WIP)
Made in a few hours because I was bored
and needed a spiffy introduction.
*/

EvaluateSystemScript("colors.js");
EvaluateSystemScript("screen.js");

// Spiffy text object
function textObject(text, font, color)
{
	this.text = text;
	this.type = "Text";
	if (color == undefined)
	{
		this.color = CreateColor(255, 255, 255);
	}
	else
		this.color = color;
	// Scaling variables
	this.scale = new Object();
	this.scale.end = 2; // When scaling will end
	this.scale.factor = 0; // Scaling factor
	this.scale.speed = 0.01; // How fast to scale
	// Fading variables
	this.fade = new Object();
	this.fade.speed = 3; // How fast to fade
	this.fade.start = 1; // When to start fading
	if (font == undefined)
	{
		this.font = GetSystemFont();
	}
	else
		this.font = LoadFont(font);
	this.width = this.font.getStringWidth(text);
	this.height = this.font.getHeight();
	this.font.setColorMask(this.color);
	this.calculate(this.scale.end, this.scale.factor, this.scale.speed, this.fade.speed);
}

// Calculates stuffs
textObject.prototype.calculate = function(scaleEnd, scaleFactor, scaleSpeed, fadeSpeed)
{
	this.scale.end = scaleEnd;
	this.scale.factor = scaleFactor;
	this.scale.speed = scaleSpeed;
	this.fade.speed = fadeSpeed;
	var iterations = Math.floor((this.scale.end - this.scale.factor) / this.scale.speed);
	this.fade.start = (iterations - (255 / fadeSpeed)) * this.scale.speed + this.scale.factor;
}

// Draws text where x,y is the center of the drawing
textObject.prototype.draw = function(x, y)
{
	this.font.drawZoomedText(x - (this.width * this.scale.factor / 2), y - (this.height * this.scale.factor / 2), this.scale.factor, this.text);
}

// Updates internal stuffs
textObject.prototype.update = function()
{
	if (this.scale.speed != 0)
	{
		this.scale.factor += this.scale.speed;
	}
	if (this.scale.factor >= this.fade.start)
	{
		if (this.color.alpha - this.fade.speed >= 0)
		{
			this.color.alpha -= this.fade.speed;
		}
	}
	if (this.color.alpha < 255)
	{
		this.font.setColorMask(this.color);
	}
}

// Returns true if the scaling and fading is complete
textObject.prototype.isDone = function()
{
	if (this.scale.factor >= this.scale.end)
	{
		return true;
	}
	else
		return false;
}

// Creates a new image object
function imageObject(fileName, displayTime)
{
	this.file = LoadImage(fileName);
	this.type = "Image";
	this.color = CreateColor(255, 255, 255, 0);
	this.fade = new Object();
	this.fade.speed = 2;
	this.current = "FadeIn"; // "FadeIn", "Display", "FadeOut", "Done"
	this.displayTime = displayTime;
	this.displayStart = 0;
	this.width = this.file.width;
	this.height = this.file.height;
}

// Draw image where x,y is the center of the image
imageObject.prototype.draw = function(x, y)
{
	this.file.blitMask(x - this.width / 2, y - this.height / 2, this.color);
}

// Updates image fade
imageObject.prototype.update = function()
{
	switch (this.current)
	{
		case "FadeIn":
			if (this.color.alpha < 255)
			{
				if (this.color.alpha + this.fade.speed > 255)
				{
					this.color.alpha = 255;
				}
				else
					this.color.alpha += this.fade.speed;
			}
			else
				this.current = "Display";
			break;
		case "Display":
			if (this.displayStart + this.displayTime <= GetTime())
			{
				this.current = "FadeOut";
			}
			break;
		case "FadeOut":
			if (this.color.alpha > 0)
			{
				if (this.color.alpha - this.fade.speed < 0)
				{
					this.color.alpha = 0;
				}
				else
					this.color.alpha -= this.fade.speed;
			}
			else
				this.current = "Done";
			break;
		case "Done":
			break;
	}
}

// Returns true if the image is done fading
imageObject.prototype.isDone = function()
{
	if (this.current == "Done")
	{
		return true;
	}
	else
		return false;
}

// Introduction object
function introObject(background, font)
{
	this.things = new Array(); // Holds text and images
	this.currentThing = 0;
	if (background == "None")
	{
		this.background = undefined;
	}
	else
		this.background = background;
	this.font = font;
	this.bgColor = CreateColor(0, 0, 0);
	this.endColor = CreateColor(0, 0, 0);
	this.endFade = 2000;
	this.lastUpdate = 0;
	this.lastThing = 0;
}

// Add a text object into the intro
introObject.prototype.addText = function(text, x, y, delay, dx, dy)
{
	var itext = new textObject(text, this.font);
	itext.x = x;
	itext.y = y;
	if (delay == undefined)
	{
		itext.delay = 0;
	}
	else
		itext.delay = delay;
	if (dx != undefined && dy != undefined)
	{
		var iterations = Math.floor((itext.scale.end - itext.scale.factor) / itext.scale.speed);
		itext.xSpeed = (dx - x) / iterations;
		itext.ySpeed = (dy - y) / iterations;
	}
	itext.going = false;
	this.things[this.things.length] = itext;
}

// Add an image to the intro
introObject.prototype.addImage = function(fileName, displayTime, x, y, delay)
{
	var iimage = new imageObject(fileName, displayTime);
	if (x == undefined)
	{
		x = 160;
	}
	if (y == undefined)
	{
		y = 120;
	}
	if (delay == undefined)
	{
		delay = 0;
	}
	iimage.x = x;
	iimage.y = y;
	iimage.delay = delay;
	iimage.displayTime = displayTime;
	iimage.going = false;
	this.things[this.things.length] = iimage;
}

// Draw the intro to the screen
introObject.prototype.draw = function()
{
	ApplyColorMask(this.bgColor);
	if (this.background != undefined)
	{
		this.background.blit(0, 0);
	}
	if (this.things.length > 0)
	{
		for (var i = 0; i < this.currentThing + 1; i ++)
		{
			if (this.things[i].going)
			{
				this.things[i].draw(this.things[i].x, this.things[i].y);
			}
		}
	}
}

// Updates the intro
introObject.prototype.update = function()
{
	if (this.things.length > 0)
	{
		for (var i = 0; i < this.currentThing + 1; i ++)
		{
			if (this.things[i].going)
			{
				this.things[i].update();
				if (this.things[i].xSpeed != undefined && this.things[i].ySpeed != undefined)
				{
					this.things[i].x += this.things[i].xSpeed;
					this.things[i].y += this.things[i].ySpeed;
				}
			}
		}
		if (this.things[this.currentThing].delay + this.lastThing <= GetTime() && !this.things[this.currentThing].going)
		{
			this.things[this.currentThing].going = true;
			this.things[this.currentThing].displayStart = GetTime();
			this.things[this.currentThing].update();
			this.lastThing = GetTime();
			if (this.currentThing + 1 < this.things.length)
			{
				this.currentThing ++;
			}
		}
		for (var i = 0; i < this.currentThing + 1; i ++)
		{
			if (this.things[i].going)
			{
				if (this.things[i].isDone())
				{
					this.things.shift();
					this.currentThing --;
				}
			}
		}
	}
}

// Plays the intro until finished or ENTER is pressed
introObject.prototype.play = function()
{
	while (this.things.length > 0)
	{
		this.draw();
		FlipScreen();
		if (this.lastUpdate + 50 / 3 <= GetTime())
		{
			this.lastUpdate = GetTime();
			this.update();
		}
		if (IsKeyPressed(KEY_ENTER))
		{
			break;
		}
	}
	this.draw();
	FadeToColor(this.endFade, this.endColor);
}