function Timer()
{
  this.start  = GetTime();
  this.total  = 0;
  this.paused = false;
}


Timer.prototype.pause = function() {
  if (this.paused == false) {
    this.total += (GetTime() - this.start);
    this.paused = true;
  }
}


Timer.prototype.unpause = function() {
  if (this.paused == true) {
    this.start = GetTime();
    this.paused = false;
  }
}


Timer.prototype.getMilliseconds = function() {
  if (this.paused) {
    return this.total;
  } else {
    return (this.total + GetTime() - this.start);
  }
}
