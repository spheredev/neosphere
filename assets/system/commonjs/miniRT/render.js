/**
 *  miniRT/render CommonJS module
 *  tools for pre-rendering common RPG UI elements
 *  (c) 2015-2016 Fat Cerberus
**/

if (typeof exports === 'undefined')
{
	throw new TypeError("script must be loaded with require()");
}

var prerender =
module.exports = (function()
{
	return {
		TextRender: TextRender,
	};

	function TextRender(text, options)
	{
		options = options != null ? options : {};
		
		var font = 'font' in options ? options.font : GetSystemFont();
		var color = 'color' in options ? options.color : new Color(255, 255, 255, 255);
		var shadow = 'shadow' in options ? options.shadow : 0;

		var shadowColor = new Color(0, 0, 0, color.alpha);
		var width = font.getStringWidth(text) + shadow;
		var height = font.height + shadow;
		var surface = new Surface(width, height);
		var lastColorMask = font.getColorMask();
		font.setColorMask(shadowColor);
		surface.drawText(font, shadow, shadow, text);
		font.setColorMask(color);
		surface.drawText(font, 0, 0, text);
		font.setColorMask(lastColorMask);
		var image = surface.createImage();

		this.draw = function draw(x, y)
		{
			image.blit(x, y);
		}
	}
})();
