// Use like this: PlayAnimation("filename.flc", 50, 50, 3 (if you want it to repeat 3 times), false);
// Or like this: PlayAnimation("filename.flc", 50, 50, 0, true (if you want to play it forever));
// Every time you call the PlayAnimation function, you insert an animation into the array
// If you want to clear the array, simply call the ClearMapAnimationArray function and begin again
// Don't forget to use SetRenderScript("UpdateMapAnimation()")); right after all calls
// to PlayAnimation to render the animations. If you want to setup a new array of anims
// just call the ClearMapAnimationArray() function. Give the engine a FlipScreen or preferably an Engine
// Update so the engine doesn't crash with all the animations it has to play in set period
// of time

// Variables //
var g_map_animations = new Array();

//Clear the Map Animation Array//
// this used to be called ClearArray (which is just a bad name)
function ClearMapAnimationArray()
{
  g_map_animation = new Array();
}

// Map Animation Object Function //
function MapAnimation(x, y, animation, count, forever) {
  this.x = x;
  this.y = y;
  this.animation = animation;
  this.repeat_count = count;
  this.repeat_forever = forever;
  this.next_update = 0;
  this.frame_index = 0;
}

function PlayAnimation(filename, x, y, count, forever) {
  var animation = LoadAnimation(filename); // Load the animation filename
  var map_animation = new MapAnimation(x, y, animation, count, forever); //setup a new object
  map_animation.playing = true; // set the anim to play
  g_map_animations.push(map_animation); // insert animation into array
}

function UpdateMapAnimation() {
  for (var i = 0; i < g_map_animations.length; ++i)
  { // loop through the animations in the array
    if (g_map_animations[i].playing) { // check to see if we play them
      var anim = g_map_animations[i].animation; //set the filename to anim
      anim.drawFrame(g_map_animations[i].x, g_map_animations[i].y); //draw the animation
      if (g_map_animations[i].next_update <= GetTime()) // check to see if we need to update
      {
        g_map_animations[i].frame_index += 1; // if we do, kick the frame up a notch
        anim.readNextFrame(); //read the next frame
        g_map_animations[i].next_update = GetTime() + anim.getDelay(); // update it and play with delay
        if (g_map_animations[i].frame_index >= anim.getNumFrames()) // if we go past the number of frames the anim has
        {
          g_map_animations[i].frame_index = 0; // set index to 0 and start again
          if (g_map_animations[i].repeat_forever) //if the user sets this to repeat forever
          { // do nothing
          } else {
            g_map_animations[i].repeat_count -= 1; // take out a repeat count
            if (g_map_animations[i].repeat_count <= 0) //once we reach zero...
            {
              g_map_animations[i].repeat_count = 0; //set repeat to 0
              g_map_animations[i].playing = false; // and finally, stop the animation
            }
          }
        }
      }
    }
  }
}