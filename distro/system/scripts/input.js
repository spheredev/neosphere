function GetString(x, y, font, max_chars)
{
  var background = GrabImage(0, 0, GetScreenWidth(), GetScreenHeight());
  
  var str = "";
  var cursor_position = 0;

  // No max given, default to 256
  if (typeof(max_chars) != "number") {
    max_chars = 256;
  }

  while (true) {
    
    background.blit(0, 0);
    font.drawText(x, y, str);
    font.drawText(x - 1 + font.getStringWidth(str.slice(0, cursor_position), cursor_position), y, (Math.sin(GetTime()>>8) > 0) ? "|" : " ");

    FlipScreen();
    
    while (AreKeysLeft()) {
    
      var key = GetKey();
      switch (key) {
      
        // done
        case KEY_ENTER: { 
          return str;
        }
        
        // backspace
        case KEY_BACKSPACE: {
          if (str != "") {
           str = str.slice(0, cursor_position - 1) + str.slice(cursor_position + 1);
          }
          if (cursor_position > 0)
            cursor_position -= 1;
          break;
        }

        // delete
        case KEY_DELETE : {
          if (str != "") {
           str = str.slice(0, cursor_position ) + str.slice(cursor_position +1 );
          }

          break;
        }

        case KEY_LEFT: {
           if (cursor_position > 0)
             cursor_position -= 1;
          break;
        }

        case KEY_RIGHT: {
          if (cursor_position <= str.length - 1)
            cursor_position += 1;
          break;
        }

        case KEY_HOME: {
          cursor_position = 0;
          break;
        }

        case KEY_END: {
          cursor_position = str.length;
          break;
        }        
        
        default: {
          var shift = IsKeyPressed(KEY_SHIFT);
          if (GetKeyString(key, shift) != "" && (str.length < max_chars)) {
            str = str.slice(0, cursor_position) + GetKeyString(key, shift) + str.slice(cursor_position);
            cursor_position += 1;
          }
        }
      } // end switch
      
    } // end while (keys left)

  } // end while (true)
}
