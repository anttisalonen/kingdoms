#include <stdio.h>
#include <algorithm>
#include <cmath>
#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "round.h"

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

void combat(unit* u1, unit* u2)
{
	if(!can_attack(*u1, *u2))
		return;
	if(u1->strength == 0 || u2->strength == 0)
		return;
	if(u2->uconf.max_strength == 0) {
		u2->strength = 0;
		return;
	}
	unsigned int s1 = u1->strength;
	unsigned int s2 = u2->strength;
	if(u1->veteran)
		s1 *= 1.5f;
	if(u2->veteran)
		s2 *= 1.5f;
	if(u2->is_fortified())
		s2 *= 2;
	unsigned int u1chance = s1 * s1;
	unsigned int u2chance = s2 * s2;
	unsigned int val = rand() % (u1chance + u2chance);
	printf("Combat on (%d, %d) - chances: (%d vs %d - %3.2f) - ",
			u2->xpos, u2->ypos, u1chance, u2chance,
			u1chance / ((float)u1chance + u2chance));
	if(val < u1chance) {
		u1->strength = u1->strength * (val + 1) / u1chance;
		u2->strength = 0;
		printf("attacker won\n");
	}
	else {
		u1->strength = 0;
		u2->strength = u2->strength * (val + 1 - u1chance) / u2chance;
		printf("defender won\n");
	}
}

action::action(action_type t)
	: type(t)
{
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

round::round(const unit_configuration_map& uconfmap_,
		const advance_map& amap_,
		const city_improv_map& cimap_,
		map& m_, unsigned int road_moves_)
	: uconfmap(uconfmap_),
	amap(amap_),
	cimap(cimap_),
	m(m_),
	round_number(0),
	road_moves(road_moves_)
{
	current_civ = civs.begin();
}

void round::add_civilization(civilization* civ)
{
	civs.push_back(civ);
	current_civ = civs.begin();
	refill_moves();
}

void round::refill_moves()
{
	for(std::vector<civilization*>::iterator it = civs.begin();
	    it != civs.end();
	    ++it) {
		(*it)->refill_moves(uconfmap);
	}
}

void round::increment_resources()
{
	for(std::vector<civilization*>::iterator it = civs.begin();
	    it != civs.end();
	    ++it) {
		(*it)->increment_resources(uconfmap, amap, cimap,
				road_moves);
	}
}

bool round::next_civ()
{
	if(civs.empty())
		return false;
	current_civ++;
	if(current_civ == civs.end()) {
		round_number++;
		fprintf(stderr, "Round: %d\n", round_number);
		current_civ = civs.begin();
		increment_resources();
		check_for_city_updates();
		refill_moves();
		return true;
	}
	return false;
}

int round::needed_food_for_growth(int city_size) const
{
	return city_size * 10;
}

int round::needed_culture_for_growth(int culture_level) const
{
	return std::pow(10, culture_level);
}

void round::check_for_city_updates()
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
				coord rescoord = next_good_resource_spot(c, &m);
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
	}
	if(update_land)
		update_land_owners();
}

const unit_configuration* round::get_unit_configuration(int uid) const
{
	unit_configuration_map::const_iterator it = uconfmap.find(uid);
	if(it == uconfmap.end())
		return NULL;
	return &it->second;
}

bool round::perform_action(int civid, const action& a)
{
	if(civid < 0 || civid != current_civ_id()) {
		return false;
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
						bool can_build = a.data.unit_data.u->uconf.settler;
						if(can_build && 
							m.city_on_spot(a.data.unit_data.u->xpos, 
								a.data.unit_data.u->ypos) == NULL && 
							m.can_found_city_on(a.data.unit_data.u->xpos,
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
					a.data.unit_data.u->fortify();
					return true;
				case action_improvement:
					if(a.data.unit_data.unit_action_data.improv == improv_none) {
						return false;
					}
					if(!a.data.unit_data.u->uconf.worker) {
						return false;
					}
					if(!m.can_improve_terrain(a.data.unit_data.u->xpos,
							a.data.unit_data.u->ypos,
							a.data.unit_data.u->civ_id,
							a.data.unit_data.unit_action_data.improv))
						return false;
					a.data.unit_data.u->start_improving_to(a.data.unit_data.unit_action_data.improv,
							m.get_needed_turns_for_improvement(a.data.unit_data.unit_action_data.improv));
					return true;
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

void round::check_city_conquer(int tgtxpos, int tgtypos, int conquering_civid)
{
	city* c = m.city_on_spot(tgtxpos, tgtypos);
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
		}
		else {
			civ->remove_city(c, true);
			update_land_owners();
		}
	}
}

void round::destroy_improvements(city* c)
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

void round::update_land_owners()
{
	for(int i = 0; i < m.size_x(); i++)
		for(int j = 0; j < m.size_y(); j++)
			m.set_land_owner(-1, i, j);

	for(std::vector<civilization*>::iterator it = civs.begin();
	    it != civs.end();
	    ++it) {
		for(std::map<unsigned int, city*>::iterator cit = (*it)->cities.begin();
				cit != (*it)->cities.end();
				++cit) {
			m.grab_land(cit->second);
		}
	}
}

void round::check_civ_elimination(int civ_id)
{
	civilization* civ = civs[civ_id];
	if(civ->cities.size() == 0) {
		int num_settlers = std::count_if(civ->units.begin(),
				civ->units.end(),
				boost::bind(&unit::is_settler, boost::lambda::_1));
		if(num_settlers == 0) {
			civ->eliminate();
		}
	}
}

bool round::try_move_unit(unit* u, int chx, int chy)
{
	int tgtxpos = u->xpos + chx;
	int tgtypos = u->ypos + chy;

	// attack square?
	int def_id = m.get_spot_resident(tgtxpos, tgtypos);
	if(def_id >= 0 && def_id != u->civ_id) {
		if(in_war(u->civ_id, def_id)) {
			const std::list<unit*>& units = m.units_on_spot(tgtxpos, tgtypos);
			if(units.size() != 0) {
				unit* defender = units.front();
				if(!can_attack(*u, *defender)) {
					return false;
				}
				combat(u, defender);
				if(u->strength == 0) {
					// lost combat
					(*current_civ)->remove_unit(u);
					return true;
				}
				else if(defender->strength == 0) {
					// won combat
					(*civs[def_id]).remove_unit(defender);
				}
			}
			if(m.units_on_spot(tgtxpos, tgtypos).size() == 0) {
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
	if((*current_civ)->can_move_to(tgtxpos, tgtypos)) {
		if((*current_civ)->try_move_unit(u, chx, chy)) {
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
		return false;
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

int round::current_civ_id() const
{
	if(current_civ == civs.end())
		return -1;
	return (*current_civ)->civ_id;
}

void round::declare_war_between(unsigned int civ1, unsigned int civ2)
{
	civs[civ1]->set_war(civ2);
	civs[civ2]->set_war(civ1);
}

void round::peace_between(unsigned int civ1, unsigned int civ2)
{
	civs[civ1]->set_peace(civ2);
	civs[civ2]->set_peace(civ1);
}

bool round::in_war(unsigned int civ1, unsigned int civ2) const
{
	return civs[civ1]->get_relationship_to_civ(civ2) == relationship_war;
}

int round::get_round_number() const
{
	return round_number;
}

unsigned int round::get_num_road_moves() const
{
	return road_moves;
}

