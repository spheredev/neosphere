/*//////////////////////////////////////////////////////////
// This script is so that you can define colors in terms of:
// color name  |  value range
// ----------------------------
// red         |  0 to 255
// green       |  0 to 255
// blue        |  0 to 255
// alpha       |  0 to 255
// cyan        |  0 to 255
// magenta     |  0 to 255
// yellow      |  0 to 255
// hue         |  0 to 2PI
// saturation  |  0 to 1
// intensity   |  0 to 1

e.g.
  var Red = new Color(255, 0, 0);

  var DarkerRed = Red.clone(); // copy Red into DarkerRed
      DarkerRed.intensity -= 0.1; // this lowers the brightness of the red

  var Yellow = new Color(255, 255, 255); // this is white
      Yellow.yellow = 255; // this is now yellow

  // Note that changing the yellow compontent of a color,
  // doesn't mean you actually end up with yellow
  // (you only do in this example because you started with white

//////////////////////////////////////////////////////////*/

function Clamp(v, l, h) {
  if (v < l)
    v = l;
  if (v > h)
    v = h;
  return v;
}

function Color(red, green, blue, alpha) {
  var red = red;
  var green = green;
  var blue = blue;
  var alpha = typeof(alpha) == "number" ? Clamp(0, 255, alpha) : 255;

  this.red   getter = function( ) { return red; }
  this.red   setter = function(v) { red = Clamp(0, 255, v); }
  this.green getter = function( ) { return green; }
  this.green setter = function(v) { green = Clamp(0, 255, v); }
  this.blue  getter = function( ) { return blue; }
  this.blue  setter = function(v) { blue = Clamp(0, 255, v); }

  this.alpha  getter = function( ) { return alpha; }
  this.alpha  setter = function(v) { alpha = Clamp(0, 255, v); }

  this.cyan    getter = function( ) { return 255 - red; }
  this.cyan    setter = function(v) { red = Clamp(0, 255, 255 - v); }
  this.magenta getter = function( ) { return 255 - green; }
  this.magenta setter = function(v) { green = Clamp(0, 255, 255 - v); }
  this.yellow  getter = function( ) { return 255 - blue; }
  this.yellow  setter = function(v) { blue = Clamp(0, 255, 255 - v); }

  this.hue        getter = function( ) { return (RGBtoHSI(new RGB(red, green, blue)).hue); }
  this.hue        setter = function(v) {
    var in_clr = RGBtoHSI(new RGB(red, green, blue));
    in_clr.hue = Clamp(0.0, 2.0 * Math.PI, v);
    var out_clr = HSItoRGB(in_clr);
    red = out_clr.red;
    green = out_clr.green;
    blue = out_clr.blue;
  }

  this.saturation getter = function( ) { return (RGBtoHSI(new RGB(red, green, blue)).saturation); }
  this.saturation setter = function(v) {
    var in_clr = RGBtoHSI(new RGB(red, green, blue));
    in_clr.saturation = Clamp(0.0, 1.0, v);
    var out_clr = HSItoRGB(in_clr);
    red = out_clr.red;
    green = out_clr.green;
    blue = out_clr.blue;
  }

  this.intensity  getter = function( ) { return (RGBtoHSI(new RGB(red, green, blue)).intensity); }
  this.intensity  setter = function(v) {
    var in_clr = RGBtoHSI(new RGB(red, green, blue));
    in_clr.intensity = Clamp(0.0, 1.0, v);
    var out_clr = HSItoRGB(in_clr);
    red = out_clr.red;
    green = out_clr.green;
    blue = out_clr.blue;
  }
  
  this.clone = function() {
    return new Color(this.red, this.green, this.blue);
  }

  this.toString = function() {
    return "red "    + this.red
         + " green " + this.green
         + " blue "  + this.blue
         + " alpha " + this.alpha
         + " cyan "    + this.cyan
         + " magenta " + this.magenta
         + " yellow "  + this.yellow
         + " hue = "      + this.hue
         + " saturation " + this.saturation
         + " intensity "  + this.intensity;
  }
}

