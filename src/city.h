#ifndef CITY_H
#define CITY_H

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/set.hpp>

#include "city_improvement.h"
#include "coord.h"

struct city_production {
	city_production(bool u = true, int i = -1) 
		: producing_unit(u), current_production_id(i) { }
	bool producing_unit;
	int current_production_id;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & producing_unit;
		ar & current_production_id;
	}
};

class city {
	public:
		city(std::string name, int x, int y, 
				unsigned int civid, 
				unsigned int cityid);
		city(); // for serialization
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
		bool pop_resource_worker();
		void drop_resource_worker(const coord& c);
		bool add_resource_worker(const coord& c);
		int get_num_entertainers() const;
		void clear_resource_workers();
		void clear_stored_resources();
		void set_city_id(int i);
		void set_civ_id(int i);
		std::string cityname;
		const int xpos;
		const int ypos;
		unsigned int civ_id;
		unsigned int city_id;
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

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & cityname;
			ar & const_cast<int&>(xpos);
			ar & const_cast<int&>(ypos);
			ar & civ_id;
			ar & city_id;
			ar & stored_food;
			ar & stored_prod;
			ar & production;
			ar & built_improvements;
			ar & accum_culture;
			ar & culture_level;
			ar & city_size;
			ar & entertainers;
			ar & resource_coords;
		}
};

#endif
