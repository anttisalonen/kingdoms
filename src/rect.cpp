#include "rect.h"

rect::rect(int x_, int y_, int w_, int h_)
	: x(x_),
	y(y_),
	w(w_),
	h(h_)
{
}

bool in_rectangle(const rect& r, int x, int y)
{
	return x >= r.x && x <= (r.x + r.w) &&
		y >= r.y && y <= (r.y + r.h);
}


