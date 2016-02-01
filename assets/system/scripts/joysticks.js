///////////////////////////////////////////////////////////
// A simple map engine joystick support script
// To use the following functions you must do:
// EvaluateSystemScript("joysticks.js");
//
// And make sure you do:
// SetUpdateScript("UpdateJoysticks()");
//
// And then to give input to the person called 'player':
// if (GetNumJoysticks() >= 1)
//   AttachJoystick(0, "player");
//
// Note: If you do AttachInput("player") this will not work
//
// To give input to the next player, just make sure there's enough joysticks for that person too.
// For example:
//
// if (GetNumJoysticks() >= 2) {
//   AttachJoystick(0, "player_one");
//   AttachJoystick(1, "player_two");
// }
//
//
// BindJoystickButton(joystick, joystick_button, on_press_script, on_release_script)
// i.e. 
// if (GetNumJoysticks() >= 1)
//   BindJoystickButton(0, 1, "mode = 'in'", "mode = 'out'")
//
// Will call "mode = 'in'" every time joystick zero pressed button one
// And call "mode = 'out'" when joystick zero lets go of button one
//
// Final Note:
//   I don't have a joystick, so don't lock me and others out of your games.
//   Also thank you to WIP for testing this
//
//  - Flik
///////////////////////////////////////////////////////////

var gPersonJoysticks = new Array();
var gBindedJoysticks = new Array();

///////////////////////////////////////////////////////////

function DoesPersonExist(person)
{
  if (IsMapEngineRunning()) {
    var person_list = GetPersonList();
    for (var i = 0; i < person_list.length; ++i) {
      if (person_list[i] == person) {
        return true;
      }
    }
  }
  return false;  
}

///////////////////////////////////////////////////////////

function GetNumJoystickPlayers() {
  var num = 0;
  for (var i = 0; i < gPersonJoysticks.length; i++) {
    if (gPersonJoysticks[i].destroy == false) {
      num += 1;
    }
  }
  return num;
}

///////////////////////////////////////////////////////////

function IsJoystickAttached(joystick) {
  if (joystick < 0 || joystick >= GetNumJoysticks())
  {
    Abort("Joystick " + joystick + " does not exist");
  }

  for (var i = 0; i < gPersonJoysticks.length; i++) {
    if (gPersonJoysticks[i].joystick == joystick)
      return (gPersonJoysticks[i].destroy == false);
  }

  return false;
}

///////////////////////////////////////////////////////////

function GetJoystickPerson(joystick) {
  if (joystick < 0 || joystick >= GetNumJoysticks())
  {
    Abort("Joystick " + joystick + " does not exist");
  }

  for (var i = 0; i < gPersonJoysticks.length; i++) {
    if (gPersonJoysticks[i].joystick == joystick)
      return gPersonJoysticks[i].name;
  }

  Abort("Joystick " + joystick + " is not attached");
}

///////////////////////////////////////////////////////////

function BindJoystickButton(joystick, joystick_button, on_press_script, on_release_script) {
  if (joystick < 0 || joystick >= GetNumJoysticks())
  {
    Abort("Joystick " + joystick + " does not exist");
  }

  if (joystick_button < 0 || joystick_button >= GetNumJoystickButtons(joystick)) {
    Abort("Joystick button " + joystick_button  + " does not exist");
  }

  var joystick_index = -1;
  var joystick_found = false;

  for (var i = 0; i < gBindedJoysticks.length; i++) {
    if (gBindedJoysticks[i].joystick == joystick) {
      joystick_index = i;
      joystick_found = true;
      break;
    }
  }

  if (joystick_found == false) {
    joystick_index = gBindedJoysticks.length;
    var binded_joystick = new Object();
        binded_joystick.joystick = joystick;
        binded_joystick.buttons  = new Array();
        

    gBindedJoysticks.push(binded_joystick);
  }

  var button_found = false;

  for (var k = 0; k < gBindedJoysticks[joystick_index].buttons.length; k++) {
    if (gBindedJoysticks[joystick_index].buttons[k].name == joystick_button) {
      gBindedJoysticks[joystick_index].buttons[k].on_press   = new Function(on_press_script);
      gBindedJoysticks[joystick_index].buttons[k].on_release = new Function(on_release_script);
      gBindedJoysticks[joystick_index].buttons[k].destroy = false;
      button_found = true;
      break;
    }
  }


  if (button_found == false) {
    var binded_button = new Object();
        binded_button.name = joystick_button;
        binded_button.on_press   = new Function(on_press_script);
        binded_button.on_release = new Function(on_release_script);
        binded_button.destroy = false;
    
    gBindedJoysticks[joystick_index].buttons.push(binded_button);
  }
}

///////////////////////////////////////////////////////////

function UnbindJoystickButton(joystick, joystick_button) {
  if (joystick < 0 || joystick >= GetNumJoysticks())
  {
    Abort("Joystick " + joystick + " does not exist");
  }

  if (joystick_button < 0 || joystick_button >= GetNumJoystickButtons(joystick)) {
    Abort("Joystick button " + joystick_button  + " does not exist");
  }

  for (var i = 0; i < gBindedJoysticks.length; i++) {
    if (gBindedJoysticks[i].joystick == joystick) {
      joystick_index = i;
      break;
    }
  }

  for (var k = 0; k < gBindedJoysticks[joystick_index].buttons.length; k++) {
    if (gBindedJoysticks[joystick_index].buttons[k].name == joystick_button) {
      gBindedJoysticks[joystick_index].buttons[k].destroy = true;
      break;
    }
  }
}

