#ifndef COORD_H
#define COORD_H

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

struct coord {
	coord();
	coord(int x_, int y_);
	int x;
	int y;
	bool operator<(const coord& oth) const;
	bool operator==(const coord& oth) const;
	bool operator!=(const coord& oth) const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & x;
		ar & y;
	}
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
