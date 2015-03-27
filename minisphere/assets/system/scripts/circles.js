/**
 * circles.js
 * 
 * Inspired by the same system script by Eric Duvic.
 * 
 * API:
 * LineCircle(radius, color[, antialias])
 * Circle(radius, color[, antialias])
 * GradientCircle(radius, color1, color2[, antialias])
 * 
 * @author	Tung Nguyen
 */

/**
 * Traces the outline of a circle into a new image.
 * 
 * @param	radius	The radius of the circle.
 * @param	color	Sphere Color object used to colour line.
 * @param	antialias	If true, circle will be antialiased.
 * @return	Sphere Image object containing the circle.
 */
function LineCircle(radius, color, antialias)
{
	if (radius < 0 || !color)
		throw "LineCircle(): invalid parameter(s).";
	else if (radius == 0)
		return CreateSurface(1, 1, CreateColor(0, 0, 0, 0)).createImage();
	
	var pi = Math.PI;
	var sin = Math.sin;
	var cos = Math.cos;
	var sqrt = Math.sqrt;
	var abs = Math.abs;
	
	var w = radius * 2;
	var h = w;
	var s = CreateSurface(w, h, CreateColor(0, 0, 0, 0));
	var c = CreateColor(color.red, color.green, color.blue, color.alpha);
	var r = Math.max(radius - 0.5, 0);
	
	if (antialias)
	{
		// Smooth antialias for "one-pixel" width
		var pi_1_2 = pi / 2;
		s.setBlendMode(REPLACE);
		for (var y = 0; y < radius; ++y)
		{
			for (var x = 0; x < radius; ++x)
			{
				var dist = sqrt(x * x + y * y);
				if (dist > radius - 2 && dist < radius)
				{
					c.alpha = color.alpha * sin(sin((1 - abs(dist - radius + 1)) * pi_1_2) * pi_1_2);
					s.setPixel(x + r, y + r, c);	// Bottom-right
					s.setPixel(r - x, y + r, c);	// Bottom-left
					s.setPixel(r - x, r - y, c);	// Top-left
					s.setPixel(x + r, r - y, c);	// Top-right
				}
			}
		}
	}
	else
	{
		// Quick rough outline circle
		var pi_2 = pi * 2;
		var step = pi_2 / (Math.min(360, pi_2 * radius));
		for (var pt = 0, pt2 = step; pt < pi_2; pt += step, pt2 += step)
		{
			s.line(
					r + r * sin(pt),
					r + r * cos(pt),
					r + r * sin(pt2),
					r + r * cos(pt2),
					color);
		}
	}
	
	return s.createImage();
}

/**
 * Draws a filled circle into a new image.
 * 
 * @param	radius	The radius of the circle.
 * @param	color	The color to fill the circle with.
 * @param	antialias	If true, antialiasing will smooth the edges.
 * @return	Sphere Image object containing the circle.
 */
function Circle(radius, color, antialias)
{
	if (radius < 0 || !color)
		throw "Circle(): invalid parameter(s).";
	else if (radius == 0)
		return CreateSurface(1, 1, CreateColor(0, 0, 0, 0)).createImage();
	
	var pi_1_2 = Math.PI / 2;
	var sin = Math.sin;
	var asin = Math.asin;
	var cos = Math.cos;
	var sqrt = Math.sqrt;
	var abs = Math.abs;
	
	var w = radius * 2;
	var h = w;
	var s = CreateSurface(w, h, CreateColor(0, 0, 0, 0));
	var c = CreateColor(color.red, color.green, color.blue, color.alpha);
	var r = Math.max(radius - 0.5, 0);
	
	if (antialias)
	{
		// Scan quadrant
		s.setBlendMode(REPLACE);
		for (var y = 0; y < radius; ++y)
		{
			for (var x = 0; x < radius; ++x)
			{
				var dist = sqrt(x * x + y * y);
				if (dist < radius)
				{
					if (dist > radius - 1)
						c.alpha = color.alpha * sin(sin((radius - dist) * pi_1_2) * pi_1_2);
					else
						c.alpha = color.alpha;
					s.setPixel(x + r, y + r, c);	// Bottom-right
					s.setPixel(r - x, y + r, c);	// Bottom-left
					s.setPixel(r - x, r - y, c);	// Top-left
					s.setPixel(x + r, r - y, c);	// Top-right
				}
			}
		}
	}
	else
	{
		// Scanline for each pixel row of circle
		for (var y = 0; y < radius; ++y)
		{
			var lw = r * cos(asin(1 - y / r));
			s.line(r - lw, y, r + lw, y, c);
			s.line(r - lw, h - y - 1, r + lw, h - y - 1, c);
		}
	}
	
	return s.createImage();
}

/**
 * Draws a radially-gradient colored circle.
 * The fade is sinusoidal, not linear.
 * 
 * @param	radius	The radius of the circle.
 * @param	color1	The internal color.
 * @param	color2	The external color.
 * @param	antialias	If true, antialiasing will smooth the edges.
 * @return	Sphere Image object containing the circle.
 */
function GradientCircle(radius, color1, color2, antialias)
{
	if (radius < 0 || !color1 || !color2)
		throw "Circle(): invalid parameter(s).";
	else if (radius == 0)
		return CreateSurface(1, 1, CreateColor(0, 0, 0, 0)).createImage();
	
	var pi_1_2 = Math.PI / 2;
	var sin = Math.sin;
	var sqrt = Math.sqrt;
	
	var w = radius * 2;
	var h = w;
	var s = CreateSurface(w, h, CreateColor(0, 0, 0, 0));
	var c = CreateColor(0, 0, 0, 0);
	var r = Math.max(radius - 0.5, 0);
	
	var rd = color2.red - color1.red;
	var gd = color2.green - color1.green;
	var bd = color2.blue - color1.blue;
	var ad = color2.alpha - color1.alpha;
	
	// Quadrant scan
	s.setBlendMode(REPLACE);
	for (var y = 0; y < radius; ++y)
	{
		for (var x = 0; x < radius; ++x)
		{
			var dist = sqrt(x * x + y * y);
			if (dist < radius)
			{
				var factor = sin((1 - dist / radius) * pi_1_2)
				c.red = color2.red - rd * factor;
				c.green = color2.green - gd * factor;
				c.blue = color2.blue - bd * factor;
				c.alpha = color2.alpha - ad * factor;
				if (antialias && dist > radius - 1)
					c.alpha *= radius - dist;
				
				s.setPixel(x + r, y + r, c);	// Bottom-right
				s.setPixel(r - x, y + r, c);	// Bottom-left
				s.setPixel(r - x, r - y, c);	// Top-left
				s.setPixel(x + r, r - y, c);	// Top-right
			}
		}
	}
	
	return s.createImage();
}
