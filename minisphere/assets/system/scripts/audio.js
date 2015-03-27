var Sphere_Audio = new Object;

with (Sphere_Audio) {
  Music = undefined;
  Sounds = new Array(16);
  CurrentSound = 0;
}


function ChangeMusic(filename)
{
  with (Sphere_Audio) {
    if (Music != undefined) {
      Music.stop();
    }
    
    if (filename.length != 0) {
      Music = LoadSound(filename);
      Music.play(true);
    }
  }
}


function PlaySound(filename)
{
  with (Sphere_Audio) {
  
    // stop the current sound
    if (Sounds[CurrentSound] != undefined) {
      Sounds[CurrentSound].stop();
    }
    
    // load new one
    Sounds[CurrentSound] = LoadSound(filename);
    Sounds[CurrentSound].play(false);
  
    // go to the next sound
    CurrentSound++;
    if (CurrentSound >= Sounds.length) {
      CurrentSound = 0;
    }
  }
}
