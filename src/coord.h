#ifndef COORD_H
#define COORD_H

struct coord {
	coord();
	coord(int x_, int y_);
	int x;
	int y;
	bool operator<(const coord& oth) const;
	bool operator==(const coord& oth) const;
};

#endif
