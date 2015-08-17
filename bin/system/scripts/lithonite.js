//___________________________________
//                                   \
// Lithonite Engine.  version 2.0	$Id: lithonite.js 63 2007-10-12 23:53:39Z nilton $
//___________________________________/

/** 
 * @fileoverview Lithonite Engine.
 * An anti-obstruction walking algorithm for the Sphere Game development engine.
 * It is part of the {@link http://members.chello.nl/n.castillox/chibitutorials/ ChibiTutorials}
 *
 * @author Nilton Castillo ncastill@mixmail.com
 * @version 2.0
 */

/* Changelog:
	Added push variable
	Fixed historical vector error.
	faster getCommand() and getFaceDir()
	Fixed Diagonal Deobstruction.
	New check-facing algorithm. Merged 1.3n and 1.4 into 1.4n
	Added zone()
	Changed some of the variables so Lithonite works during cutscenes.
	Lithonite now works better when Running.
	removed getFaceDir in favour of getDir('face_',
	moved SwapDirections from phenibut into Lithonite.
	Fixed Sphere touch bug for pushing.
	Implemented zone('s'): Snow Slippery walk
	Moved zone functions to ZONES with long names. i.e:zone('slip').
	Fixed typo in LoadDirections().
	Fixed bug when pushing a Person.
	unix2dos. Added inline help (makes it slower, use 1.9f if speed is critical)
	Added this.is, so we can repos back if we've been deObstructed unnecessarily.
	Added .touched and .touching, .IsTouching() and .reposBack()
	Fixed .slip added .o
	Fixed IsTouching()/GetObstructingPerson()/.touching FPS-speedup (need to optimize subrouting)
	Added 1.9c branch (move_x changes correctly when deobstructing to another direction)
	Moved facing-towards-person functions from ainimation towards lithonite.
	Added comments, 
	Added all cardinals for face_ SwapDirections()
	Renamed slope into enZone and added new zones: Mirror and Bridge
*/

//----------------------------------------------------------------------------//

SetPersonScript_ = SetPersonScript;
/**
 * Redefine SetPersonScript, so we have also a GetPersonScript!
 * Unfortunately, you cant use GetPersonScript if you define the personscript directly in the map editor.
 * @param {string} person A name of a sprite
 * @param {event} what One of these events: SCRIPT_ON_DESTROY SCRIPT_ON_ACTIVATE_TOUCH SCRIPT_ON_ACTIVATE_TALK SCRIPT_COMMAND_GENERATOR
 * @param {string} script The script you want to set
 */
SetPersonScript = function(person,what,script){
	SetPersonValue(person,what,script);
	SetPersonScript_(person,what,script);
}

/**
 * Gets a person script, if set with the redefined SetPersonScript()
 * @param {string} person A name of a sprite
 * @param {event} what One of these events: SCRIPT_ON_DESTROY SCRIPT_ON_ACTIVATE_TOUCH SCRIPT_ON_ACTIVATE_TALK SCRIPT_COMMAND_GENERATOR
 */
GetPersonScript = function(person,what){
	return GetPersonValue(person,what);
}

/**
 * Internal Function. Sets a personscript without storing it, so when you call UnshiftPersonScript() 
 * the originalpersonscript (defined with SetPersonScript() ) is restored.
 * @param {string} person A name of a sprite
 * @param {event} what One of these events: SCRIPT_ON_DESTROY SCRIPT_ON_ACTIVATE_TOUCH SCRIPT_ON_ACTIVATE_TALK SCRIPT_COMMAND_GENERATOR
 * @param {string} script The script you want to set
 */
ShiftPersonScript = function(person,what,script){
	SetPersonScript_(person,what,script);
}

/**
 * Internal Function. Restores a personscript.
 * the originalpersonscript (defined with SetPersonScript() ) is restored.
 * @param {string} person A name of a sprite
 * @param {event} what One of these events: SCRIPT_ON_DESTROY SCRIPT_ON_ACTIVATE_TOUCH SCRIPT_ON_ACTIVATE_TALK SCRIPT_COMMAND_GENERATOR
 */
UnshiftPersonScript = function(person,what){
	SetPersonScript_(person,what,GetPersonValue(person,what));
}
 

/**
 * Create a new Lithonite object
 * This will create a Lithonite object:
 *		var Lithonite= new LithoniteEngine("Lithonite");
 * @constructor
 *
 * @param {string} ObjectName Mandatory must be the same as the created variable.
 * @param {string} InputPersonName optional Input Person (can also be set with Lithonite.setInputPerson() )
 *
 * @return A new LithoniteEngine Object
 */
function LithoniteEngine(objname,GIP)
{
	if(this instanceof LithoniteEngine== false) {
		return new LithoniteEngine(objname,GIP);
	}

	this.objname=objname;
	this.active=true;
	this.sX=1;
	this.sY=1;
	this.speed_normalX=1;
	this.speed_normalY=1;
	this.speed_dashX=1.75;
	this.speed_dashY=1.75;
	this.speed_crawlX=.6;
	this.speed_crawlY=.6;
	this.isDashing=false;
	this.isCrawling=false;
	this.enspeed=false; //If speed is not 1
	this.isZoning=false;
	this.setDashDir= function(){};
	this.setCrawlDir= function(){};
	this.setNormalDir= function(){};
	this.moving=0;	//0: standing 1: walking 2: Cinematics 3: whatever
	this.GIP=(typeof GIP=="string")?GIP:"";
	this.camera=this.GIP;
	this.idle=0; // Idle counter (not moving)
	this.pushing=0; //if you are pushing
	this.LMaxLookupX=8; //If you are 8 pixels off in the X-axis, it will reposition you to be not obstructed.
	this.LMaxLookupY=8; //If you are 8 pixels off in the Y-axis, it will reposition you to be not obstructed.
	this.move_x=0; this.move_y=0; //#Primary Lithonite vectors (integer -1,0 or 1)
	this.hist_x=0; this.hist_y=0; //#Historical Lithonite vectors (integer -1,0 or 1)
	this.old_x=0; this.old_y=0;   //#Zone auxiliary vectors
	this.i_x=0;this.i_y=0; //#Inertia: Zone's icewalk/slippery speed vectors (float)
	this.i_x_max=1;this.i_y_max=1; //#Inertia: Zone's icewalk/slippery maximum speed
	this.ix=0;this.iy=0; //#Inertia Lithonite Vectors (integer -1,0 or 1)
	this.iif=0.01;	//Inertia Factor, at which speed increases
	this.isf=1.07;	//Slip Factor, at which speed decreases
	this.dir=0;	//Zone auxiliary direction = (this.hist_x+(this.hist_y<<2))
	this.counter=0; // General Purpose action counter
	this.is = 0; //Tells us if we just used lithonite to deobstruct, so we can move back if we touched an item that dissapears
	this.o = 0; //Tells us if we are obstructed (by a Tile or a Person)
	this.touched=""; //Last person who has been touched
	this.touching=""; //Holds for 1 cycle the name of a newly touched person.
	this.DIRS=new Object(); //Holds saved Directions of spritesets
	this.RSS=new Object(); //Holds saved spritesets
	this.face = "face_"; //Because the SCG uses 'look_'...

	// zone variables for brige, bridgewave and mirror
	this.bridge = {
		offset: { x:0, y:0 },
		frame: 0,
		dir:0,
		color: CreateColor(215,215,255,128),
		layer: 0
	};

}

//----------------------------------------------------------------------------//

/**
 * Sets the person on which all Lithonite effects will be applied by default.
 * If you are not going to change your hero character, then this only needs to be called once.
 * @param {string} person If left empty, will default to GetInputPerson()
 * To see what's the name of the input person, check the variable Lithonite.GIP
 */
LithoniteEngine.prototype.setInputPerson = function(person) {
	return this.GIP=person||(IsInputAttached()?GetInputPerson():this.GIP);
}

/**
 * Sets the person on which the camera will be attached by default
 * If you are not going to change your camera, then this only needs to be called once.
 * @param {string} person If left empty, will default to GetCameraPerson()
 * To see what's the name of the camera person, check the variable Lithonite.camera
 */
LithoniteEngine.prototype.setCameraPerson = function(person) {
	this.camera=person||(IsCameraAttached()?GetCameraPerson():this.camera);
}

/**
 * Sets KEY that will be used to as talk activation key
 * @param {KEY} TalkActivationKey A key. If left empty, will default to GetTalkActivationKey();
 * To see what's the talk key, check the variable Lithonite.talkKey
 */
