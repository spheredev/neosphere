function TimedAnimation(animation)
{
  if (this instanceof TimedAnimation == false) {
    return new TimedAnimation(animation);
  }

  this.animation = animation;
  this.next_update = GetTime() + animation.getDelay();
}


TimedAnimation.prototype.blit = function(x, y) {
  if (GetTime() > this.next_update) {
    this.animation.readNextFrame();
    this.next_update = GetTime() + this.animation.getDelay();
  }
  
  this.animation.drawFrame(x, y);
}
