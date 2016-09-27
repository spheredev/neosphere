/**
 *  miniRT struct CommonJS module
 *  (c) 2016 Fat Cerberus
**/

'use strict';
module.exports =
{
	load: load,
	read: read,
	Enum: Enum
};

const assert = require('assert');
const link   = require('link');

function load(filename, schema)
{
	if (!Array.isArray(schema))
		throw new TypeError("expected an Array object for `schema`");

	var stream = fs.open(filename, 'rb');
	var object = read(stream, schema);
	stream.close();
	return object;
}

function read(stream, schema)
{
	if (!Array.isArray(schema))
		throw new TypeError("expected an Array object for `schema`");

	return _readObject(stream, schema, null);
}

function Enum(memberNames)
{
	if (!(this instanceof Enum))
		return new Enum(memberNames);

	this.max = memberNames.length;
	for (var i = 0; i < memberNames.length; ++i) {
		this[memberNames[i]] = i;
		this[i] = memberNames[i];
	}
}

function _fixup(value, data)
{
	// this fixes up backreferences in the schema, for example '@numPigs'.
	// because files are loaded sequentially, references can only be to
	// earlier fields.

	if (typeof value === 'function')
		return value.call(data);
	if (typeof value === 'string' && value[0] == '@')
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
			var view = new DataView(stream.read(1));
			value = view.getUint8(0) != 0;
			break;
		case 'double':
			var view = new DataView(stream.read(8));
			value = view.getFloat64(0);
			break;
		case 'doubleLE':
			var view = new DataView(stream.read(8));
			value = view.getFloat64(0, true);
			break;
		case 'float':
			var view = new DataView(stream.read(4));
			value = view.getFloat32(0);
			break;
		case 'floatLE':
			var view = new DataView(stream.read(4));
			value = view.getFloat32(0, true);
			break;
		case 'fstring':
			var bytes = stream.read(dataSize);
			value = new TextDecoder('utf-8').decode(bytes);
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
			value = _readInt(stream, dataSize, true, false);
			break;
		case 'intLE':
			value = _readInt(stream, dataSize, true, true);
			break;
		case 'object':
			value = _readObject(stream, fieldType.schema, data);
			break;
		case 'pstring':
			var length = _readInt(stream, dataSize, false, false);
			var bytes = stream.read(length);
			value = new TextDecoder('utf-8').decode(bytes);
			break;
		case 'pstringLE':
			var length = _readInt(stream, dataSize, false, true);
			var bytes = stream.read(length);
			value = new TextDecoder('utf-8').decode(bytes);
			break;
		case 'raw':
			value = stream.read(dataSize);
			break;
		case 'reserved':
			stream.read(dataSize);
			break;
		case 'uint':
			value = _readInt(stream, dataSize, false, false);
			break;
		case 'uintLE':
			value = _readInt(stream, dataSize, false, true);
			break;
		default:
			throw new TypeError("unknown field type `" + fieldType.type + "` in schema");
	}

	// verify the value we got against the schema
	var isValidValue = true;
	if (typeof fieldType.regex === 'string' && typeof value === 'string') {
		var regex = new RegExp(fieldType.regex);
		if (!regex.test(value))
			isValidValue = false;
	}
	else if (Array.isArray(fieldType.values)) {
		if (!link(fieldType.values).contains(value))
			isValidValue = false;
	}
	else if (Array.isArray(fieldType.range) && fieldType.range.length == 2
		&& typeof fieldType.range[0] === typeof value
		&& typeof fieldType.range[1] === typeof value)
	{
		if (value < fieldType.range[0] || value > fieldType.range[1])
			isValidValue = false;
	}

	// if the value passes muster, return it.  otherwise, throw.
	if (!isValidValue) {
		throw new Error("binary field `" + fieldType.id + "` is invalid");
	}
	return value;
}

function _readInt(stream, size, signed, littleEndian)
{
	// variable size two's complement integer decoding algorithm stolen from
	// Node.js.  this allows us to read integer values up to 48 bits in size.
	
	assert.ok(size >= 1 && size <= 6, "(u)int field size out of range");

	var bytes = new Uint8Array(stream.read(size));
	var mul = 1;
	var value;
	if (littleEndian) {
		var ptr = 0;
		value = bytes[ptr];
		while (++ptr < size && (mul *= 0x100))
			value += bytes[ptr] * mul;
	}
	else {
		var ptr = size - 1;
		value = bytes[ptr];
		while (ptr > 0 && (mul *= 0x100))
			value += bytes[--ptr] * mul;
	}
	if (signed && value > mul * 0x80)
		value -= Math.pow(2, 8 * size);
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
