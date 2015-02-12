/**
	FadeOut module
	
	By tunginobi
*/


/**
	Fade out the screen. Draw stuff in draw_func, and it lasts for duraction ms.
*/
function FadeOut(duration, draw_func)
{
	var start_time = GetTime();
	
	while (GetTime() < start_time + duration)
	{
		var state = (GetTime() - start_time) / duration;
		if (state > 1) state = 1;
		
		draw_func();
		Rectangle(0, 0, GetScreenWidth(), GetScreenHeight(), CreateColor(0, 0, 0, 255 * state));
		FlipScreen();
	}
	
	return;
}
