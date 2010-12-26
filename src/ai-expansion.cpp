#include <queue>

#include "map-astar.h"
#include "ai-expansion.h"
#include "ai-debug.h"
#include "ai-defense.h"

class escort_orders : public goto_orders {
	public:
		escort_orders(const civilization* civ_, unit* u_, 
				unsigned int escortee_id_, int etgtx_, int etgty_);
		action get_action();
		void drop_action();
		bool finished();
		bool replan();
		void clear();
	private:
		unsigned int escortee_id;
		bool failed;
		int etgtx;
		int etgty;
};

class found_city_orders : public goto_orders {
	public:
		found_city_orders(const civilization* civ_, unit* u_, 
				const city_plan_map_t& planned_,
				const ai_tunables_found_city& found_city_, 
				int x_, int y_);
		action get_action();
		void drop_action();
		bool finished();
		bool replan();
		void clear();
	private:
		const ai_tunables_found_city& found_city;
		int city_points;
		bool failed;
		const city_plan_map_t& planned;
};

class transport_orders : public goto_orders {
	public:
		transport_orders(const civilization* civ_, unit* u_, 
				int tgtx_, int tgty_);
		action get_action();
		bool finished();
		void add_transportee_id(unsigned int i);
	private:
		std::list<unsigned int> transportee_ids;
};

class transported_orders : public goto_orders {
	public:
		transported_orders(const civilization* civ_, unit* u_, 
				unsigned int transporter_id_,
				int unloadx_, int unloady_);
		action get_action();
		bool finished();
		bool replan();
	private:
		unsigned int transporter_id;
		bool done;
		int unloadx;
		int unloady;
};

int points_for_city_founding(const civilization* civ,
		const ai_tunables_found_city& found_city,
		const city_plan_map_t& planned,
		unsigned int unit_id, int counter, const coord& co)
{
	if(counter <= 0) {
		return -1;
	}

	if(!civ->m->can_found_city_on(co.x, co.y)) {
		return -1;
	}

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
		if(civ->m->manhattan_distance_x(it->second.x, co.x) +
		   civ->m->manhattan_distance_y(it->second.y, co.y)
			<= found_city.min_dist_to_friendly_city)
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
			comm_points < found_city.min_comm_points) {
		return -1;
	}

	return clamp<int>(0,
			found_city.found_city_coeff * 
			(food_points + prod_points + comm_points),
			found_city.max_found_city_prio);
}