///////////////////////////////////////////////////////////

function RGB(red, green, blue)
{
  this.red   = red;
  this.green = green;
  this.blue  = blue;
}

///////////////////////////////////////////////////////////

function HSI(hue, saturation, intensity)
{
  this.hue = hue;
  this.saturation = saturation;
  this.intensity = intensity;
}

///////////////////////////////////////////////////////////

function DegreesToRadians(degrees) {
  return (degrees * Math.PI / 180.0);
}

function RadiansToDegrees(radians) {
  return (radians * 180.0 / Math.PI);
}

///////////////////////////////////////////////////////////

function RGBtoHSI(clr)
{
  var r = 1.0/255.0 * clr.red;
  var g = 1.0/255.0 * clr.green;
  var b = 1.0/255.0 * clr.blue;

  var total = (r + g + b);
  var third_total = 0;
  var three_over_total = 0;

  var hue = 0.0;
  var saturation = 0.0;
  var intensity = 0.0;

  var minvalue = r;
  if (g < minvalue) minvalue = g;
  if (b < minvalue) minvalue = b;

  if (total) {
    third_total = (total / 3.0);
    three_over_total = 3.0 / total;
  }

  intensity = third_total;
  saturation = 1.0 - three_over_total * minvalue;

  if (saturation <= 0) {
    hue = Math.PI;
  }
  else {
    hue = 0.5 * ( (r - g) + (r - b) );
    hue = Math.acos( hue / Math.sqrt( Math.pow((r-g), 2) + ( (r - b) * (g - b) ) ) );

    if ((b > g)) {
      hue = DegreesToRadians(360) - hue;
    }
  }

  return new HSI(hue, saturation, intensity);
}

///////////////////////////////////////////////////////////

function HSItoRGB(clr)
{
  var hue = clr.hue;
  var saturation = clr.saturation;
  var intensity = clr.intensity;

  var red = 0;
  var green = 0;
  var blue = 0;
  
  var hue_in_degrees = Math.floor(RadiansToDegrees(hue));

  if (intensity <= 0.0001) {
    return new RGB(0, 0, 0);
  }

  if (intensity >= 1.0)  {
    return new RGB(255, 255, 255);
  }

  if (saturation <= 0.0001) {
    return new RGB(intensity * 255.0, intensity * 255.0, intensity * 255.0);
  }

  if (hue_in_degrees >= 0 && hue_in_degrees <= 120) {
    blue  = intensity * (1.0 - saturation);
    red   = intensity * (1.0 + (saturation * Math.cos(hue)) / (Math.cos(DegreesToRadians(60) - hue)));
    green = (3.0 * intensity) * (1.0 - ((red + blue) / (3.0 * intensity)));
  }
  else
  if (hue_in_degrees > 120 && hue_in_degrees <= 240) {
    hue = hue - DegreesToRadians(120);
    green = intensity * (1.0 + (saturation * Math.cos(hue)) / (Math.cos(DegreesToRadians(60) - hue)));
    red   = intensity * (1.0 - saturation);
    blue  = (3.0 * intensity) * (1.0 - ((red + green) / (3.0 * intensity)));
  }
  else
  if (hue_in_degrees > 240 && hue_in_degrees <= 360)
  {
    hue = hue - DegreesToRadians(240);
    blue  = intensity * (1.0 + (saturation * Math.cos(hue)) / (Math.cos(DegreesToRadians(60) - hue)));
    green = intensity * (1.0 - saturation);
    red   = (3.0 * intensity) * (1.0 - ((green + blue) / (3.0 * intensity)));
  }

  if (red > 1.0)    red = 1.0;
  if (red < 0.0001) red = 0.0;
  if (green > 1.0)    green = 1.0;
  if (green < 0.0001) green = 0.0;
  if (blue > 1.0)    blue = 1.0;
  if (blue < 0.0001) blue = 0.0;

  red   *= 255.0;
  green *= 255.0;
  blue  *= 255.0;

  return new RGB(red, green, blue);
}

///////////////////////////////////////////////////////////
