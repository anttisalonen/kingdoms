#include <stdio.h>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "pompelmous.h"

void set_default_city_production(city* c, const unit_configuration_map& uconfmap)
{
	c->production.producing_unit = true;
	for(unit_configuration_map::const_iterator it = uconfmap.begin();
			it != uconfmap.end();
			++it) {
		if(it->second.max_strength) {
			c->production.current_production_id = it->first;
			return;
		}
	}
}

bool can_attack(const map* m, const unit& u1, const unit& u2)
{
	if(m->manhattan_distance_x(u1.xpos, u2.xpos) > 1)
		return false;
	if(m->manhattan_distance_y(u1.ypos, u2.ypos) > 1)
		return false;
	return u1.uconf->max_strength > 0;
}

action::action(action_type t)
	: type(t)
{
}

std::string action::to_string() const
{
	switch(type) {
		case action_give_up:
			return std::string("give up");
		case action_eot:
			return std::string("end of turn");
		case action_unit_action:
			switch(data.unit_data.uatype) {
				case action_move_unit:
					{
						std::stringstream ss;
						ss << "unit move: (" << data.unit_data.unit_action_data.move_pos.chx 
							<< ", " << data.unit_data.unit_action_data.move_pos.chy << ")";
						return ss.str();
					}
				case action_found_city:
					return std::string("unit found city");
				case action_skip:
					return std::string("unit skip");
				case action_fortify:
					return std::string("unit fortify");
				case action_improvement:
					return std::string("unit improvement");
				case action_load:
					return std::string("unit load");
				case action_unload:
					return std::string("unit unload");
			}
		case action_city_action:
			return std::string("city action");
		case action_none:
			return std::string("none");
	}
	return std::string("");
}

action unit_action(unit_action_type t, unit* u)
{
	action a = action(action_unit_action);
	a.data.unit_data.uatype = t;
	a.data.unit_data.u = u;
	return a;
}

action move_unit_action(unit* u, int chx, int chy)
{
	action a = unit_action(action_move_unit, u);
	a.data.unit_data.unit_action_data.move_pos.chx = chx;
	a.data.unit_data.unit_action_data.move_pos.chy = chy;
	return a;
}

action improve_unit_action(unit* u, improvement_type i)
{
	action a = unit_action(action_improvement, u);
	a.data.unit_data.unit_action_data.improv = i;
	return a;
}

visible_move_action::visible_move_action(const unit* un, int chx, int chy,
		combat_result res, const unit* opp)
	: u(un), change(coord(chx, chy)),
	combat(res), opponent(opp)
{
}

pompelmous::pompelmous(const unit_configuration_map& uconfmap_,
		const advance_map& amap_,
		const city_improv_map& cimap_,
		const government_map& govmap_,
		map* m_, unsigned int road_moves_,
		unsigned int food_eaten_per_citizen_,
		unsigned int anarchy_period_turns_,
		int num_turns_)
	: uconfmap(uconfmap_),
	amap(amap_),
	cimap(cimap_),
	govmap(govmap_),
	m(m_),
	round_number(0),
	road_moves(road_moves_),
	food_eaten_per_citizen(food_eaten_per_citizen_),
	anarchy_period_turns(anarchy_period_turns_),
	num_turns(num_turns_),
	winning_civ(-1),
	victory(victory_none)
{
	current_civ = civs.begin();
}

pompelmous::pompelmous()
	: m(NULL),
	road_moves(1337),
	food_eaten_per_citizen(1337)
{
}

void pompelmous::add_diplomat(int civid, diplomat* d)
{
	diplomat_handlers[civid] = d;
}

void pompelmous::add_civilization(civilization* civ)
{
	civs.push_back(civ);
	current_civ = civs.begin();
	refill_moves();
}

void pompelmous::refill_moves()
{
	for(std::vector<civilization*>::iterator it = civs.begin();
	    it != civs.end();
	    ++it) {
		(*it)->refill_moves(uconfmap);
	}
}

