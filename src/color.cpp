#include "color.h"

color::color(int r_, int g_, int b_)
	: r(r_),
	g(g_),
	b(b_)
{
}

bool color::operator<(const color& oth) const
{
	if(r < oth.r)
		return true;
	if(r > oth.r)
		return false;
	if(g < oth.g)
		return true;
	if(g > oth.g)
		return false;
	return b < oth.b;
}

