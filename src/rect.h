#ifndef RECT_H
#define RECT_H

struct rect {
	rect(int x_, int y_, int w_, int h_);
	int x;
	int y;
	int w;
	int h;
};

bool in_rectangle(const rect& r, int x, int y);

#endif

