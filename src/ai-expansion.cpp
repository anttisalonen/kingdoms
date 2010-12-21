#include <queue>

#include "map-astar.h"
#include "ai-expansion.h"
#include "ai-debug.h"
#include "ai-defense.h"

int points_for_city_founding(const civilization* civ,
		const ai_tunables_found_city& found_city,
		const city_plan_map_t& planned,
		unsigned int unit_id, int counter, const coord& co)
{
	if(counter <= 0)
		return -1;

	if(!civ->m->can_found_city_on(co.x, co.y))
		return -1;

	// do not found a city nearer than N squares to a friendly city
	for(int i = -found_city.min_dist_to_friendly_city; 
			i <= found_city.min_dist_to_friendly_city; i++)
		for(int j = -found_city.min_dist_to_friendly_city; 
				j <= -found_city.min_dist_to_friendly_city; j++)
			if(civ->m->has_city_of(co.x + i, co.y + j, civ->civ_id))
				return -1;

	// do not found a city nearer than N squares to any city
	for(int i = -found_city.min_dist_to_city; i <= found_city.min_dist_to_city; i++)
		for(int j = -found_city.min_dist_to_city; j <= found_city.min_dist_to_city; j++)
			if(civ->m->city_on_spot(co.x + i, co.y + j))
				return -1;

	// do not found a city nearer than N squares to a planned city site
	for(city_plan_map_t::const_iterator it = planned.begin();
			it != planned.end();
			++it) {
		if(it->first == unit_id)
			continue;
		int manh_diff = civ->m->manhattan_distance(it->second.x, it->second.y, co.x, co.y);
		if(manh_diff < found_city.min_dist_to_friendly_city)
			return -1;
	}

	int food_points = 0;
	int prod_points = 0;
	int comm_points = 0;
	civ->m->get_total_city_resources(co.x, co.y, &food_points,
			&prod_points, &comm_points);

	food_points *= found_city.food_coeff;
	prod_points *= found_city.prod_coeff;
	comm_points *= found_city.comm_coeff;

	// filter out very bad locations
	if(food_points < found_city.min_food_points || 
			prod_points < found_city.min_prod_points || 
			comm_points < found_city.min_comm_points)
		return -1;

	return clamp<int>(0,
			found_city.found_city_coeff * 
			(food_points + prod_points + comm_points),
			found_city.max_found_city_prio);
}

class found_city_picker {
	private:
		const civilization* myciv;
		const ai_tunables_found_city& found_city;
		// counter acts as a sort of distance-to-origin meter
		int counter;
		const city_plan_map_t& planned;
		const unit* u;
	public:
		std::priority_queue<std::pair<int, coord> > pq;
		// only inspect 100 nearest locations
		found_city_picker(const civilization* myciv_, 
				const ai_tunables_found_city& found_city_,
				const city_plan_map_t& planned_, const unit* u_) :
			myciv(myciv_), found_city(found_city_), 
			counter(found_city_.max_search_range),
			planned(planned_), u(u_) { }
		bool operator()(const coord& co);
};

bool found_city_picker::operator()(const coord& co)
{
	counter -= found_city.range_coeff;
	if(counter <= 0)
		return true;

	int points = points_for_city_founding(myciv,
			found_city, planned, u->unit_id, counter, co);

	pq.push(std::make_pair(points, co));
	return false;
}

bool find_best_city_pos(const civilization* myciv,
		const ai_tunables_found_city& found_city,
		const city_plan_map_t& planned,
		const unit* u, int* tgtx, int* tgty, int* prio)
{
	found_city_picker picker(myciv, found_city, planned, u);
	boost::function<bool(const coord& a)> testfunc = boost::ref(picker);
	map_path_to_nearest(*myciv, 
			*u, 
			false,
			coord(u->xpos, u->ypos), 
			testfunc);
	if(picker.pq.empty()) {
		*tgtx = u->xpos;
		*tgty = u->ypos;
		if(prio) 
			*prio = -1;
		return false;
	}
	std::pair<int, coord> val = picker.pq.top();
	*tgtx = val.second.x;
	*tgty = val.second.y;
	if(prio)
		*prio = val.first;
	return true;
}

int found_new_city(const unit* u, civilization* myciv, 
		const city_plan_map_t& planned,
		const ai_tunables_found_city& found_city)
{
	if(myciv->cities.size() == 0) {
		return 1000;
	}
	else {
		int tgtx, tgty;
		int prio;
		if(!find_best_city_pos(myciv, found_city, 
					planned, u, &tgtx, &tgty, &prio)) {
			return -1;
		}
		else {
			return prio;
		}
	}
}

expansion_objective::expansion_objective(round* r_, civilization* myciv_,
		const std::string& n)
	: objective(r_, myciv_, n),
	need_settler(false)
{
}

bool expansion_objective::compare_units(const unit_configuration& lhs,
		const unit_configuration& rhs) const
{
	if(need_settler) {
		if(lhs.max_moves > rhs.max_moves)
			return true;
		if(lhs.max_moves < lhs.max_moves)
			return false;
		if(lhs.production_cost < lhs.production_cost)
			return true;
		if(lhs.production_cost > lhs.production_cost)
			return false;
		return lhs.max_strength > lhs.max_strength;
	}
	else {
		return compare_defense_units(lhs, rhs);
	}
}

bool expansion_objective::usable_unit(const unit_configuration& uc) const
{
	if(need_settler) {
		return uc.settler;
	}
	else {
		return usable_defense_unit(uc);
	}
}