LithoniteEngine.prototype.setTalkActivationKey = function(TalkActivationKey) {
	this.talkKey = TalkActivationKey || GetTalkActivationKey() || this.talkKey;
}

//----------------------------------------------------------------------------//
/**
 * Define the speeds for dashing, crawling and normal walk.
 * 
 * @param {number} speed_normalX By default 1
 * @param {number} speed_normalY By default 1
 * @param {number} speed_dashX By default 1.75
 * @param {number} speed_dashY By default 1.75
 * @param {number} speed_crawlX By default 0.6
 * @param {number} speed_crawlY By default 0.6
 * @param {number} i_x_max By default 1
 * @param {number} i_y_max By default 1
 *
 *	To also make the character move differently (by, for example, swapping directions)
 *	You need to define the following lithonite functions:
 *	setDashDir()
 *	setCrawlDir()
 *	setNormalDir()

	for example:
 *	Lithonite.setDashDir= function(){this.SwapDirections('dash_');}
 *	Lithonite.setCrawlDir= function(){this.SwapDirections('tiptoe_');}
 *	Lithonite.setNormalDir= function(){this.LoadDirections();}
	Note that you need to use SaveDirections() once, at the beginning.
	You can also swap the entire spriteset if you like. This may be necessary if you need to change the spritebase.
 *
 */
LithoniteEngine.prototype.setSpeeds= function(speed_normalX,speed_normalY, speed_dashX,speed_dashY, speed_crawlX,speed_crawlY, i_x_max,i_y_max){
	if(speed_normalX)this.speed_normalX=speed_normalX;
	if(speed_normalY)this.speed_normalY=speed_normalY;
	if(speed_dashX)this.speed_dashX=speed_dashX;
	if(speed_dashY)this.speed_dashY=speed_dashY;
	if(speed_crawlX)this.speed_crawlX=speed_crawlX;
	if(speed_crawlY)this.speed_crawlY=speed_crawlY;
	if(i_x_max)this.i_x_max=i_x_max;
	if(i_y_max)this.i_y_max=i_y_max;
	this.checkSpeedOne();
}

/**
 * Internal function to see if the speed of your sprite is one.
 * @returns True if the current speed is not equal to 1.
 * @type Boolean
 */
LithoniteEngine.prototype.checkSpeedOne= function() {
	return this.enspeed = this.sX!=1 || this.sY!=1;
}

/**
 * Will check if you are pressing a certain key, and will speed up your sprite
 * @param {key} KEY See docs/keys.txt for a set of keycodes you can use in Sphere
 * See {@link LithoniteEngine#setSpeeds} on how to define the speeds and change your sprite
 * @returns true if dashing, false if not
 * @type Boolean
 */
LithoniteEngine.prototype.checkDashing = function(KEY) {
	if(this.moving!=1)return;
	if(IsKeyPressed(KEY)) {
		if(!this.isDashing) {
			this.isDashing=true;
			this.isCrawling=false;
			SetPersonSpeedXY(this.GIP,this.sX=this.speed_dashX,this.sY=this.speed_dashY);
			this.setDashDir();
			this.checkSpeedOne();
		}
		return true;
	}else if(this.isDashing) {
		this.isDashing=false;
		SetPersonSpeedXY(this.GIP,this.sX=this.speed_normalX,this.sY=this.speed_normalY); 
		this.setNormalDir();
		this.checkSpeedOne();
	}
	return false;
}

/**
 * Will check if you are pressing a certain key, and will slow down your person 
 * @param {key} KEY See docs/keys.txt for a set of keycodes you can use in Sphere
 * @returns true if crawling, false if not
 * @type Boolean
 */
LithoniteEngine.prototype.checkCrawling = function(KEY) {
	if(this.moving!=1)return;
	if(IsKeyPressed(KEY)) {
		if(!this.isCrawling) {
			this.isDashing=false;
			this.isCrawling=true;
			SetPersonSpeedXY(this.GIP,this.sX=this.speed_crawlX,this.sY=this.speed_crawlY);
			this.setCrawlDir();
			this.checkSpeedOne();
		} 
		return true;
	} else if(this.isCrawling) {
		this.isCrawling=false;
		SetPersonSpeedXY(this.GIP,this.sX=this.speed_normalX,this.sY=this.speed_normalY); 
		this.setNormalDir();
		this.checkSpeedOne();
	}
	return false;
}

/**
 * When you defined your sprite, or after you have played a cutscene, or did something to your sprite, reset the speeds. 
 * It replaces SetPersonSpeedXY() for your sprite.
 */
LithoniteEngine.prototype.resetSpeed = function() {
	if(this.isDashing){
		SetPersonSpeedXY(this.GIP,this.sX=this.speed_dashX,this.sY=this.speed_dashY);
		this.setDashDir();
	} else if(this.isCrawling) {
		SetPersonSpeedXY(this.GIP,this.sX=this.speed_crawlX,this.sY=this.speed_crawlY);
		this.setCrawlDir();
	} else {
		SetPersonSpeedXY(this.GIP,this.sX=this.speed_normalX,this.sY=this.speed_normalY); 
		this.setNormalDir();
	}
	this.checkSpeedOne();
}

//----------------------------------------------------------------------------//

/**
 * Calculates the movement vectors, based on the cursor keys you are pressing. 
 * You need to call this each frame inside the Update script
 * @returns 0 if there is no cursor movent (pressing up and down at the same time results in no movement), 1 if there is. 
 * After calcVectors, you need to call {@link LithoniteEngine#deObstruct} 
 * @type integer
 */
LithoniteEngine.prototype.calcVectors = function() {
	if(this.moving==2) return 0;
	this.move_x=this.move_y=0;
	if(IsKeyPressed(KEY_UP)   ) {this.moving=1; --this.move_y;}
	if(IsKeyPressed(KEY_RIGHT)) {this.moving=1; ++this.move_x;}
	if(IsKeyPressed(KEY_DOWN) ) {this.moving=1; ++this.move_y;}
	if(IsKeyPressed(KEY_LEFT) ) {this.moving=1; --this.move_x;}
	if(!this.moving){++this.idle;this.pushing=0;return 0;};
	this.idle=0;
	if(this.move_x||this.move_y) {this.hist_x=this.move_x;this.hist_y=this.move_y; return 1;}
	return 0;
}

//----------------------------------------------------------------------------//

/**
 * Will tell you if you just stopped using the cursor keys
 * @returns true if the sprite just stopped moving
 * @type Boolean
 */
LithoniteEngine.prototype.justStopped = function() {
	return(this.moving&&!this.move_x&&!this.move_y);
}

//----------------------------------------------------------------------------//

/**
 * Will tell you if you just started touching another person.
 * @returns 1 if the sprite started touching another person, 0 if not.
 * You can get the the name of the touched person by reading this.touched
 * Will tell you if you are touching another person. This value is set by deObstruct().
 * Once you keep pushing the same person, this function will return false, however, the
 * counter this.pushing will keep increasing.
 * Our responsability to reset Lithonite.touched if we want to be able to retrigger-touch the same Person again.
 * This function is used for bullets and such, it will be true only one frame, so it will 'hit' us only once, even if the spriteset is huge.
 * If after touching the other person, it isnt destroyed, think maybe about a timer to reset .touched="", so we can touch the Person again.
 * @type number
 */
LithoniteEngine.prototype.IsTouching = function() {
	if(this.moving && this.touching && this.pushing==1)return 1;
	return 0;
}

//----------------------------------------------------------------------------//
/**
 * This function helps your sprite de-obstruct itself.
 * You need to call this each frame inside the Update script (or at least, after calling {@link LithoniteEngine#calcVectors})
 * if(Lithonite.calcVectors())Lithonite.deObstruct();
 * @returns 0 if not deobstructed, 1 or 2 if we deobstructed.
 */
