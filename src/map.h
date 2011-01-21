#ifndef CIV_MAP_H
#define CIV_MAP_H

#include <set>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>

#include "unit.h"
#include "coord.h"
#include "buf2d.h"
#include "resource.h"
#include "resource_configuration.h"
#include "city.h"

class map {
	public:
		map(int x, int y, const resource_configuration& resconf_,
				const resource_map& rmap_);
		map(); // for serialization
		void create();
		int get_data(int x, int y) const;
		void set_data(int x, int y, int terr);
		int size_x() const;
		int size_y() const;
		void get_resources_by_terrain(int terr, bool city, int* food, int* prod, int* comm) const;
		void get_resources_on_spot(int x, int y, int* food, int* prod, int* comm,
				const std::set<unsigned int>* advances) const;
		void get_total_city_resources(int x, int y, int* food_points,
				int* prod_points, int* comm_points,
				const std::set<unsigned int>* advances) const;
		unsigned int get_resource(int x, int y) const;
		void set_resource(int x, int y, unsigned int res);
		void add_unit(unit* u);
		void remove_unit(unit* u);
		bool has_river(int x, int y) const;
		void set_river(int x, int y, bool riv);
		int get_spot_owner(int x, int y) const; // land, unit or city
		int get_spot_resident(int x, int y) const; // unit or city
		const std::list<unit*>& units_on_spot(int x, int y) const;
		city* city_on_spot(int x, int y) const;
		int city_owner_on_spot(int x, int y) const;
		bool has_city_of(int x, int y, unsigned int civ_id) const;
		void add_city(city* c, int x, int y);
		void grab_land(city* c);
		void remove_city(const city* c);
		int get_move_cost(const unit& u, int x1, int y1, int x2, int y2, bool* road) const;
		bool terrain_allowed(const unit& u, int x, int y) const;
		void set_land_owner(int civ_id, int x, int y);
		int get_land_owner(int x, int y) const;
		void remove_civ_land(unsigned int civ_id);
		int wrap_x(int x) const;
		int wrap_y(int y) const;
		std::vector<coord> random_starting_places(int num) const;
		std::map<int, coord> get_starting_places() const;
		int get_starter_at(int x, int y) const;
		void add_starting_place(const coord& c, int civid);
		void remove_starting_place(const coord& c);
		void remove_starting_place_of(int civid);
		coord get_starting_place_of(int civid) const;
		bool x_wrapped() const;
		bool y_wrapped() const;
		bool can_found_city_on(int x, int y) const;
		bool can_improve_terrain(int x, int y, unsigned int civ_id, improvement_type i) const;
		bool try_improve_terrain(int x, int y, unsigned int civ_id, improvement_type i);
		int get_improvements_on(int x, int y) const;
		int get_needed_turns_for_improvement(improvement_type i) const;
		bool road_between(int x1, int y1, int x2, int y2) const;
		bool connected_to_sea(int x, int y) const;
		int manhattan_distance(int x1, int y1, int x2, int y2) const;
		int manhattan_distance_x(int x1, int x2) const;
		int manhattan_distance_y(int y1, int y2) const;
		int vector_from_to_x(int x1, int x2) const;
		int vector_from_to_y(int y1, int y2) const;
		void resize(int newx, int newy);
	private:
		void init_to_water();
		int get_index(int x, int y) const;
		void create_mountains(int x, int y, int width);
		int get_temperature(int x, int y) const;
		std::vector<int> get_types_by_temperature(int temp) const;
		int get_humidity_at(int x, int y) const;
		std::vector<int> get_terrain_candidates(const std::vector<int>& types, 
				int humidity) const;
		float get_latitude(int y) const;
		void sea_around_land(int x, int y, int sea_tile);
		int dist_to_sea_incl_mountains(int x, int y) const;
		buf2d<int> data;
		buf2d<std::list<unit*> > unit_map;
		buf2d<city*> city_map;
		buf2d<int> land_map;
		buf2d<int> improv_map;
		buf2d<int> res_map;
		buf2d<bool> river_map;
		std::map<int, coord> starting_places;
	public:
		const resource_configuration resconf;
		const resource_map rmap;
	private:
		bool x_wrap;
		bool y_wrap;
		static const std::list<unit*> empty_unit_spot;

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & data;
			ar & unit_map;
			ar & city_map;
			ar & land_map;
			ar & improv_map;
			ar & res_map;
			ar & river_map;
			ar & const_cast<resource_configuration&>(resconf);
			ar & const_cast<resource_map&>(rmap);
			ar & x_wrap;
			ar & y_wrap;
		}
};

#endif

