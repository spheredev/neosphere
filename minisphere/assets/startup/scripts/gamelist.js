/**
	Sphere Game list object module
	
	By tunginobi
	
	
	Contains both the SphereGameList and the SphereGameListItem objects.
*/

const TITLE_AUTHOR_RATIO = 0.65;		// Adjust to taste (between 0 and 1 exclusive)
var GAME_EVEN_BG = CreateColor(0, 0, 128, 64);	// Colours! Glorious colours!
var GAME_ODD_BG = CreateColor(0, 0, 128, 128);
var GAME_SELECTED_BG = CreateColor(0, 64, 192);	// Alpha ignored, doesn't work.

RequireScript("movetext.js");


function SphereGameList ()
{
	if (this instanceof SphereGameList == false)
	{
		return new SphereGameList(list);
	}
	
	this.list = new Array();
	
	this.x = 8;
	this.y = 36;
	this.width = 300;
	this.height = 180;
	
	this.offset = 0;		// Higher number == lower parts of list visible
	this.oldOffset = 0;
	this.start = 0;
	this.duration = 300;
	
	this.sel = 0;
	this.active = true;
	
	this.rectSurface = undefined;	// Auto-initialised when needed.
	this.textSurface = undefined;	// Ain't that somethin'?
	this.textSurface2 = undefined;	// This too, for authors.
	
	return this;
}

SphereGameList.prototype.init = function (list)
{
	this.list = new Array();
	
	for (var i = 0; i < list.length; ++i)
	{
		var temp_item = new SphereGameListItem(list[i]);
		temp_item.x = 8;
		this.list.push(temp_item);
	}
	
	if (this.list.length < 1)
	{
		var temp_item = new SphereGameListItem({name: "<no games found>", directory: "", author: "", description: ""});
		temp_item.x = 8;
		this.list.push(temp_item);
	}
	
	var cut_off = Math.floor((this.width - 8) * TITLE_AUTHOR_RATIO);
	if (cut_off < 1) cut_off = 1;
	if (cut_off > this.width - 9) cut_off = this.width - 9;
	
	if (!this.rectSurface)
	{
		this.rectSurface = CreateSurface(this.width, this.height, CreateColor(0, 0, 0));
		this.rectSurface.setAlpha(192);	// No effect :(
		this.rectSurface.setBlendMode(REPLACE);
	}
	if (!this.textSurface)
	{
		this.textSurface = CreateSurface(cut_off, this.height, CreateColor(0, 0, 0, 0));
		this.textSurface.setBlendMode(REPLACE);
	}
	if (!this.textSurface2)
	{
		this.textSurface2 = CreateSurface((this.width - 8) - cut_off, this.height, CreateColor(0, 0, 0, 0));
		this.textSurface2.setBlendMode(REPLACE);
	}
	
	this.focusOnSel();
	
	return;
}

SphereGameList.prototype.fullHeight = function ()
{
	var h = 0;
	
	for (var i = 0; i < this.list.length; ++i)
	{
		h += this.list[i].font.getHeight();
	}
	
	return h;
}

SphereGameList.prototype.scrollTo = function (new_y)
{
	var state = (GetTime() - this.start) / this.duration;
	if (state > 1) state = 1;
	this.oldOffset += (this.offset - this.oldOffset) * state;
	
	this.offset = new_y;
	if (new_y + this.height > this.fullHeight()) this.offset = this.fullHeight() - this.height;
	if (this.offset < 0) this.offset = 0;
	this.start = GetTime();
	
	return;
}

SphereGameList.prototype.focusOnSel = function ()
{
	// Focus on the selection cursor.
/*	var cursor_offset = mainFont.getHeight() * (this.sel + 0.5);
	var target_offset = 0;
	if (cursor_offset < this.offset + this.height * 0.4) target_offset = cursor_offset - Math.floor(this.height * 0.4);
	if (cursor_offset > this.offset + this.height * 0.6) target_offset = cursor_offset - Math.floor(this.height * 0.6);
	
	if (target_offset != 0) this.scrollTo(target_offset);
*/	
	this.scrollTo(mainFont.getHeight() * (this.sel + 0.5) - this.height * 0.5);
	
	return;
}

