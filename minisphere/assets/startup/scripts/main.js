/**
	Startup System (compat)
	
	By tunginobi
	
	
	<cursor keys> navigate the menus.
	<home> and <end> go to the top and bottom of the games list.
	<page up> and <page down> go up and down a page-worth of games.
	
	<enter> is the confirm key (and sometimes <space> too).
	<esc> is the return/cancel key, and options menu key.
	
	Type <alphanumeric characters> to search.
	<backspace> will delete characters from search term.
	Search term is searched in both the title and author.
	Search is case-insensitive.
	
	This is *NOT* a proud example of my usual coding standards. :(
	
	
	@2006-02-27: Backgrounds now work under sphere_dx8 and sphere_gl.
	@2006-02-27: Clipping works correctly under all drivers.
	
	
	HAVE A FRUITY OATY GOOD TIME
*/

RequireScript("globalcursor.js");
RequireScript("gamelist.js");

RequireScript("phaseintext.js");
RequireScript("movetext.js");
RequireScript("bounceoutimage.js");
RequireScript("moveimage.js");
RequireScript("fadeout.js");
RequireScript("tickerbar.js");

var SCREEN_WIDTH = GetScreenWidth();
var SCREEN_HEIGHT = GetScreenHeight();
var COL_WHITE = CreateColor(255, 255, 255, 255);
var COL_LIGHT_GREY = CreateColor(192, 192, 192, 255);
var COL_DARK_GREY = CreateColor(128, 128, 128, 255);
var COL_BLACK = CreateColor(0, 0, 0, 255);

var mainFont = GetSystemFont();
var sphereIcon = LoadImage("sphereicon.png");
var sphereVersion = "Sphere " + GetVersionString();

var globalCursor = new GlobalCursor(0, 0);
globalCursor.duration = 200;
var tickerBarItems = ["Startup coded by tunginobi (2008-01-16)",
	"<up> and <down> scroll through games list",
	"<enter> and <space> confirms game play/info",
	"<esc> opens the options dialog",
	"<esc> and <backspace> cancel game play",
	"Type other keys to filter games list",
	"<home> and <end> select the 1st and last games",
	"<page up> and <page down> scroll games by pages",
	"HAVE A FRUITY OATY GOOD TIME",
	"Spheriki always has the latest Sphere version",
	"http://www.spheredev.org/",
	"SphereDev.net forums rock! :P",
	"http://www.spheredev.org/smforums/",
	"Visit my site for updates of this startup game",
	"http://tunginobi.spheredev.org/",
	"Many thanks to the guys over at the forums!"];
var tickerBar = new TickerBar(0, SCREEN_HEIGHT - mainFont.getHeight(), tickerBarItems);

var background;		// Sphere Surface object

var searchTerm = "";

var fullGameList = GetGameList();
var sortedGameList;		// Full game list, sorted
var filteredGameList;	// Game list, filtered to match searchTerm
var sphereGameList;		// Contains game list and presentation data

var sortByAuthor = false;	// If false, sorts by title, since it's the only other option.


function game()
{
	if (GetVersion() < 1.11)
		Abort("Sphere version outdated!\n\nGet the latest at\nhttp://www.spheredev.org/\n");
	PreRenderBackground();
	LogoIntro();
	
	GameMenu();
	
	return 0;
}


