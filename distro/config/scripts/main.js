// miniconfig 1.1: Configuration Utility for minisphere
// Copyright (C) 2015 Fat Cerberus

RequireSystemScript('mini/Console.js');
RequireSystemScript('mini/Link.js');
RequireSystemScript('mini/Scenes.js');
RequireSystemScript('mini/Threads.js');

EvaluateScript('MainMenu.js');

var VKEY_NAMES = [ 'up', 'down', 'left', 'right', 'a', 'b', 'x', 'y', 'menu' ];

var DEFAULT_KEY_MAP = [
	// Player 1
	{ up: KEY_UP, down: KEY_DOWN, left: KEY_LEFT, right: KEY_RIGHT,
		a: KEY_DELETE, b: KEY_END, x: KEY_INSERT, y: KEY_HOME,
		menu: KEY_ESCAPE },
	
	// Player 2
	{ up: KEY_W, down: KEY_S, left: KEY_A, right: KEY_D,
		a: KEY_F, b: KEY_E, x: KEY_Y, y: KEY_X,
		menu: KEY_ESCAPE },

	// Player 3
	{ up: KEY_I, down: KEY_K, left: KEY_J, right: KEY_L,
		a: KEY_H, b: KEY_U, x: KEY_N, y: KEY_M,
		menu: KEY_ESCAPE },

	// Player 4
	{ up: KEY_NUM_8, down: KEY_NUM_5, left: KEY_NUM_4, right: KEY_NUM_6,
		a: KEY_NUM_1, b: KEY_NUM_2, x: KEY_NUM_7, y: KEY_NUM_9,
		menu: KEY_ESCAPE },
]

var keyMap = [ {}, {}, {}, {} ];

function LoadKeyMap()
{
	var file = new File("minisphere.cfg");
	
	mini.Link(DEFAULT_KEY_MAP)
		.each(function(map, i)
	{
		for (var k in map) {
			keyMap[i][k] = file.read("keymap_Player" + (i + 1) + "_" + k.toUpperCase(),
				DEFAULT_KEY_MAP[i][k]);
		}
	});
	file.close();
	
	return keyMap;
}

function SaveKeyMap(keyMap)
{
	var file = new File("minisphere.cfg");
	mini.Link(keyMap)
		.each(function(playerMap, i)
	{
		for (var k in playerMap) {
			file.write("keymap_Player" + (i + 1) + "_" + k.toUpperCase(), playerMap[k]);
		}
	});
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
	SaveKeyMap(keyMap);
	
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
			handle = handle.toUpperCase();
			keyName = keyName.toUpperCase();
			if (!mini.Link(VKEY_NAMES).contains(handle.toLowerCase())) {
				mini.Console.write("No such button '" + handle + "'");
				return;
			} else if (!(keyConstName in engine)) {
				mini.Console.write([ "Key constant doesn't exist 'KEY_", keyName, "'" ].join(""));
			}
			playerNum = arguments.length >= 3 ? parseInt(playerID) - 1 : 0;
			var keyConstName = [ "KEY_", keyName ].join("");
			keyMap[playerNum][handle] = engine[keyConstName];
			mini.Console.write([ "Player ", playerNum + 1, "'s button ", handle, " mapped to ", keyConstName ].join(""));
			SaveKeyMap(keyMap);
		},
	});
	
	mini.Console.show();
	new MainMenu().run();
}
