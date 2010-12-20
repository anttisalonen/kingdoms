#include <utility>
#include <stdio.h>

#include "map-astar.h"
#include "ai.h"
#include "ai-debug.h"

ai_tunable_parameters::ai_tunable_parameters()
	: max_defense_prio(1000),
	defense_units_prio_coeff(400),
	exploration_min_prio(100),
	exploration_max_prio(600),
	exploration_length_decr_coeff(50),
	unit_prodcost_prio_coeff(0),
	offense_dist_prio_coeff(50),
	unit_strength_prio_coeff(1),
	max_offense_prio(1000),
	worker_prio(700),
	ci_barracks_value(300),
	ci_granary_value(600),
	ci_comm_bonus_coeff(10),
	ci_culture_coeff(100),
	ci_happiness_coeff(300),
	ci_cost_coeff(1)
{
}

ai::ai(map& m_, round& r_, civilization* c)
	: m(m_),
	r(r_),
	myciv(c)
{
	objectives.push_back(std::make_pair(new defense_objective(&r, myciv, "defense"), 1200));
	objectives.push_back(std::make_pair(new offense_objective(&r, myciv, "offense"), 1100));
	objectives.push_back(std::make_pair(new expansion_objective(&r, myciv, "expansion"), 1000));
	objectives.push_back(std::make_pair(new exploration_objective(&r, myciv, "exploration"), 900));
	objectives.push_back(std::make_pair(new commerce_objective(&r, myciv, "commerce"), 800));
}

bool ai::play()
{
	// handle messages
	while(!myciv->messages.empty())  {
		msg& m = myciv->messages.front();
		switch(m.type) {
			case msg_new_unit:
				handle_new_unit(m);
				break;
			case msg_civ_discovery:
				handle_civ_discovery(m.msg_data.discovered_civ_id);
				break;
			case msg_new_advance:
				handle_new_advance(m.msg_data.new_advance_id);
				break;
			case msg_new_city_improv:
				handle_new_improv(m);
				break;
			case msg_unit_disbanded:
				handle_unit_disbanded(m);
				break;
			default:
				break;
		}
		myciv->messages.pop_front();
	}

	// assign orders to cities not producing anything
	for(std::map<unsigned int, city*>::iterator it = myciv->cities.begin();
			it != myciv->cities.end();
			++it) {
		city* c = it->second;
		if(!c->producing_something()) {
			create_city_orders(c);
		}
	}

	// check for new units
	for(std::map<unsigned int, unit*>::iterator it = myciv->units.begin();
			it != myciv->units.end();
			++it) {
		if(handled_units.find(it->second->unit_id) == handled_units.end()) {
			handled_units.insert(it->second->unit_id);
			ai_debug_printf(myciv->civ_id, "adding unhandled unit %s (%d) as free.\n",
					it->second->uconf.unit_name.c_str(), it->second->unit_id);
			free_units.insert(it->second->unit_id);
		}
	}

	// assign free units to objectives
	{
		std::set<unsigned int>::iterator it = free_units.begin();
		while(it != free_units.end()) {
			std::map<unsigned int, unit*>::iterator uit = myciv->units.find(*it);
			if(uit == myciv->units.end()) {
				free_units.erase(it++);
			}
			else {
				ai_debug_printf(myciv->civ_id, "assigning free unit %s (%d).\n",
						uit->second->uconf.unit_name.c_str(),
						uit->second->unit_id);
				bool succ = assign_free_unit(uit->second);
				if(succ) {
					free_units.erase(it++);
				}
				else {
					ai_debug_printf(myciv->civ_id,
							"could not assign free unit.\n");
					++it;
				}
			}
		}
	}

	// perform unit orders
	for(std::list<std::pair<objective*, int> >::iterator it = objectives.begin();
			it != objectives.end();
			++it) {
		std::set<unsigned int> freed_units;
		it->first->process(&freed_units);
		for(std::set<unsigned int>::const_iterator fit = freed_units.begin();
				fit != freed_units.end();
				++fit) {
			free_units.insert(*fit);
		}
	}

	// send end of turn
	int success = r.perform_action(myciv->civ_id, action(action_eot));
	return !success;
}

void ai::create_city_orders(city* c)
{
	int max_points = -1;
	objective* chosen = NULL;
	ai_debug_printf(myciv->civ_id, "===\ndeciding what to build in %s...\n", 
			c->cityname.c_str());
	city_production cp(false, 0);

	for(std::list<std::pair<objective*, int> >::const_iterator it = objectives.begin();
			it != objectives.end();
			++it) {
		int points = 0;
		city_production cp2 = it->first->get_city_production(*c, &points);
		if(cp.current_production_id != -1) {
			int tot_points = points * it->second;
			if(tot_points > max_points) {
				max_points = tot_points;
				chosen = it->first;
				cp = cp2;
			}
		}
	}
	if(chosen) {
		c->set_production(cp);
		building_cities[c->city_id] = chosen;
		ai_debug_printf(myciv->civ_id, "building %s ID %d for objective '%s'.\n",
				cp.producing_unit ? "unit" : "improvement",
				cp.current_production_id, chosen->get_name().c_str());
	}
	else {
		ai_debug_printf(myciv->civ_id, "AI: could not find any production at %s.\n",
				c->cityname.c_str());
	}
}

