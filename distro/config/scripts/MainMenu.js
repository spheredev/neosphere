// miniconfig 1.1: Configuration Utility for minisphere
// Copyright (C) 2015 Fat Cerberus

function MainMenu()
{
	var res = { x: GetScreenWidth(), y: GetScreenHeight() };
	
	this.font = GetSystemFont();
	this.zoomText = { text: "Sphere " + GetVersionString(), fadeness: 0.0 };
	var textWidth = this.font.getStringWidth(this.zoomText.text);
	this.surface = new Surface(textWidth, this.font.height);
	this.surface.drawText(this.font, 0, 0, this.zoomText.text);
	this.fadeness = 0.0;
}

MainMenu.prototype.update = function()
{
	return true;
}

MainMenu.prototype.render = function()
{
	Rectangle(0, 0, GetScreenWidth(), GetScreenHeight(), new Color(0, 64, 0));
	this.surface.blit(10, 10);
	ApplyColorMask(new Color(0, 0, 0, (1.0 - this.fadeness) * 255));
}

MainMenu.prototype.run = function()
{
	var tid = mini.Threads.create(this);
	new mini.Scene()
		.tween(this, 1.0, 'easeOutQuad', { fadeness: 1.0 })
		.run();
	mini.Threads.join(tid);
}

