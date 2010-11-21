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
};

typedef std::map<int, unit_configuration*> unit_configuration_map;

class unit
{
	public:
		unit(int uid, int x, int y, const color& c_);
		~unit();
		void refill_moves(unsigned int m);
		int unit_id;
		int xpos;
		int ypos;
		color c;
		unsigned int moves;
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
		map(int x, int y);
		int get_data(int x, int y) const;
		int size_x() const;
		int size_y() const;
	private:
		int get_index(int x, int y) const;
		buf2d<int> data;
};

class civilization {
	public:
		civilization(const char* name, const color& c_, const map& m_);
		~civilization();
		void add_unit(int uid, int x, int y);
		int try_move_unit(unit* u, int chx, int chy);
		void refill_moves(const unit_configuration_map& uconfmap);
		char fog_at(int x, int y) const;

		const char* civname;
		color col;
		std::list<unit*> units;
		const map& m;
		fog_of_war fog;
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
	private:
		const unit_configuration_map& uconfmap;
		void refill_moves();
};

#endif
