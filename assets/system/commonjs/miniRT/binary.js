/**
 *  miniRT/binary CommonJS module
 *  allows loading structured data from binary files based on a JSON schema
 *  (c) 2015-2016 Fat Cerberus
**/

if (typeof exports === 'undefined')
{
	throw new TypeError("script must be loaded with require()");
}

const link = require('link');

var binary =
module.exports = (function() {
	return {
		load: load,
		read: read,
	};

	function load(filename, schema)
	{
		if (!Array.isArray(schema))
			throw new TypeError("expected an Array object for `schema`");

		var stream = new FileStream(filename, 'rb');
		var object = binary.read(stream, schema);
		stream.close();
		return object;
	}
	
	function read(stream, schema)
	{
		if (!Array.isArray(schema))
			throw new TypeError("expected an Array object for `schema`");

		return _readObject(stream, schema, null);
	}

	function _fixup(value, data)
	{
		// this fixes up backreferences in the schema, for example '@numPigs'.
		// because files are loaded sequentially, references can only be to
		// earlier fields.

		if (typeof value == 'string' && value[0] == '@')
			return data[value.substr(1)];
		else
			return value;
	}

	function _readField(stream, fieldType, data)
	{
		var dataSize = _fixup(fieldType.size, data);
		var itemCount = _fixup(fieldType.count, data);

		// read a value or object from the stream
		var value;
		switch (fieldType.type) {
			case 'switch':
				var control = data[fieldType.field];
				var checkType = null;
				for (var i = 0; i < fieldType.cases.length; ++i) {
					var checkType = fieldType.cases[i];
					if (('value' in checkType && typeof control == typeof checkType.value && control === checkType.value)
					    || ('range' in checkType && typeof control == 'number' && control >= checkType.range[0] && control <= checkType.range[1]))
					{
						break;
					}
					checkType = null;					
				}
				if (checkType != null)
					value = _readField(stream, checkType, data);
				else
					throw new Error("binary field `" + fieldType.field + "` is invalid");
				break;
			case 'array':
				value = [];
				for (var i = 0; i < itemCount; ++i) {
					value[i] = _readField(stream, fieldType.subtype, data);
				}
				break;
			case 'bool':
				value = stream.readUInt(1) != 0;
				break;
			case 'double':
				value = stream.readDouble();
				break;
			case 'doubleLE':
				value = stream.readDouble(true);
				break;
			case 'float':
				value = stream.readFloat();
				break;
			case 'floatLE':
				value = stream.readFloat(true);
				break;
			case 'fstring':
				value = stream.readString(dataSize);
				break;
			case 'image':
				var width = _fixup(fieldType.width, data);
				var height = _fixup(fieldType.height, data);
				if (typeof width != 'number' || typeof height != 'number')
					throw new Error("missing or invalid width/height for `image` field");
				var pixelData = stream.read(width * height * 4);
				value = new Image(width, height, pixelData);
				break;
			case 'int':
				value = stream.readInt(dataSize);
				break;
			case 'intLE':
				value = stream.readInt(dataSize, true);
				break;
			case 'object':
				value = _readObject(stream, fieldType.schema, data);
				break;
			case 'pstring':
				value = stream.readPString(dataSize);
				break;
			case 'pstringLE':
				value = stream.readPString(dataSize, true);
				break;
			case 'raw':
				value = stream.read(dataSize);
				break;
			case 'reserved':
				var buf = stream.read(dataSize);
				value = undefined;
				break;
			case 'uint':
				value = stream.readUInt(dataSize);
				break;
			case 'uintLE':
				value = stream.readUInt(dataSize, true);
				break;
			default:
				throw new TypeError("unknown field type `" + fieldType.type + "` in schema");
		}

		// verify the value we got against the schema
		var isValidValue = true;
		if (Array.isArray(fieldType.values)) {
			if (!link(fieldType.values).contains(value)) {
				isValidValue = false;
			}
		}
		else if (Array.isArray(fieldType.range) && fieldType.range.length == 2
		    && typeof fieldType.range[0] === typeof value
		    && typeof fieldType.range[1] === typeof value)
		{
			if (value < fieldType.range[0] || value > fieldType.range[1]) {
				isValidValue = false;
			}
		}

		// if the value passes muster, return it.  otherwise, throw.
		if (!isValidValue) {
			throw new Error("binary field `" + fieldType.id + "` is invalid");
		}
		return value;
	}
	
	function _readObject(stream, schema, parentObj)
	{
		var object = Object.create(parentObj);
		for (var i = 0; i < schema.length; ++i) {
			var fieldType = schema[i];
			var value = _readField(stream, fieldType, object);
			if ('id' in fieldType) {
				object[fieldType.id] = value;
			}
		}
		return object;
	}
})();
