#ifndef CIV_H
#define CIV_H

#include <stdlib.h>

#include <vector>
#include <string>
#include <list>
#include <set>
#include <map>

#include "coord.h"
#include "color.h"
#include "buf2d.h"

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

const int num_terrain_types = 16;

class resource_configuration {
	public:
		resource_configuration();
		std::string resource_name[num_terrain_types];
		int terrain_food_values[num_terrain_types];
		int terrain_prod_values[num_terrain_types];
		int terrain_comm_values[num_terrain_types];
		int terrain_type[num_terrain_types];
		int city_food_bonus;
		int city_prod_bonus;
		int city_comm_bonus;
};

class advance {
	public:
		advance();
		unsigned int advance_id;
		std::string advance_name;
		int cost;
		unsigned int needed_advances[4];
};

typedef std::map<unsigned int, advance> advance_map;

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

class unit
{
	public:
		unit(int uid, int x, int y, int civid, const unit_configuration& uconf_);
		~unit();
		void refill_moves(unsigned int m);
		bool is_settler() const;
		const int unit_id;
		const int civ_id;
		int xpos;
		int ypos;
		unsigned int moves;
		bool fortified;
		const unit_configuration& uconf;
		unsigned int strength;
		bool veteran;
};

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
};

class fog_of_war {
	public:
		fog_of_war(int x, int y);
		void reveal(int x, int y, int radius);
		void shade(int x, int y, int radius);
		char get_value(int x, int y) const;
	private:
		int get_refcount(int x, int y) const;
		int get_raw(int x, int y) const;
		void set_value(int x, int y, int val);
		void up_refcount(int x, int y);
		void down_refcount(int x, int y);
		buf2d<int> fog;
};

class map {
	public:
		map(int x, int y, const resource_configuration& resconf_);
		int get_data(int x, int y) const;
		int size_x() const;
		int size_y() const;
		void get_resources_by_terrain(int terr, bool city, int* food, int* prod, int* comm) const;
		void add_unit(unit* u);
		void remove_unit(unit* u);
		bool free_spot(unsigned int civ_id, int x, int y) const;
		int get_spot_owner(int x, int y) const;
		const std::list<unit*>& units_on_spot(int x, int y) const;
		city* city_on_spot(int x, int y) const;
		bool has_city_of(const coord& co, unsigned int civ_id) const;
		void add_city(city* c, int x, int y);
		void remove_city(const city* c);
		int get_move_cost(const unit& u, int x, int y) const;
	private:
		int get_index(int x, int y) const;
		buf2d<int> data;
		buf2d<std::list<unit*> > unit_map;
		buf2d<city*> city_map;
		const resource_configuration& resconf;
		static const std::list<unit*> empty_unit_spot;
};

enum msg_type {
	msg_new_unit,
	msg_civ_discovery,
	msg_new_advance,
	msg_new_city_improv,
};

struct msg {
	msg_type type;
	union {
		struct {
			city* building_city;
			int prod_id;
		} city_prod_data;
		int discovered_civ_id;
		unsigned int new_advance_id;
	} msg_data;
};

class civilization {
	public:
		civilization(std::string name, unsigned int civid, const color& c_, map* m_, bool ai_);
		~civilization();
		unit* add_unit(int uid, int x, int y, const unit_configuration& uconf);
		void remove_unit(unit* u);
		int try_move_unit(unit* u, int chx, int chy);
		void refill_moves(const unit_configuration_map& uconfmap);
		void increment_resources(const unit_configuration_map& uconfmap,
				const advance_map& amap,
				const city_improv_map& cimap);
		char fog_at(int x, int y) const;
		city* add_city(std::string name, int x, int y);
		void remove_city(city* c);
		void add_message(const msg& m);
		int get_relationship_to_civ(unsigned int civid) const;
		void set_relationship_to_civ(unsigned int civid, int val);
		bool discover(unsigned int civid);
		void undiscover(unsigned int civid);
		std::vector<unsigned int> check_discoveries(int x, int y, int radius);
		bool unit_discovered(const unit_configuration& uconf) const;
		bool improv_discovered(const city_improvement& uconf) const;
		void eliminate();
		void set_map(map* m_);
		std::string civname;
		const unsigned int civ_id;
		color col;
		std::list<unit*> units;
		std::list<city*> cities;
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
	private:
		std::vector<int> relationships;
};

enum action_type {
	action_give_up,
	action_eot,
	action_unit_action,
	action_city_action,
	action_none,
};

enum unit_action_type {
	action_move_unit,
	action_found_city,
	action_skip,
	action_fortify,
};

struct action {
	action(action_type t);
	action_type type;
	union {
		struct {
			unit_action_type uatype;
			unit* u;
			union {
				struct {
					int chx;
					int chy;
				} move_pos;
			} unit_action_data;
		} unit_data;
	} data;
};

action unit_action(unit_action_type t, unit* u);
action move_unit_action(unit* u, int chx, int chy);

class round
{
	public:
		round(const unit_configuration_map& uconfmap_, 
				const advance_map& amap_, 
				const city_improv_map& cimap_);
		void add_civilization(civilization* civ);
		bool perform_action(int civid, const action& a, map* m);
		const unit_configuration* get_unit_configuration(int uid) const;
		int current_civ_id() const;
		std::vector<civilization*> civs;
		const unit_configuration_map uconfmap;
		const advance_map amap;
		const city_improv_map cimap;
	private:
		bool next_civ();
		void refill_moves();
		void increment_resources();
		bool try_move_unit(unit* u, int chx, int chy, map* m);
		void check_city_conquer(map* m, int tgtxpos, int tgtypos);
		void check_civ_elimination(int civ_id);
		std::vector<civilization*>::iterator current_civ;
};

void total_resources(const city& c, const map& m,
		int* food, int* prod, int* comm);
void set_default_city_production(city* c, 
		const unit_configuration_map& uconfmap);
void combat(unit* u1, unit* u2);
bool can_attack(const unit& u1, const unit& u2);
bool terrain_allowed(const map& m, const unit& u, int x, int y);

#endif