void pompelmous::increment_resources()
{
	for(std::vector<civilization*>::iterator it = civs.begin();
	    it != civs.end();
	    ++it) {
		(*it)->increment_resources(uconfmap, amap, cimap,
				road_moves, food_eaten_per_citizen);
	}
}

int pompelmous::get_winning_civ() const
{
	return winning_civ;
}

void pompelmous::check_for_victory_conditions()
{
	if(winning_civ != -1)
		return;
	if(winning_civ == -1 && round_number >= num_turns) {
		// win by score
		unsigned int highest_score_civid = 0;
		int highest_score = civs[0]->get_points();
		for(unsigned int i = 1; i < civs.size(); i++) {
			if(civs[i]->eliminated())
				continue;
			int this_score = civs[i]->get_points();
			if(this_score > highest_score) {
				highest_score = this_score;
				highest_score_civid = i;
			}
		}
		winning_civ = highest_score_civid;
		printf("Win by score: %s - %d points!\n",
				civs[highest_score_civid]->civname.c_str(),
				highest_score);
		victory = victory_score;
		return;
	}

	// win by elimination
	bool elimination_win = false;
	unsigned int eliminator = 0;
	for(unsigned int i = 0; i < civs.size(); i++) {
		if(!civs[i]->eliminated()) {
			if(elimination_win) {
				elimination_win = false;
				break;
			}
			else {
				eliminator = i;
				elimination_win = true;
			}
		}
	}
	if(elimination_win) {
		winning_civ = eliminator;
		printf("Win by elimination: %s!\n",
				civs[winning_civ]->civname.c_str());
		victory = victory_elimination;
		return;
	}

	// win by domination
	int controlled_area[civs.size()];
	for(unsigned int i = 0; i < civs.size(); i++) {
		controlled_area[i] = 0;
	}
	int total_controlled_area = 0;
	for(int i = 0; i < m->size_x(); i++) {
		for(int j = 0; j < m->size_y(); j++) {
			int val = m->get_data(i, j);
			if(!m->resconf.is_water_tile(val)) {
				int lo = m->get_land_owner(i, j);
				if(lo >= 0 && lo < (int)civs.size()) {
					total_controlled_area++;
					controlled_area[lo]++;
				}
			}
		}
	}
	for(unsigned int i = 0; i < civs.size(); i++) {
		float area = controlled_area[i] / (float)total_controlled_area;
		if(area > 0.80f) {
			winning_civ = i;
			printf("Win by domination: %s!\n",
					civs[i]->civname.c_str());
			victory = victory_domination;
			break;
		}
	}
}

bool pompelmous::finished() const
{
	return winning_civ != -1 || round_number >= num_turns;
}

victory_type pompelmous::get_victory_type() const
{
	return victory;
}

bool pompelmous::next_civ()
{
	if(civs.empty())
		return false;
	current_civ++;
	if(current_civ == civs.end()) {
		round_number++;
		fprintf(stdout, "Round: %d\n", round_number);
		check_for_victory_conditions();
		current_civ = civs.begin();
		increment_resources();
		check_for_city_updates();
		update_civ_points();
		refill_moves();
		return true;
	}
	return false;
}

void pompelmous::update_civ_points()
{
	for(std::vector<civilization*>::iterator it = civs.begin();
	    it != civs.end();
	    ++it) {
		unsigned int added = 0;
		for(std::map<unsigned int, city*>::iterator cit = (*it)->cities.begin();
				cit != (*it)->cities.end();
				++cit) {
			added += cit->second->get_city_size();
			added += cit->second->culture_level;
		}
		(*it)->add_points(added);
	}
}

int pompelmous::needed_food_for_growth(int city_size) const
{
	return city_size * 10;
}

int pompelmous::needed_culture_for_growth(int culture_level) const
{
	return std::pow(10, culture_level);
}

