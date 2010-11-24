#ifndef CIV_H
#define CIV_H

#include <stdlib.h>

#include <vector>
#include <list>
#include <map>

#include "color.h"
#include "buf2d.h"

class unit_configuration {
	public:
		const char* unit_name;
		unsigned int max_moves;
		bool settler;
		unsigned int production_cost;
		unsigned int max_strength;
};

typedef std::map<int, unit_configuration*> unit_configuration_map;

const int num_terrain_types = 16;

class resource_configuration {
	public:
		resource_configuration();
		int terrain_food_values[num_terrain_types];
		int terrain_prod_values[num_terrain_types];
		int terrain_comm_values[num_terrain_types];
		int city_food_bonus;
		int city_prod_bonus;
		int city_comm_bonus;
};

class unit
{
	public:
		unit(int uid, int x, int y, int civid, const unit_configuration& uconf_);
		~unit();
		void refill_moves(unsigned int m);
		const int unit_id;
		const int civ_id;
		int xpos;
		int ypos;
		unsigned int moves;
		bool fortified;
		const unit_configuration& uconf;
		unsigned int strength;
};

struct coord {
	coord(int x_, int y_);
	int x;
	int y;
};

class city {
	public:
		city(const char* name, int x, int y, unsigned int civid);
		const char* cityname;
		const int xpos;
		const int ypos;
		unsigned int civ_id;
		std::list<coord> resource_coords;
		int population;
		int stored_food;
		int stored_prod;
		int current_production_unit_id;
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
		const std::vector<unit*>& units_on_spot(int x, int y) const;
		city* city_on_spot(int x, int y) const;
		void add_city(city* c, int x, int y);
		void remove_city(const city* c);
	private:
		int get_index(int x, int y) const;
		buf2d<int> data;
		buf2d<std::vector<unit*> > unit_map;
		buf2d<city*> city_map;
		const resource_configuration& resconf;
		static const std::vector<unit*> empty_unit_spot;
};

enum msg_type {
	msg_new_unit,
	msg_civ_discovery,
};

struct msg {
	msg_type type;
	union {
		unit* new_unit;
		int discovered_civ_id;
	} msg_data;
};

class civilization {
	public:
		civilization(const char* name, unsigned int civid, const color& c_, map& m_);
		~civilization();
		unit* add_unit(int uid, int x, int y, const unit_configuration& uconf);
		void remove_unit(unit* u);
		int try_move_unit(unit* u, int chx, int chy);
		void refill_moves(const unit_configuration_map& uconfmap);
		void increment_resources(const unit_configuration_map& uconfmap);
		char fog_at(int x, int y) const;
		city* add_city(const char* name, int x, int y);
		void add_message(const msg& m);
		int get_relationship_to_civ(unsigned int civid) const;
		void set_relationship_to_civ(unsigned int civid, int val);
		bool discover(unsigned int civid);
		std::vector<unsigned int> check_discoveries(int x, int y, int radius);
		const char* civname;
		const unsigned int civ_id;
		color col;
		std::list<unit*> units;
		std::list<city*> cities;
		map& m;
		fog_of_war fog;
		int gold;
		std::list<msg> messages;
	private:
		std::vector<int> relationships;
};

class round
{
	public:
		round(const unit_configuration_map& uconfmap_);
		void add_civilization(civilization* civ);
		bool next_civ();
		std::vector<civilization*> civs;
		std::vector<civilization*>::iterator current_civ;
		const unit_configuration* get_unit_configuration(int uid) const;
		const unit_configuration_map& uconfmap;
	private:
		void refill_moves();
		void increment_resources();
};

void total_resources(const city& c, const map& m,
		int* food, int* prod, int* comm);
void set_default_city_production(city* c, 
		const unit_configuration_map& uconfmap);
void combat(unit* u1, unit* u2);
bool can_attack(const unit& u1, const unit& u2);

#endif
