#ifndef COLOR_H
#define COLOR_H

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

struct color {
	int r;
	int g;
	int b;
	color(int r_, int g_, int b_);
	color();
	bool operator<(const color& oth) const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & r;
		ar & g;
		ar & b;
	}
};

#endif

