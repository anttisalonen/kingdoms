#ifndef CIV_H
#define CIV_H

#include <stdlib.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>

#include "coord.h"
#include "color.h"
#include "buf2d.h"

#include "unit_configuration.h"
#include "resource_configuration.h"
#include "city.h"
#include "unit.h"
#include "map.h"
#include "fog_of_war.h"
#include "advance.h"
#include "government.h"

enum relationship {
	relationship_unknown,
	relationship_peace,
	relationship_war,
};

enum msg_type {
	msg_new_unit,
	msg_civ_discovery,
	msg_new_advance,
	msg_new_city_improv,
	msg_unit_disbanded,
	msg_new_relationship,
};

struct msg {
	msg_type type;
	union {
		struct {
			int building_city_id;
			int prod_id;
			int unit_id;
		} city_prod_data;
		int discovered_civ_id;
		unsigned int new_advance_id;
		unsigned int disbanded_unit_id;
		struct {
			int other_civ_id;
			relationship new_relationship;
		} relationship_data;
	} msg_data;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & type;
		switch(type) {
			case msg_new_unit:
			case msg_new_city_improv:
				ar & msg_data.city_prod_data.building_city_id;
				ar & msg_data.city_prod_data.prod_id;
				ar & msg_data.city_prod_data.unit_id;
				break;
			case msg_civ_discovery:
				ar & msg_data.discovered_civ_id;
				break;
			case msg_new_advance:
				ar & msg_data.new_advance_id;
				break;
			case msg_unit_disbanded:
				ar & msg_data.disbanded_unit_id;
				break;
			case msg_new_relationship:
				ar & msg_data.relationship_data.other_civ_id;
				ar & msg_data.relationship_data.new_relationship;
				break;
		}
	}
};

class civilization {
	public:
		civilization(std::string name, unsigned int civid, const color& c_, map* m_, bool ai_,
				const std::vector<std::string>::iterator& names_start,
				const std::vector<std::string>::iterator& names_end,
				const government* gov_);
		civilization(); // for serialization
		~civilization();
		unit* add_unit(int uid, int x, int y, 
				const unit_configuration& uconf,
				unsigned int road_moves);
		void remove_unit(unit* u);
		bool move_acceptable_by_land_and_units(int x, int y) const;
		bool can_move_unit(const unit* u, int chx, int chy) const;
		void move_unit(unit* u, int chx, int chy, bool fought);
		void refill_moves(const unit_configuration_map& uconfmap);
		void increment_resources(const unit_configuration_map& uconfmap,
				const advance_map& amap,
				const city_improv_map& cimap,
				unsigned int road_moves,
				unsigned int food_eaten_per_citizen);
		char fog_at(int x, int y) const;
		city* add_city(int x, int y);
		void add_city(city* c);
		void remove_city(city* c, bool del);
		void add_message(const msg& m);
		relationship get_relationship_to_civ(unsigned int civid) const;
		void set_relationship_to_civ(unsigned int civid, relationship val);
		bool discover(unsigned int civid);
		void undiscover(unsigned int civid);
		void set_war(unsigned int civid);
		void set_peace(unsigned int civid);
		std::vector<unsigned int> check_discoveries(int x, int y, int radius);
		bool unit_discovered(const unit_configuration& uconf) const;
		bool improv_discovered(const city_improvement& uconf) const;
		bool blocked_by_land(int x, int y) const;
		int get_known_land_owner(int x, int y) const;
		void eliminate();
		bool eliminated() const;
		void set_map(map* m_);
		void set_government(const government* g);
		int get_national_income() const;
		int get_military_expenses() const;
		void update_city_resource_workers(city* c);
		bool can_build_unit(const unit_configuration& uc, const city& c) const;
		bool can_build_improvement(const city_improvement& ci, const city& c) const;
		bool can_load_unit(unit* loadee, unit* loader) const;
		void load_unit(unit* loadee, unit* loader);
		const std::map<unsigned int, int>& get_built_units() const;
		const std::map<unsigned int, int>& get_lost_units() const;
		void add_points(unsigned int num);
		void reset_points();
		int get_points() const;
		bool can_cross_oceans() const;
		bool allowed_research_goal(const advance_map::const_iterator& amap) const;
		void unload_unit(unit* loadee);
		void total_resources(const city& c, int* food, int* prod, int* comm) const;
		coord next_good_resource_spot(const city* c) const;
		bool can_add_resource_worker(const coord& c) const;
		void update_resource_worker_map();
		std::string civname;
		const unsigned int civ_id;
		color col;
		std::map<unsigned int, unit*> units;
		std::map<unsigned int, city*> cities;
		map* m;
		fog_of_war fog;
		int gold;
		int science;
		std::list<msg> messages;
		int alloc_gold;
		int alloc_science;
		unsigned int research_goal_id;
		std::set<unsigned int> researched_advances;
		bool ai;
		const government* gov;
	private:
		void reveal_land(int x, int y, int r);
		void update_military_expenses();
		void setup_default_research_goal(const advance_map& amap);
		void destroy_old_palace(const city* c, const city_improv_map& cimap);
		void update_ocean_crossing(const unit_configuration_map& uconfmap,
				const advance_map& amap, int adv_id);
		void calculate_total_city_commerce(const city& c,
				const city_improv_map& cimap,
				int orig_comm, int* add_gold, int* add_science) const;
		bool has_access_to_resource(const city& c, unsigned int res_id) const;
		std::vector<relationship> relationships;
		buf2d<int> known_land_map;
		std::vector<std::string> city_names;
		unsigned int curr_city_name_index;
		unsigned int next_city_id;
		unsigned int next_unit_id;
		int national_income;
		int military_expenses;
		std::map<unsigned int, int> built_units;
		std::map<unsigned int, int> lost_units;
		unsigned int points;
		bool cross_oceans;
		std::map<coord, unsigned int> resource_workers_map;

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & civname;
			ar & const_cast<unsigned int&>(civ_id);
			ar & col;
			ar & units;
			ar & cities;
			ar & m;
			ar & fog;
			ar & gold;
			ar & science;
			ar & messages;
			ar & alloc_gold;
			ar & alloc_science;
			ar & research_goal_id;
			ar & researched_advances;
			ar & ai;
			ar & const_cast<government*&>(gov);
			ar & relationships;
			ar & known_land_map;
			ar & city_names;
			ar & curr_city_name_index;
			ar & next_city_id;
			ar & next_unit_id;
			ar & national_income;
			ar & military_expenses;
			ar & built_units;
			ar & lost_units;
			ar & points;
			ar & cross_oceans;
			ar & resource_workers_map;
		}
};

#endif

