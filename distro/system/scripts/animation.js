function Animation(){
  this.images = new Array();
  this.timing = new Array();
  this.tempTime = GetTime();
  this.total = 0;
}

Animation.prototype.addImage = function(unknown, timing){
  if(typeof unknown == "string"){
    this.images[this.images.length] = LoadImage(unknown);
  } else {
    this.images[this.images.length] = unknown;
  }
  this.timing[this.timing.length] = this.total + timing;
  this.total += timing;
}							

Animation.prototype.reset = function(){
  this.tempTime = GetTime();
}

Animation.prototype.blit = function(x, y){
  while(GetTime() - this.tempTime >= this.total){
    this.tempTime += this.total;
  }
  for(var i = 0; i < this.timing.length; i++){
    if(GetTime() - this.tempTime < this.timing[i]){
      this.images[i].blit(x, y);
      return;
    }
  }
  // Abort("Timing error, please please please tell fenix that his code sucks.");
}