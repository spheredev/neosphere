/**
	TickerBar module
	
	By tunginobi
	
	
	Displays that funky ticker at the bottom of the screen.
*/

RequireScript("movetext.js");
RequireScript("phaseintext.js");


function TickerBar(x, y, str_list)
{
	if (this instanceof TickerBar == false)
	{
		return new TickerBar(x, y, str_list);
	}
	
	this.x = x || 0;
	this.y = y || 0;
	this.strList = str_list || new Array();
	
	this.start = 0;
	this.duration = 15000;		// 15 seconds between switches by default
	this.current = 0;
	
	this.lastText = new MoveText(this.x, this.y, "");
	this.currentText = new PhaseInText(this.x, this.y, "");
	
	return this;
}

/**
	Call this to start the ticker bar.
*/
TickerBar.prototype.init = function ()
{
	this.current = 0;
	
	this.lastText = new MoveText(this.x, this.y, "");
	this.currentText = new PhaseInText(this.x, this.y, this.strList[this.current]);
	
	this.lastText.move(SCREEN_WIDTH, this.lastText.y);
	this.currentText.begin();
	
	this.start = GetTime();
	
	return;
}

/**
	Decides whether or not to switch texts, and if so, does so.
*/
TickerBar.prototype.update = function ()
{
	if (GetTime() > this.start + this.duration)
	{
		this.current = (this.current + 1) % this.strList.length;
		this.lastText = new MoveText(this.x, this.y, this.currentText.text);
		this.currentText = new PhaseInText(this.x, this.y, this.strList[this.current]);
		
		this.lastText.move(SCREEN_WIDTH, this.lastText.y);
		this.currentText.begin();
		
		this.start = GetTime();
	}
	
	return;
}

/**
	Draw the ticker bar.
*/
TickerBar.prototype.draw = function ()
{
	this.lastText.draw();
	this.currentText.draw();
	
	return;
}
