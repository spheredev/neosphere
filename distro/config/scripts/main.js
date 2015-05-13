// miniconfig 1.1: Configuration Utility for minisphere
// Copyright (C) 2015 Fat Cerberus

RequireSystemScript('mini/Console.js');
RequireSystemScript('mini/Link.js');
RequireSystemScript('mini/Scenes.js');
RequireSystemScript('mini/Threads.js');

EvaluateScript('MainMenu.js');

var engine = this;

function game()
{
	SetFrameRate(60);
	mini.initialize({
		frameRate: 60,
		consolePrompt: "miniconfig:",
	});
	
	var keyMap = [ {}, {}, {}, {} ];
	
	mini.Console.register('key', null,
	{
		'set': function(handle, keyName, playerID)
		{
			playerNum = arguments.length >= 3 ? parseInt(playerID) - 1 : 0;
			handle = handle.toUpperCase();
			keyName = keyName.toUpperCase();
			var keyConstName = [ "KEY_", keyName.toString().toUpperCase() ].join("");
			if (keyConstName in engine) {
				keyMap[playerNum][handle] = keyConstName;
				mini.Console.write([ "Player ", playerNum + 1, "'s vkey `", handle, "` mapped to ", keyConstName ].join(""));
				mini.Console.write(JSON.stringify(keyMap));
			} else {
				mini.Console.write([ "Unknown key name `", keyName, "`" ].join(""));
			}
		},
	});
	
	new MainMenu().run();
}
