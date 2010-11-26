#include "coord.h"

coord::coord()
	: x(0),
	y(0)
{
}

coord::coord(int x_, int y_)
	: x(x_),
	y(y_)
{
}

bool coord::operator<(const coord& oth) const
{
	if(x < oth.x)
		return true;
	if(x > oth.x)
		return false;
	return y < oth.y;
}

bool coord::operator==(const coord& oth) const
{
	return x == oth.x && y == oth.y;
}

bool coord::operator!=(const coord& oth) const
{
	return !(*this == oth);
}

