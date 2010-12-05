#ifndef CIV_MAP_H
#define CIV_MAP_H

#include <vector>
#include <list>

#include "unit.h"
#include "coord.h"
#include "buf2d.h"
#include "resource_configuration.h"
#include "city.h"

class map {
	public:
		map(int x, int y, const resource_configuration& resconf_);
		int get_data(int x, int y) const;
		int size_x() const;
		int size_y() const;
		void get_resources_by_terrain(int terr, bool city, int* food, int* prod, int* comm) const;
		void add_unit(unit* u);
		void remove_unit(unit* u);
		int get_spot_owner(int x, int y) const; // land, unit or city
		int get_spot_resident(int x, int y) const; // unit or city
		const std::list<unit*>& units_on_spot(int x, int y) const;
		city* city_on_spot(int x, int y) const;
		int city_owner_on_spot(int x, int y) const;
		bool has_city_of(int x, int y, unsigned int civ_id) const;
		void add_city(city* c, int x, int y);
		void remove_city(const city* c);
		int get_move_cost(const unit& u, int x, int y) const;
		void set_land_owner(int civ_id, int x, int y);
		int get_land_owner(int x, int y) const;
		void remove_civ_land(unsigned int civ_id);
		int wrap_x(int x) const;
		int wrap_y(int y) const;
		std::vector<coord> get_starting_places(int num) const;
		bool x_wrapped() const;
		bool y_wrapped() const;
	private:
		int get_index(int x, int y) const;
		buf2d<int> data;
		buf2d<std::list<unit*> > unit_map;
		buf2d<city*> city_map;
		buf2d<int> land_map;
		const resource_configuration& resconf;
		bool x_wrap;
		bool y_wrap;
		static const std::list<unit*> empty_unit_spot;
};


#endif

