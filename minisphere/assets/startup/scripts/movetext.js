/**
	MoveText object module
	
	By tunginobi
*/

RequireScript("textwrap.js");


function MoveText(x, y, text)
{
	if (this instanceof MoveText == false)
	{
		return new MoveText(x, y, text);
	}
	
	this.x = x || 0;
	this.y = y || 0;
	this.text = text || "";
	this.wrapText = undefined;
	
	this.font = GetSystemFont();
	this.width = 200;
	this.height = 100;
	
	this.start = 0;
	this.duration = 500;
	this.oldX = 0;
	this.oldY = 0;
	
	return this;
}

MoveText.prototype.draw = function ()
{
	if (this.done())
	{
		this.font.drawText(this.x, this.y, this.text);
	}
	else
	{
		var state = (GetTime() - this.start) / this.duration;
		if (state > 1) state = 1;
		state = Math.sin(state * Math.PI / 2);
		
		var real_x = this.oldX + (this.x - this.oldX) * state;
		var real_y = this.oldY + (this.y - this.oldY) * state;
		
		this.font.drawText(real_x, real_y, this.text);
	}
	
	return;
}

MoveText.prototype.drawWrapped = function ()
{
/*	if (this.done())
	{
		this.font.drawTextBox(this.x, this.y, this.width, this.height, 0, this.text);
	}
	else
	{
*/		if (!this.wrapText)
		{
			this.wrapText = WordWrapByFontWidth(this.text, this.font, this.width);
		}
		
		var state = (GetTime() - this.start) / this.duration;
		if (state > 1) state = 1;
		state = Math.sin(state * Math.PI / 2);
		
		var real_x = this.oldX + (this.x - this.oldX) * state;
		var real_y = this.oldY + (this.y - this.oldY) * state;
		
		//this.font.drawTextBox(real_x, real_y, this.width, this.height, 0, this.text);
		// Go!  ->       ->      ->       ->      -> vvv hax0r!
		for (var i = 0; i < this.wrapText.length && i < 5; ++i)
		{
			this.font.drawText(real_x, real_y + this.font.getHeight() * i, this.wrapText[i]);
		}
/*	}
*/	
	return;
}

MoveText.prototype.move = function (new_x, new_y)
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

MoveText.prototype.done = function ()
{
	return GetTime() >= this.start + this.duration;
}
