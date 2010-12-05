#include <algorithm>
#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "round.h"

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

round::round(const unit_configuration_map& uconfmap_,
		const advance_map& amap_,
		const city_improv_map& cimap_)
	: uconfmap(uconfmap_),
	amap(amap_),
	cimap(cimap_)
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
		(*it)->increment_resources(uconfmap, amap, cimap);
	}
}

bool round::next_civ()
{
	if(civs.empty())
		return false;
	current_civ++;
	if(current_civ == civs.end()) {
		current_civ = civs.begin();
		increment_resources();
		refill_moves();
		return true;
	}
	return false;
}

const unit_configuration* round::get_unit_configuration(int uid) const
{
	unit_configuration_map::const_iterator it = uconfmap.find(uid);
	if(it == uconfmap.end())
		return NULL;
	return &it->second;
}

bool round::perform_action(int civid, const action& a, map* m)
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
							a.data.unit_data.unit_action_data.move_pos.chy, m);
				case action_found_city:
					{
						const unit_configuration* uconf = get_unit_configuration(a.data.unit_data.u->unit_id);
						bool can_build = uconf == NULL ? false : uconf->settler;
						if(can_build && 
							m->city_on_spot(a.data.unit_data.u->xpos, 
								a.data.unit_data.u->ypos) == NULL) {
							city* c = (*current_civ)->add_city("city name", a.data.unit_data.u->xpos,
									a.data.unit_data.u->ypos);
							set_default_city_production(c, uconfmap);
							(*current_civ)->remove_unit(a.data.unit_data.u);
							return true;
						}
						return false;
					}
				case action_skip:
					a.data.unit_data.u->moves = 0;
					return true;
				case action_fortify:
					a.data.unit_data.u->fortified = true;
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

void round::check_city_conquer(map* m, int tgtxpos, int tgtypos)
{
	city* c = m->city_on_spot(tgtxpos, tgtypos);
	if(c) {
		civilization* civ = civs[c->civ_id];
		civ->remove_city(c);
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

bool round::try_move_unit(unit* u, int chx, int chy, map* m)
{
	int tgtxpos = u->xpos + chx;
	int tgtypos = u->ypos + chy;

	// attack square?
	int def_id = m->get_spot_resident(tgtxpos, tgtypos);
	if(def_id >= 0 && def_id != u->civ_id) {
		if(in_war(u->civ_id, def_id)) {
			const std::list<unit*>& units = m->units_on_spot(tgtxpos, tgtypos);
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
			if(m->units_on_spot(tgtxpos, tgtypos).size() == 0) {
				// check if a city was conquered
				check_city_conquer(m, tgtxpos, tgtypos);
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
				check_city_conquer(m, tgtxpos, tgtypos);
				check_civ_elimination(def_id);
			}
			return true;
		}
		return false;
	}
	else {
		if(def_id >= 0 && def_id != u->civ_id) {
			// won combat - but no move
			u->moves--;
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

