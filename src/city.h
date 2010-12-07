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
		bool producing_something() const;
		void set_unit_production(int uid);
		void set_improv_production(int uid);
		void set_production(const city_production& c);
		bool has_barracks(const city_improv_map& cimap) const;
		bool has_granary(const city_improv_map& cimap) const;
		void decrement_city_size();
		void increment_city_size();
		int get_city_size() const;
		const std::list<coord>& get_resource_coords() const;
		void pop_resource_worker();
		void drop_resource_worker(const coord& c);
		void add_resource_worker(const coord& c);
		int get_num_entertainers() const;
		std::string cityname;
		const int xpos;
		const int ypos;
		unsigned int civ_id;
		int stored_food;
		int stored_prod;
		city_production production;
		std::set<unsigned int> built_improvements;
		int accum_culture;
		int culture_level;
	private:
		int city_size;
		int entertainers;
		std::list<coord> resource_coords;
};

#endif