bool ai::assign_free_unit(unit* u)
{
	int max_points = -1;
	objective* chosen = NULL;
	ai_debug_printf(myciv->civ_id, "assigning unit %s...\n", u->uconf.unit_name.c_str());
	for(std::list<std::pair<objective*, int> >::const_iterator it = objectives.begin();
			it != objectives.end();
			++it) {
		int points = it->first->get_unit_points(*u);
		ai_debug_printf(myciv->civ_id, "%s: %s: %d\n",
				it->first->get_name().c_str(),
				u->uconf.unit_name.c_str(), points);
		if(points > 0) {
			int tot_points = points * it->second;
			if(tot_points > max_points) {
				max_points = tot_points;
				chosen = it->first;
			}
		}
	}
	if(chosen) {
		return chosen->add_unit(u);
	}
	else {
		return false;
	}
}

void ai::handle_new_advance(unsigned int adv_id)
{
}

void ai::handle_civ_discovery(int civ_id)
{
	r.declare_war_between(myciv->civ_id, civ_id);
}

void ai::handle_new_improv(const msg& m)
{
}

void ai::handle_new_unit(const msg& m)
{
	// allocate new unit to an objective.
	std::map<unsigned int, unit*>::iterator uit = myciv->units.find(m.msg_data.city_prod_data.unit_id);
	if(uit != myciv->units.end()) {
		bool unit_added = false;
		std::map<unsigned int, objective*>::iterator oit = 
			building_cities.find(m.msg_data.city_prod_data.building_city_id);

		// destination objective predefined.
		if(oit != building_cities.end()) {
			bool succ = oit->second->add_unit(uit->second);
			ai_debug_printf(myciv->civ_id, "adding %s to %s.\n",
					uit->second->uconf.unit_name.c_str(),
					oit->second->get_name().c_str());
			if(!succ) {
				ai_debug_printf(myciv->civ_id, "could not add unit.\n");
			}
			building_cities.erase(oit);
			if(succ) {
				ai_debug_printf(myciv->civ_id, "adding handled unit %s (%d).\n",
						uit->second->uconf.unit_name.c_str(), 
						uit->second->unit_id);
				handled_units.insert(uit->second->unit_id);
				unit_added = true;
			}
		}
		if(!unit_added) {
			// unit could not be added to the predefined 
			// objective - try best fit.
			if(assign_free_unit(uit->second)) {
				ai_debug_printf(myciv->civ_id, "adding handled unit %s (%d).\n",
						uit->second->uconf.unit_name.c_str(), 
						uit->second->unit_id);
				handled_units.insert(uit->second->unit_id);
			}
			else {
				// could not assign - play() will add the
				// unit to free units.
				ai_debug_printf(myciv->civ_id, "could not assign free unit.\n");
			}
		}
	}

	// find a new build objective for the city.
	std::map<unsigned int, city*>::iterator cit = myciv->cities.find(m.msg_data.city_prod_data.building_city_id);
	if(cit != myciv->cities.end()) {
		ai_debug_printf(myciv->civ_id, "new unit %s built in %s.\n",
				uit->second->uconf.unit_name.c_str(),
				cit->second->cityname.c_str());
		create_city_orders(cit->second);
	}
}

void ai::handle_unit_disbanded(const msg& m)
{
	ai_debug_printf(myciv->civ_id, "unit disbanded.\n");
}

#if 0
std::pair<int, int> ai::create_city_improv_orders(city* c)
{
	std::pair<int, int> top = std::make_pair(-1, -1);
	for(city_improv_map::const_iterator it = r.cimap.begin();
			it != r.cimap.end();
			++it) {
		if(!myciv->can_build_improvement(it->second, *c))
			continue;
		int pr = get_city_improv_value(it->second);
		if(pr > top.first) {
			top.first = pr;
			top.second = it->first;
		}
	}
	return top;
}

int ai::get_city_improv_value(const city_improvement& ci) const
{
	int val = 0;
	if(ci.palace)
		return -1;
	if(ci.barracks)
		val += param.ci_barracks_value;
	if(ci.granary)
		val += param.ci_granary_value;
	val += param.ci_comm_bonus_coeff * ci.comm_bonus;
	val += param.ci_culture_coeff * ci.culture;
	val += param.ci_happiness_coeff * ci.happiness;
	val -= param.ci_cost_coeff * ci.cost;
	if(debug) {
		printf("%s: %d\n", ci.improv_name.c_str(),
				val);
	}
	return val;
}

ai::orderprio_t ai::get_exploration_prio(unit* u)
{
	explore_orders* e = new explore_orders(myciv, u, false);
	int len = e->path_length();
	int prio;
	if(len == 0)
		prio = 0;
	else
		prio = clamp<int>(param.exploration_min_prio, 
				param.exploration_max_prio + 
				param.unit_strength_prio_coeff * math::pow(u->uconf.max_strength, 2) - 
				e->path_length() * param.exploration_length_decr_coeff, 
				param.exploration_max_prio);
	if(debug)
		printf("exploration: %d; ", prio);
	return std::make_pair(prio, e);
}

#endif