void pompelmous::check_for_city_updates()
{
	bool update_land = false;
	for(std::vector<civilization*>::iterator it = civs.begin();
	    it != civs.end();
	    ++it) {
		for(std::map<unsigned int, city*>::iterator cit = (*it)->cities.begin();
				cit != (*it)->cities.end();
				++cit) {
			city* c = cit->second;
			if(c->stored_food < 0) {
				if(c->get_city_size() <= 1) {
					std::map<unsigned int, city*>::iterator cit2(cit);
					cit2--;
					(*it)->remove_city(c, true);
					update_land = true;
					cit = cit2;
					continue;
				}
				else {
					c->decrement_city_size();
				}
			}
			if(c->stored_food >= needed_food_for_growth(c->get_city_size())) {
				c->increment_city_size();
				coord rescoord = civs[c->civ_id]->next_good_resource_spot(c);
				c->add_resource_worker(rescoord);
				if(c->has_granary(cimap)) {
					c->stored_food = needed_food_for_growth(c->get_city_size()) / 2;
				}
				else {
					c->stored_food = 0;
				}
			}
			if(c->accum_culture >= needed_culture_for_growth(c->culture_level)) {
				c->culture_level++;
				update_land = true;
			}
		}
		(*it)->update_resource_worker_map();
	}
	if(update_land)
		update_land_owners();
}

const unit_configuration* pompelmous::get_unit_configuration(int uid) const
{
	unit_configuration_map::const_iterator it = uconfmap.find(uid);
	if(it == uconfmap.end())
		return NULL;
	return &it->second;
}

void pompelmous::broadcast_action(const visible_move_action& a) const
{
	for(std::list<action_listener*>::const_iterator it = action_listeners.begin();
			it != action_listeners.end();
			++it) {
		(*it)->handle_action(a);
	}
}

bool pompelmous::perform_action(int civid, const action& a)
{
	if(civid < 0 || civid != current_civ_id()) {
		return false;
	}

	if(finished()) {
		return a.type == action_eot;
	}

	switch(a.type) {
		case action_eot:
			// end of turn for this civ
			next_civ();
			break;
		case action_unit_action:
			switch(a.data.unit_data.uatype) {
				case action_move_unit:
					return try_move_unit(a.data.unit_data.u, a.data.unit_data.unit_action_data.move_pos.chx,
							a.data.unit_data.unit_action_data.move_pos.chy);
				case action_found_city:
					{
						bool can_build = a.data.unit_data.u->uconf->settler;
						if(can_build && 
							m->can_found_city_on(a.data.unit_data.u->xpos,
								a.data.unit_data.u->ypos)) {
							city* c = (*current_civ)->add_city(a.data.unit_data.u->xpos,
									a.data.unit_data.u->ypos);
							if((*current_civ)->cities.size() == 1) {
								for(city_improv_map::const_iterator pit = cimap.begin();
										pit != cimap.end();
										++pit) {
									if(pit->second.palace) {
										c->built_improvements.insert(pit->first);
										break;
									}
								}
							}
							set_default_city_production(c, uconfmap);
							(*current_civ)->remove_unit(a.data.unit_data.u);
							return true;
						}
						return false;
					}
				case action_skip:
					a.data.unit_data.u->skip_turn();
					return true;
				case action_fortify:
					if(a.data.unit_data.u->carried() &&
						m->terrain_allowed(*a.data.unit_data.u,
							a.data.unit_data.u->xpos,
							a.data.unit_data.u->ypos)) {
						// in city => unload
						(*current_civ)->unload_unit(a.data.unit_data.u);
					}
					a.data.unit_data.u->fortify();
					return true;
				case action_improvement:
					if(a.data.unit_data.unit_action_data.improv == improv_none) {
						return false;
					}
					if(!a.data.unit_data.u->uconf->worker) {
						return false;
					}
					if(!m->can_improve_terrain(a.data.unit_data.u->xpos,
							a.data.unit_data.u->ypos,
							a.data.unit_data.u->civ_id,
							a.data.unit_data.unit_action_data.improv))
						return false;
					a.data.unit_data.u->start_improving_to(a.data.unit_data.unit_action_data.improv,
							m->get_needed_turns_for_improvement(a.data.unit_data.unit_action_data.improv));
					return true;
				case action_load:
					if(can_load_unit(a.data.unit_data.u,
							a.data.unit_data.u->xpos, 
							a.data.unit_data.u->ypos)) {
						load_unit(a.data.unit_data.u,
							a.data.unit_data.u->xpos,
							a.data.unit_data.u->ypos);
						return true;
					}
					else {
						return false;
					}
				case action_unload:
					return try_wakeup_loaded(a.data.unit_data.u);
				default:
					break;
			}
		case action_city_action:
			break;
		case action_give_up:
		default:
			break;
	}
	return true;
}