int find_overseas_city_spot(const civilization* civ,
		const ai_tunables_found_city& found_city,
		const city_plan_map_t& planned, const unit* u,
		int* tgtx, int* tgty)
{
	int best_val = -1;
	*tgtx = -1;
	*tgty = -1;
	for(int i = 0; i < civ->m->size_x(); i++) {
		for(int j = 0; j < civ->m->size_y(); j++) {
			if(civ->fog_at(i, j)) {
				if(civ->m->connected_to_sea(i, j)) {
					int this_val = points_for_city_founding(civ,
							found_city, planned,
							u->unit_id, 1,
							coord(i, j));
					if(this_val > best_val) {
						best_val = this_val;
						*tgtx = i;
						*tgty = j;
					}
				}
			}
		}
	}
	ai_debug_printf(civ->civ_id, "found overseas spot: (%d, %d) (%d).\n",
			*tgtx, *tgty, best_val);
	return best_val;
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
		bool also_overseas,
		const unit* u, int* tgtx, int* tgty, int* prio,
		bool* overseas)
{
	found_city_picker picker(myciv, found_city, planned, u);
	boost::function<bool(const coord& a)> testfunc = boost::ref(picker);
	if(overseas)
		*overseas = false;
	map_path_to_nearest(*myciv, 
			*u, 
			false,
			coord(u->xpos, u->ypos), 
			testfunc);
	if(picker.pq.empty() || picker.pq.top().first < 1) {
		if(also_overseas && myciv->can_cross_oceans()) {
			int overseas_points = find_overseas_city_spot(myciv,
					found_city, planned, u,
					tgtx, tgty);
			if(overseas_points > 0) {
				if(prio) {
					*prio = overseas_points;
				}
				if(overseas)
					*overseas = true;
				return true;
			}
		}
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

expansion_objective::expansion_objective(pompelmous* r_, civilization* myciv_,
		const std::string& n)
	: objective(r_, myciv_, n),
	need_settler(false),
	need_transporter(false)
{
}

bool expansion_objective::compare_units(const unit_configuration& lhs,
		const unit_configuration& rhs) const
{
	if(need_transporter) {
		if(lhs.ocean_unit && rhs.is_land_unit())
			return true;
		if(lhs.is_land_unit() && rhs.ocean_unit)
			return false;
		if(lhs.carry_units >= 2 && rhs.carry_units < 2)
			return true;
		if(lhs.carry_units < 2 && rhs.carry_units >= 2)
			return false;
	}
	if(need_transporter || need_settler) {
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
	if(need_transporter) {
		return uc.ocean_unit && uc.carry_units >= 2;
	}
	else if(need_settler) {
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
	need_transporter = transportees.find(coord(c.xpos, c.ypos)) != transportees.end();
	need_settler = !need_transporter && escorters.find(coord(c.xpos, c.ypos)) != escorters.end();
	city_production cp = objective::get_city_production(c, points);
	need_settler = false;
	need_transporter = false;
	return cp;
}

int expansion_objective::get_unit_points(const unit& u) const
{
	if(myciv->cities.size() == 0) {
		return 1000;
	}
	else {
		int tgtx, tgty;
		int prio;
		if(!find_best_city_pos(myciv, found_city, 
					planned_cities,
					myciv->m->connected_to_sea(u.xpos, u.ypos),
					&u,
					&tgtx, &tgty, &prio, NULL)) {
			return -1;
		}
		else {
			return prio;
		}
	}
}

bool expansion_objective::add_unit(unit* u)
{
	int tgtx, tgty, prio;
	bool found_pos;
	bool overseas;
	ai_debug_printf(myciv->civ_id, "settler: %d\n",
			u->uconf.settler);
	if(u->uconf.settler && myciv->cities.size() == 0) {
		tgtx = u->xpos;
		tgty = u->ypos;
		found_pos = true;
		prio = 1000;
		overseas = false;
	}
	else if(!u->uconf.ocean_unit) {
		found_pos = find_best_city_pos(myciv, found_city, planned_cities, 
				myciv->m->connected_to_sea(u->xpos, u->ypos),
				u, &tgtx, &tgty, &prio, &overseas);
		if(!found_pos) {
			return false;
		}
		if(prio < 1) {
			return false;
		}
	}
	if(u->uconf.settler) {
		orders* o;
		std::map<coord, unsigned int>::iterator eit = escorters.find(coord(u->xpos, u->ypos));
		if(!overseas) {
			ai_debug_printf(myciv->civ_id, "not overseas\n");
			o = new found_city_orders(myciv, u, planned_cities, found_city, tgtx, tgty);

			// set up escorter
			if(eit != escorters.end()) {
				ordersmap_t::iterator oit = ordersmap.find(eit->second);
				if(oit != ordersmap.end()) {
					// orders for the designated escorter found
					delete oit->second;
					std::map<unsigned int, unit*>::iterator uit = myciv->units.find(eit->second);
					if(uit != myciv->units.end()) {
						// designated escorter still exists
						ordersmap[eit->second] = new escort_orders(myciv,
								uit->second, u->unit_id,
								tgtx, tgty);
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
			// take the sea route
			city* c = myciv->m->city_on_spot(u->xpos, u->ypos);
			if(!c) {
				return false;
			}
			o = new wait_orders(u, 200);
			std::list<std::pair<coord, unsigned int> > transportees_here;
			transportees_here.push_back(std::make_pair(coord(tgtx, tgty), u->unit_id));
			// set up escorter
			if(eit != escorters.end()) {
				transportees_here.push_back(std::make_pair(coord(tgtx, tgty), eit->second));
			}
			transportees.insert(std::make_pair(coord(c->xpos, c->ypos),
						transportees_here));
		}
		planned_cities[u->unit_id] = coord(tgtx, tgty);
		ai_debug_printf(myciv->civ_id, "Adding planned city at (%d, %d) by %d.\n",
				tgtx, tgty, u->unit_id);
		ordersmap.insert(std::make_pair(u->unit_id, o));
	}
	else if(u->uconf.ocean_unit) {
		// transporter
		std::map<coord, std::list<std::pair<coord, unsigned int> > >::iterator trit = transportees.find(coord(u->xpos, u->ypos));
		if(trit == transportees.end()) {
			// no transportees?
			ai_debug_printf(myciv->civ_id, "No transportees for %d at (%d, %d).\n",
					u->unit_id, u->xpos, u->ypos);
			return false;
		}
		tgtx = -1;
		tgty = -1;
		transport_orders* o = NULL;
		std::list<unsigned int> transportee_ids;
		ai_debug_printf(myciv->civ_id, "Checking %d potential transportees at (%d, %d).\n",
				trit->second.size(), u->xpos, u->ypos);
		for(std::list<std::pair<coord, unsigned int> >::iterator it = trit->second.begin();
				it != trit->second.end();) {
			ai_debug_printf(myciv->civ_id, "Checking potential transportee %d going to (%d, %d).\n",
					it->second, it->first.x, it->first.y);
			ordersmap_t::iterator oit = ordersmap.find(it->second);
			if(oit != ordersmap.end()) {
				// delete transportee's previous orders (wait_orders)
				delete oit->second;
			}
			std::map<unsigned int, unit*>::iterator transportee_it = myciv->units.find(it->second);
			if(transportee_it == myciv->units.end()) {
				// transportee's gone missing - oh hell, 
				// transport the rest anyway.
				ai_debug_printf(myciv->civ_id, "transportee %d not found.\n", it->second);
				trit->second.erase(it++);
				continue;
			}
			if(!o) {
				tgtx = it->first.x;
				tgty = it->first.y;
				o = new transport_orders(myciv, u, tgtx, tgty);
				ai_debug_printf(myciv->civ_id, "First transportee for %d going to (%d, %d).\n",
						u->unit_id, tgtx, tgty);
			}
			else {
				if(it->first.x != tgtx || it->first.y != tgty) {
					ai_debug_printf(myciv->civ_id, "Transportee for %d going to (%d, %d) instead.\n",
							u->unit_id, it->first.x, it->first.y);
					++it;
					continue;
				}
			}
			ai_debug_printf(myciv->civ_id, "transportee %d (%d, %d) at transporter %d (%d, %d) for (%d, %d).\n",
					transportee_it->second->unit_id, 
					transportee_it->second->xpos,
					transportee_it->second->ypos,
					u->unit_id, u->xpos, u->ypos, tgtx, tgty);
			transported_orders* tro = new transported_orders(myciv, 
					transportee_it->second,
					u->unit_id, tgtx, tgty);
			if(oit != ordersmap.end()) {
				oit->second = tro;
			}
			else {
				ordersmap.insert(std::make_pair(it->second, tro));
			}
			o->add_transportee_id(it->second);
			trit->second.erase(it++);
		}
		if(trit->second.empty()) {
			transportees.erase(trit);
		}
		if(o) {
			ai_debug_printf(myciv->civ_id, "Transporter %d ready.\n",
					u->unit_id);
			ordersmap.insert(std::make_pair(u->unit_id, o));
		}
		else {
			ai_debug_printf(myciv->civ_id, "No transportees for transporter %d.\n",
					u->unit_id);
			return false;
		}
	}
	else {
		escorters.insert(std::make_pair(coord(u->xpos, u->ypos), u->unit_id));
		wait_orders* o = new wait_orders(u, 200);
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
	ai_debug_printf(civ->civ_id, "constructor: %d (%d, %d)\n",
			city_points, tgtx, tgty);
}

action found_city_orders::get_action()
{
	ai_debug_printf(civ->civ_id, "action at path size: %d\n", path.size());
	if(path.empty()) {
		int new_city_points = points_for_city_founding(civ,
				found_city, planned, u->unit_id, 1, coord(u->xpos, u->ypos));
		ai_debug_printf(civ->civ_id, "new: %d; old: %d; coord: (%d, %d)\n",
				new_city_points, city_points, u->xpos, u->ypos);
		if(new_city_points < 1 || new_city_points < city_points) {
			if(replan() && !path.empty()) {
				return get_action();
			}
			else {
				failed = true;
				return action_none;
			}
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
	ai_debug_printf(civ->civ_id, "replanning as %d.\n",
			u->unit_id);
	bool succ = find_best_city_pos(civ, found_city,
			planned, false, u, &tgtx, &tgty, NULL, NULL);
	if(succ)
		city_points = points_for_city_founding(civ, found_city,
				planned, u->unit_id, 1, coord(tgtx, tgty));
	if(!(succ && city_points > 0))
		failed = true;
	else
		goto_orders::replan();
	ai_debug_printf(civ->civ_id, "replan: %d (%d, %d) - path %d\n",
			city_points, tgtx, tgty, path.size());
	return !failed;
}

void found_city_orders::clear()
{
	goto_orders::clear();
}

escort_orders::escort_orders(const civilization* civ_, unit* u_, 
		unsigned int escortee_id_, int etgtx_, int etgty_)
	: goto_orders(civ_, u_, false, u_->xpos, u_->ypos),
	escortee_id(escortee_id_),
	failed(false), etgtx(etgtx_),
	etgty(etgty_)
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
		if(tgtx == u->xpos && tgty == u->ypos) {
			tgtx = etgtx;
			tgty = etgty;
		}
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

transport_orders::transport_orders(const civilization* civ_, unit* u_, 
		int tgtx_, int tgty_)
	: goto_orders(civ_, u_, false, tgtx_, tgty_, true)
{
}

action transport_orders::get_action()
{
	for(std::list<unsigned int>::iterator trit = transportee_ids.begin();
			trit != transportee_ids.end();) {
		bool found = false;
		for(std::list<unit*>::iterator it = u->carried_units.begin();
				it != u->carried_units.end();
				++it) {
			if((*it)->unit_id == (int)*trit) {
				found = true;
				break;
			}
		}
		if(!found) {
			ai_debug_printf(civ->civ_id, "Transporter %d: still missing %d.\n",
					u->unit_id, *trit);
			if(civ->units.find(*trit) == civ->units.end()) {
				ai_debug_printf(civ->civ_id, "Transporter %d: transportee %d gone AWOL.\n",
						u->unit_id, *trit);
				transportee_ids.erase(trit++);
			}
			else {
				// wait for transportee arrival
				return action_none;
			}
		}
		else {
			// this transportee is loaded, check the next one.
			++trit;
		}
	}
	ai_debug_printf(civ->civ_id, "Transporter %d: off to (%d, %d) (Now at (%d, %d)): %s (%d)\n",
			u->unit_id, tgtx, tgty, u->xpos, u->ypos, goto_orders::get_action().to_string().c_str(),
			path.size());
	return goto_orders::get_action();
}

bool transport_orders::finished()
{
	return path.size() <= 1 && u->carried_units.size() == 0;
}

void transport_orders::add_transportee_id(unsigned int i)
{
	transportee_ids.push_back(i);
}

transported_orders::transported_orders(const civilization* civ_, unit* u_, 
		unsigned int transporter_id_, int unloadx_,
		int unloady_)
	: goto_orders(civ_, u_, false, u_->xpos, u_->ypos),
	transporter_id(transporter_id_),
	done(false),
	unloadx(unloadx_),
	unloady(unloady_)
{
	replan();
}

action transported_orders::get_action()
{
	ai_debug_printf(civ->civ_id, "transportee: (%d, %d) => (%d, %d) => (%d, %d) - carried: %d; done: %d.\n",
			u->xpos, u->ypos, tgtx, tgty, unloadx, unloady, u->carried(), done);
	// check for unload
	if(civ->m->manhattan_distance_x(u->xpos, unloadx) <= 1 &&
			civ->m->manhattan_distance_y(u->ypos, unloady) <= 1) {
		int chx = unloadx - u->xpos;
		int chy = unloady - u->ypos;
		done = true;
		return move_unit_action(u, chx, chy);
	}
	else if(!u->carried()) {
		if(u->xpos == tgtx && u->ypos && tgty) {
			// at the transporter
			return unit_action(action_load, u);
		}
		else {
			// going to the transporter
			return goto_orders::get_action();
		}
	}
	else {
		// carried, not at destination => be transported
		return action_none;
	}
}

bool transported_orders::finished()
{
	return done;
}

bool transported_orders::replan()
{
	if(done)
		return false;
	std::map<unsigned int, unit*>::const_iterator it = civ->units.find(transporter_id);
	if(it == civ->units.end()) {
		// no transporter
		done = true;
		return false;
	}
	else {
		// NOTE: only works when the transporter is on a terrain valid
		// for the transportee (coastal city)
		tgtx = it->second->xpos;
		tgty = it->second->ypos;
		if(tgtx == u->xpos && tgty == u->ypos)
			return true;
		if(!goto_orders::replan()) {
			done = true;
			return false;
		}
		else {
			return true;
		}
	}
}