LithoniteEngine.prototype.deObstruct = function(Person) {
	if(!this.moving||!this.active)return this.is=this.o=this.pushing=0; 
	Person=Person||this.GIP; 
	var $GPXX=GetPersonX(Person)+this.move_x;
	var $GPYY=GetPersonY(Person)+this.move_y;
	if(this.enspeed){$GPXX+=this.move_x*this.sX; $GPYY+=this.move_y*this.sY;}
	if(IsPersonObstructed(Person,$GPXX,$GPYY)) {
		var $GPX = GetPersonX(Person);
		var $GPY = GetPersonY(Person);
		this.o = 1;
		this.touched=this.touching;
		this.touching=GetObstructingPerson(Person, $GPXX, $GPYY);
		if(this.touching){
			++this.pushing;
			if(this.repush && this.pushing==this.repush){
				this.repush=0;
				this.touched=0;
			}
			if(this.touching!=this.touched){
				CallPersonScript(this.touching, SCRIPT_ON_ACTIVATE_TOUCH);
				this.pushing=0;
			}
		}else
			this.pushing=0;

		//Do nothing if we pressed 2 keys and it can go in one of those directions
		if(this.move_x && this.move_y && ( !IsPersonObstructed(Person,$GPXX,$GPY)  || !IsPersonObstructed(Person,$GPX,$GPYY))) 
			return this.is=0;
		else { //#1keypressed

			if(this.enspeed) {
				SetPersonSpeedXY(this.GIP,this.sX=this.speed_normalX,this.sX=this.speed_normalY);
				this.enspeed=0;
				SetDelayScript(1,this.objname+".rectifySpeed()");
			}
			var $dmL=0;
			var $dmR=0;
			var $dHI=0;
			var $dVI=0;
			var $i=this.move_x?this.LMaxLookupX-1:this.LMaxLookupY-1;
			do{
				$dHI=this.move_y*$i;
				$dVI=this.move_x*$i;
				$dmL=!IsPersonObstructed(Person,$GPXX+$dHI,$GPYY-$dVI);
				$dmR=!IsPersonObstructed(Person,$GPXX-$dHI,$GPYY+$dVI);
				if($dmL!=$dmR)$i=0;
			}while($i--);
			if($dmL && ($dmL>=$dmR)){
				SetPersonX(Person,$GPX+this.move_y);
				SetPersonY(Person,$GPY-this.move_x);
				return this.is=1;
			}
			else
				if($dmR){
					SetPersonX(Person,$GPX-this.move_y);
					SetPersonY(Person,$GPY+this.move_x);
					return this.is=2;
				}else{
					SetPersonX(Person,$GPX);
					SetPersonY(Person,$GPY);
					return this.is=0;
				}	
		}//Else 1keypressed
	}//End_if:IsPersonObstructed
	this.o = this.pushing= 0;
	return this.is=0;
}

//----------------------------------------------------------------------------//

/**
 * Makes the sprite stand still. By default, it will use {@link LithoniteEngine#setDir} to face a direction,
 * note that this direction is a substring of a real direction in the spriteset.
 * so if your spriteset has a face_north, face_south etc, it will set that direction by default
 * Or it will just revert to frame 0. This replaced SetPersonFrameRevert(), when it
 * still didnt exist. If you overload this function, dont forget to set this.moving=0;
 * justStopped() will then be false again. And to skip if inside a cutscene/cinematics (this.moving==2)
 * @param {string} ds Directional substring. Will default to Lithonite.face
 */
LithoniteEngine.prototype.standStill = function(ds,person) { 
	if(this.moving==2) return;
	this.moving = 0;
	this.setDir(this.getDir(ds||this.face,this.hist_x,this.hist_y),person);
}

/**
 * Set the direction of a person, without aborting Sphere if that direction does not exist.
 * @param {string} dir Direction you want to set
 * @param {string} person Name of the sprite you want to set the direction for. Defaults to this.GIP
 * @returns true if we could set the direction, false if not
 * @type Boolean
 */
LithoniteEngine.prototype.setDir = function(dir,person) {
	person=person||this.GIP;
	SetPersonFrame(person,0); //Just return to frame 0
	//Now determine if we have <dir>_<direction>, save it in the persons object if not.
	var data = GetPersonValue(person,dir);
	if(typeof data != 'boolean') {
		data = false;
		var ssd=GetPersonSpriteset(person).directions;
		var i=ssd.length-1;
		do{if (ssd[i].name==dir) data = true; }while(i--);
		SetPersonValue(person,dir,data);
	}
	if(data){
		SetPersonDirection(person, dir);
		return true;
	}
	return false;
}

/**
 * unSets the lookups directions of a person used in {@link setDir}
 * Must be called before using SetPersonSpriteset() on the GIP.
 * @param {string} person Name of the sprite you want to set the direction for. Defaults to this.GIP
 */
LithoniteEngine.prototype.clearDirs = function(person) {
	person = person||this.GIP;
	var ssd = GetPersonSpriteset(person).directions;
	var i = ssd.length-1;
	do{ SetPersonValue(person, ssd[i].name, false); }while(i--);
}
/**
 * Determine a direction if we have (x,y) vectors
 * @param {int} x This value may be -1, 0 or 1
 * @param {int} y This value may be -1, 0 or 1
 * @returns a cardinal direction
 * @type String
 */
LithoniteEngine.prototype.getMoveDir = function(x,y) {
	switch(x+(y<<2)) {
		case -5: return 'northwest'; break;
		case -4: return 'north'; break;
		case -3: return 'northeast'; break;
		case -1: return 'west'; break;
		case 1: return 'east'; break;
		case 3: return 'southwest'; break;
		case 4: return 'south'; break;
		case 5: return 'southeast'; break;
	}
	return 'south';
}

/**
 * Like {@link LithoniteEngine#getMoveDir} but with a substring prepended
 * @param {string} ds Directional substring
 * @param {int} x This value may be -1, 0 or 1
 * @param {int} y This value may be -1, 0 or 1
 * @returns a direction you can use with SetPersonDirection()
 * @type String
 */
LithoniteEngine.prototype.getDir = function(ds,x,y) {
	return ds+this.getMoveDir(x,y);
}

/**
 * Swaps the normal spriteset directions with one constructed with a directional substring
 * So if you have directions 'north' and 'handup_north' in your spriteset, use 
 * SwapDirections('handsup_',person) to swap them.
 * @param {string} sd Directional substring
 * @param {string} person Person we want to swap its directions
 * @param {boolean} swapface Do also swap "face_" directions (false by default). Set this string with Lithonite.face
 */
LithoniteEngine.prototype.SwapDirections = function(sd,person,swapface) {
	person=person||this.GIP;
	var GPSS= GetPersonSpriteset(person);
	var GPSSd=GPSS.directions;
	var i=GPSSd.length-1;
	do{
		switch(GPSSd[i].name) {
			case "north":		GPSSd[i].name=sd+"north"; break;
			case sd+"north":	GPSSd[i].name="north"; break;
			case "south":		GPSSd[i].name=sd+"south"; break;
			case sd+"south":	GPSSd[i].name="south"; break;
			case "east":		GPSSd[i].name=sd+"east"; break;
			case sd+"east":		GPSSd[i].name="east"; break;
			case "west":		GPSSd[i].name=sd+"west"; break;
			case sd+"west":		GPSSd[i].name="west"; break;
			case "northwest":	GPSSd[i].name=sd+"northwest"; break;
			case sd+"northwest":	GPSSd[i].name="northwest"; break;
			case "northeast":	GPSSd[i].name=sd+"northeast"; break;
			case sd+"northeast":	GPSSd[i].name="northeast"; break;
			case "southwest":	GPSSd[i].name=sd+"southwest"; break;
			case sd+"southwest":	GPSSd[i].name="southwest"; break;
			case "southeast":	GPSSd[i].name=sd+"southeast"; break;
			case sd+"southeast":	GPSSd[i].name="southeast"; break;
		};
		if(swapface)
		switch(GPSSd[i].name) {
			case this.face+"north":	GPSSd[i].name=sd+this.face+"north"; break;
			case sd+this.face+"north":	GPSSd[i].name=this.face+"north"; break;
			case this.face+"south":	GPSSd[i].name=sd+this.face+"south"; break;
			case sd+this.face+"south":	GPSSd[i].name=this.face+"south"; break;
			case this.face+"east":	GPSSd[i].name=sd+this.face+"east"; break;
			case sd+this.face+"east":	GPSSd[i].name=this.face+"east"; break;
			case this.face+"west":	GPSSd[i].name=sd+this.face+"west"; break;
			case sd+this.face+"west":	GPSSd[i].name=this.face+"west"; break;
			case this.face+"northwest":	 GPSSd[i].name=sd+this.face+"northwest"; break;
			case sd+this.face+"northwest":GPSSd[i].name=this.face+"northwest"; break;
			case this.face+"northeast":	 GPSSd[i].name=sd+this.face+"northeast"; break;
			case sd+this.face+"northeast":GPSSd[i].name=this.face+"northeast"; break;
			case this.face+"southwest":	 GPSSd[i].name=sd+this.face+"southwest"; break;
			case sd+this.face+"southwest":GPSSd[i].name=this.face+"southwest"; break;
			case this.face+"southeast":	 GPSSd[i].name=sd+this.face+"southeast"; break;
			case sd+this.face+"southeast":GPSSd[i].name=this.face+"southeast"; break;
		};
	}while(i--);
	//var d=GetPersonData(Person); //Still a bug in map_engine.cpp CMapEngine::SetPersonSpriteset()
	SetPersonSpriteset(person,GPSS);
	//SetPersonData(Person,d);
}