void pompelmous::check_city_conquer(int tgtxpos, int tgtypos, int conquering_civid)
{
	city* c = m->city_on_spot(tgtxpos, tgtypos);
	if(c && c->civ_id != (unsigned int)conquering_civid) {
		civilization* civ = civs[c->civ_id];
		if(c->get_city_size() > 1) {
			civ->remove_city(c, false);
			civilization* civ2 = civs[conquering_civid];
			c->decrement_city_size();
			c->clear_stored_resources();
			civ2->add_city(c);
			destroy_improvements(c);
			update_land_owners();
			civ2->update_city_resource_workers(c);
			set_default_city_production(c, uconfmap);
		}
		else {
			civ->remove_city(c, true);
			update_land_owners();
		}
	}
}

void pompelmous::destroy_improvements(city* c)
{
	for(std::set<unsigned int>::iterator it = c->built_improvements.begin();
			it != c->built_improvements.end();) {
		city_improv_map::const_iterator ciit = cimap.find(*it);
		if(ciit != cimap.end() && (ciit->second.palace || ((rand() % 3) == 0))) {
			c->built_improvements.erase(it++);
		}
		else {
			it++;
		}
	}
}

void pompelmous::update_land_owners()
{
	for(int i = 0; i < m->size_x(); i++)
		for(int j = 0; j < m->size_y(); j++)
			m->set_land_owner(-1, i, j);

	for(std::vector<civilization*>::iterator it = civs.begin();
	    it != civs.end();
	    ++it) {
		for(std::map<unsigned int, city*>::iterator cit = (*it)->cities.begin();
				cit != (*it)->cities.end();
				++cit) {
			m->grab_land(cit->second);
		}
	}
}

void pompelmous::check_civ_elimination(int civ_id)
{
	civilization* civ = civs[civ_id];
	if(civ->cities.size() == 0) {
		bool no_settlers = true;
		for(std::map<unsigned int, unit*>::const_iterator uit = civ->units.begin();
				uit != civ->units.end();
				++uit) {
			if(uit->second->is_settler()) {
				no_settlers = false;
				break;
			}
		}
		if(no_settlers) {
			civ->eliminate();
		}
	}
}

int pompelmous::get_offense_bonus(const unit* off, const unit* def) const
{
	return 0;
}

int pompelmous::get_defense_bonus(const unit* def, const unit* off) const
{
	int bonus = 0;
	int city_bonus = 0;
	city* c = m->city_on_spot(def->xpos, def->ypos);
	if(c) {
		for(std::set<unsigned int>::const_iterator cit = c->built_improvements.begin();
				cit != c->built_improvements.end();
				++cit) {
			city_improv_map::const_iterator ciit = cimap.find(*cit);
			if(ciit != cimap.end()) {
				if(ciit->second.defense_bonus > city_bonus) {
					city_bonus = ciit->second.defense_bonus;
				}
			}
		}
	}
	bonus += city_bonus;
	return bonus;
}

