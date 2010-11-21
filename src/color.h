#ifndef COLOR_H
#define COLOR_H

struct color {
	int r;
	int g;
	int b;
	color(int r_, int g_, int b_);
	bool operator<(const color& oth) const;
};

#endif

