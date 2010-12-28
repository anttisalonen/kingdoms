#include <stdio.h>

#include "ai-objective.h"
#include "ai-debug.h"

objective::objective(pompelmous* r_, civilization* myciv_, const std::string& obj_name_)
	: r(r_), myciv(myciv_), obj_name(obj_name_)
{
}

city_production objective::get_city_production(const city& c, int* points) const
{
	int upoints = -1;
	int ipoints = -1;
	city_production up = best_unit_production(c, &upoints);
	city_production ip = best_improv_production(c, &ipoints);
	if(upoints > ipoints) {
		*points = upoints;
		return up;
	}
	else {
		*points = ipoints;
		return ip;
	}
}

city_production objective::best_unit_production(const city& c, int* points) const
{
	unit_configuration_map::const_iterator chosen = choose_best_unit(*r,
			*myciv, c);
	if(chosen != r->uconfmap.end()) {
		unit dummy(0, chosen->first, c.xpos, c.ypos, myciv->civ_id,
				chosen->second, r->get_num_road_moves());
		*points = get_unit_points(dummy);
		ai_debug_printf(myciv->civ_id, "%-12s: %-12s: %4d\n",
				obj_name.c_str(),
				chosen->second.unit_name.c_str(), *points);
		return city_production(true, chosen->first);
	}
	else {
		return city_production(true, -1);
	}
}

city_production objective::best_improv_production(const city& c, int* points) const
{
	*points = -1;
	city_improv_map::const_iterator chosen = r->cimap.end();
	for(city_improv_map::const_iterator it = r->cimap.begin();
			it != r->cimap.end();
			++it) {
		if(!myciv->can_build_improvement(it->second, c) || it->second.palace)
			continue;
		int tp = improvement_value(it->second);
		if(tp > *points) {
			chosen = it;
			*points = tp;
		}
	}
	if(chosen != r->cimap.end()) {
		ai_debug_printf(myciv->civ_id, "%-12s: %-12s: %4d\n",
				obj_name.c_str(),
				chosen->second.improv_name.c_str(), *points);
		return city_production(false, chosen->first);
	}
	else {
		return city_production(false, -1);
	}
}

void objective::process(std::set<unsigned int>* freed_units)
{
	ordersmap_t::iterator oit = ordersmap.begin();
	while(oit != ordersmap.end()) {
		std::map<unsigned int, unit*>::iterator uit = myciv->units.find(oit->first);
		if(uit == myciv->units.end()) {
			// unit lost
			ordersmap.erase(oit++);
		}
		else {
			if(uit->second->num_moves() == 0 && uit->second->num_road_moves() == 0) {
				++oit;
				continue;
			}
			if(oit->second->finished()) {
				if(!oit->second->replan()) {
					ai_debug_printf(myciv->civ_id, 
							"replan failed - freeing unit.\n");
					freed_units->insert(oit->first);
					ordersmap.erase(oit++);
					continue;
				}
			}
			action a = oit->second->get_action();
#if 0
			ai_debug_printf(myciv->civ_id, "%s - %s - %d: %s.\n",
						obj_name.c_str(), uit->second->uconf->unit_name.c_str(),
						uit->second->unit_id, a.to_string().c_str());
#endif
			int success = r->perform_action(myciv->civ_id, a);
			if(!success) {
				ai_debug_printf(myciv->civ_id, "%s - %s - %d: could not perform action: %s.\n", 
						obj_name.c_str(), uit->second->uconf->unit_name.c_str(),
						uit->second->unit_id, a.to_string().c_str());
				oit->second->replan();
				action a = oit->second->get_action();
				success = r->perform_action(myciv->civ_id, a);
				if(!success) {
					ai_debug_printf(myciv->civ_id, "%s: still could not perform action: %s.\n",
							obj_name.c_str(), a.to_string().c_str());
					freed_units->insert(oit->first);
					ordersmap.erase(oit++);
				}
				else {
					++oit;
				}
			}
			else {
				oit->second->drop_action();
				if(!(a.type == action_unit_action && 
				     a.data.unit_data.uatype == action_move_unit)) {
					++oit;
				}
			}
		}
	}
}

const std::string& objective::get_name() const
{
	return obj_name;
}

unit_configuration_map::const_iterator objective::choose_best_unit(const pompelmous& r, 
		const civilization& myciv, const city& c) const
{
	unit_configuration_map::const_iterator chosen = r.uconfmap.end();
	for(unit_configuration_map::const_iterator it = r.uconfmap.begin();
			it != r.uconfmap.end();
			++it) {
		if(!myciv.can_build_unit(it->second, c))
			continue;
		if(usable_unit(it->second)) {
			if(chosen == r.uconfmap.end() || 
				compare_units(it->second, chosen->second)) {
				chosen = it;
			}
		}
	}
	return chosen;
}