bool pompelmous::try_move_unit(unit* u, int chx, int chy)
{
	if(abs(chx) > 1 || abs(chy) > 1)
		return false;

	int tgtxpos = u->xpos + chx;
	int tgtypos = u->ypos + chy;
	bool fought = false;

	if(!m->terrain_allowed(*u, tgtxpos, tgtypos)) {
		if(u->carrying()) {
			if(can_unload_unit(u, chx, chy)) {
				broadcast_action(visible_move_action(*u->carried_units.begin(),
							chx, chy, combat_result_none, NULL));
				unload_unit(u, chx, chy);
				return true;
			}
			else {
				return false;
			}
		}
		else if(can_load_unit(u, tgtxpos, tgtypos)) {
			broadcast_action(visible_move_action(u, chx, chy, combat_result_none, NULL));
			load_unit(u, tgtxpos, tgtypos);
			return true;
		}
		else {
			return false;
		}
	}

	// attack square?
	int def_id = m->get_spot_resident(tgtxpos, tgtypos);
	if(def_id >= 0 && def_id != u->civ_id) {
		if(!u->carried() && in_war(u->civ_id, def_id)) {
			const std::list<unit*>& units = m->units_on_spot(tgtxpos, tgtypos);
			if(units.size() != 0) {
				unit* defender = units.front();
				if(!can_attack(m, *u, *defender)) {
					return false;
				}
				combat(u, defender);
				fought = true;
				if(u->strength == 0) {
					// lost combat
					broadcast_action(visible_move_action(u,
								chx, chy,
								combat_result_lost,
								defender));
					(*current_civ)->remove_unit(u);
					return true;
				}
				else if(defender->strength == 0) {
					// won combat
					broadcast_action(visible_move_action(u,
								chx, chy,
								combat_result_won,
								defender));
					(*civs[def_id]).remove_unit(defender);
				}
			}
			if(m->units_on_spot(tgtxpos, tgtypos).size() == 0) {
				// check if a city was conquered
				check_city_conquer(tgtxpos, tgtypos, u->civ_id);
				check_civ_elimination(def_id);
			}
		}
		else {
			return false;
		}
	}

	// move to square
	if((*current_civ)->can_move_unit(u, chx, chy)) {
		broadcast_action(visible_move_action(u, chx, chy, combat_result_none, NULL));
		(*current_civ)->move_unit(u, chx, chy, fought);
		std::vector<unsigned int> discs = (*current_civ)->check_discoveries(u->xpos,
				u->ypos, 1);
		for(std::vector<unsigned int>::const_iterator it = discs.begin();
				it != discs.end();
				++it) {
			civs[*it]->discover((*current_civ)->civ_id);
		}
		if(def_id >= 0 && def_id != u->civ_id) {
			check_city_conquer(tgtxpos, tgtypos, u->civ_id);
			check_civ_elimination(def_id);
		}
		return true;
	}
	else {
		if(def_id >= 0 && def_id != u->civ_id) {
			// won combat - but no move
			u->decrement_moves();
			return true;
		}
		else {
			// could not move to a friendly neighbour's land
			return false;
		}
	}
}

int pompelmous::current_civ_id() const
{
	if(current_civ == civs.end())
		return -1;
	return (*current_civ)->civ_id;
}

void pompelmous::declare_war_between(unsigned int civ1, unsigned int civ2)
{
	civs[civ1]->set_war(civ2);
	civs[civ2]->set_war(civ1);
}

void pompelmous::peace_between(unsigned int civ1, unsigned int civ2)
{
	civs[civ1]->set_peace(civ2);
	civs[civ2]->set_peace(civ1);
}

bool pompelmous::in_war(unsigned int civ1, unsigned int civ2) const
{
	return civs[civ1]->get_relationship_to_civ(civ2) == relationship_war;
}

int pompelmous::get_round_number() const
{
	return round_number;
}

unsigned int pompelmous::get_num_road_moves() const
{
	return road_moves;
}