///////////////////////////////////////////////////////////

function AttachJoystick(joystick, name) {

  if (joystick < 0 || joystick >= GetNumJoysticks())
  {
    Abort("Joystick " + joystick + " does not exist");
  }

  if (!DoesPersonExist(name)) {
    Abort("Person " + name + " does not exist");
  }

  for (var i = 0; i < gPersonJoysticks.length; i++) {
    if (gPersonJoysticks[i].name == name) {
      gPersonJoysticks[i].joystick = joystick;
      return true;
    }
  }

  gPersonJoysticks[i] = new Object();
  gPersonJoysticks[i].name = name;
  gPersonJoysticks[i].joystick = joystick;

  return true;
}

///////////////////////////////////////////////////////////

function DetachJoystick(joystick, name) {

  if (joystick < 0 || joystick >= GetNumJoysticks()) {
    Abort("Joystick " + joystick + " does not exist");
  }

  if (!DoesPersonExist(name)) {
    Abort("Person " + name + " does not exist");
  }

  for (var i = 0; i < gPersonJoysticks.length; i++) {
    if (gPersonJoysticks[i].name == name) {
      gPersonJoysticks[i].destroy = true;
      return true;
    }
  }

}

///////////////////////////////////////////////////////////

function UpdateJoysticks() {
  UpdateBindedJoysticks();
  UpdatePersonJoysticks();

  for (var i = 0; i < gPersonJoysticks.length; i++) {
    if (gPersonJoysticks[i].destroy) {
      gPersonJoysticks.splice(i, 1);
      i -= 1;
    }
  }

  for (var j = 0; j < gBindedJoysticks.length; j++) {
    for (var k = 0; k < gBindedJoysticks[j].buttons.length; k++) {
      if (gBindedJoysticks[j].buttons[k].destroy) {
        gBindedJoysticks[j].buttons.splice(k, 1);
        k -= 1;
      }
    }
  }
}

///////////////////////////////////////////////////////////

function UpdateBindedJoysticks() {

  for (var j = 0; j < gBindedJoysticks.length; j++) {
    for (var k = 0; k < gBindedJoysticks[j].buttons.length; k++) {

      if (gBindedJoysticks[j].buttons[k].destroy)
        continue;

      if (IsJoystickButtonPressed(gBindedJoysticks[j].joystick, gBindedJoysticks[j].buttons[k].name)) {
        if (gBindedJoysticks[j].buttons[k].pressed == false) {
          gBindedJoysticks[j].buttons[k].pressed = true;
          gBindedJoysticks[j].buttons[k].on_press();
        }
      }
      else {
        if (gBindedJoysticks[j].buttons[k].pressed) {
          gBindedJoysticks[j].buttons[k].pressed = false;
          gBindedJoysticks[j].buttons[k].on_release();
        }
      }

    }
  }
}

///////////////////////////////////////////////////////////

function UpdatePersonJoysticks()
{
  for (var i = 0; i < gPersonJoysticks.length; i++) {

    if (!gPersonJoysticks[i].destroy && gPersonJoysticks[i].joystick >= 0 && gPersonJoysticks[i].joystick < GetNumJoysticks())
    {

      var dx = GetJoystickAxis(gPersonJoysticks[i].joystick, JOYSTICK_AXIS_X);
      var dy = GetJoystickAxis(gPersonJoysticks[i].joystick, JOYSTICK_AXIS_Y);

      dx = Math.round(dx * 10) / 10;
      dy = Math.round(dy * 10) / 10;

      if (dy < 0) QueuePersonCommand(gPersonJoysticks[i].name, COMMAND_MOVE_NORTH, true);
      if (dx > 0) QueuePersonCommand(gPersonJoysticks[i].name, COMMAND_MOVE_EAST,  true);
      if (dy > 0) QueuePersonCommand(gPersonJoysticks[i].name, COMMAND_MOVE_SOUTH, true);
      if (dx < 0) QueuePersonCommand(gPersonJoysticks[i].name, COMMAND_MOVE_WEST,  true);

      // set the direction
      var command = -1;

      if (dx < 0) {
        if (dy < 0) {
          command = COMMAND_FACE_NORTHWEST;
        } else if (dy > 0) {
          command = COMMAND_FACE_SOUTHWEST;
        } else {
          command = COMMAND_FACE_WEST;
        }
      } else if (dx > 0) {
        if (dy < 0) {
          command = COMMAND_FACE_NORTHEAST;
        } else if (dy > 0) {
          command = COMMAND_FACE_SOUTHEAST;
        } else {
          command = COMMAND_FACE_EAST;
        }
      } else {
        if (dy < 0) {
          command = COMMAND_FACE_NORTH;
        } else if (dy > 0) {
          command = COMMAND_FACE_SOUTH;
        }
      }

      if (command != -1)
        QueuePersonCommand(gPersonJoysticks[i].name, command, false);

    }
  }
}

///////////////////////////////////////////////////////////
