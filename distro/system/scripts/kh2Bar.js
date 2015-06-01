/**
 * kh2Bar for Sphere  (c) 2013-2015 Bruce Pascoe
 * A multi-segment HP gauge styled after the enemy HP bars in Kingdom Hearts 2.
**/

var kh2Bar =
(function(undefined) {
    'use strict';
	
    function drawSegment(x, y, width, height, color)
    {
        var halfHeight = Math.ceil(height / 2);
        var dimColor = BlendColorsWeighted(color, new Color(0, 0, 0, color.alpha), 66, 33);
        var yHalf = y + Math.floor(height / 2);
        GradientRectangle(x, y, width, halfHeight, dimColor, dimColor, color, color);
        GradientRectangle(x, yHalf, width, halfHeight, color, color, dimColor, dimColor);
    };
	
    function fadeColor(color, fadeness)
    {
        return new Color(color.red, color.green, color.blue, color.alpha * (1.0 - fadeness));
    }
	
    function tween(start, time, duration, end)
    {
        return -(end - start) / 2 * (Math.cos(Math.PI * time / duration) - 1) + start;
    }
	
    // HPGauge() constructor
    // Creates a Kingdom Hearts-style segmented HP gauge.
    // Arguments:
    //     capacity:   The largest HP value representable by the gauge.
    //     sectorSize: Optional. The amount of HP represented by each full bar of the gauge. (default: 100)
    //     color:      Optional. The initial color of the gauge. If not specified, a default color of bright green (#00FF00 @ 100%)
    //                 will be used, the same color Kingdom Hearts 2 uses.
    //     maxSectors: Optional. The maximum number of sectors the gauge can display. If this is not specified or
    //                 is null, the gauge will be rendered as it would be in Kingdom Hearts 2, with the maximum number
    //                 of sectors determined by the width and height of the gauge.
    function HPGauge(capacity, sectorSize, color, maxSectors)
    {
        if (arguments.length < 1) {
            Abort("kh2Bar(): 1 or more arguments expected, got " + arguments.length, -1);
        }
        
        sectorSize = sectorSize !== undefined ? sectorSize : 100;
        color = color !== undefined ? BlendColors(color, color) : new Color(0, 255, 0, 255);
        maxSectors = maxSectors !== undefined ? maxSectors : null;
        
        this.borderColor = new Color(0, 0, 0, color.alpha);
        this.capacity = capacity;
        this.colorFadeDuration = 0.0;
        this.colorFadeTimer = 0.0;
        this.damage = 0;
        this.damageColor = new Color(192, 0, 0, color.alpha);
        this.damageFadeness = 1.0;
        this.drainSpeed = 2.0;
        this.emptyColor = new Color(32, 32, 32, color.alpha);
        this.fadeSpeed = 0.0;
        this.fadeness = 1.0;
        this.hpColor = BlendColors(color, color);
        this.isVisible = true;
        this.maxSectors = maxSectors;
        this.newColor = color;
        this.newReading = this.capacity;
        this.numCombosRunning = 0;
        this.oldColor = color;
        this.oldReading = this.capacity;
        this.reading = this.capacity;
        this.sectorSize = sectorSize;
    };
	
    // .beginCombo() method
    // Begins a combo. Damage displayed on the gauge will accumulate without fading out until
    // the combo is ended by calling .endCombo().
    HPGauge.prototype.beginCombo = function()
    {
        ++this.numCombosRunning;
    };
	
    // .changeColor() method
    // Changes the color of the gauge, optionally easing into the new color.
    // Arguments:
    //     color:    The color to change the gauge to.
    //     duration: Optional. The length of time over which to ease in the new color. A duration of 0
    //               means no easing (immediate). (default: 0.0)
    HPGauge.prototype.changeColor = function(color, duration)
    {
        duration = duration !== undefined ? duration : 0.0;
        
        this.oldColor = BlendColors(this.hpColor, this.hpColor);
        this.newColor = BlendColors(color, color);
        if (duration != 0.0) {
            this.colorFadeDuration = duration;
            this.colorFadeTimer = 0.0;
        } else {
            this.hpColor = this.newColor;
        }
    };
	
    // .draw() method
    // Draws the gauge in its current state on the screen.
    // Arguments:
    //     x:       The X coordinate of the top left corner of the gauge, in pixels.
    //     y:       The Y coordinate of the top left corner of the gauge, in pixels.
    //     width:   The width of the gauge, in pixels.
    //     height:  The height of the gauge, in pixels.
    HPGauge.prototype.draw = function(x, y, width, height)
    {
        if (arguments.length < 4) {
            Abort("kh2Bar:draw(): Wrong number of arguments\n" +
                "4 arguments were expected; caller passed " + arguments.length + ".",
                -1);
        }
        
        if (this.fadeness >= 1.0) {
            return;
        }
        var damageShown = Math.max(this.damage - (this.reading - this.newReading), 0);
        var numReserves = Math.ceil(this.capacity / this.sectorSize - 1);
        var numReservesFilled = Math.max(Math.ceil(this.reading / this.sectorSize - 1), 0);
        var numReservesDamaged = Math.ceil((damageShown + this.reading) / this.sectorSize - 1);
        var barInUse;
        if (numReservesFilled == numReserves) {
            barInUse = this.capacity % this.sectorSize;
            if (barInUse == 0) {
                barInUse = this.sectorSize;
            }
        } else {
            barInUse = this.sectorSize;
        }
        var barFilled = this.reading % this.sectorSize;
        if (barFilled == 0 && this.reading > 0) {
            barFilled = barInUse;
        }
        var barDamaged = Math.min(damageShown, this.sectorSize - barFilled);
        var barHeight = Math.ceil(height * 0.5 + 0.5);
        var widthInUse = Math.round((width - 2) * barInUse / this.sectorSize);
        var fillWidth = Math.ceil(widthInUse * barFilled / barInUse);
        var damageWidth = Math.ceil(widthInUse * (barFilled + barDamaged) / barInUse) - fillWidth;
        var emptyWidth = widthInUse - (fillWidth + damageWidth);
        var borderColor = fadeColor(this.borderColor, this.fadeness);
        var fillColor = fadeColor(this.hpColor, this.fadeness);
        var emptyColor = fadeColor(this.emptyColor, this.fadeness);
        var usageColor = BlendColorsWeighted(emptyColor, fadeColor(this.damageColor, this.fadeness), this.damageFadeness, 1.0 - this.damageFadeness);
        if (barInUse < this.sectorSize && numReservesFilled > 0) {
            OutlinedRectangle(x, y, width, barHeight, BlendColorsWeighted(borderColor, new Color(0, 0, 0, 0), 25, 75));
            drawSegment(x + 1, y + 1, width - 2, barHeight - 2, BlendColorsWeighted(fillColor, new Color(0, 0, 0, 0), 25, 75));
        }
        var barEdgeX = x + width - 1;
        OutlinedRectangle(barEdgeX - widthInUse - 1, y, widthInUse + 2, barHeight, borderColor);
        drawSegment(barEdgeX - fillWidth, y + 1, fillWidth, barHeight - 2, fillColor);
        drawSegment(barEdgeX - fillWidth - damageWidth, y + 1, damageWidth, barHeight - 2, usageColor);
        drawSegment(barEdgeX - fillWidth - damageWidth - emptyWidth, y + 1, emptyWidth, barHeight - 2, emptyColor);
        var slotYSize = height - barHeight + 1;
        var slotXSize = this.maxSectors !== null ? Math.ceil(width / (this.maxSectors - 1)) : Math.round(slotYSize * 1.25);
        var slotX;
        var slotY = y + height - slotYSize;
        for (i = 0; i < numReserves; ++i) {
            var color;
            if (i < numReservesFilled) {
                color = fillColor;
            } else if (i < numReservesDamaged) {
                color = usageColor;
            } else {
                color = emptyColor;
            }
            slotX = x + (width - slotXSize) - i * (slotXSize - 1);
            OutlinedRectangle(slotX, slotY, slotXSize, slotYSize, borderColor);
            drawSegment(slotX + 1, slotY + 1, slotXSize - 2, slotYSize - 2, color);
        }
    };
	
    // .endCombo() method
    // Ends a combo, causing all damage sustained since .beginCombo() was called to fade out.
    HPGauge.prototype.endCombo = function()
    {
        --this.numCombosRunning;
        if (this.numCombosRunning < 0) {
            this.numCombosRunning = 0;
        }
    };
	
    // .hide() method
    // Hides the gauge.
    // Arguments:
    //     duration: Optional. The duration of the hide animation, in seconds. (default: 0.0)
    HPGauge.prototype.hide = function(duration)
    {
        duration = duration !== undefined ? duration : 0.0;
        
        if (duration > 0.0) {
            this.fadeSpeed = 1.0 / duration * (1.0 - this.fadeness);
        } else {
            this.fadeSpeed = 0.0;
            this.fadeness = 1.0;
        }
    };
	
    // .set() method
    // Sets the gauge's current HP reading.
    HPGauge.prototype.set = function(value)
    {
        if (arguments.length < 1) {
            Abort("kh2Bar:set(): Wrong number of arguments\n" +
                "1 argument was expected; caller passed " + arguments.length + ".",
                -1);
        }
        
        value = Math.min(Math.max(Math.round(value), 0), this.capacity);
        if (value != this.reading) {
            if (this.numCombosRunning > 0) {
                this.damage += this.reading - value;
            } else {
                this.damage = this.reading - value;
            }
            this.damageFadeness = 0.0;
            this.oldReading = this.reading;
            this.newReading = value;
            this.drainTimer = 0.0;
        }
    };
	
    // .show() method
    // Displays the gauge.
    // Arguments:
    //     duration: Optional. The duration of the show animation, in seconds. (default: 0.0)
    HPGauge.prototype.show = function(duration)
    {
        duration = duration !== undefined ? duration : 0.0;
        
        if (duration > 0.0) {
            this.fadeSpeed = 1.0 / duration * (0.0 - this.fadeness);
        } else {
            this.fadeSpeed = 0.0;
            this.fadeness = 0.0;
        }
    };
	
    // .update() method
    // Updates the gauge for the next frame.
    HPGauge.prototype.update = function()
    {
        var frameRate = IsMapEngineRunning() ? GetMapEngineFrameRate() : GetFrameRate();
        this.colorFadeTimer += 1.0 / frameRate;
        if (this.colorFadeDuration != 0.0 && this.colorFadeTimer < this.colorFadeDuration) {
            this.hpColor.red = tween(this.oldColor.red, this.colorFadeTimer, this.colorFadeDuration, this.newColor.red);
            this.hpColor.green = tween(this.oldColor.green, this.colorFadeTimer, this.colorFadeDuration, this.newColor.green);
            this.hpColor.blue = tween(this.oldColor.blue, this.colorFadeTimer, this.colorFadeDuration, this.newColor.blue);
            this.hpColor.alpha = tween(this.oldColor.alpha, this.colorFadeTimer, this.colorFadeDuration, this.newColor.alpha);
        } else {
            this.hpColor = this.newColor;
            this.colorFadeDuration = 0.0;
        }
        this.fadeness = Math.min(Math.max(this.fadeness + this.fadeSpeed / frameRate, 0.0), 1.0);
        this.drainTimer += 1.0 / this.drainSpeed / frameRate;
        if (this.newReading != this.reading && this.drainTimer < 0.25) {
            this.reading = Math.round(tween(this.oldReading, this.drainTimer, 0.25, this.newReading));
        } else {
            this.reading = this.newReading;
        }
        if (this.numCombosRunning <= 0 && this.reading == this.newReading) {
            this.damageFadeness += 1.0 / frameRate;
            if (this.damageFadeness >= 1.0) {
                this.damage = 0;
                this.damageFadeness = 1.0;
            }
        }
    };
    
    return {
        HPGauge: HPGauge
    };
})();

// populate CommonJS export table
if (typeof exports !== 'undefined')
{
    exports.HPGauge = kh2Bar.HPGauge;
}