function LogoIntro()
{
	var start_time = GetTime();
	
	var sphere_logo = new BounceOutImage(Math.floor(SCREEN_WIDTH / 2), Math.floor((SCREEN_HEIGHT - sphereIcon.height) / 2), sphereIcon);
	var sphere_version = new PhaseInText(Math.floor((SCREEN_WIDTH - mainFont.getStringWidth(sphereVersion)) / 2), sphere_logo.y + sphereIcon.height, sphereVersion);
	
/*	GradientRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COL_LIGHT_GREY, COL_DARK_GREY, COL_BLACK, COL_DARK_GREY);
	var temp_back = GrabSurface(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
*/	
	// DEBUG
/*	temp_back.save("bgi_" + SCREEN_WIDTH + "_" + SCREEN_HEIGHT + ".png");
	Abort("bgi_" + SCREEN_WIDTH + "_" + SCREEN_HEIGHT + ".png");
*/	
	// Load the intro background
	var temp_back = LoadSurface("bgi_" + SCREEN_WIDTH + "_" + SCREEN_HEIGHT + ".png");
	
	sphere_logo.duration = 500;
	
	while (GetTime() < start_time + 750)
	{
		var state = (GetTime() - start_time) / 750;
		if (state > 1) state = 1;
		Rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COL_BLACK);
		temp_back.setAlpha(255 * state);
		temp_back.blit(0, 0);
		FlipScreen();
	}
	
	start_time = GetTime();
	sphere_version.begin();
	sphere_logo.begin();
	globalCursor.move(sphere_version.x - 8, sphere_version.y + Math.floor(sphere_version.font.getHeight() / 2));
	while (GetTime() < start_time + 2000)
	{
		temp_back.blit(0, 0);
		sphere_version.draw();
		sphere_logo.draw();
		globalCursor.draw();
		FlipScreen();
	}
	
	var sphere_logo = new MoveImage(sphere_logo.x - Math.floor(sphere_logo.image.width / 2), sphere_logo.y - Math.floor(sphere_logo.image.height / 2), sphere_logo.image);
	var sphere_version = new MoveText(sphere_version.x, sphere_version.y, sphere_version.text);
	sphere_logo.move(0, 0);
	sphere_version.move(sphere_logo.image.width + 16, 0);
	
	var search_prompt = new MoveText(SCREEN_WIDTH, mainFont.getHeight(), "Search: \"|\"");
	var fake_clock = new MoveText(SCREEN_WIDTH, 0, "");
	{
		var now = new Date();
		var secStr = (now.getSeconds() < 10 ? "0" : "") + now.getSeconds();
		var minStr = (now.getMinutes() < 10 ? "0" : "") + now.getMinutes();
		var hrsStr = (now.getHours() < 10 ? "0" : "") + now.getHours();
		fake_clock.text = hrsStr + ":" + minStr + ":" + secStr;
	}
	fake_clock.move(SCREEN_WIDTH - fake_clock.font.getStringWidth(hrsStr + ":" + minStr + ":" + secStr), 0);
	search_prompt.move(sphereIcon.width + 16, mainFont.getHeight());
	
	start_time = GetTime();
	while (GetTime() < start_time + 1500)
	{
		temp_back.blit(0, 0);
		var state = (GetTime() - start_time) / 1500;
		if (state > 1) state = 1;
		background.setAlpha(255 * state);
		background.blit(0, 0);
		fake_clock.draw();
		search_prompt.draw();
		sphere_version.draw();
		sphere_logo.draw();
		globalCursor.draw();
		FlipScreen();
	}
	
	return;
}


function GameMenu ()
{
	var quit_menu = false;
	var sphere_version = new MoveText(sphereIcon.width + 16, 0, sphereVersion);
	var sphere_logo = new MoveImage(0, 0, sphereIcon);
	
	// Initialises game list
	SortGameList();
	
	while (AreKeysLeft()) GetKey();
	
	// Initialise ticker bar
	tickerBar.duration = 10000;
	tickerBar.init();
	
	while (!quit_menu)
	{
		while (!AreKeysLeft())
		{
			background.blit(0, 0);
			DrawTickerBar();
			DrawClock();
			DrawSearch();
			sphere_version.draw();
			sphere_logo.draw();
			sphereGameList.draw();
			
			globalCursor.draw();
			
			FlipScreen();
		}
		
		var inkey = GetKey();
		var shifted = IsKeyPressed(KEY_SHIFT);
		switch (inkey)
		{
			case KEY_UP:
				// Move up games list
				sphereGameList.switchTo(sphereGameList.sel - 1);
				break;
			case KEY_DOWN:
				// Move down games list
				sphereGameList.switchTo(sphereGameList.sel + 1);
				break;
			case KEY_PAGEUP:
				// Move up a page in games list.
				sphereGameList.switchTo(Math.max(sphereGameList.sel - Math.ceil(sphereGameList.height / mainFont.getHeight()), 0));
				break;
			case KEY_PAGEDOWN:
				// Move down a page in games list.
				sphereGameList.switchTo(Math.min(sphereGameList.sel + Math.ceil(sphereGameList.height / mainFont.getHeight()), sphereGameList.list.length - 1));
				break;
			case KEY_SPACE:
			case KEY_ENTER:
				// Select the current game for more info
				sphereGameList.list[sphereGameList.sel].select();
				sphereGameList.switchTo(sphereGameList.sel);
				break;
			case KEY_ESCAPE:
				// Options menu
				quit_menu = OptionsMenu();
				sphereGameList.switchTo(sphereGameList.sel);
				break;
			case KEY_HOME:
				// Move to beginning of list
				sphereGameList.switchTo(0);
				break;
			case KEY_END:
				// Move to end of list
				sphereGameList.switchTo(sphereGameList.list.length - 1);
				break;
			default:
				// Input of some kind, I suppose
				SearchKey(inkey, shifted);
				break;
		}
	}
	
	return;
}


