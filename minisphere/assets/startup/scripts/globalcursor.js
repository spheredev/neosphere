/**
	GlobalCursor object module
	
	By tunginobi
	
	
	Contains the spinning triangle cursor.
*/


function GlobalCursor(x, y)
{
	if (this instanceof GlobalCursor == false)
	{
		return new GlobalCursor(x, y);
	}
	
	this.x = x || 0;
	this.y = y || 0;
	
	this.col = CreateColor(0, 255, 0, 192);
	
	this.oldX = x;
	this.oldY = y;
	this.start = 0;
	this.duration = 300;
	
	return this;
}


GlobalCursor.prototype.draw = function ()
{
	var state = (GetTime() - this.start) / this.duration;
	var real_x = this.x;
	var real_y = this.y;
	
	if (state < 1)
	{
		// Location is shifting linearly
		real_x = this.oldX + (this.x - this.oldX) * state;
		real_y = this.oldY + (this.y - this.oldY) * state;
	}
	
	// Triangle traces a circle with a radius of 4 pixels
	Triangle(
		real_x + Math.cos(Math.PI * 2 * (GetTime() % 1000) / 1000) * 6,
		real_y + Math.sin(Math.PI * 2 * (GetTime() % 1000) / 1000) * 6,
		real_x + Math.cos(Math.PI * 2 * (GetTime() % 1000) / 1000 + Math.PI * 2 / 3) * 6,
		real_y + Math.sin(Math.PI * 2 * (GetTime() % 1000) / 1000 + Math.PI * 2 / 3) * 6,
		real_x + Math.cos(Math.PI * 2 * (GetTime() % 1000) / 1000 + Math.PI * 4 / 3) * 6,
		real_y + Math.sin(Math.PI * 2 * (GetTime() % 1000) / 1000 + Math.PI * 4 / 3) * 6,
		this.col
	);
	
	return;
}

GlobalCursor.prototype.move = function (new_x, new_y)
{
	var state = (GetTime() - this.start) / this.duration;
	if (state > 1) state = 1;
	
	if (state < 1)
	{
		this.oldX += (this.x - this.oldX) * state;
		this.oldY += (this.y - this.oldY) * state;
	}
	else
	{
		this.oldX = this.x;
		this.oldY = this.y;
	}
	this.x = new_x;
	this.y = new_y;
	this.start = GetTime();
	
	return;
}