int expansion_objective::improvement_value(const city_improvement& ci) const
{
	return -1;
}

city_production expansion_objective::get_city_production(const city& c, int* points) const
{
	need_settler = escorters.find(coord(c.xpos, c.ypos)) != escorters.end();
	city_production cp = objective::get_city_production(c, points);
	need_settler = false;
	return cp;
}

int expansion_objective::get_unit_points(const unit& u) const
{
	return found_new_city(&u, myciv, planned_cities, found_city);
}

bool expansion_objective::add_unit(unit* u)
{
	int tgtx, tgty, prio;
	if(!find_best_city_pos(myciv, found_city, planned_cities, 
				u, &tgtx, &tgty, &prio)) {
		return false;
	}
	if(prio < 0)
		return false;
	if(u->uconf.settler) {
		found_city_orders* o = new found_city_orders(myciv, u, planned_cities, found_city, tgtx, tgty);
		planned_cities[u->unit_id] = o->get_target_position();
		ordersmap.insert(std::make_pair(u->unit_id, o));
		std::map<coord, unsigned int>::iterator eit = escorters.find(coord(u->xpos, u->ypos));
		if(eit != escorters.end()) {
			ordersmap_t::iterator oit = ordersmap.find(eit->second);
			if(oit != ordersmap.end()) {
				// orders for the designated escorter found
				delete oit->second;
				std::map<unsigned int, unit*>::iterator uit = myciv->units.find(eit->second);
				if(uit != myciv->units.end()) {
					// designated escorter still exists
					ordersmap[eit->second] = new escort_orders(myciv, uit->second, u->unit_id);
				}
				else {
					// designated escorter lost
					ordersmap.erase(oit);
				}
			}
			escorters.erase(eit);
		}
	}
	else {
		escorters.insert(std::make_pair(coord(u->xpos, u->ypos), u->unit_id));
		wait_orders* o = new wait_orders(u, 20);
		ordersmap.insert(std::make_pair(u->unit_id, o));
	}
	return true;
}

void expansion_objective::process(std::set<unsigned int>* freed_units)
{
	objective::process(freed_units);
	for(city_plan_map_t::iterator it = planned_cities.begin();
			it != planned_cities.end();) {
		if(ordersmap.find(it->first) == ordersmap.end()) {
			ai_debug_printf(myciv->civ_id, "Planned city has been found at (%d, %d).\n",
					it->second.x, it->second.y);
			planned_cities.erase(it++);
		}
		else {
			++it;
		}
	}
}

ai_tunables_found_city::ai_tunables_found_city()
	: min_dist_to_city(3),
	min_dist_to_friendly_city(4),
	food_coeff(3),
	prod_coeff(1),
	comm_coeff(0),
	min_food_points(12),
	min_prod_points(2),
	min_comm_points(0),
	max_search_range(500),
	range_coeff(1),
	max_found_city_prio(600),
	found_city_coeff(6)
{
}

found_city_orders::found_city_orders(const civilization* civ_,
		unit* u_, const city_plan_map_t& planned_,
		const ai_tunables_found_city& found_city_, 
		int x_, int y_)
	: goto_orders(civ_, u_, false, x_, y_),
	found_city(found_city_), failed(false),
	planned(planned_)
{
	city_points = points_for_city_founding(civ, found_city,
			planned, u->unit_id, 1, coord(tgtx, tgty));
}

action found_city_orders::get_action()
{
	if(path.empty()) {
		int new_city_points = points_for_city_founding(civ,
				found_city, planned, u->unit_id, 1, coord(u->xpos, u->ypos));
		if(new_city_points < 1 || new_city_points < city_points) {
			if(replan())
				return get_action();
			else
				return action_none;
		}
		else {
			return unit_action(action_found_city, u);
		}
	}
	else {
		return goto_orders::get_action();
	}
}

void found_city_orders::drop_action()
{
	goto_orders::drop_action();
}

bool found_city_orders::finished()
{
	return failed;
}

bool found_city_orders::replan()
{
	bool succ = find_best_city_pos(civ, found_city,
			planned, u, &tgtx, &tgty, NULL);
	if(succ)
		city_points = points_for_city_founding(civ, found_city,
				planned, u->unit_id, 0, coord(tgtx, tgty));
	if(!(succ && city_points > 0))
		failed = true;
	return !failed;
}

void found_city_orders::clear()
{
	goto_orders::clear();
}

escort_orders::escort_orders(const civilization* civ_, unit* u_, 
		unsigned int escortee_id_)
	: goto_orders(civ_, u_, false, u_->xpos, u_->ypos),
	escortee_id(escortee_id_),
	failed(false)
{
	replan();
}

action escort_orders::get_action()
{
	if(!replan()) {
		return action_none;
	}
	else {
		return goto_orders::get_action();
	}
}

void escort_orders::drop_action()
{
	goto_orders::drop_action();
}

bool escort_orders::finished()
{
	return failed;
}

bool escort_orders::replan()
{
	std::map<unsigned int, unit*>::const_iterator uit = civ->units.find(escortee_id);
	if(uit != civ->units.end()) {
		tgtx = uit->second->xpos;
		tgty = uit->second->ypos;
		return goto_orders::replan();
	}
	else {
		failed = true;
		return false;
	}
}

void escort_orders::clear()
{
	goto_orders::clear();
	failed = true;
}

