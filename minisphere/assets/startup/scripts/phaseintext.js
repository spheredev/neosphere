/**
	Phase in text module
	
	By tunginobi
	
	
	Using this object, you can make a line of text fade in.
	
	eg.
	
	var blah = new PhaseInText(10, 20, "Hahaha");
	blah.begin();
	while (!blah.done())
	{
		blah.draw();
		FlipScreen();
	}
*/


/**
	PhaseInText object prototype.
*/
function PhaseInText(x, y, text)
{
	if (this instanceof PhaseInText == false)
	{
		return new PhaseInText(x, y, text);
	}
	
	this.x = x || 0;
	this.y = y || 0;
	this.text = text || "";
	
	this.font = GetSystemFont();
	this.start = 0;
	this.duration = 0;
	
	return this;
}

/**
	Begin phasing in text.
*/
PhaseInText.prototype.begin = function ()
{
	this.start = GetTime();
	this.duration = this.text.length * (0.8 + Math.random() * 0.4) * 100;
	
	return;
}

/**
	Checks if the phasing is done. Returns true if it is.
*/
PhaseInText.prototype.done = function ()
{
	return GetTime() >= this.start + this.duration;
}

/**
	Draws the text in its current state.
*/
PhaseInText.prototype.draw = function ()
{
	if (this.done())
	{
		// Draw the text as normal
		this.font.drawText(this.x, this.y, this.text);
	}
	else
	{
		// Draw the text fading in
		var state = (GetTime() - this.start) / this.duration;
		if (state > 1) state = 1;
		var whole_chars = Math.floor(this.text.length * state);
		var part_char = this.text.length * state - whole_chars;
		var string_so_far = this.text.slice(0, whole_chars);
		
		this.font.drawText(this.x, this.y, string_so_far);
		var old_mask = this.font.getColorMask();
		this.font.setColorMask(CreateColor(old_mask.red, old_mask.green, old_mask.blue, old_mask.alpha * state));
		this.font.drawText(this.x + this.font.getStringWidth(string_so_far), this.y, this.text.charAt(whole_chars));
		this.font.setColorMask(old_mask);
	}
	
	return;
}
