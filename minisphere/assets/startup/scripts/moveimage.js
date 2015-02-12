/**
	MoveImage object module
	
	By tunginobi
*/


function MoveImage(x, y, image)
{
	if (this instanceof MoveImage == false)
	{
		return new MoveImage(x, y, image);
	}
	
	this.x = x || 0;
	this.y = y || 0;
	this.image = image || 0;
	
	this.start = 0;
	this.duration = 500;
	this.oldX = 0;
	this.oldY = 0;
	
	return this;
}

MoveImage.prototype.draw = function ()
{
	if (this.done())
	{
		this.image.blit(this.x, this.y);
	}
	else
	{
		var state = (GetTime() - this.start) / this.duration;
		if (state > 1) state = 1;
		state = Math.sin(state * Math.PI / 2);
		
		var real_x = this.oldX + (this.x - this.oldX) * state;
		var real_y = this.oldY + (this.y - this.oldY) * state;
		
		this.image.blit(real_x, real_y);
	}
	
	return;
}

MoveImage.prototype.move = function (new_x, new_y)
{
	if (this.done())
	{
		this.oldX = this.x;
		this.oldY = this.y;
		this.x = new_x;
		this.y = new_y;
	}
	else
	{
		var state = (GetTime() - this.start) / this.duration;
		if (state > 1) state = 1;
		state = Math.sin(state * Math.PI / 2);
		
		this.oldX += (this.x - this.oldX) * state;
		this.oldY += (this.y - this.oldY) * state;
		this.x = new_x;
		this.y = new_y;
	}
	
	this.start = GetTime();
	
	return;
}

MoveImage.prototype.done = function ()
{
	return GetTime() >= this.start + this.duration;
}
