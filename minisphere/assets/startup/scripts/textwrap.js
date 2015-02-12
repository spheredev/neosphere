/***************************************
Text Wrapper Module

Author: tunginobi
Version: 2005.06.05

Enables text to be character- or word-
wrapped, given either a character width,
or a font and a pixel width.

All functions return an array of
strings, each element of which is a
string representing each line of wrapped
text.

All functions can be filtered through
the ToLongString() function to convert
output into straight strings with line
breaks.
***************************************/

/***************************************
function WordWrapByFontWidth(
  <string> textToWrap,
  <SphereFont> font,
  <integer> pixelWidth
  )
Wraps textToWrap, according to the width
of its drawn representation with font.
Words are kept together.
***************************************/
function WordWrapByFontWidth(textToWrap, font, pixelWidth)
{
  var lines = new Array();
  var wordBuffer = "";
  var wsBuffer = "";
  var c = 0;
  
  lines[0] = "";
  
  // Process the entire string
  while (c < textToWrap.length)
  {
    // Grab a word and trailing whitespaces
    if (textToWrap.charAt(c) == '\n')
    {
      // Special catering for linefeeds
      wordBuffer = '\n';
      c++;
    }
    while (textToWrap.charAt(c) != ' ' && textToWrap.charAt(c) != '\t' && textToWrap.charAt(c) != '\n' && c < textToWrap.length)
    {
      wordBuffer += textToWrap.charAt(c);
      c++;
    }
    wsBuffer = "";
    while ((textToWrap.charAt(c) == ' ' || textToWrap.charAt(c) == '\t') && c < textToWrap.length)
    {
      wsBuffer += textToWrap.charAt(c);
      c++;
    }
    
    // Add the word to the lines
    while (wordBuffer != "")
    {
      if (wordBuffer == '\n')
      {
        // More special linefeed catering
        lines.push("");
        wordBuffer = "";
      }
      else
      {
        // The wrapping condition!!!
        if (font.getStringWidth(lines[lines.length - 1] + wordBuffer) <= pixelWidth)
        {
          lines[lines.length - 1] += wordBuffer;
          wordBuffer = "";
        }
        else
        {
          // If the line is already empty, fit as much of the word as we can
          if (lines[lines.length - 1] == "")
          {
            // Test the word and line char by char
            while (wordBuffer != "")
            {
              if (font.getStringWidth(lines[lines.length - 1] + wordBuffer.charAt(0)) <= pixelWidth)
              {
                lines[lines.length - 1] += wordBuffer.charAt(0);
                wordBuffer = wordBuffer.slice(1);
              }
              else
              {
                lines[lines.length] = "";
              }
            }
            
          }
          else
          {
            // Get over it and try the next line
            lines[lines.length] = "";
          }
        }
      }
    }
    // Add that whitespace... if it fits
    if (font.getStringWidth(lines[lines.length - 1] + wsBuffer) <= pixelWidth)
    {
      lines[lines.length - 1] += wsBuffer;
    }
    else
    {
      lines[lines.length] = "";
    }
  }
  
  return lines;
}

/***************************************
function WordWrapByStrLen(
  <string> textToWrap,
  <integer> charsPerLine
  )
Wraps textToWrap, with each line
containing at most charsPerLine
characters. Words are kept together.
***************************************/
function WordWrapByStrLen(textToWrap, charsPerLine)
{
  var lines = new Array();
  var wordBuffer = "";
  var wsBuffer = "";
  var c = 0;
  
  lines[0] = "";
  
  // Process the entire string
  while (c < textToWrap.length)
  {
    // Grab a word and trailing whitespaces
    if (textToWrap.charAt(c) == '\n')
    {
      // Special catering for linefeeds
      wordBuffer = '\n';
      c++;
    }
    while (textToWrap.charAt(c) != ' ' && textToWrap.charAt(c) != '\t' && textToWrap.charAt(c) != '\n' && c < textToWrap.length)
    {
      wordBuffer += textToWrap.charAt(c);
      c++;
    }
    wsBuffer = "";
    while ((textToWrap.charAt(c) == ' ' || textToWrap.charAt(c) == '\t') && c < textToWrap.length)
    {
      wsBuffer += textToWrap.charAt(c);
      c++;
    }
    
    // Add the word to the lines
    while (wordBuffer != "")
    {
      if (wordBuffer == '\n')
      {
        // More special linefeed catering
        lines.push("");
        wordBuffer = "";
      }
      else
      {
        // The wrapping condition!!!
        if ((lines[lines.length - 1] + wordBuffer).length <= charsPerLine)
        {
          lines[lines.length - 1] += wordBuffer;
          wordBuffer = "";
        }
        else
        {
          // If the line is already empty, fit as much of the word as we can
          if (lines[lines.length - 1] == "")
          {
            // Test the word and line char by char
            while (wordBuffer != "")
            {
              if ((lines[lines.length - 1] + wordBuffer.charAt(0)).length <= charsPerLine)
              {
                lines[lines.length - 1] += wordBuffer.charAt(0);
                wordBuffer = wordBuffer.slice(1);
              }
              else
              {
                lines[lines.length] = "";
              }
            }
          }
          else
          {
            // Get over it and try the next line
            lines[lines.length] = "";
          }
        }
      }
    }
    // Add that whitespace... if it fits
    if ((lines[lines.length - 1] + wsBuffer).length <= charsPerLine)
    {
      lines[lines.length - 1] += wsBuffer;
    }
    else
    {
      lines[lines.length] = "";
    }
  }
  
  return lines;
}

/***************************************
function CharWrapByFontWidth(
  <string> textToWrap,
  <SphereFont> font,
  <integer> pixelWidth
  )
Wraps textToWrap, according to
pixelWidth pixels when the string is
represented in font.
***************************************/
function CharWrapByFontWidth(textToWrap, font, pixelWidth)
{
  var lines = new Array();
  var buffer = "";
  var nextLine = false;
  var c = 0;
  
  while (c < textToWrap.length)
  {
    buffer = "";
    nextLine = false;
    while (!nextLine && c < textToWrap.length)
    {
      if ((font.getStringWidth(buffer + textToWrap.charAt(c)) <= pixelWidth && textToWrap.charAt(c) != '\n') || buffer == "")
      {
        buffer += textToWrap.charAt(c);
        c++;
      }
      else
      {
        nextLine = true;
        if (textToWrap.charAt(c) == '\n')
        {
          c++;
        }
      }
    }
    lines.push(buffer);
  }
  
  return lines;
}

/***************************************
function CharWrapByStrLen(
  <string> textToWrap,
  <integer> charsPerLine
  )
Wraps textToWrap, with each line
containing at most charsPerLine
characters.
***************************************/
function CharWrapByStrLen(textToWrap, charsPerLine)
{
  var lines = new Array();
  var buffer = "";
  var c = 0;
  
  while (c < textToWrap.length)
  {
    buffer = "";
    while ((buffer.length < charsPerLine && c < textToWrap.length && textToWrap.charAt(c) != '\n') || buffer == "")
    {
      buffer += textToWrap.charAt(c);
      c++;
    }
    if (textToWrap.charAt(c) == '\n')
    {
      c++;
    }
    lines.push(buffer);
  }
  
  return lines;
}

/***************************************
function ToLongString(
  <Array of string> linesArray
  )
Converts the linesArray into one long
string with linebreak characters.
***************************************/
function ToLongString(linesArray)
{
  var result_string = "";
  
  for (var i = 0; i < (linesArray.length - 1); i++)
  {
    result_string += linesArray[i] + '\n';
  }
  result_string += linesArray[linesArray.length - 1];
  
  return result_string;
}
