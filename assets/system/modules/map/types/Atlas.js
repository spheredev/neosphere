'use strict';
module.exports = Atlas;

function Atlas(cellWidth, cellHeight, maxImages)
{
	var _numAcross = Math.ceil(Math.sqrt(maxImages));
	var _width = cellWidth * _numAcross;
	var _height = cellHeight * _numAcross;
	var _buffer = new Uint32Array(_width * _height);
	var _image = null;
	
	Object.defineProperty(this, 'cellWidth', {
		get: function() { return cellWidth; }
	});
	
	Object.defineProperty(this, 'cellHeight', {
		get: function() { return cellHeight; }
	});
	
	this.blit = function blit(index, pixels)
	{
		if (index < 0 || index >= _numAcross * _numAcross)
			throw new RangeError("index out of range");
		
		var x = index % _numAcross * cellWidth;
		var y = Math.floor(index / _numAcross) * cellHeight;
		pixels = new Uint32Array(pixels);
		var pSrc = 0;
		for (var iY = 0; iY < cellHeight; ++iY) {
			var pDest = x + (y + iY) * _width;
			for (var iX = 0; iX < cellWidth; ++iX) {
				_buffer[pDest++] = pixels[pSrc++];
			}
		}
		_image = null;
	};
	
	this.toImage = function toImage()
	{
		if (_image === null) {
			_image = new Image(_width, _height, _buffer);
		}
		return _image;
	};
	
	this.uv = function uv(index)
	{
		var x1 = index % _numAcross * cellWidth;
		var y1 = Math.floor(index / _numAcross) * cellHeight;
		var x2 = x1 + cellWidth;
		var y2 = y1 + cellHeight;
		return {
			u1: x1 / _width,
			v1: 1.0 - (y1 / _height),
			u2: x2 / _width,
			v2: 1.0 - (y2 / _height)
		};
	};
}
