// miniconfig 1.1: Configuration Utility for minisphere
// Copyright (C) 2015 Fat Cerberus

RequireSystemScript('mini/Console.js');
RequireSystemScript('mini/Link.js');
RequireSystemScript('mini/Scenes.js');
RequireSystemScript('mini/Threads.js');

EvaluateScript('MainMenu.js');

var engine = this;

var VKEY_NAMES = [ 'menu', 'up', 'down', 'left', 'right', 'a', 'b', 'x', 'y' ];
var VKEYS =	[
	PLAYER_KEY_MENU,
	PLAYER_KEY_UP, PLAYER_KEY_DOWN, PLAYER_KEY_LEFT, PLAYER_KEY_RIGHT,
	PLAYER_KEY_A, PLAYER_KEY_B, PLAYER_KEY_X, PLAYER_KEY_Y ];

function LoadKeyMap()
{
	keyMap = mini.Link.create(4, VKEYS.length, undefined);
	
	var file = new File("#~/../minisphere.conf");
	for (var i = 0; i < 4; ++i) {
		for (var j = 0; j < VKEYS.length; ++j) {
			keyMap[i][j] = file.read("keymap_Player" + (i + 1) + "_" + VKEY_NAMES[j].toUpperCase(),
				GetPlayerKey(i, VKEYS[j]));
		}
	}
	file.close();
	
	return keyMap;
}

function SaveKeyMap(keyMap)
{
	var file = new File("#~/../minisphere.conf");
	for (var i = 0; i < 4; ++i) {
		for (var j = 0; j < VKEYS.length; ++j) {
			file.write("keymap_Player" + (i + 1) + "_" + VKEY_NAMES[j].toUpperCase(), keyMap[i][j]);
		}
	}
	file.close();
}

function game()
{
	if (!mini.Link(GetExtensions())
		.contains('minisphere'))
	{
		Abort("miniconfig must be run under minisphere.");
	}
	
	var keyMap = LoadKeyMap();
	//SaveKeyMap(keyMap);
	
	SetFrameRate(60);
	mini.initialize({
		frameRate: 60,
		consolePrompt: "miniconfig:",
	});
	
	mini.Console.register('key', null,
	{
		'set': function(handle, keyName, playerID)
		{
			if (arguments.length < 2) {
				mini.Console.write("`key set` expects 2-3 arguments");
				return;
			}
			
			// check that arguments are valid
			handle = handle.toUpperCase();
			keyName = keyName.toUpperCase();
			var keyConstName = [ "KEY_", keyName ].join("");
			playerNum = arguments.length >= 3 ? parseInt(playerID) - 1 : 0;
			if (!mini.Link(VKEY_NAMES).contains(handle.toLowerCase())) {
				mini.Console.write("No such button '" + handle + "'");
				return;
			} else if (!(keyConstName in engine)) {
				mini.Console.write([ "Key constant doesn't exist 'KEY_", keyName, "'" ].join(""));
				return;
			}
			
			// map the key
			var keyID = engine["PLAYER_KEY_" + handle];
			keyMap[playerNum][keyID] = engine[keyConstName];
			SetPlayerKey(playerNum, keyID, engine[keyConstName]);
			mini.Console.write([ "Player ", playerNum + 1, "'s button ", handle, " mapped to ", keyConstName ].join(""));
			SaveKeyMap(keyMap);
		},
	});
	
	mini.Console.show();
	new MainMenu().run();
}
