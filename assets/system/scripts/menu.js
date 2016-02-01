EvaluateSystemScript("colors.js");


function Menu()
{
  if (this instanceof Menu == false) {
    return new Menu();
  }

  // default properties
  this.font            = GetSystemFont()
  this.window_style    = GetSystemWindowStyle()
  this.arrow           = GetSystemArrow();
  this.up_arrow        = GetSystemUpArrow();
  this.down_arrow      = GetSystemDownArrow();
  this.escape_function = function() { }

  this.items = new Array();
}

// add item
Menu.prototype.addItem = function(name, callback, color) {

  if (color == undefined) {
    color = White;
  }

  var item = new Object;
  item.name     = name;
  item.callback = callback;
  item.color    = color;
  this.items[this.items.length] = item;
}

// execute
Menu.prototype.execute = function(x, y, w, h) {
  with (this) {
    var background = GrabImage(0, 0, GetScreenWidth(), GetScreenHeight());

    var text_height = font.getHeight();
    var shown_items = Math.floor(h / text_height);

    var selection = 0;
    var top_selection = 0;

    while (true) {
      // draw background
      background.blit(0, 0);

      // draw the window
      window_style.drawWindow(x, y, w, h);

      // draw the menu items
      for (var i = 0; i < shown_items; i++) {
        if (i < items.length) {
          font.setColorMask(Black);
          font.drawText(x + 16 + 1, y + i * text_height + 1, items[i + top_selection].name);
          font.setColorMask(items[i + top_selection].color);
          font.drawText(x + 16,     y + i * text_height,     items[i + top_selection].name);
        }
      }

      // draw the selection arrow
      arrow.blit(x, y + (selection - top_selection) * text_height);

      // draw the up and down arrows if necessary
      if (top_selection > 0) {
        up_arrow.blit(x + w - up_arrow.width, y);
      }
      if (top_selection + shown_items < items.length) {
        down_arrow.blit(x + w - down_arrow.width, y + text_height * shown_items - down_arrow.height);
      }

      FlipScreen();

      // handle keypresses
      while (AreKeysLeft()) {
        switch (GetKey()) {
          case KEY_ENTER: {
            var item = items[selection];
            item.callback();
            return;
          }

          case KEY_ESCAPE: {
            escape_function();
            return;
          }

          case KEY_DOWN: {
            if (selection < items.length - 1) {
              selection++;
              if (selection >= top_selection + shown_items) {
                top_selection++;
              }
            }
            break;
          }

          case KEY_UP: {
            if (selection > 0) {
              selection--;
              if (selection < top_selection) {
                top_selection--;
              }
            }
            break;
          }

        }
      } // end handle input
    }

  } // end with
}
