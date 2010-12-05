#ifndef CITY_IMPROVEMENT_H
#define CITY_IMPROVEMENT_H

#include <string>
#include <map>

class city_improvement {
	public:
		city_improvement();
		unsigned int improv_id;
		std::string improv_name;
		int cost;
		bool barracks;
		unsigned int needed_advance;
};

typedef std::map<unsigned int, city_improvement> city_improv_map;

#endif
