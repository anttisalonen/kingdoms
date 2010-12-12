#ifndef ROUND_H
#define ROUND_H

#include <vector>

#include "unit_configuration.h"
#include "advance.h"
#include "city_improvement.h"
#include "civ.h"
#include "map.h"

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
	action_improvement,
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
				improvement_type improv;
			} unit_action_data;
		} unit_data;
	} data;
};

action unit_action(unit_action_type t, unit* u);
action move_unit_action(unit* u, int chx, int chy);
action improve_unit_action(unit* u, improvement_type i);

class round
{
	public:
		round(const unit_configuration_map& uconfmap_, 
				const advance_map& amap_, 
				const city_improv_map& cimap_,
				map& m_,
				unsigned int road_moves_);
		void add_civilization(civilization* civ);
		bool perform_action(int civid, const action& a);
		const unit_configuration* get_unit_configuration(int uid) const;
		int current_civ_id() const;
		void declare_war_between(unsigned int civ1, unsigned int civ2);
		void peace_between(unsigned int civ1, unsigned int civ2);
		std::vector<civilization*> civs;
		const unit_configuration_map uconfmap;
		const advance_map amap;
		const city_improv_map cimap;
		bool in_war(unsigned int civ1, unsigned int civ2) const;
		int get_round_number() const;
		unsigned int get_num_road_moves() const;
	private:
		bool next_civ();
		void refill_moves();
		void increment_resources();
		bool try_move_unit(unit* u, int chx, int chy);
		void check_city_conquer(int tgtxpos, int tgtypos);
		void check_civ_elimination(int civ_id);
		int needed_food_for_growth(int city_size) const;
		int needed_culture_for_growth(int city_culture) const;
		void check_for_city_updates();
		void update_land_owners();
		std::vector<civilization*>::iterator current_civ;
		map& m;
		int round_number;
		const unsigned int road_moves;
};

#endif