//----------------------------------------------------------------------------//

/**
 * clears the DIRS object, which holds spriteset directions added by {@link LithoniteEngine#SaveDirections}.
 */
LithoniteEngine.prototype.ClearDirections = function(){
	this.DIRS = new Object();
}

/**
 * Saves the spriteset directions of a person, so you can modify it without worries, then restore it.
 * @param {string} person The name of the person you want to store the current directions of its spriteset.
 * @param {string} dname Optional. The name you want to save the current directions under.
 * @param {Boolean} rssToo. Also store the full spriteset. Defaults to false.
 * @returns true if it stored the directions (always true)
 * Example:
 * We have a spriteset that has north, zombie_north and run_north, we save the directions:
 * Lithonite.SaveDirections('hero'); //Save original directions 'north' is the real north
 * Lithonite.SwapDirections('zombie_'); // Player now has 'zombie_north' as direction 'north'
 * Lithonite.SaveDirections('hero','hero_zombie'); //Save zombie directions for hero
 * Lithonite.SwapDirections('run_'); // Player now has 'north' as direction 'run_north'
 * Lithonite.SaveDirections('hero','hero_run'); //Save run directions.
 * //You can now use LoadDirections to easily swap between the 3 directions.
 * another example: Lithonite.SaveDirections('hero', GetPersonSpriteset('hero').filename, true);
 * @type Boolean
 */
LithoniteEngine.prototype.SaveDirections = function(person,dname,rssToo) {
	person=person||this.GIP;
	dname=dname||person;
	var GPSSd= GetPersonSpriteset(person).directions;
	var i=GPSSd.length-1;
	this.DIRS[dname]=new Array(i);
	do{
		this.DIRS[dname][i]=GPSSd[i].name;
	}while(i--);
	if(rssToo)
		this.RSS[dname] = GetPersonSpriteset(person);
	return true;
}

/**
 * Restores the spriteset directions of a person. The directions have been stored first with {@link LithoniteEngine#SaveDirections}
 * @param {string} person The name of the person you want to restore the current directions of its spriteset.
 * @param {string} dname Optional. The name you want to restore the directions from.
 * @param {Boolean} rssToo. Also restore the full spriteset. Defaults to false.
 * @returns true if it restored the directions, false if the spriteset does not match the stored version.
 * @type Boolean
 */
LithoniteEngine.prototype.LoadDirections = function(person,dname,rssToo) {
	person=person||this.GIP;
	dname=dname||person;
	if(rssToo){
		this.clearDirs();
		SetPersonSpriteset(person, this.RSS[(typeof rssToo == 'string'? rssToo:dname)]);
	};
	var GPSS= GetPersonSpriteset(person);
	var GPSSd=GPSS.directions;
	if(!this.DIRS[dname] || GPSSd.length != this.DIRS[dname].length)return false;
	var i=GPSSd.length-1;
	do{
		GPSSd[i].name=this.DIRS[dname][i];
	}while(i--);
	SetPersonSpriteset(person,GPSS);
	return true;
}

//----------------------------------------------------------------------------//

/**
 * Determine a command if we have (x,y) unitarian vectors
 * Note that we dont have COMMAND_MOVE_SOUTHEAST, in those cases it returns east or west.
 * @param {Boolean} move True or 1 for COMMAND_MOVE_*, false or 0 for COMMAND_FACE_*
 * @param {int} x This value may be -1, 0 or 1
 * @param {int} y This value may be -1, 0 or 1
 * @returns a command 
 * @type Integer 
 */
LithoniteEngine.prototype.getCommand = function(move,x,y) {
	if(move) {
		switch(x+(y<<2)) {
			case -4: return COMMAND_MOVE_NORTH;
			case 4: return COMMAND_MOVE_SOUTH;
			case -3: case 5: case 1: return COMMAND_MOVE_EAST;
			case 3: case -5: case -1: return COMMAND_MOVE_WEST;
		}
	}
	switch(x+(y<<2)) {
		case -5: return COMMAND_FACE_NORTHWEST;
		case -4: return COMMAND_FACE_NORTH;
		case -3: return COMMAND_FACE_NORTHEAST;
		case -1: return COMMAND_FACE_WEST;
		case 1: return COMMAND_FACE_EAST;
		case 3: return COMMAND_FACE_SOUTHWEST;
		case 4: case 0:return COMMAND_FACE_SOUTH;
		case 5: return COMMAND_FACE_SOUTHEAST;
	}
	return COMMAND_WAIT;
}

//----------------------------------------------------------------------------//
/**
 * Undoes what {@link LithoniteEngine#facePerson} did
 * @param {string} me a name of a person, preferably the input person.
 * @param {string} npc another name of a person
 * @param {Boolean} NotRestoreSCG Set only if the SCRIPT_COMMAND_GENERATOR doesnt have to be restored
 * @returns 1 if it could face back to the original direction, 0 if not
 * @type Integer 
 */
LithoniteEngine.prototype.unfacePerson= function(me,npc,NotRestoreSCG) {
        me=me||this.GIP;
	if(me == this.GIP){
		this.moving=0;
		this.idle=0;
	}else{
		if(GetPersonValue(me,'facing')){
			SetPersonValue(me,'facing',false);
			SetPersonDirection(me, GetPersonValue(me,'OldPersonDirection'));
			SetPersonFrame(me, GetPersonValue(me,'OldPersonFrame'));
        		if(!NotRestoreSCG) UnshiftPersonScript(me, SCRIPT_COMMAND_GENERATOR);
		}
	}
	if(!npc)return 0;
        if(!GetPersonValue(npc,'facing')) return 0;
	SetPersonValue(npc,'facing',false);

	ClearPersonCommands(npc);
        SetPersonDirection(npc, GetPersonValue(npc,'OldPersonDirection'));
//	QueuePersonCommand(npc, GetPersonValue(npc,'OldPersonDirection'), true);
        SetPersonFrame(npc, GetPersonValue(npc,'OldPersonFrame'));
        if(!NotRestoreSCG) UnshiftPersonScript(npc, SCRIPT_COMMAND_GENERATOR);
	return 1;
}

/**
 * Makes two persons face eachother (and store their original direction, so it can be restored)
 * @param {string} me a name of a person, preferably the input person.
 * @param {string} npc another name of a person
 * @param {Boolean/string} IfaceOther True if 'me' faces the person, usually false, because to talk to someone, we already are facing towards that person
 *                         If it is a string, it will use that string to find a direction. Try 'face_' for directions face_north, face_east, etc
 * @param {Boolean/string} OtherFacesMe Usually True
 * @param {Boolean} FourWays True if we face 4 cardinal directions (false by default). If you also have diagonal directions in your spriteset, set to false
 * @param {Boolean} NotRestoreSCG Set only if the SCRIPT_COMMAND_GENERATOR of 'npc' and 'me' dont have to be restored afterwards. note: The SCG of me is only stored if that person is not the input person
 * @returns 1 if the persons could face eachother, 0 if otherwise
 * @type Integer 
 */
