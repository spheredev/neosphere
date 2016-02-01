EvaluateSystemScript("colors.js");


function ClearScreen()
{
  ApplyColorMask(Black);
}


function FadeOut(milliseconds)
{
  FadeToColor(milliseconds, Black);
}


function FadeIn(milliseconds)
{
  FadeFromColor(milliseconds, Black);
}


function FadeToColor(msecs, clr)
{
  var image = GrabImage(0, 0, GetScreenWidth(), GetScreenHeight());
  var color = CreateColor(clr.red, clr.green, clr.blue, clr.alpha);

  var time = GetTime();
  while (GetTime() - time < msecs) {
    color.alpha = (GetTime() - time) * 255 / msecs;
    image.blit(0, 0);
    ApplyColorMask(color);
    FlipScreen();
  }

  color.alpha = 255;
  image.blit(0, 0);
  ApplyColorMask(color);
  FlipScreen();

  image.blit(0, 0);
}


function FadeFromColor(msecs, clr)
{
  var image = GrabImage(0, 0, GetScreenWidth(), GetScreenHeight());
  var color = CreateColor(clr.red, clr.green, clr.blue, clr.alpha);

  var time = GetTime();
  while (GetTime() - time < msecs)
  {
    color.alpha = 255 - (GetTime() - time) * 255 / msecs;
    image.blit(0, 0);
    ApplyColorMask(color);
    FlipScreen();
  }

  color.alpha = 0;
  image.blit(0, 0);
  ApplyColorMask(color);
  FlipScreen();

  image.blit(0, 0);
}
