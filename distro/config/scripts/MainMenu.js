// miniconfig 1.1: Configuration Utility for minisphere
// Copyright (C) 2015 Fat Cerberus

function MainMenu()
{
	var res = { x: GetScreenWidth(), y: GetScreenHeight() };
	
	this.font = GetSystemFont();
	this.zoomText = { text: "Sphere " + GetVersionString(), fadeness: 0.0 };
	var textWidth = this.font.getStringWidth(this.zoomText.text);
	
	var surface = new Surface(textWidth + 1, this.font.height + 1);
	this.font.setColorMask(new Color(0, 0, 0, 255));
	surface.drawText(this.font, 1, 1, this.zoomText.text);
	this.font.setColorMask(new Color(255, 255, 255, 255));
	surface.drawText(this.font, 0, 0, this.zoomText.text);
	var shape = new Shape([
		{ x: 0, y: 0 },
		{ x: textWidth + 1, y: 0 },
		{ x: textWidth + 1, y: this.font.height + 1 },
		{ x: 0, y: this.font.height + 1 }
	], new Image(surface));
	this.title = new Group([ shape ], GetDefaultShaderProgram());
	this.fadeness = 0.0;
}

MainMenu.prototype.update = function()
{
	return true;
}

MainMenu.prototype.render = function()
{
	Rectangle(0, 0, GetScreenWidth(), GetScreenHeight(), new Color(0, 64, 0));
	this.title.draw();
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