bool pompelmous::can_load_unit(unit* u, int x, int y) const
{
	if(!u->is_land_unit())
		return false;
	if(u->carried())
		return false;
	const std::list<unit*>& units = m->units_on_spot(x, y);
	for(std::list<unit*>::const_iterator it = units.begin();
			it != units.end();
			++it) {
		if((*it)->civ_id != u->civ_id)
			continue;
		if(civs[u->civ_id]->can_load_unit(u, *it)) {
			return true;
		}
	}
	return false;
}

void pompelmous::load_unit(unit* u, int x, int y)
{
	const std::list<unit*>& units = m->units_on_spot(x, y);
	for(std::list<unit*>::const_iterator it = units.begin();
			it != units.end();
			++it) {
		if((*it)->civ_id != u->civ_id)
			continue;
		if(civs[u->civ_id]->can_load_unit(u, *it)) {
			civs[u->civ_id]->load_unit(u, *it);
			return;
		}
	}
}

bool pompelmous::can_unload_unit(unit* u, int chx, int chy) const
{
	std::list<unit*>::iterator it = u->carried_units.begin();
	if(it == u->carried_units.end()) {
		return false;
	}
	else {
		return civs[u->civ_id]->can_move_unit(*it, chx, chy);
	}
}

void pompelmous::unload_unit(unit* u, int chx, int chy)
{
	civs[u->civ_id]->move_unit(*u->carried_units.begin(), chx, chy, false);
}

bool pompelmous::try_wakeup_loaded(unit* u)
{
	if(!u->carrying())
		return false;
	for(std::list<unit*>::iterator it = u->carried_units.begin();
			it != u->carried_units.end();
			++it) {
		(*it)->wake_up();
	}
	return true;
}

int pompelmous::get_num_turns() const
{
	return num_turns;
}

map& pompelmous::get_map()
{
	return *m;
}

const map& pompelmous::get_map() const
{
	return *m;
}

void pompelmous::add_action_listener(action_listener* cb)
{
	action_listeners.push_back(cb);
}

void pompelmous::remove_action_listener(action_listener* cb)
{
	action_listeners.remove(cb);
}

unsigned int pompelmous::get_city_growth_turns(const city* c) const
{
	int food, prod, comm;
	civs[c->civ_id]->total_resources(*c, &food, &prod, &comm);
	food -= c->get_city_size() * food_eaten_per_citizen;
	if(food < 1)
		return 0;
	int needed_food = needed_food_for_growth(c->get_city_size()) - c->stored_food;
	if(needed_food < 1)
		return 1;
	return (needed_food + food - 1) / food;
}

unsigned int pompelmous::get_city_production_turns(const city* c,
		const city_production& cp) const
{
	if(cp.current_production_id >= 0) {
		if(cp.producing_unit) {
			unit_configuration_map::const_iterator it = 
				uconfmap.find(cp.current_production_id);
			if(it != uconfmap.end()) {
				return get_city_production_turns(c, it->second);
			}
		}
		else {
			city_improv_map::const_iterator it = 
				cimap.find(cp.current_production_id);
			if(it != cimap.end()) {
				return get_city_production_turns(c, it->second);
			}
		}
	}
	return 0;
}

unsigned int pompelmous::get_city_production_turns(const city* c,
		const unit_configuration& uc) const
{
	int food, prod, comm;
	civs[c->civ_id]->total_resources(*c, &food, &prod, &comm);
	if(prod < 1)
		return 0;
	int needed_prod = uc.production_cost - c->stored_prod;
	if(needed_prod < 1)
		return 1;
	return (needed_prod + prod - 1) / prod;
}

unsigned int pompelmous::get_city_production_turns(const city* c,
		const city_improvement& ci) const
{
	int food, prod, comm;
	civs[c->civ_id]->total_resources(*c, &food, &prod, &comm);
	if(prod < 1)
		return 0;
	int needed_prod = ci.cost - c->stored_prod;
	if(needed_prod < 1)
		return 1;
	return (needed_prod + prod - 1) / prod;
}

const unsigned int pompelmous::get_food_eaten_per_citizen() const
{
	return food_eaten_per_citizen;
}

