#ifndef CITY_H
#define CITY_H

#include <string>
#include <list>
#include <set>
#include "city_improvement.h"
#include "coord.h"

struct city_production {
	bool producing_unit;
	int current_production_id;
};

class city {
	public:
		city(std::string name, int x, int y, unsigned int civid);
		std::string cityname;
		const int xpos;
		const int ypos;
		unsigned int civ_id;
		std::list<coord> resource_coords;
		int population;
		int stored_food;
		int stored_prod;
		city_production production;
		std::set<unsigned int> built_improvements;
		bool producing_something() const;
		void set_unit_production(int uid);
		void set_improv_production(int uid);
		void set_production(const city_production& c);
		bool has_barracks(const city_improv_map& cimap) const;
		int culture;
};


#endif