SphereGameList.prototype.globalCursorFocus = function ()
{
	// Move the global cursor to the final position of the current selection.
	globalCursor.move(this.x + 6, this.y + Math.floor((this.sel + 0.5) * this.list[0].font.getHeight()) - this.offset);
	
	return;
}

/**
	Switch the selected game.
*/
SphereGameList.prototype.switchTo = function (which)
{
	this.list[this.sel].move(8);
	this.list[this.sel].selected = false;
	this.sel = which;
	while (this.sel < 0) this.sel += this.list.length;
	while (this.sel >= this.list.length) this.sel -= this.list.length;
	this.list[this.sel].move(16);
	this.list[this.sel].selected = true;
	
	this.focusOnSel();
	this.globalCursorFocus();
	
	return;
}

/**
	Fixified for sphere_dx8 and sphere_gl.
	Have a Fruity Oaty Good Time!(tm)
*/
SphereGameList.prototype.draw = function ()
{
	// Draw the game list in its current state
	//SetClippingRectangle(this.x, this.y, this.width, this.height);
	
	// Faint background
	//Rectangle(this.x, this.y, this.width, this.height, CreateColor(0, 0, 0, 64));
	this.rectSurface.rectangle(0, 0, this.rectSurface.width, this.rectSurface.height, CreateColor(0, 0, 0, 64));
	
	var actual_offset = 0;
	
	// Scroll bar! Hardcoded and stuff.
	var state = (GetTime() - this.start) / this.duration;
	if (state > 1) state = 1;
	actual_offset = this.oldOffset + Math.floor((this.offset - this.oldOffset) * state);
	var sb_height = this.height / this.fullHeight() * (this.height - 2);
	if (sb_height > this.height - 2) sb_height = this.height - 2;
	var sb_pos = actual_offset / (this.fullHeight() - this.height) * (this.height - 2 - sb_height);
	
	// The games!
	for (var i = Math.max(0, Math.floor(actual_offset / this.list[0].font.getHeight())), j = Math.ceil(this.height / this.list[0].font.getHeight()); i < this.list.length && j >= 0; ++i, --j)
	{
		//this.list[i].draw(this.x, this.y - actual_offset + this.list[i].font.getHeight() * i, i & 1);
		this.list[i].drawOn(this.textSurface, this.textSurface2, this.rectSurface, this.list[i].font.getHeight() * i - actual_offset, i & 1);
	}
	//SetClippingRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	
	var cut_off = Math.floor((this.width - 8) * TITLE_AUTHOR_RATIO);
	if (cut_off < 1) cut_off = 1;
	if (cut_off > this.width - 9) cut_off = this.width - 9;
	
	// Draw it all to screen
	this.rectSurface.blit(this.x, this.y);
	this.textSurface.blit(this.x, this.y);
	this.textSurface2.blit(this.x + cut_off, this.y);
	// Rest of the scrollbar code
	Rectangle(this.x + this.width - 8, this.y, 8, this.height, CreateColor(0, 0, 0, 128));
	Rectangle(this.x + this.width - 7, this.y + 1 + sb_pos, 6, sb_height, CreateColor(255, 255, 255, 255));
	
	return;
}


function SphereGameListItem (list_item)
{
	if (this instanceof SphereGameListItem == false)
	{
		return new SphereGameListItem(list_item);
	}
	
	this.name = list_item.name;
	this.directory = list_item.directory;
	this.author = list_item.author;
	this.description = list_item.description;
	
	this.font = GetSystemFont();
	this.selected = false;
	this.x = 0;
	this.oldX = 0;
	this.start = 0;
	this.duration = 200;
	
	return this;
}

