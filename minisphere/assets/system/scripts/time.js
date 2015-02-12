function Delay(milliseconds)
{
  var start = GetTime();
  while (start + milliseconds > GetTime()) {
  }
}