LithoniteEngine.prototype.facePerson= function(me,other,IfaceOther,OtherFacesMe,FourWays, NotRestoreSCG){
        me=me||this.GIP;
	if(IfaceOther==undefined)IfaceOther=true;
	if(me==this.GIP){
		this.standStill();
		this.moving=2; //cinematics on.
	}else{
		if(!GetPersonValue(me,'facing')){
			SetPersonValue(me,'facing',true);
			SetPersonValue(me,'OldPersonDirection',GetPersonDirection(me));
			SetPersonValue(me,'OldPersonFrame',GetPersonFrame(me));
		}
        	if(!NotRestoreSCG) ShiftPersonScript(me, SCRIPT_COMMAND_GENERATOR);
	}
	if(!other)return 0;
	if(!IfaceOther&&!OtherFacesMe)return 0;
	var x=0; var y=0;
	var theta=Math.atan2(GetPersonY(other)-GetPersonY(me),GetPersonX(other)-GetPersonX(me));
	if(FourWays){
		if(-this.p8[6]<=theta&&theta<=-this.p8[2])y=-1;
		if( this.p8[6]>=theta&&theta>= this.p8[2])y= 1;
		if(-this.p8[2]<=theta&&theta<= this.p8[2])x= 1;
		if(-this.p8[6]>=theta||theta>= this.p8[6])x=-1;
	}else{
		if(-this.p8[7]<=theta&&theta<=-this.p8[1])y=-1;
		if( this.p8[7]>=theta&&theta>= this.p8[1])y= 1;
		if(-this.p8[3]<=theta&&theta<= this.p8[3])x= 1;
		if(-this.p8[5]>=theta||theta>= this.p8[5])x=-1;
	}
	if(IfaceOther){
		if(typeof IfaceOther=='string')
			this.setDir(this.getDir(IfaceOther, x,y),me);
		else
			QueuePersonCommand(me, this.getCommand(0,x,y), true);
		//SetPersonDirection(me,this.getMoveDir(x,y));
		//SetPersonFrame(me,0);
        }
	if(OtherFacesMe){
		//Save current direction so later we can face away.
		if(!GetPersonValue(other,'facing')){
			SetPersonValue(other,'facing',true);
			SetPersonValue(other,'OldPersonDirection',GetPersonDirection(other));
			SetPersonValue(other,'OldPersonFrame',GetPersonFrame(other));
		}
		if(!NotRestoreSCG) {ShiftPersonScript(other, SCRIPT_COMMAND_GENERATOR, '');ClearPersonCommands(other);}

		SetPersonFrame(other,0);

		if(typeof OtherFacesMe=='string') {
			if(IsCommandQueueEmpty(other))
				ClearPersonCommands(other);
			this.setDir(this.getDir(OtherFacesMe, -x,-y), other);
		} else {
			//ClearPersonCommands(other); 
			SetPersonDirection(other,this.getMoveDir(-x,-y));
			//QueuePersonCommand(other, this.getCommand(0,-x,-y), true);
		}
	}
	return 1;
}
LithoniteEngine.prototype.p8=new Array();
LithoniteEngine.prototype.p8[1]=Math.PI*0.125;
for(var i=2 ; i<8 ; i++ ){
	LithoniteEngine.prototype.p8[i]=i*LithoniteEngine.prototype.p8[1];
}



//----------------------------------------------------------------------------//
/*
	Due to a rounding bug, sometimes $GPZZ != GetPersonZFloat(Person) (Z = X and Y)
	when dx or dy are float. This is intrinsic to Javascript. 
*/

/**
 * offset a person (dx,dy), if its not obstructed there (can be changed with parameter ign)
 * @param {number} dx X-axis offset
 * @param {number} dy Y-axis offset
 * @param {string} person A name of a person, defaults to the current Input Person.
 * @param {Boolean} ign Ignore obstructions (defaults to false)
 * @returns 1 if the person has been repositioned, 0 if not
 */
LithoniteEngine.prototype.repos = function(dx,dy,person,ign) {
	person=person||this.GIP;
	var $GPXX = Math.round(GetPersonXFloat(person)+(dx||0));
	var $GPYY = Math.round(GetPersonYFloat(person)+(dy||0));
	if(IsPersonObstructed(person,$GPXX,$GPYY)&&!ign)return 0;
	SetPersonX(person,$GPXX);
	SetPersonY(person,$GPYY);
	return 1;
}	

/**
 * Move person onto another person and offsets it, if its not obstructed there (can be changed with parameter ign)
 * @param {string} who The name the person we will move 'person' to
 * @param {number} dx X-axis offset
 * @param {number} dy Y-axis offset
 * @param {string} person A name of a person, defaults to the current Input Person.
 * @param {Boolean} ign Ignore obstructions (defaults to false)
 * @returns 1 if the person has been repositioned, 0 if not
 * note: The layer of the person will not be changed
 */
LithoniteEngine.prototype.reposNextTo = function(who,dx,dy,person,ign) {
	person=person||this.GIP;
	var X = Math.round(GetPersonXFloat(who)+(dx||0));
	var Y = Math.round(GetPersonYFloat(who)+(dy||0));
	if(!IsPersonObstructed(person,X,Y)||ign){
		SetPersonX(person,X);
		SetPersonY(person,Y);
		return 1;
	}
	return 0;
}

/**
 * Try to undo what deObstruct() did. 
 * @param {string} person The name of the person to repos back, leave empty for .GIP
 * In the very specific case when we pick up a person (for example a potion), we could have been deobstructed, 
 * so we destroy this person, and with this function we put ourselves back where we should be, had we walked through that other person.
 * @returns 1 if the person has been repositioned, 0 if not, -1 if this function was not required.
 */
LithoniteEngine.prototype.reposBack = function(person) {
	switch(this.is){
		case 0: return this.repos(-this.move_x*this.sX,-this.move_y*this.sY,person); break;
		case 1: return this.repos(-this.move_y*this.sX,+this.move_x*this.sY,person); break;
		case 2: return this.repos(+this.move_y*this.sX,-this.move_x*this.sY,person); break;
	}
	return -1;
}


/**
 * repos() but with deObstruct() if we failed to reposition our sprite.
 * @param {integer} dx X-axis offset
 * @param {integer} dy Y-axis offset
 * @param {string} person The name of the person
 * @returns -1 if it shoved the person without being obstructed.  Returns 0,1 or 2 if deObstruct() was used and returned one of its values
 */
LithoniteEngine.prototype.shove = function(dx,dy,person) {
	if(!this.repos(dx,dy,person||this.GIP)){
		var active=this.active;
		this.active=1;
		this.move_x = dx;
		this.move_y = dy;
		this.active=active;
		var r=this.deObstruct(person||this.GIP);
		this.active=active;
		return r;
	}
	return -1;
}	

//----------------------------------------------------------------------------//
/**
 * Internal function, similar to {@link LithoniteEngine#resetSpeed}, but will not touch the person direction and will reset the inertial speed vectors.
 */
LithoniteEngine.prototype.rectifySpeed=function(){
	if(this.isDashing)SetPersonSpeedXY(this.GIP,this.sX=this.speed_dashX,this.sY=this.speed_dashY);
	else if(this.isCrawling)SetPersonSpeedXY(this.GIP,this.sX=this.speed_crawlX,this.sY=this.speed_crawlY);
	else SetPersonSpeedXY(this.GIP,this.sX=this.speed_normalX,this.sY=this.speed_normalY);
	this.i_x=this.i_y=0;
	this.checkSpeedOne();
}

//----------------------------------------------------------------------------//

/**
 * Put this function inside your updatescript if you are going to use enZone(string)
 * Lithonite.doZone();
 */
LithoniteEngine.prototype.doZone=function(){}; //Is being redefined by enZone()

//----------------------------------------------------------------------------//

/**
 * Helper function, see {@link LithoniteEngine#enZone}
 *
 * You can add your own 'zone' to the ZONES object, just define it like this:
 *  Lithonite.ZONES['myzone'] = function(){ <my stuff here> };
 * Then, inside a zone, put this code:
 *   Lithonite.enZone('myzone');
 *
 * Remember that inside this function you have access to all LithoniteEngine properties, for example: this.GIP
 *
 * The following terrains are defined:
 *
 * u:zone goes up,going up; d:zone goes down going up. Changes the speed of the player to create this illusion.
 * \u /u zone goes upleft or upright. Speeds do change when going up or down.
 * \ / zone goes upleft or upright. Speeds do not change when going up or down.
 * \\-half and /-half Similar to \ and /, but will move one tile up for each 2 tiles to the side.
 * u-stairs: same as u, only that it chokes to emulate 'stairs'. same goes for d-stairs, /-stairs and \-stairs.
 * !U:going UP is impossible, down is ok. The other directions also exist: !D !L !R
 * E:auto-east, Person cant be controlled with the keyboard. The other directions also exist: W N S
 * E+:auto-east, but you can still walk. The other directions also exist: W+ N+ S+
 * ice: cannot change direction while skating, until you hit a wall.
 * slip: skid a bit. You can modify the parameters of this function:
 * 	Lithonite.i_x_max and Lithonite.i_y_max  Set maximum speed
 * 	Lithonite.iif  Inertia Factor, at which speed increases
 * 	Lithonite.isf  Slip Factor, at which speed decreases
 * | and -: Only movement in that axis is allowed
 * x: Rotate directional movement by 45 degrees.
 * tipsy: swap up-down and left-right directions (DRUNK!)
 * fast: Like velocity, will speed up the Person by one pixel each frame
 * float: float in air sfx, careful, the camera gets detached.
 * mud: slowdown to half speed
 * velocity:speedup to double speed
 * hover:like ice, but standing still,can change dir.
 * fall: fall down the zone. You will want fall_north, fall_south, etc directions in your spriteset.
 * mirror: Will display your mirror image on a layer below or the one specified. The Y offset can also be changed, it defaults to 0
 * bridge: Will emulate reflective layer, but with Y-offset. Lithonite.enZone('bridge', <layer>, <Y-offset>); Layer defaults to layer below current one.
 * bridgewave: Same as bridge, but will make the image wave (requires more CPU).
 */
