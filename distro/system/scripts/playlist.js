/*
  simple music playlist script (m3u files)

  Example usage:
  
  EvaluateSystemScript("playlist.js");

  var playlist = new Playlist("../sounds/list.m3u");

  SetUpdateScript("UpdateScript()");
  function UpdateScript() {
    playlist.update();
  }

*/

/**
  perl like chomp function, removes end of line characters
*/
function chomp(str) {  
  if (str.charAt(str.length - 1) == '\n')
    str = str.substr(0, str.length - 1);
  if (str.charAt(str.length - 1) == '\r')
    str = str.substr(0, str.length - 1);
  return str;
}

/**
  tells you if filename is a file in the sounds directory of the game
*/
function DoesSoundExist(filename) {
  var list = GetFileList("sounds");
  for (var i = 0; i < list.length; i++)
    if (list[i] == filename)
      return true;
  return false;
}

///////////////////////////////////////////////////////////

/**
  returns an array of lines read from 'file'
*/
function ReadLines(file) {
  var lines = new Array();
      lines.push("");

  var text = CreateStringFromByteArray(file.read(file.getSize()));

  for (var i = 0; i < text.length; i++) {
    var c = text.charAt(i);
    lines[lines.length - 1] += c;

    if (c == '\n') {
      lines.push("");
    }

  }
 
  for (var i = 0; i < lines.length; i++) {
    lines[i] = chomp(lines[i]);
  }
 
  return lines;
}

///////////////////////////////////////////////////////////

/**
  Playlist object,
  
  m3u files are just a bunch of lines, that are either:
  1) files
  2) # a comment
  3) a blank line 
*/
function Playlist(filename) {
  if (this instanceof Playlist == false)
    return new Playlist(filename);

  this.file = OpenRawFile(filename);
  this.lines = ReadLines(this.file);

  this.sound = null;
  this.current = -1;
}

///////////////////////////////////////////////////////////

/**
  returns the next filename in the playlist
  the filename will exist
*/
Playlist.prototype.getNextFile = function(first_call) {
  var filename = "";

  this.current += 1;

  while (this.current < this.lines.length) {
    if (this.lines[this.current].length > 0 && this.lines[this.current].charAt(0) != '#') {
      if (DoesSoundExist(this.lines[this.current])) {
        filename = this.lines[this.current];
        break;
      }
    }
    
    this.current += 1;
  }

  if (first_call) {
    if (filename == "") {
      this.current = -1;
      filename = this.getNextFile(false);
    }
  }

  return filename;
}

///////////////////////////////////////////////////////////

/**
  called to advance the playlist and to play the next song if ready
*/
Playlist.prototype.update = function() {
  if (this.sound == null || this.sound.isPlaying() == false) {
    var filename = this.getNextFile(true);

    if (filename != "") {
      this.sound = LoadSound(filename);
      this.sound.play(false);
    }
  }
}

///////////////////////////////////////////////////////////