/**
	Shows the game information subscreen, as well as the option to play.
*/
SphereGameListItem.prototype.select = function ()
{
	if (this.directory == "") return;
	
	var sphere_version = new MoveText(sphereIcon.width + 16, 0, sphereVersion);
	var sphere_logo = new MoveImage(0, 0, sphereIcon);
	
	var width = Math.min(Math.floor(SCREEN_WIDTH * 0.65), 300);
	var height = 120;
	var x = Math.floor((SCREEN_WIDTH - width) / 2);
	var y = Math.floor((SCREEN_HEIGHT - height) / 2);
	
	var sel = 0;
	var done = false;
	
	var txt_name = new MoveText(Math.floor((SCREEN_WIDTH - mainFont.getStringWidth(this.name)) / 2), y - mainFont.getHeight(), this.name);
	var txt_dir = new MoveText(x - mainFont.getStringWidth("Dir: " + this.directory + "/"), y + mainFont.getHeight(), "Dir: " + this.directory + "/");
	var txt_auth = new MoveText(x + width, y + mainFont.getHeight() * 2, "Author: " + this.author);
	var txt_desc = new MoveText(x, y + height, this.description);
	txt_desc.width = width;
	txt_desc.height = height - mainFont.getHeight() * 5;
	var txt_play = new MoveText(Math.floor((SCREEN_WIDTH - mainFont.getStringWidth("Play")) / 2), y + height, "Play");
	
	txt_name.move(txt_name.x, y);
	txt_dir.move(x, txt_dir.y);
	txt_auth.move(x, txt_auth.y);
	txt_desc.move(x, y + mainFont.getHeight() * 4);
	txt_play.move(txt_play.x, y + height - mainFont.getHeight());
	
	globalCursor.move(txt_play.x - 8, txt_play.y + 6);
	
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
			
			var old_rect = GetClippingRectangle();
			SetClippingRectangle(x, y, width, height);
				// Draw box
				Rectangle(x, y, width, height, CreateColor(0, 0, 0, 192));
				Rectangle(x, y, width, mainFont.getHeight(), CreateColor(0, 0, 0, 255));
				Rectangle(x, y + height - mainFont.getHeight(), width, mainFont.getHeight(), CreateColor(0, 0, 0, 255));
				
				// Draw text
				txt_name.draw();
				txt_dir.draw();
				txt_auth.draw();
				txt_desc.drawWrapped();
				txt_play.draw();
			SetClippingRectangle(old_rect.x, old_rect.y, old_rect.width, old_rect.height);
			
			globalCursor.draw();
			FlipScreen();
		}
		
		var inkey = GetKey();
		switch (inkey)
		{
			case KEY_SPACE:
			case KEY_ENTER:
				FadeOut(1000, function ()
					{
						background.blit(0, 0);
						DrawTickerBar();
						DrawClock();
						DrawSearch();
						sphere_version.draw();
						sphere_logo.draw();
						sphereGameList.draw();
						
						var old_rect = GetClippingRectangle();
						SetClippingRectangle(x, y, width, height);
							// Draw box
							Rectangle(x, y, width, height, CreateColor(0, 0, 0, 192));
							Rectangle(x, y, width, mainFont.getHeight(), CreateColor(0, 0, 0, 255));
							Rectangle(x, y + height - mainFont.getHeight(), width, mainFont.getHeight(), CreateColor(0, 0, 0, 255));
							
							// Draw text
							txt_name.draw();
							txt_dir.draw();
							txt_auth.draw();
							txt_desc.drawWrapped();
							txt_play.draw();
						SetClippingRectangle(old_rect.x, old_rect.y, old_rect.width, old_rect.height);
						
						globalCursor.draw();
					}
				);
				ExecuteGame(this.directory);
				break;
			case KEY_BACKSPACE:
			case KEY_ESCAPE:
				done = true;
				break;
		}
	}
	
	return;
}