LithoniteEngine.prototype.ZONES = {

	'\\': function(){this.repos(0,this.move_x*this.sY);this.deObstruct();},
	'/' : function(){this.repos(0,-this.move_x*this.sY);this.deObstruct();},
	'\\u': function(){ var n=this.sX; if(this.move_x<0)n*=0.5; if(this.move_x>0)n*=1.5; SetPersonSpeedXY(this.GIP,n,this.sY); this.repos(0,this.move_x*n);this.deObstruct(); },
	'/u': function(){ var n=this.sX; if(this.move_x>0)n*=0.5; if(this.move_x<0)n*=1.5; SetPersonSpeedXY(this.GIP,n,this.sY); this.repos(0,-this.move_x*n);this.deObstruct(); },
	'\\-half': function(){this.repos(0,this.move_x*this.sY*0.5);this.deObstruct();},
	'/-half' : function(){this.repos(0,-this.move_x*this.sY*0.5);this.deObstruct();},
	'u': function(){if(this.move_y<0)SetPersonSpeedXY(this.GIP,this.sX,this.sY*0.5);if(this.move_y>0)SetPersonSpeedXY(this.GIP,this.sX,this.sY*1.5);},
	'd': function(){if(this.move_y>0)SetPersonSpeedXY(this.GIP,this.sX,this.sY*0.5);if(this.move_y<0)SetPersonSpeedXY(this.GIP,this.sX,this.sY*1.5);},
	'\\-stairs' : function(){var n=Math.abs(Math.round(Math.sin(GetTime()>>6)));SetPersonSpeedXY(this.GIP,n,1); this.repos(0,this.move_x);this.deObstruct()},
	'/-stairs' : function(){var n=Math.abs(Math.round(Math.sin(GetTime()>>6)));SetPersonSpeedXY(this.GIP,n,1); this.repos(0,-this.move_x);this.deObstruct()},
	'u-stairs': function(){var n=Math.abs(Math.round(Math.sin(GetTime()>>6)));if(this.move_y<0)SetPersonSpeedXY(this.GIP,1,n);if(this.move_y>0)SetPersonSpeedXY(this.GIP,1,n*1.5);},
	'd-stairs': function(){var n=Math.abs(Math.round(Math.sin(GetTime()>>6)));if(this.move_y>0)SetPersonSpeedXY(this.GIP,1,n);if(this.move_y<0)SetPersonSpeedXY(this.GIP,1,n*1.5);},
	'!U': function(){if(this.move_y<0){SetPersonSpeedXY(this.GIP,GetPersonSpeedX(this.GIP),1);this.repos(0,-this.move_y);}else this.rectifySpeed();},
	'!D': function(){if(this.move_y>0){SetPersonSpeedXY(this.GIP,GetPersonSpeedX(this.GIP),1);this.repos(0,-this.move_y);}else this.rectifySpeed();},
	'!R': function(){if(this.move_x>0){SetPersonSpeedXY(this.GIP,1,GetPersonSpeedY(this.GIP));this.repos(-this.move_x,0);}else this.rectifySpeed();},
	'!L': function(){if(this.move_x<0){SetPersonSpeedXY(this.GIP,1,GetPersonSpeedY(this.GIP));this.repos(-this.move_x,0);}else this.rectifySpeed();},
	'fast': function(){this.repos(this.move_x,this.move_y);},
	'tipsy': function(){SetPersonSpeedXY(this.GIP,-this.sX,-this.sY);},
	'E': function(){SetPersonSpeedXY(this.GIP,0,0);if(!this.repos(1,0)){this.moving=1;this.move_x=1;this.deObstruct()};},
	'W': function(){SetPersonSpeedXY(this.GIP,0,0);if(!this.repos(-1,0)){this.moving=1;this.move_x=-1;this.deObstruct()};},
	'N': function(){SetPersonSpeedXY(this.GIP,0,0);if(!this.repos(0,-1)){this.moving=1;this.move_y=-1;this.deObstruct()};},
	'S': function(){SetPersonSpeedXY(this.GIP,0,0);if(!this.repos(0,1)){this.moving=1;this.move_y=1;this.deObstruct()};},

	'E+': function(){if(!this.repos(1,0)){this.moving=1;this.move_x=1;this.deObstruct()};},
	'W+': function(){if(!this.repos(-1,0)){this.moving=1;this.move_x=-1;this.deObstruct()};},
	'N+': function(){if(!this.repos(0,-1)){this.moving=1;this.move_y=-1;this.deObstruct()};},
	'S+': function(){if(!this.repos(0,1)){this.moving=1;this.move_y=1;this.deObstruct()};},

	'fall': function(){
		if(this.active)
			this.active = false;
		this._fO = IsPersonObstructed(this.GIP,GetPersonX(this.GIP),GetPersonY(this.GIP)+1);
		if(this.sY <1) this.sY=1; else if(this.sY<6) this.sY+=0.25;
		this.repos(0,this.sY,this.GIP,true);this.moving=1;this.move_y=1;
		if(!this._fi && this._fO){this._fi=1;this.sY+=1;}
		if(this._fi==1 && !this._fO){
			this.active =true;
			this.repos(0,this.sY,this.GIP,false);
			return this.rectifySpeed();
		}
	},

	'-': function(){SetPersonSpeedXY(this.GIP,GetPersonSpeedX(this.GIP),0);},
	'|': function(){SetPersonSpeedXY(this.GIP,0,GetPersonSpeedY(this.GIP));},

	'x': function(){this.repos(-this.move_y,this.move_x);},
	'mud': function(){SetPersonSpeedXY(this.GIP, this.speed_normalX*0.5,this.speed_normalY*0.5);},
	'velocity': function(){SetPersonSpeedXY(this.GIP, this.speed_normalX*2,this.speed_normalY*2);},

	'slip': function(){if(this.o){this.i_x=0;this.i_y=0;}else{
		this.i_x+=this.iif*this.move_x; if(this.i_x<-this.i_x_max)this.i_x=-this.i_x_max; else if(this.i_x>this.i_x_max)this.i_x=this.i_x_max;
		this.i_y+=this.iif*this.move_y; if(this.i_y<-this.i_y_max)this.i_y=-this.i_y_max; else if(this.i_y>this.i_y_max)this.i_y=this.i_y_max;
		}
		this.ix=this.iy=0;
		if(!this.move_x)this.i_x/=this.isf; if(this.i_x*this.i_x<0.0001)this.i_x=0; else {this.ix=this.i_x<0?-1:1; QueuePersonCommand(this.GIP, this.getCommand(true,this.ix*this.ix,0), true);}
		if(!this.move_y)this.i_y/=this.isf; if(this.i_y*this.i_y<0.0001)this.i_y=0; else {this.iy=this.i_y<0?-1:1; QueuePersonCommand(this.GIP, this.getCommand(true,0,this.iy*this.iy), true);}
		if(this.move_x||this.move_y)QueuePersonCommand(this.GIP, this.getCommand(false,this.move_x,this.move_y), true); else SetPersonFrame(this.GIP,0);
		SetPersonSpeedXY(this.GIP, this.i_x=this.i_x||0, this.i_y=this.i_y||0 );},
	'ice': function(){SetPersonSpeedXY(this.GIP,0,0);var d=(this.hist_x+(this.hist_y<<2));if(!this.repos(this.old_x,this.old_y)&&!this.idle&&d!=this.dir){this.dir=d; this.old_x=this.hist_x; this.old_y=this.hist_x?0:this.hist_y;return true;}},
	'float': function(){DetachCamera();var p=Math.sin(GetTime()>>7);this.repos(0,p>>1);},
	'hover': function(){this.standStill();var d=(this.hist_x+(this.hist_y<<2)); if(this.shove(this.old_x*this.sX,this.old_y*this.sY)>-1){this.old_x=this.old_y=0}; if(d!=this.dir){this.old_x=this.hist_x; this.old_y=this.hist_x?0:this.hist_y; this.dir=d;}},
	'mirror': function(){
	if((this.bridge.frame != GetPersonFrame(this.GIP)) || (this.bridge.dir != GetPersonDirection(this.GIP))){
		this.bridge.dir = GetPersonDirection(this.GIP);
		this.bridge.frame = GetPersonFrame(this.GIP);
		var dir_index = GetPersonValue(this.GIP, this.bridge.dir +'R'+ this.bridge.frame);
		if(dir_index == '') {
			var ssd=GetPersonSpriteset(this.GIP).directions;
			var dir = GetPersonDirection(this.GIP);
			var flipdir = dir.replace("north","NNN").replace("south","north").replace("NNN","south");
			var i=ssd.length-1;
			do{if (ssd[i].name==flipdir) break; }while(i--);
			dir_index = i;
			var dir = GetPersonDirection(this.GIP);
			SetPersonValue(this.GIP, this.bridge.dir +'R'+ this.bridge.frame, dir_index);
		};
		var ss = GetPersonSpriteset(this.GIP);
		this.bridge.image = ss.images[ ss.directions[dir_index].frames[this.bridge.frame].index ];
	}
	var X = MapToScreenX(this.bridge.layer,GetPersonX(this.GIP)) + this.bridge.offset.x;
	var Y = MapToScreenY(this.bridge.layer,GetZoneY(this.zone)) - GetPersonY(this.GIP) + GetZoneY(this.zone) + this.bridge.offset.y;
	var X2 = X + this.bridge.image.width;
	var Y2 = Y - this.bridge.image.height;
	this.bridge.image.transformBlitMask(X,Y2, X2,Y2, X2,Y, X,Y, this.bridge.color);
	},

	'bridge': function(){
	if((this.bridge.frame != GetPersonFrame(this.GIP)) || (this.bridge.dir != GetPersonDirection(this.GIP))){
		this.bridge.dir = GetPersonDirection(this.GIP);
		this.bridge.frame = GetPersonFrame(this.GIP);
		var dir_index = GetPersonValue(this.GIP, this.bridge.dir + this.bridge.frame);
		if(dir_index == '') {
			var dir = GetPersonDirection(this.GIP);
			var ssd=GetPersonSpriteset(this.GIP).directions;
			var i=ssd.length-1;
			do{if (ssd[i].name==dir) break; }while(i--);
			dir_index = i;
			SetPersonValue(this.GIP, this.bridge.dir + this.bridge.frame, dir_index);
		};
		var ss = GetPersonSpriteset(this.GIP);
		this.bridge.image = ss.images[ ss.directions[dir_index].frames[this.bridge.frame].index ];
	}
	var X = MapToScreenX(this.bridge.layer,GetPersonX(this.GIP)) + this.bridge.offset.x;
	var Y = MapToScreenY(this.bridge.layer,GetPersonY(this.GIP)) + this.bridge.offset.y;
	var X2 = X + this.bridge.image.width;
	var Y2 = Y + this.bridge.image.height;
	this.bridge.image.transformBlitMask(X,Y2, X2,Y2, X2,Y, X,Y, this.bridge.color);
	},

	'bridgewave': function(){
	if((this.bridge.frame != GetPersonFrame(this.GIP)) || (this.bridge.dir != GetPersonDirection(this.GIP))){
		this.bridge.dir = GetPersonDirection(this.GIP);
		this.bridge.frame = GetPersonFrame(this.GIP);
		var dir_index = GetPersonValue(this.GIP, this.bridge.dir + this.bridge.frame);
		if(dir_index == '') {
			var dir = GetPersonDirection(this.GIP);
			var ssd=GetPersonSpriteset(this.GIP).directions;
			var i=ssd.length-1;
			do{if (ssd[i].name==dir) break; }while(i--);
			dir_index = i;
			SetPersonValue(this.GIP, this.bridge.dir + this.bridge.frame, dir_index);
		};
		var ss = GetPersonSpriteset(this.GIP);
                this.bridge.cuts = GetPersonValue(Lithonite.GIP,'height')>>2;
		this.bridge.surface = ss.images[ ss.directions[dir_index].frames[this.bridge.frame].index ].createSurface();
		this.bridge.surface.flipVertically();
		this.bridge.image1 = this.bridge.surface.cloneSection(0, 0, this.bridge.surface.width, this.bridge.cuts).createImage();
		this.bridge.image2 = this.bridge.surface.cloneSection(0,   this.bridge.cuts+1, this.bridge.surface.width, this.bridge.cuts).createImage();
		this.bridge.image3 = this.bridge.surface.cloneSection(0, 2*this.bridge.cuts+1, this.bridge.surface.width, this.bridge.cuts).createImage();
		this.bridge.image4 = this.bridge.surface.cloneSection(0, 3*this.bridge.cuts+1, this.bridge.surface.width, this.bridge.surface.height-3*this.bridge.cuts-1).createImage();
	}
	var X = MapToScreenX(this.bridge.layer,GetPersonX(this.GIP)) + this.bridge.offset.x;
	var Y = MapToScreenY(this.bridge.layer,GetPersonY(this.GIP)) + this.bridge.offset.y;
	var X2 = X + this.bridge.surface.width;
	var Y2 = Y + this.bridge.surface.height;
	var n = GetTime()>>8;
	var s1n = Math.sin(n);
	var s2n = Math.sin(n+1);
	var s3n = Math.sin(n+2);
	this.bridge.image1.transformBlitMask(X, Y, X2, Y, X2+s1n, Y + this.bridge.cuts, X+s1n, Y + this.bridge.cuts, this.bridge.color);
	this.bridge.image2.transformBlitMask(X+s1n, Y+  this.bridge.cuts+1, X2+s1n, Y+   this.bridge.cuts+1, X2+s2n, Y + 2*this.bridge.cuts, X+s2n, Y + 2*this.bridge.cuts, this.bridge.color);
	this.bridge.image3.transformBlitMask(X+s2n, Y+2*this.bridge.cuts+1, X2+s2n, Y+ 2*this.bridge.cuts+1, X2+s3n, Y + 3*this.bridge.cuts, X+s3n, Y + 3*this.bridge.cuts, this.bridge.color);
	this.bridge.image4.transformBlitMask(X+s3n, Y+3*this.bridge.cuts+1, X2+s3n, Y+ 3*this.bridge.cuts+1, X2+s1n, Y2, X+s1n, Y2, this.bridge.color);
	},


}

