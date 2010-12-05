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
		void declare_war_between(unsigned int civ1, unsigned int civ2);
		void peace_between(unsigned int civ1, unsigned int civ2);
		std::vector<civilization*> civs;
		const unit_configuration_map uconfmap;
		const advance_map amap;
		const city_improv_map cimap;
		bool in_war(unsigned int civ1, unsigned int civ2) const;
	private:
		bool next_civ();
		void refill_moves();
		void increment_resources();
		bool try_move_unit(unit* u, int chx, int chy, map* m);
		void check_city_conquer(map* m, int tgtxpos, int tgtypos);
		void check_civ_elimination(int civ_id);
		std::vector<civilization*>::iterator current_civ;
};

#endif

