///////////////////////////////////////////////////////////

function Spriteset(spriteset) {
  this.spriteset = spriteset;
}

///////////////////////////////////////////////////////////

Spriteset.prototype.getFrame = function(direction_index, frame_index) {
  return (this.spriteset.images[this.spriteset.directions[direction_index].frames[frame_index].index]);
}

///////////////////////////////////////////////////////////

Spriteset.prototype.getNumFrames = function(direction_index){
  // returns total frames for the direction index
  return (this.spriteset.directions[direction_index].frames.length);
}

///////////////////////////////////////////////////////////

Spriteset.getNumDirections = function() {
  return (this.spriteset.directions.length);
}

///////////////////////////////////////////////////////////

var OldLoadSpriteset      = LoadSpriteset;
var OldSetPersonDirection = SetPersonDirection;
var OldGetPersonDirection = GetPersonDirection;

///////////////////////////////////////////////////////////

function init_old_spriteset_code() {
  LoadSpriteset = function(filename) {
    return new Spriteset(OldLoadSpriteset(filename));
  }

  SetPersonDirection = function(name, direction_index) {
    OldSetPersonDirection(name, GetPersonSpriteset(name).directions[direction_index].name);
  }

  GetPersonDirection = function(name) {
    var direction = OldGetPersonDirection(name);
    for (var i = 0, spriteset = GetPersonSpriteset(name); i < spriteset.directions.length; ++i)
      if (spriteset.directions[i].name == direction)
        return (i);       
    return 0;
  }

}

///////////////////////////////////////////////////////////

init_old_spriteset_code();

///////////////////////////////////////////////////////////