/**
 * Use this function inside a zone, it will modify the movement behavior of your spriteset.
 * Lithonite.enZone(<name>);
 *
 * While your PC is inside a zone, it will run a defined script. We have a pre-defined set 
 * of functions that will cause the PC to move differently.
 * Of course, you can use this function to check other things, like opening a door when 
 * you're in a zone. Once the zone script is running, if we hop onto another zone that 
 * overlaps our zone, it will continue to zone, unless each zonescript starts with: 
 *
 * Lithonite.isZoning=0;
 *
 * You can add your own 'zone' to the ZONES object, just add it to your Lithonite object, 
 * for example:
 * Lithonite.ZONES['climb'] = function(){ 
 *  if (Lithonite.isZoning !== 1) return; // Only continue if not initializing 
 *  Lithonite.SwapDirections( 'climb_' ); 
 * };
 * Now you can call it inside a zone like so: Lithonite.enZone('climb');
 * note: Lithonite.isZoning is 1 when just activated, then stays true while active. The rest of the time its false.
 *
 * @param {string} c The type of terrain you want this zone to be. Read the ZONES function to see what types are available.
 * @param {string} p1 Parameter1 (used in bridge, bridgewave and mirror). In your custom zone: This is a string containing extra deZone() commands
 * @returns 0 if called but already active, 1 if it activates.
 *
 * note: You probably want to define this at the beginning: 
 *   function zone(){Lithonite.enzone.apply(Lithonite,arguments)}; //short-write as a function
 * This way, just set this inside your zone: zone('climb');

 */