/**
	Deprecated function. Was used, now isn't.
*/
SphereGameListItem.prototype.draw = function (x, y, odd)
{
	var old_clip = GetClippingRectangle();
	
	// Draw box
	if (this.selected)
	{
		Rectangle(old_clip.x, y, old_clip.width - 8, this.font.getHeight(), GAME_SELECTED_BG);
	}
	else
	{
		if (odd) Rectangle(old_clip.x, y, old_clip.width - 8, this.font.getHeight(), GAME_ODD_BG);
		else Rectangle(old_clip.x, y, old_clip.width - 8, this.font.getHeight(), GAME_EVEN_BG);
	}
	
	// Ratios: 8 pixels on right reserved for scrollbar. 65% Game name, 35% Author.
	
	// Draw game name
	var clip_y = y;
	if (y < old_clip.y) clip_y = old_clip.y;
	if (y + this.font.getHeight() > old_clip.y + old_clip.height) clip_y = old_clip.y + old_clip.height - this.font.getHeight();
	SetClippingRectangle(old_clip.x, clip_y, Math.floor((old_clip.width - 8) * 0.65) + x, this.font.getHeight());
	// Go! ->      ->     ->         ->        ->         ->          ->        ->    ^^^ WTF?!!!1one
	var state = (GetTime() - this.start) / this.duration;
	if (state > 1) state = 1;
	this.font.drawText(x + this.oldX + (this.x - this.oldX) * state, y, this.name);
	
	// Draw game author (the _name_ of the game's author :P)
	SetClippingRectangle(old_clip.x + Math.floor((old_clip.width - 8) * 0.65), clip_y, Math.floor((old_clip.width - 8) * 0.35), this.font.getHeight());
	this.font.drawText(x + old_clip.x + Math.floor((old_clip.width - 8) * 0.65) + 1, y, this.author);
	
	// Reset clipping rectangle
	SetClippingRectangle(old_clip.x, old_clip.y, old_clip.width, old_clip.height);
	
	return;
}

/**
	The new drawing function. Draws to surfaces.
*/
SphereGameListItem.prototype.drawOn = function (fg_srf, fg_srf2, bg_srf, y, odd)
{
	var h = this.font.getHeight();
	
	fg_srf.setBlendMode(REPLACE);
	fg_srf2.setBlendMode(REPLACE);
	
	// Clear relevant areas on surfaces (REPLACE mode)
	bg_srf.rectangle(0, y, bg_srf.width - 8, h, CreateColor(0, 0, 0, 0));
	fg_srf.rectangle(0, y, fg_srf.width, h, CreateColor(0, 0, 0, 0));
	fg_srf2.rectangle(0, y, fg_srf2.width, h, CreateColor(0, 0, 0, 0));
	
	fg_srf.setBlendMode(BLEND);
	fg_srf2.setBlendMode(BLEND);
	
	// Draw selection/background box
	if (this.selected)
	{
		fg_srf.rectangle(0, y, fg_srf.width, h, GAME_SELECTED_BG);
		fg_srf2.rectangle(0, y, fg_srf2.width, h, GAME_SELECTED_BG);
	}
	else if (odd) bg_srf.rectangle(0, y, bg_srf.width - 8, h, GAME_ODD_BG);
	else bg_srf.rectangle(0, y, bg_srf.width - 8, h, GAME_EVEN_BG);
	
	// Draw game name and author
	var state = (GetTime() - this.start) / this.duration;
	if (state > 1) state = 1;
	fg_srf.drawText(this.font, this.oldX + (this.x - this.oldX) * state, y, this.name);
	fg_srf2.drawText(this.font, 0, y, this.author);
	
	return;
}

SphereGameListItem.prototype.move = function (new_x)
{
	var state = (GetTime() - this.start) / this.duration;
	if (state > 1) state = 1;
	
	this.oldX += (this.x - this.oldX) * state;
	this.x = new_x;
	this.start = GetTime();
	
	return;
}