void pompelmous::combat(unit* u1, unit* u2)
{
	unsigned int u1chance, u2chance;
	if(!combat_chances(u1, u2, &u1chance, &u2chance))
		return;
	if(u2chance == 0) {
		u2->strength = 0;
		return;
	}
	unsigned int val = rand() % (u1chance + u2chance);
	printf("Combat on (%d, %d) - chances: (%d vs %d - %3.2f) - ",
			u2->xpos, u2->ypos, u1chance, u2chance,
			u1chance / ((float)u1chance + u2chance));
	if(val < u1chance) {
		u1->strength = std::max<unsigned int>(1, u1->strength * (val + 1) / u1chance);
		u2->strength = 0;
		printf("attacker won\n");
	}
	else {
		u1->strength = 0;
		u2->strength = std::max<unsigned int>(1, u2->strength * (val + 1 - u1chance) / u2chance);
		printf("defender won\n");
	}
}

bool pompelmous::combat_chances(const unit* u1, const unit* u2,
		unsigned int* u1chance,
		unsigned int* u2chance) const
{
	int off_bonus = get_offense_bonus(u1, u2);
	int def_bonus = get_defense_bonus(u2, u1);
	if(!can_attack(m, *u1, *u2))
		return false;
	if(u1->strength == 0 || u2->strength == 0)
		return false;
	unsigned int s1 = u1->strength;
	unsigned int s2 = u2->uconf->max_strength ? u2->strength : 0;
	for(unsigned int i = 0; i < max_num_unit_bonuses; i++) {
		switch(u1->uconf->unit_bonuses[i].type) {
			case unit_bonus_group:
				if(u1->uconf->unit_bonuses[i].bonus_data.group_mask & u2->uconf->unit_group_mask) {
					s1 *= (100 + u1->uconf->unit_bonuses[i].bonus_amount) / 100.0f;
				}
				break;
			case unit_bonus_city:
				if(m->city_on_spot(u2->xpos, u2->ypos) != NULL) {
					s1 *= (100 + u1->uconf->unit_bonuses[i].bonus_amount) / 100.0f;
				}
				break;
			case unit_bonus_none:
			default:
				break;
		}
	}
	for(unsigned int i = 0; i < max_num_unit_bonuses; i++) {
		switch(u2->uconf->unit_bonuses[i].type) {
			case unit_bonus_group:
				if(u2->uconf->unit_bonuses[i].bonus_data.group_mask & u1->uconf->unit_group_mask) {
					s2 *= (100 + u2->uconf->unit_bonuses[i].bonus_amount) / 100.0f;
				}
				break;
			case unit_bonus_city:
			case unit_bonus_none:
			default:
				break;
		}
	}
	if(u1->veteran)
		s1 *= 1.5f;
	if(u2->veteran)
		s2 *= 1.5f;
	s1 *= (100 + off_bonus) / 100.0f;
	if(s2) {
		s2 *= (100 + def_bonus) / 100.0f;
		if(u2->is_fortified())
			s2 *= 2;
		if(m->has_river(u2->xpos, u2->ypos))
			s2 *= 1.25f;
	}
	*u1chance = s1 * s1;
	*u2chance = s2 * s2;
	return true;
}

void pompelmous::start_revolution(civilization* civ)
{
	set_government(civ, ANARCHY_INDEX);
	civ->set_anarchy_period(anarchy_period_turns);
}

void pompelmous::set_government(civilization* civ, int gov_id)
{
	government_map::const_iterator git = govmap.find(gov_id);
	if(git != govmap.end()) {
		civ->set_government(&git->second);
	}
}

bool pompelmous::suggest_peace(int civ_id1, int civ_id2)
{
	std::map<int, diplomat*>::iterator it = diplomat_handlers.find(civ_id2);
	bool do_peace = true;
	if(it != diplomat_handlers.end() && it->second) {
		do_peace = it->second->peace_suggested(civ_id1);
	}
	if(do_peace)
		peace_between(civ_id1, civ_id2);
	return do_peace;
}