LithoniteEngine.prototype.enZone=function(c,p1,p2){
	if(this.isZoning)return 0;
	this.isZoning=1;
	this.zone=GetCurrentZone();
	this.SaveDirections(this.GIP,'_');
	var extra = "";
	switch(c){

		case 'hover':
			this.dir=(this.hist_x+(this.hist_y<<2));
			this.old_x=this.hist_x;
			this.old_y=this.hist_x?0:this.hist_y;
			this.standStill();
			SetPersonSpeedXY(this.GIP,0,0); //stop walking
			this.doZone=this.ZONES[c];
			break;

		case 'fall':
			this.standStill();
			SetPersonSpeedXY(this.GIP,0,0);	//stop walking
			this.SwapDirections('fall_');
			this._fi=0;
			this.doZone=this.ZONES[c]; 
			break;

		case 'slip':
			this.i_x=this.move_x;
			this.i_y=this.move_y;
			this.doZone=this.ZONES[c];
			break;

		case 'ice': 
			this.dir=(this.hist_x+(this.hist_y<<2));
			this.old_x=this.hist_x;
			this.old_y=this.hist_x?0:this.hist_y;
			this.active=false;
			SetPersonSpeedXY(this.GIP,0,0);
			this.doZone=this.ZONES[c];
			break;

		case 'bridge':
			var base=GetPersonBase(this.GIP);
			this.bridge.offset.x = -GetPersonValue(this.GIP,'width')>>1;
			this.bridge.offset.y = (p2||0) + ((GetPersonBase(this.GIP).y2 - GetPersonBase(this.GIP).y1)>>1);
			this.bridge.frame = 0;
			this.bridge.dir = 0;
			var l = GetPersonLayer(this.GIP) - 1; if(l<0) l=0;
			this.bridge.layer = typeof p1 == 'number'? p1 : l;
			this.bridgeZone=this.ZONES[c];
			this.bridgeZone();
			SetLayerRenderer(this.bridge.layer, this.objname+".bridgeZone()");
			extra = "SetLayerRenderer("+ this.bridge.layer+",'');";
			break;

		case 'bridgewave':
			var base=GetPersonBase(this.GIP);
			this.bridge.offset.x = -GetPersonValue(this.GIP,'width')>>1;
			this.bridge.offset.y = (p2||0) + ((GetPersonBase(this.GIP).y2 - GetPersonBase(this.GIP).y1)>>1);
			this.bridge.frame = 0;
			this.bridge.dir = 0;
			var l = GetPersonLayer(this.GIP) - 1; if(l<0) l=0;
			this.bridge.layer = typeof p1 == 'number'? p1 : l;
			this.bridgeZone=this.ZONES[c];
			this.bridgeZone();
			SetLayerRenderer(this.bridge.layer, this.objname+".bridgeZone()");
			extra = "SetLayerRenderer("+ this.bridge.layer+",'');";
			break;

		case 'mirror':
			var base=GetPersonBase(this.GIP);
			this.bridge.offset.x = -GetPersonValue(this.GIP,'width')>>1;
			this.bridge.offset.y = (p2||0) + (GetPersonValue(this.GIP,'height')>>1) - (GetPersonValue(this.GIP,'height')-GetPersonBase(this.GIP).y1);
			this.bridge.frame = 0;
			this.bridge.dir = 0;
			var l = GetPersonLayer(this.GIP) - 1; if(l<0) l=0;
			this.bridge.layer = typeof p1 == 'number'? p1 : l;
			this.bridgeZone=this.ZONES[c];
			this.bridgeZone();
			SetLayerRenderer(this.bridge.layer, this.objname+".bridgeZone()");
			extra = "SetLayerRenderer("+ this.bridge.layer+",'');";
			break;

		default:
			if(typeof this.ZONES[c] == 'function')this.doZone=this.ZONES[c];
			else this.doZone=new Function(c);
			if(p1) extra = p1;
	}

	this.doZone();
	this.isZoning=true;

	this.testFromUntil(0,
	"!AreZonesAt(GetPersonX('"+this.GIP+"'),GetPersonY('"+this.GIP+"')||!"+this.objname+".isZoning,GetPersonLayer('"+this.GIP+"'))",
	this.objname+".deZone();"+extra,
	1, undefined, true);
	return 1;
}

/**
 * Internal function. Stop Zoning
 */
LithoniteEngine.prototype.deZone = function(){
	this.isZoning = false;
	this.doZone = function(){};
	this.zone = undefined;
	this.rectifySpeed();
	this.LoadDirections(this.GIP, '_');
	this.active = true;
}

/**
 * testFromUntil - helper function for zone()
 * @param {interger} delayCounter Will start testing in 'delayCounter' frames
 * @param {Boolean} condition String which has to be true to execute runCommand. Keep this string small and simple.
 * @param {string} runCommand Command string that will run as soon as condition is true
 * @param {integer} retryDelay If the condition is false, it will test again in 'retryDelay' frames. The default is 1.
 * @param {integer} retryCounter This tells us how many times we can use the retryDelay, infinite times by default
 * @param {Boolean} immediate If true, it will execute runCommand immediately, else it will delay one frame. Defaults to true
 */
LithoniteEngine.prototype.testFromUntil=function(delayCounter,condition,runCommand,retryDelay,retryCounter,immediate)
{
	if(retryCounter!=undefined && (retryCounter--<0))
		return;
	if(delayCounter>0){
		SetDelayScript(delayCounter, this.objname+".testFromUntil(0,\""+condition+"\",\""+runCommand+"\","+retryDelay+","+(retryCounter+1)+");");
		return;
	}
	if(!eval(condition)){
		SetDelayScript(retryDelay||1, this.objname+".testFromUntil(0,\""+condition+"\",\""+runCommand+"\","+retryDelay+","+retryCounter+","+immediate+");");
		return;
	}
	if(immediate==undefined)
		immediate=true;
	if(immediate)
		eval(runCommand);
	else 
		SetDelayScript(1,runCommand);
}


//----------------------------------------------------------------------------//

/**
 * Detach Camera and Input
 */
LithoniteEngine.prototype.DetachAll = function(){
	this.camera = IsCameraAttached()?GetCameraPerson():"";
	this.GIP = IsInputAttached()?GetInputPerson():this.GIP;
	if(IsCameraAttached()) DetachCamera();
	if(IsInputAttached()) DetachInput();
	if(this.setTalkActivationKey()) SetTalkActivationKey("");
}

/**
 * Attach Camera and Input
 * @param {string} InputPerson If not left empty, it will set the default InputPerson
 * @param {string} CameraPerson If not left empty, it will set the default CameraPerson (else it will try the InputPerson)
 * @param {KEY} TalkActivationKey If not left empty, it will set the default TalkActivationKey
 */
LithoniteEngine.prototype.AttachAll = function(InputPerson, CameraPerson, TalkActivationKey){
	if(InputPerson) this.setInputPerson(InputPerson);
	if(CameraPerson||InputPerson) this.setCameraPerson(CameraPerson||InputPerson);
	if(TalkActivationKey) this.setTalkActivationKey(TalkActivationKey);
	if(this.camera)AttachCamera(this.camera);
	if(this.talkKey) SetTalkActivationKey(this.talkKey);
	AttachInput(this.GIP);
}

/**
 * Detach Input
 */
LithoniteEngine.prototype.DetachGIP = function(){
	this.camera=IsCameraAttached()?GetCameraPerson():"";
	this.GIP=IsInputAttached()?GetInputPerson():this.GIP;
	if(IsInputAttached())DetachInput();
}

/**
 * Attach Input
 */
LithoniteEngine.prototype.AttachGIP = function(InputPerson){
	if(typeof InputPerson != 'undefined') this.setInputPerson(InputPerson);
	AttachInput(this.GIP);
}

/**
 * Detach Camera
 */
LithoniteEngine.prototype.DetachCamera = function(){
	this.camera=IsCameraAttached?GetCameraPerson():"";
	this.GIP=IsInputAttached()?GetInputPerson():this.GIP;
	if(IsCameraAttached())DetachCamera();
}

/**
 * Attach Camera
 */
LithoniteEngine.prototype.AttachCamera = function(CameraPerson){
	if(typeof CameraPerson!= 'undefined') this.setCameraPerson(InputPerson);
	if(this.camera)AttachCamera(this.camera);
}

