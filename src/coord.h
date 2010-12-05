#ifndef COORD_H
#define COORD_H

struct coord {
	coord();
	coord(int x_, int y_);
	int x;
	int y;
	bool operator<(const coord& oth) const;
	bool operator==(const coord& oth) const;
	bool operator!=(const coord& oth) const;
};

inline coord::coord()
	: x(0),
	y(0)
{
}

inline coord::coord(int x_, int y_)
	: x(x_),
	y(y_)
{
}

inline bool coord::operator<(const coord& oth) const
{
	if(x < oth.x)
		return true;
	if(x > oth.x)
		return false;
	return y < oth.y;
}

inline bool coord::operator==(const coord& oth) const
{
	return x == oth.x && y == oth.y;
}

inline bool coord::operator!=(const coord& oth) const
{
	return !(*this == oth);
}

#endif
