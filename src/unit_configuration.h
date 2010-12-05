#ifndef UNIT_CONFIGURATION_H
#define UNIT_CONFIGURATION_H

#include <string>
#include <map>

class unit_configuration {
	public:
		std::string unit_name;
		unsigned int max_moves;
		bool settler;
		unsigned int production_cost;
		unsigned int max_strength;
		unsigned int needed_advance;
};

typedef std::map<int, unit_configuration> unit_configuration_map;

#endif
