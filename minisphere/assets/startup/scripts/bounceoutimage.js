/**
	Bounce out image module
	
	By tunginobi
	
	
	Has an image bounce out of a specified set of coordinates.
*/


/**
	BounceOutImage object prototype.
*/
function BounceOutImage(x, y, image)
{
	if (this instanceof BounceOutImage == false)
	{
		return new BounceOutImage(x, y, image);
	}
	
	this.x = x || 0;
	this.y = y || 0;
	this.image = image;
	
	this.start = 0;
	this.duration = 1000;
	
	return this;
}


/**
	Begins the bouncing out of the image.
*/
BounceOutImage.prototype.begin = function ()
{
	this.start = GetTime();
	return;
}

/**
	Checks if the bounce is done. True if it is.
*/
BounceOutImage.prototype.done = function ()
{
	return GetTime() >= this.start + this.duration;
}

/**
	Draws the image in its current state. x and y are centre coordinates.
*/
BounceOutImage.prototype.draw = function ()
{
	var correct_x = this.x - Math.floor(this.image.width / 2);
	var correct_y = this.y - Math.floor(this.image.height / 2);
	
	if (this.done())
	{
		// Draw image normally
		this.image.blit(correct_x, correct_y);
	}
	else
	{
		// Draw image bouncing out
		var state = (GetTime() - this.start) / this.duration;
		if (state > 1) state = 1;
		var zoom_factor = Math.sin(Math.PI * state) + state;
		
		correct_x = this.x - Math.floor(this.image.width * zoom_factor / 2);
		correct_y = this.y - Math.floor(this.image.height * zoom_factor / 2);
		
		this.image.zoomBlit(correct_x, correct_y, zoom_factor);
	}
	
	return;
}