/**
	Handles keys for the search term. key is the keyboard scancode.
*/
function SearchKey (key, shift)
{
	var search_term_changed = false;
	var search_maximum = 24;
	var real_key = GetKeyString(key, shift);
	
	if (searchTerm.length < search_maximum)
	{
		if (real_key == "")
		{
			// Probably a control key of some sort. Handle appropriately.
			switch (key)
			{
				case KEY_BACKSPACE:
					if (searchTerm != "")
					{
						searchTerm = searchTerm.slice(0, searchTerm.length - 1);
						search_term_changed = true;
					}
					break;
			}
		}
		else
		{
			// Append key to search term
			searchTerm += real_key;
			search_term_changed = true;
		}
	}
	else if (key == KEY_BACKSPACE)
	{
		// Special handling for backspacing max length strings
		if (searchTerm != "")
		{
			searchTerm = searchTerm.slice(0, searchTerm.length - 1);
			search_term_changed = true;
		}
	}
	
	if (search_term_changed)
	{
		// Search again
		FilterGameList();
	}
	
	return;
}


function OptionsMenu ()
{
	var sphere_version = new MoveText(sphereIcon.width + 16, 0, sphereVersion);
	var sphere_logo = new MoveImage(0, 0, sphereIcon);
	
	var quit_menu = false;
	var done = false;
	var sel = 0;
	
	var menu_title = "Options";
	var menu_title_bg = CreateColor(0, 0, 0, 255);
	var menu_bg = CreateColor(0, 0, 0, 192);
	var menu_strings = ["Sort by title", "Sort by author", "320 * 240", "640 * 480", "Exit"];
	var menu_x = Math.floor((SCREEN_WIDTH - 112) / 2);
	var menu_y = Math.floor((SCREEN_HEIGHT - mainFont.getHeight() * menu_strings.length) / 2);
	
	var menu_items = new Array(menu_strings.length);
	for (var i = 0; i < menu_strings.length; ++i)
	{
		menu_items[i] = new MoveText(menu_x + 8, menu_y + mainFont.getHeight() * i, menu_strings[i]);
		menu_items[i].duration = 200;
	}
	
	menu_items[0].move(menu_x + 16, menu_y);
	globalCursor.move(menu_x + 6, menu_y + 6);
	
	while (!done)
	{
		while (!AreKeysLeft())
		{
			background.blit(0, 0);
			DrawTickerBar();
			DrawClock();
			DrawSearch();
			sphere_version.draw();
			sphere_logo.draw();
			sphereGameList.draw();
			
			// Menu title
			Rectangle(menu_x, menu_y - mainFont.getHeight(), 112, mainFont.getHeight(), menu_title_bg);
			mainFont.drawText(Math.floor((SCREEN_WIDTH - mainFont.getStringWidth(menu_title)) / 2), menu_y - mainFont.getHeight(), menu_title);
			
			// Menu background
			var old_rect = GetClippingRectangle();
			SetClippingRectangle(menu_x, menu_y, 112, mainFont.getHeight() * menu_items.length);
			Rectangle(menu_x, menu_y, 112, mainFont.getHeight() * menu_items.length, menu_bg);
			for (var i = 0; i < menu_items.length; ++i) menu_items[i].draw();
			SetClippingRectangle(old_rect.x, old_rect.y, old_rect.width, old_rect.height);
			
			globalCursor.draw();
			
			FlipScreen();
		}
		
		var inkey = GetKey();
		var old_sel = sel;
		switch (inkey)
		{
			case KEY_UP:
				sel--;
				if (sel < 0) sel = menu_items.length - 1;
				menu_items[old_sel].move(menu_x + 8, menu_y + mainFont.getHeight() * old_sel);
				menu_items[sel].move(menu_x + 16, menu_y + mainFont.getHeight() * sel);
				globalCursor.move(menu_x + 6, menu_y + 6 + mainFont.getHeight() * sel);
				break;
			case KEY_DOWN:
				sel++;
				if (sel >= menu_items.length) sel = 0;
				menu_items[old_sel].move(menu_x + 8, menu_y + mainFont.getHeight() * old_sel);
				menu_items[sel].move(menu_x + 16, menu_y + mainFont.getHeight() * sel);
				globalCursor.move(menu_x + 6, menu_y + 6 + mainFont.getHeight() * sel);
				break;
			case KEY_SPACE:
			case KEY_ENTER:
				switch (sel)
				{
					case 0:		// Sort games by title
						sortByAuthor = false;
						SortGameList();
						done = true;
						break;
					case 1:		// Sort games by author
						sortByAuthor = true;
						SortGameList();
						done = true;
						break;
					case 2:		// 320 * 240
						SwitchResolution(320, 240);
						break;
					case 3:		// 640 * 480
						SwitchResolution(640, 480);
						break;
					case 4:		// Exit
						FadeOut(1000, function ()
							{
								background.blit(0, 0);
								DrawClock();
								DrawSearch();
								sphere_version.draw();
								sphere_logo.draw();
								sphereGameList.draw();
								
								// Menu title
								Rectangle(menu_x, menu_y - mainFont.getHeight(), 112, mainFont.getHeight(), menu_title_bg);
								mainFont.drawText(Math.floor((SCREEN_WIDTH - mainFont.getStringWidth(menu_title)) / 2), menu_y - mainFont.getHeight(), menu_title);
								
								// Menu background
								var old_rect = GetClippingRectangle();
								SetClippingRectangle(menu_x, menu_y, 112, mainFont.getHeight() * menu_items.length);
								Rectangle(menu_x, menu_y, 112, mainFont.getHeight() * menu_items.length, menu_bg);
								for (var i = 0; i < menu_items.length; ++i) menu_items[i].draw();
								SetClippingRectangle(old_rect.x, old_rect.y, old_rect.width, old_rect.height);
								
								globalCursor.draw();
							}
						);
						done = true;
						quit_menu = true;
						break;
				}
				break;
			case KEY_ESCAPE:
				done = true;
				quit_menu = false;
				break;
		}
	}
	
	return quit_menu;
}


