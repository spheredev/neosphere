//System script by Tim Fitz aka FireMoth.
//Claim this is your own, and I will kill you.

function Intro ()
{
  if( !(this instanceof Intro) ) return new Intro( );
  this.obj = new Array();
  this.step = -GetScreenHeight();
  this.speed = 1;
  this.font = new Array();
  this.images = new Array();
  this.height = new Array();
  this.type = new Array(); // I hate doing this.
  this.color = new Array();
  this.height[-1]=0; // I dunno how to eliminate this. Oh well! It works! Who cares!
  this.gradsize=GetScreenHeight()/16;
  this.grad=true;
  this.lheight=0;
  this.spacing=0;
}

Intro.prototype.addText = function (string,fnt,clr)
{
  if (fnt ==  undefined) fnt = GetSystemFont();
  if (clr == undefined) clr = CreateColor(255,255,255);
  if (string == undefined) return false; //You dumbfuck.
  with (this)
  {
    var c = obj.length;
    height[c]=lheight+height[c-1];
    lheight=fnt.getHeight()+spacing;
    obj[c]=string;
    color[c]=clr;
    type[c]="String";
    font[c]=fnt;
  }
  return true; 
}

Intro.prototype.addImage = function (image)
{
  if (image == undefined) return false; //You asshole.
  with (this)
  {
    var c = obj.length;
    height[c]=lheight+height[c-1];
    lheight=image.height+spacing;
    obj[c]=image;
    type[c]="Image";
  }
}

Intro.prototype.addSpace = function (spc)
{
  this.lheight += spc; //Wow, that was painless!
}

Intro.prototype.play = function (spd)
{
  with (this)
  {
    if (spd !== undefined) speed=spd;
    step=-GetScreenHeight();
    while (!loopStep()) {FlipScreen()}
  }
}

Intro.prototype.loopStep = function ()
{
  with (this)
  {
    draw();
    step+=speed;
    if ( step > height[height.length-1]+lheight)  return true;
    return false;
  }
}

Intro.prototype.draw = function ()
{
  with (this)
  {
    for (var n = 0; n<obj.length; n++)
    {
      if (type[n] == "String")
      {
        font[n].setColorMask(color[n]);
        font[n].drawText(GetScreenWidth()/2-font[n].getStringWidth(obj[n])/2,height[n]-step,obj[n]);
       }
       else if (type[n] == "Image")
       {
         obj[n].blit(GetScreenWidth()/2-obj[n].width/2,height[n]-step);
       }
     }
     if (grad)
     {
       for (var n = 0; n<gradsize; n++)
       {
         Line(0,n,GetScreenWidth(),n,CreateColor(0,0,0,255-n/gradsize*255));
         Line(0,GetScreenHeight()-n,GetScreenWidth(),GetScreenHeight()-n,CreateColor(0,0,0,255-n/gradsize*255));
       }
     }
  }
}

