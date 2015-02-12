// by rizen
// to be updated later


function TraceRectangle(x, y, w, h, c) {
  --w; --h;
  Line(x, y, x+w, y, c);
  Line(x, y, x, y+h, c);
  Line(x+w, y, x+w, y+h, c);
  Line(x, y+h, x+w, y+h, c);
}

function TracePoly(x, y, c) {
  for (i = 1; i<x.length; i++)
    Line(x[i - 1], y[i - 1], x[i], y[i], c);
  Line(x[x.length - 1], y[y.length - 1], x[0], y[0], c);
}