function PreRenderBackground ()
{
/*	// Draw the main background
	GradientRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COL_LIGHT_GREY, COL_DARK_GREY, COL_BLACK, COL_DARK_GREY);
	
	// Draw the top bars
	GradientRectangle(0, 0, SCREEN_WIDTH, mainFont.getHeight(), COL_LIGHT_GREY, COL_LIGHT_GREY, COL_DARK_GREY, COL_DARK_GREY);
	GradientRectangle(0, mainFont.getHeight(), SCREEN_WIDTH, mainFont.getHeight(), COL_LIGHT_GREY, COL_LIGHT_GREY, COL_DARK_GREY, COL_DARK_GREY);
	
	// Clock space
	Rectangle(SCREEN_WIDTH - mainFont.getStringWidth(" 00:00:00"), 0, mainFont.getStringWidth(" 00:00:00"), mainFont.getHeight(), CreateColor(0, 0, 0, 32));
	
	// Draw the Sphere icon box
	GradientRectangle(0, 0, sphereIcon.width, sphereIcon.height, COL_LIGHT_GREY, COL_LIGHT_GREY, COL_DARK_GREY, COL_DARK_GREY);
	
	// Draw the bottom bar
	Rectangle(0, SCREEN_HEIGHT - mainFont.getHeight(), SCREEN_WIDTH, mainFont.getHeight(), CreateColor(0, 0, 0, 128));
	
	// Store it and clear it
	background = GrabSurface(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	Rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COL_BLACK);
*/	
	// DEBUG
/*	background.save("bg_" + SCREEN_WIDTH + "_" + SCREEN_HEIGHT + ".png");
	Abort("bg_" + SCREEN_WIDTH + "_" + SCREEN_HEIGHT + ".png");
*/	
	
	// Load the appropriate background instead
	background = LoadSurface("bg_" + SCREEN_WIDTH + "_" + SCREEN_HEIGHT + ".png");
	
	return;
}


