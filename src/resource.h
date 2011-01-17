#ifndef RESOURCE_H
#define RESOURCE_H

#include <map>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/string.hpp>

const unsigned int max_num_resource_terrains = 4;

struct resource {
	std::string name;
	int food_bonus;
	int prod_bonus;
	int comm_bonus;
	int luxury_bonus;
	unsigned int needed_advance;
	unsigned int terrain[max_num_resource_terrains];
	unsigned int terrain_abundance[max_num_resource_terrains];
	bool allowed_on(unsigned int t) const;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & name;
		ar & food_bonus;
		ar & prod_bonus;
		ar & comm_bonus;
		ar & luxury_bonus;
		ar & needed_advance;
		ar & terrain;
		ar & terrain_abundance;
	}
};

typedef std::map<int, resource> resource_map;

#endif

