// Draws a textbox with faded text

// v 1 Beta

/* ============== FADE MESSA'! ================= */
/* My first system scriptie, hope it works ?!    */
/* ============================================= */
/* - Dragonumber7				 */

// How to use this(how it should work!)
// ---
// EvaluateSystemScript("fademessage.js");
// setWindowBehevior("myfont.stuff", "mywindowstyle.stuff");
// setColours(CreateColor(0,0,0,50), CreateColor(0,0,0,50), CreateColor(0,0,0,50), CreateColor(0,0,0,50), CreateColor(0,0,0,50));
// str = Array(3);
// str[0] = "Hello world!";
// str[1] = "The quick brown fox jumped over the lazy old uuuh";
// str[2] = "                           - Press a";
// drawCenter(str, 20, "ScrollUp");

/* Quick demo
function game()
{
 EvaluateSystemScript("fademessage.js");
 setWindowBehevior("RM2000.rfn", "menu.rws");
 setColours(CreateColor(255,255,255,250), CreateColor(100,100,100,50), CreateColor(255,255,255,50), CreateColor(100,100,100,250), CreateColor(255,255,255,255));
 str = Array(3);
 str[0] = "Hello world!";
 str[1] = "The quick brown fox jumped over the lazy old uuuh";
 str[2] = "                           - Press space";
 str[3] = "--------------------------------------------------"
 drawCenter(str, 10, "ScrollDown");
}
*/

// *** DECLARATION *** //
var fonte;
var windowse;
c1 = CreateColor(255,0,0,120);
c2 = CreateColor(100,0,0,120);
c3 = CreateColor(255,0,0,120);
c4 = CreateColor(100,0,0,120);
tcol = CreateColor(100,100,0,120);

// *** THE CODE *** //

function setWindowBehevior(font, windows)
{
	fonte = font;
	windowse = windows;
}

function setColours(ac1, ac2, ac3, ac4, atcol)
{
	c1 = ac1;
	c2 = ac2;
	c3 = ac3;
	c4 = ac4;
	tcol = atcol;
}	

function drawMessage(x,y,strings, speed, prop)
{
	SetFrameRate(speed);
	font = LoadFont(fonte);
	max_x = 0;
	wh = 0;
	pressed = 0;
	buffed = GrabImage(0, 0, GetScreenWidth(), GetScreenHeight());
	for(u2 = 0; u2 < strings.length; u2++)
	{
		max_xn = strings[u2].length * 7;
		if (max_xn > max_x)
		{
			max_x = max_xn;
		}
	}
	max_y = strings.length * 12;
	for(u = 0; u < strings.length; u++)
	{
		buffed.blit(0,0);
		for(t = 0; t < strings[u].length; t++)
		{
			buffed.blit(0,0);
			drawBox(x,y,max_x,max_y);
			font.setColorMask(CreateColor(tcol.red,tcol.green,tcol.blue, (strings[u].length/255 * 100)*t));
			font.drawText(x + (t * 7) ,y  + (u * 12),strings[u][t]);
			font.setColorMask(CreateColor(tcol.red,tcol.green,tcol.blue,255));
			for(i = 0; i < u; i++)
			{
				for(o = 0; o < strings[i].length; o++)
				{
					font.drawText(x + (o * 7), y + (i * 12), strings[i][o]);
				}
			}
			font.setColorMask(CreateColor(0,0,0, (strings[u].length/255 * 100)*t));
			for(t2 = 0; t2 < t; t2++)
			{
				swing = t + (t/2) - (t/3);
				font.setColorMask(CreateColor(tcol.red,tcol.green,tcol.blue, (strings[u].length/255 * 100)*(t2+swing)));
				font.drawText(x + (t2 * 7) ,y + (u * 12) ,strings[u][t2]);
			}
			FlipScreen();
			if(IsKeyPressed(KEY_SPACE))
			{
				break;
			}
		}
	}
	while(GetKey() != KEY_SPACE)
	{
		//POOP!		
	}
	SetFrameRate(65);
	if(prop == "ScrollUp")
	{
		while(max_y > 4)
		{
			drawBox(x,y, max_x, max_y);
			max_y = max_y - 1;
			y = y + 1;
			FlipScreen();
		}
	}
	if(prop == "ScrollDown")
	{
		while(max_y > 4)
		{
			drawBox(x,y, max_x, max_y);
			max_y = max_y - 1;
			y = y - 1;
			FlipScreen();
		}
	}
	//More candy here
	return true; // Why god, why?! (if done :O)
}

function drawBox(x,y,width,height)
{
	windo = LoadWindowStyle(windowse);
	windo.drawWindow(x, y, width, height + 1);
	GradientRectangle(x, y, width, height + 1, c1,c2,c3,c4);
}

function drawCenter(strings, speed, prop)
{
	max_x = 0;
	for(u2 = 0; u2 < strings.length; u2++)
	{
		max_xn = strings[u2].length * 7;
		if (max_xn > max_x)
		{
			max_x = max_xn;
		}
	}
	max_y = strings.length * 12;
	x = (GetScreenWidth()/2) - (max_x/2);
	y = (GetScreenHeight()/2) - (max_y/2);
	drawMessage(x,y,strings,speed,prop);
}