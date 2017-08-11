/**
 *  Sphere Runtime for Sphere games
 *  Copyright (c) 2015-2017, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of miniSphere nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

'use strict';
exports = module.exports = Person;
exports.__esModule = true;
exports.default = exports;

var Console = require('console'),
    Thread  = require('thread');

function LegacyToNewColor(o) {
	return new Color(o.red / 255, o.green / 255, o.blue / 255, o.alpha / 255);
}

function NewToLegacyColor(n) {
    return CreateColor(n.r * 255, n.g * 255, n.b * 255, n.a * 255);
}

exports.GeneratePersonObjects = GeneratePersonObjects;
function GeneratePersonObjects(arr, ignore) {
	// if ignore, arr = list of people not to use
	var objects = [];
	var personlist = GetPersonList();
	if(ignore) {
		for(var person in personlist)
			if(arr.indexOf(person) == -1)
				objects.push(new Person(person, "", true, true));
	} else {
		for(var person in personlist)
			if(arr.indexOf(person) > -1)
				objects.push(new Person(person, "", true, true));
	}
	return objects;
}

exports.Person = Person;
function Person(name,spriteset,destroy_with_map, force) {
	if(DoesPersonExist(name)) {
		if(!force) throw "\"" + name + "\" already exists. Duplicate names are not allowed.";
		else Console.log("WARNING: \"" + name + "\" already exists, this might cause trouble");
	} else {
		CreatePerson(name, spriteset, destroy_with_map);
	}
	this.name = name;

	this.alive = true;
	this.threadID = Thread.create(this);
}

Person.prototype.killThread = function() {
	Thread.kill(this.threadID);
}

Person.prototype.clone = function(new_name,same_data,generating) {
	// if generating, this object is being created from a preexisting person
	var p;
	if(generating) p = this;
	else p = new Person(new_name, "../" + this.getSpriteset().filename, true);
	if(same_data || generating) {
		p.setData(this.getData());
		p.setPositionFloat(this.getPositionFloat());
		p.setOffsetX(this.getOffsetX());
		p.setOffsetY(this.getOffsetY());
		p.setLayer(this.getLayer());
		p.setAngle(this.getAngle());
		p.setDirection(this.getDirection());
		p.setFrame(this.getFrame());
		p.setFrameRevert(this.getFrameRevert());
		p.setVisibility(this.isVisible());
		p.setSpeed(this.getSpeed());
		p.setMask(this.getMask());
	}
	if(!generating) return p;
}

Person.prototype.customUpdate = function() {
	// this should be replaced
	return true;
}

Person.prototype.update = function() {
	if(!this.exists() && !this.alive) return false;
	return this.customUpdate();
}

Person.prototype.customRender = function() {
	// this should be replaced
	return true;
}

Person.prototype.render = function() {
	return this.customRender();
}

Person.prototype.destroy = function(killthread) {
	DestroyPerson(this.name);
	this.alive = false;
	if(killthread) this.killThread();
}

Person.prototype.getOffsetX = function() {
	return GetPersonOffsetX(this.name);
}

Person.prototype.setOffsetX = function(offset) {
	SetPersonOffsetX(this.name, offset);
}

Person.prototype.getOffsetY = function() {
	return GetPersonOffsetY(this.name);
}

Person.prototype.setOffsetY = function(offset) {
	SetPersonOffsetY(this.name, offset);
}

Person.prototype.getX = function() {
	return GetPersonX(this.name);
}

Person.prototype.setX = function(x) {
	SetPersonX(this.name, x);
}

Person.prototype.getY = function() {
	return GetPersonY(this.name);
}

Person.prototype.setY = function(y) {
	SetPersonY(this.name,y)
}

Person.prototype.getLayer = function() {
	return GetPersonLayer(this.name);
}

Person.prototype.setLayer = function(layer) {
	SetPersonLayer(this.name, layer);
}

Person.prototype.getXFloat = function() {
	return GetPersonXFloat(this.name);
}

Person.prototype.getYFloat = function() {
	return GetPersonYFloat(this.name);
}

Person.prototype.setXYFloat = function(x,y) {
	SetPersonXYFloat(this.name, x, y);
}

Person.prototype.getPosition = function() {
	return {x: this.getX(), y: this.getY()};
}

Person.prototype.getPositionFloat = function() {
	return {x: this.getXFloat(), y: this.getYFloat()};
}

Person.prototype.setPosition = function() {
	// can take x and y numbers or object with x/y properties
	if(arguments.length == 1) {
		SetPersonX(this.name, arguments[0].x);
		SetPersonY(this.name, arguments[0].y);
	} else {
		SetPersonX(this.name, arguments[0]);
		SetPersonY(this.name, arguments[1]);
	}
}

Person.prototype.setPositionFloat = function() {
	// can take x and y numbers or object with x/y properties
	if(arguments.length == 1) {
		SetPersonXYFloat(this.name, arguments[0].x, arguments[0].y);
	} else {
		SetPersonXYFloat(this.name, arguments[0], arguments[1]);
	}
}

Person.prototype.getDirection = function() {
	return GetPersonDirection(this.name);
}

Person.prototype.setDirection = function(dir) {
	SetPersonDirection(this.name, dir);
}

Person.prototype.getFrame = function() {
	return GetPersonFrame(this.name);
}

Person.prototype.setFrame = function(frame) {
	SetPersonFrame(this.name,frame);
}

Person.prototype.getSpeedX = function() {
	return GetPersonSpeedX(this.name);
}

Person.prototype.getSpeedY = function() {
	return GetPersonSpeedY(this.name);
}

Person.prototype.getSpeed = function() {
	return {
		x: this.getSpeedX(),
		y: this.getSpeedY()
	}
}

Person.prototype.setSpeed = function(speed) {
	SetPersonSpeed(this.name, speed)
}

Person.prototype.setSpeedXY = function() {
	if(arguments.length == 1)
		SetPersonSpeedXY(this.name, arguments[0].x, arguments[0].y);
	else SetPersonSpeedXY(this.name, arguments[0], arguments[1]);
}

Person.prototype.getFrameRevert = function() {
	return GetPersonFrameRevert(this.name);
}

Person.prototype.setFrameRevert = function(delay) {
	SetPersonFrameRevert(this.name, delay);
}

Person.prototype.setScaleFactor = function() {
	if(arguments.length == 1)
		SetPersonScaleFactor(this.name, arguments[0].w, arguments[0].h);
	else SetPersonScaleFactor(this.name, arguments[0], arguments[1]);
}

Person.prototype.setScaleAbsolute = function() {
	if(arguments.length == 1)
		SetPersonScaleAbsolute(this.name, arguments[0].w, arguments[0].h);
	else SetPersonScaleAbsolute(this.name, arguments[0], arguments[1]);
}

Person.prototype.getSpriteset = function() {
	return GetPersonSpriteset(this.name);
}

Person.prototype.setSpriteset = function(spriteset) {
	SetPersonSpriteset(this.name, spriteset);
}

Person.prototype.getBase = function(base) {
	return GetPersonBase(this.name);
}

Person.prototype.getAngle = function() {
	return GetPersonAngle(this.name)
}

Person.prototype.setAngle = function(angle) {
	SetPersonAngle(this.name, angle);
}

Person.prototype.getMask = function(minisphere) {
	// if minisphere is true, return miniSphere Color object
	var mask = GetPersonMask(this.name);
	if(minisphere) return LegacyToNewColor(mask);
	else return mask;
}

Person.prototype.setMask = function(color) {
	if(color instanceof Color)
		SetPersonMask(this.name, NewToLegacyColor(color));
	else SetPersonMask(this.name, color);
}

Person.prototype.isVisible = function() {
	return IsPersonVisible(this.name);
}

Person.prototype.setVisibility = function(visible) {
	SetPersonVisible(this.name, visible);
}

Person.prototype.toggleVisibility = function() {
	SetPersonVisible(this.name, !this.isVisible());
}

Person.prototype.getData = function() {
	return GetPersonData(this.name);
}

Person.prototype.setData = function(data) {
	SetPersonData(this.name, data)
}

Person.prototype.getValue = function(key) {
	return GetPersonValue(this.name, key);
}

Person.prototype.setValue = function(key,value) {
	SetPersonValue(this.name, key, value);
}

Person.prototype.followPerson = function(leader, pixels) {
	FollowPerson(this.name, leader, pixels);
}

Person.prototype.setScript = function(which, script) {
	SetPersonScript(this.name, which, script);
}

Person.prototype.callScript = function(which) {
	CallPersonScript(this.name, which);
}

Person.prototype.queueCommand = function(command, immediate) {
	QueuePersonCommand(this.name, command, immediate);
}

Person.prototype.queueScript = function(script,immediate) {
	QueuePersonScript(this.name, script, immediate);
}

Person.prototype.clearQueuedCommands = function() {
	ClearPersonCommands(this.name);
}

Person.prototype.clearCommands = Person.prototype.clearQueuedCommands;

Person.prototype.isCommandQueueEmpty = function() {
	return IsCommandQueueEmpty(this.name);
}


Person.prototype.isObstructed = function(x, y) {
	return IsPersonObstructed(this.name,x,y);
}

Person.prototype.getObstructingTile = function(x,y) {
	return GetObstructingTile(this.name, x, y)
}

Person.prototype.getObstructingPerson = function(x,y) {
	return GetObstructingPerson(this.name, x, y)
}


Person.prototype.getObstruction = function(x,y) {
	return {tile: this.getObstructingTile(x,y), person: this.getObstructingPerson(x,y)}
}

Person.prototype.getAdjacentObstructions = function(offset) {
	return {
		north: this.getObstruction(this.getX(),this.getY()-offset),
		south: this.getObstruction(this.getX(),this.getY()+offset),
		west: this.getObstruction(this.getX()-offset,this.getY()),
		east: this.getObstruction(this.getX()+offset,this.getY())
	};
}

Person.prototype.ignorePersonObstructions = function(ignore) {
	IgnorePersonObstructions(this.name,ignore);
}

Person.prototype.isIgnoringPersonObstructions = function() {
	return IsIgnoringPersonObstructions(thisname);
}

Person.prototype.ignoreTileObstructions = function(ignore) {
	IgnoreTileObstructions(this.name,ignore);
}

Person.prototype.isIgnoringTileObstructions = function() {
	return IsIgnoringTileObstructions(this.name);
}

Person.prototype.getIgnoreList = function() {
	return GetPersonIgnoreList(this.name);
}

Person.prototype.setIgnoreList = function(list) {
	SetPersonIgnoreList(this.name, list);
}

Person.prototype.addToIgnoreList = function(person) {
	this.setIgnoreList(this.getIgnoreList().push(person));
}

Person.prototype.removeFromIgnoreList = function(person) {
	var ignore_list = this.getIgnoreList();
	var index = ignore_list.indexOf(person);
	if(index == -1) throw "Error: person \"" + person + "\" does not exist (removeFromIgnoreList)";
	else {
		ignore_list.splice(index,1);
		this.setIgnoreList(ignore_list);
	}
}

Person.prototype.attachCamera = function() {
	AttachCamera(this.name);
}

Person.prototype.detachCamera = function() {
	DetachCamera(this.name);
}

Person.prototype.isCameraAttached = function() {
	return GetCameraPerson() == this.name;
}

Person.prototype.attachInput = function() {
	AttachInput(this.name);
}

Person.prototype.detachInput = function() {
	DetachInput(this.name);
}

Person.prototype.isInputAttached = function() {
	return GetInputPerson() == this.name;
}

Person.prototype.setMoveByTiles = function(yesno) {
	this.setValue("moveByTiles", true)
}

Person.prototype.getMoveByTiles = function() {
	return this.getValue("moveByTiles");
}

Person.prototype.exists = function() {
	return DoesPersonExist(this.name);
}