/**
	Second tier in game list. Sorts games according to sorting options.
*/
function SortGameList ()
{
	// Sub-function. Sorts strings, but ignores case.
	function SortIgnoreCase(a, b)
	{
		var ua = a.toUpperCase();
		var ub = b.toUpperCase();
		var char_pos = 0;
		var max_chars = 0;
		var match = true;
		
		if (ua == ub) return 0;
		
		max_chars = Math.min(ua.length, ub.length);
		while (match && (char_pos < max_chars))
		{
			match = (ua.charAt(char_pos) == ub.charAt(char_pos));
			++char_pos;
		}
		
		if (match)
		{
			// All characters match, so the longer one goes last
			if (ua.length > ub.length)
			{
				return 1;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			// Discrepant character.
			--char_pos;
			if (ua.charAt(char_pos) > ub.charAt(char_pos))
			{
				return 1;
			}
			else
			{
				return -1;
			}
		}
	}
	function SortByAuthor (a, b)
	{
		//return SortIgnoreCase(a.author, b.author);
		var ua = a.author.toUpperCase();
		var ub = b.author.toUpperCase();
		var char_pos = 0;
		var max_chars = 0;
		var match = true;
		
		if (ua == ub) return SortIgnoreCase(a.name, b.name);
		
		max_chars = Math.min(ua.length, ub.length);
		while (match && (char_pos < max_chars))
		{
			match = (ua.charAt(char_pos) == ub.charAt(char_pos));
			++char_pos;
		}
		
		if (match)
		{
			// All characters match, so the longer one goes last
			if (ua.length > ub.length)
			{
				return 1;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			// Discrepant character.
			--char_pos;
			if (ua.charAt(char_pos) > ub.charAt(char_pos))
			{
				return 1;
			}
			else
			{
				return -1;
			}
		}
	}
	function SortByTitle (a, b)
	{
		//return SortIgnoreCase(a.name, b.name);
		var ua = a.name.toUpperCase();
		var ub = b.name.toUpperCase();
		var char_pos = 0;
		var max_chars = 0;
		var match = true;
		
		if (ua == ub) return SortIgnoreCase(a.author, b.author);
		
		max_chars = Math.min(ua.length, ub.length);
		while (match && (char_pos < max_chars))
		{
			match = (ua.charAt(char_pos) == ub.charAt(char_pos));
			++char_pos;
		}
		
		if (match)
		{
			// All characters match, so the longer one goes last
			if (ua.length > ub.length)
			{
				return 1;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			// Discrepant character.
			--char_pos;
			if (ua.charAt(char_pos) > ub.charAt(char_pos))
			{
				return 1;
			}
			else
			{
				return -1;
			}
		}
	}
	
	
	sortedGameList = new Array();
	for (var i = 0; i < fullGameList.length; ++i)
	{
		sortedGameList.push(fullGameList[i]);
	}
	
	// Sort the game list.
	if (sortByAuthor)
	{
		sortedGameList.sort(SortByAuthor);
	}
	else
	{
		sortedGameList.sort(SortByTitle);
	}
	
	// Filter too.
	FilterGameList();
	
	return;
}


/**
	Final tier in game list. Filters games according to the search term.
*/
function FilterGameList ()
{
	// Filter the game list according to searchTerm
	filteredGameList = new Array();
	for (var i = 0; i < sortedGameList.length; ++i)
	{
		if (searchTerm == "" ||
			sortedGameList[i].name.toUpperCase().indexOf(searchTerm.toUpperCase()) >= 0 ||
			sortedGameList[i].author.toUpperCase().indexOf(searchTerm.toUpperCase()) >= 0)
		{
			filteredGameList.push(sortedGameList[i]);
		}
	}
	
	// Save old selection if possible
	var old_sel = 0;
	var old_off = 0;
	var old_sta = 0;
	if (sphereGameList)
	{
		old_sel = sphereGameList.sel;
		old_off = sphereGameList.oldOffset;
		old_sta = sphereGameList.start;
	}
	
	// Sort out games list
	sphereGameList = new SphereGameList();
	sphereGameList.width = SCREEN_WIDTH - 32;
	sphereGameList.height = SCREEN_HEIGHT - 64;
	//sphereGameList.height = 60;
	sphereGameList.x = 16;
	sphereGameList.y = 36;
	sphereGameList.init(filteredGameList);
	
	// Check to see that select game is in viewing bounds
	if (old_sel >= sphereGameList.list.length) old_sel = sphereGameList.list.length - 1;
	sphereGameList.switchTo(old_sel);
	sphereGameList.oldOffset = old_off;
	sphereGameList.start = old_sta;
	
	return;
}


function SwitchResolution(width, height)
{
	var proj = OpenFile("../game.sgm");
	proj.write("screen_height", height);
	proj.write("screen_width", width);
	proj.flush();
	proj.close();
	RestartGame();
	
	return;
}


function DrawClock ()
{
	var now = new Date();
	
	var secStr = (now.getSeconds() < 10 ? "0" : "") + now.getSeconds();
	var minStr = (now.getMinutes() < 10 ? "0" : "") + now.getMinutes();
	var hrsStr = (now.getHours() < 10 ? "0" : "") + now.getHours();
	
	if (now.getMilliseconds() < 500)
	{
		mainFont.drawText(SCREEN_WIDTH - mainFont.getStringWidth("00:00:00"), 0, hrsStr);
		mainFont.drawText(SCREEN_WIDTH - mainFont.getStringWidth("00:00:"), 0, ":");
		mainFont.drawText(SCREEN_WIDTH - mainFont.getStringWidth("00:00"), 0, minStr);
		mainFont.drawText(SCREEN_WIDTH - mainFont.getStringWidth("00:"), 0, ":");
		mainFont.drawText(SCREEN_WIDTH - mainFont.getStringWidth("00"), 0, secStr);
	}
	else
	{
		mainFont.drawText(SCREEN_WIDTH - mainFont.getStringWidth("00:00:00"), 0, hrsStr);
		mainFont.drawText(SCREEN_WIDTH - mainFont.getStringWidth("00:00"), 0, minStr);
		mainFont.drawText(SCREEN_WIDTH - mainFont.getStringWidth("00"), 0, secStr);
	}
}


function DrawSearch ()
{
	var now = new Date();
	
	mainFont.drawText(sphereIcon.width + 16, mainFont.getHeight(), "Search: \"" + searchTerm);
	mainFont.drawText(sphereIcon.width + 16 + mainFont.getStringWidth("Search: \"" + searchTerm + "|"), mainFont.getHeight(), "\"");
	
	if (now.getMilliseconds() >= 500)
	{
		mainFont.drawText(sphereIcon.width + 16 + mainFont.getStringWidth("Search: \"" + searchTerm), mainFont.getHeight(), "|");
	}
	
	return;
}


function DrawTickerBar ()
{
	tickerBar.update();
	tickerBar.draw();
	
	return;
}
