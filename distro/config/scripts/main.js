// miniconfig 1.1: Configuration Utility for minisphere
// Copyright (C) 2015 Fat Cerberus

RequireSystemScript('mini/Link.js');
RequireSystemScript('mini/Scenes.js');

function game()
{
	mini.initialize();
	SetFrameRate(60);
	
	var font = GetSystemFont();
	var res = { x: GetScreenWidth(), y: GetScreenHeight() };
	var zoomText = { text: "Sphere " + GetVersionString(), fadeness: 0.0 };
	var textWidth = font.getStringWidth(zoomText.text);
	var surface = new Surface(textWidth, font.height);
	surface.drawText(font, 0, 0, zoomText.text);
	var shape = new Shape([
		{ x: 0, y: 0, u: 0, v: 0 },
	], new Image(surface));
	new Scenario()
		.tween(zoomText, 1.0, 'easeOutQuad', { fadeness: 1.0 })
		.run();
	while (true) {
		font.setColorMask(new Color(255, 255, 255, zoomText.fadeness * 255));
		font.drawText(res.x - 10 - textWidth, 10, zoomText.text);
		font.drawText(10, 10, "miniconfig");
		Scenario.renderAll();
		FlipScreen();
		Scenario.updateAll();
	}
}
