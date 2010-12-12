#include <utility>
#include <stdio.h>

// man 3p round
namespace math {
#include <math.h>
}

#include "map-astar.h"
#include "ai.h"

ai_tunable_parameters::ai_tunable_parameters()
	: max_defense_prio(1000),
	defense_units_prio_coeff(600),
	exploration_min_prio(100),
	exploration_max_prio(600),
	exploration_length_decr_coeff(50),
	unit_prodcost_prio_coeff(0),
	offense_dist_prio_coeff(50),
	unit_strength_prio_coeff(1),
	max_offense_prio(1000),
	worker_prio(900)
{
}

ai::ai(map& m_, round& r_, civilization* c, bool debug_)
	: m(m_),
	r(r_),
	myciv(c),
	debug(debug_)
{
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
			cityordersmap[c] = create_city_orders(c);
		}
	}

	// perform city orders
	for(cityordersmap_t::iterator oit = cityordersmap.begin();
			oit != cityordersmap.end();
			++oit) {
		oit->first->set_production(*oit->second);
		delete oit->second;
	}
	cityordersmap.clear();

	// assign orders to units and clean up the orders map for
	// lost units
	{
		ordersmap_t::iterator oit = ordersmap.begin();

		// copy the unit pointers for sorting - if the performace
		// suffers too much, maybe rather use std::set<unit*> as a
		// civ member variable instead
		std::list<unit*> units(myciv->units.begin(), myciv->units.end());
		units.sort();
		for(std::list<unit*>::iterator it = units.begin();
				it != units.end();
				++it) {
			while(oit != ordersmap.end() && *it > oit->first) {
				// orders given for a unit that doesn't exist
				// (anymore) - delete orders
				delete oit->second;
				ordersmap.erase(oit++);
			}
			if(oit != ordersmap.end() && *it == oit->first) {
				// orders already given
				if(oit->second->finished()) {
					delete oit->second;
					ordersmap.erase(oit);
				}
				else {
					++oit;
					continue;
				}
			}
			// no orders given
			orderprio_t o = create_orders(*it);
			oit = ordersmap.insert(std::make_pair(*it, o.second)).first;
			++oit;
		}
		// delete orders of the units that don't exist anymore
		// (address greater than the last element in the unit list)
		while(oit != ordersmap.end()) {
			delete oit->second;
			ordersmap.erase(oit++);
		}
	}

	// perform unit orders
	for(ordersmap_t::iterator oit = ordersmap.begin();
			oit != ordersmap.end();
			++oit) {
		action a = oit->second->get_action();
		int success = r.perform_action(myciv->civ_id, a);
		if(!success) {
			if(debug)
				printf("AI error: could not perform action.\n");
			oit->second->replan();
			action a = oit->second->get_action();
			success = r.perform_action(myciv->civ_id, a);
			if(!success) {
				if(debug)
					printf("AI error: still could not perform action.\n");
				oit->second->clear();
			}
		}
		else {
			oit->second->drop_action();
		}
	}

	// send end of turn
	int success = r.perform_action(myciv->civ_id, action(action_eot));
	return !success;
}

city_production* ai::create_city_orders(city* c)
{
	city_production* cp = new city_production();

	cp->producing_unit = true;
	std::priority_queue<std::pair<int, int> > unitpq;
	for(unit_configuration_map::const_iterator it = r.uconfmap.begin();
			it != r.uconfmap.end();
			++it) {
		unit dummy(0, c->xpos, c->ypos, myciv->civ_id,
				it->second, r.get_num_road_moves());
		orderprio_t o = create_orders(&dummy);
		unitpq.push(std::make_pair(o.first - param.unit_prodcost_prio_coeff * it->second.production_cost, 
					it->first));
	}
	if(unitpq.empty()) {
		cp->current_production_id = -1;
	}
	else {
		cp->current_production_id = unitpq.top().second;
	}
	return cp;
}

ai::orderprio_t ai::create_orders(unit* u)
{
	if(u->uconf.settler) {
		orderprio_t found = found_new_city(u);
		if(found.first <= 0) {
			return get_defense_orders(u);
		}
		else {
			return found;
		}
	}
	else if(u->uconf.worker) {
		orderprio_t work = workers_orders(u);
		if(work.first < 0) {
			return get_defense_orders(u);
		}
		else {
			return work;
		}
	}
	else {
		return military_unit_orders(u);
	}
}

class worker_searcher {
	private:
		const civilization* myciv;
		int maxrange;
		unit* found_unit;
		const unit* self;
	public:
		worker_searcher(const civilization* myciv_, const unit* self_, int maxrange_) : 
			myciv(myciv_), maxrange(maxrange_), found_unit(NULL), self(self_) { }
		const unit* get_found_unit() const { return found_unit; }
		bool operator()(const coord& co) {
			const std::list<unit*>& units = myciv->m->units_on_spot(co.x, co.y);
			for(std::list<unit*>::const_iterator it = units.begin();
					it != units.end();
					++it) {
				if((*it)->uconf.worker && (*it) != self) {
					found_unit = *it;
					return true;
				}
			}
			maxrange--;
			if(maxrange < 0) {
				return true;
			}
			return false;
		}
};

ai::orderprio_t ai::workers_orders(unit* u)
{
	int prio = param.worker_prio;
	worker_searcher searcher(myciv, u, 25);
	boost::function<bool(const coord& a)> testfunc = boost::ref(searcher);
	map_path_to_nearest(*myciv, 
			*u, 
			false,
			coord(u->xpos, u->ypos), 
			testfunc);
	if(searcher.get_found_unit() != NULL) {
		prio = -1;
	}
	orders_composite* o = new orders_composite();
	orders* io = new improve_orders(myciv, u);
	if(io->finished())
		prio = -1;
	o->add_orders(io);
	return std::make_pair(prio, o);
}

ai::orderprio_t ai::military_unit_orders(unit* u)
{
	ordersqueue_t ordersq;
	if(debug)
		printf("AI (%s): ", u->uconf.unit_name.c_str());
	get_exploration_prio(ordersq, u);
	get_defense_prio(ordersq, u);
	get_offense_prio(ordersq, u);
	if(debug)
		printf("\n");
	orderprio_t best = ordersq.top();
	ordersq.pop();
	while(!ordersq.empty()) {
		delete ordersq.top().second;
		ordersq.pop();
	}
	return best;
}

void ai::get_exploration_prio(ordersqueue_t& pq, unit* u)
{
	orders_composite* o = new orders_composite();
	explore_orders* e = new explore_orders(myciv, u, false);
	o->add_orders(e);
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
	pq.push(std::make_pair(prio, o));
}

ai::orderprio_t ai::get_defense_orders(unit* u)
{
	int tgtx, tgty;
	int prio = 1;
	tgtx = u->xpos;
	tgty = u->ypos;
	if(myciv->cities.size() != 0) {
		city* c = find_nearest_city(myciv, u, true);
		if(c) {
			tgtx = c->xpos;
			tgty = c->ypos;
			const std::list<unit*>& units = m.units_on_spot(tgtx, tgty);
			int num_units = std::count_if(units.begin(),
					units.end(),
					std::mem_fun(&unit::is_military_unit));
			if(tgtx == u->xpos && tgty == u->ypos)
				num_units--;
			prio = clamp<int>(1, param.max_defense_prio + 
					param.unit_strength_prio_coeff * math::pow(u->uconf.max_strength, 2) - 
					param.defense_units_prio_coeff * num_units,
					param.max_defense_prio);
		}
	}
	orders_composite* o = new orders_composite();
	o->add_orders(new goto_orders(myciv, u, false, tgtx, tgty));
	o->add_orders(new primitive_orders(unit_action(action_fortify, u)));
	o->add_orders(new wait_orders(u, 50)); // time not updating orders
	if(debug)
		printf("defense: %d; ", prio);
	return std::make_pair(prio, o);
}

void ai::get_defense_prio(ordersqueue_t& pq, unit* u)
{
	pq.push(get_defense_orders(u));
}

void ai::get_offense_prio(ordersqueue_t& pq, unit* u)
{
	int tgtx, tgty;
	int prio = -1;
	tgtx = u->xpos;
	tgty = u->ypos;
	if(find_nearest_enemy(u, &tgtx, &tgty)) {
		prio = std::max<int>(0, param.max_offense_prio + 
				param.unit_strength_prio_coeff * math::pow(u->uconf.max_strength, 2) - 
				param.offense_dist_prio_coeff * (abs(tgtx - u->xpos) + abs(tgty - u->ypos)));
	}
	orders_composite* o = new orders_composite();
	o->add_orders(new attack_orders(myciv, u, tgtx, tgty));
	if(debug)
		printf("offense: %d; ", prio);
	pq.push(std::make_pair(prio, o));
}

ai::orderprio_t ai::found_new_city(unit* u)
{
	int tgtx, tgty;
	int prio;
	if(myciv->cities.size() == 0) {
		prio = 1000;
		tgtx = u->xpos;
		tgty = u->ypos;
	}
	else {
		if(!find_best_city_pos(myciv, param.found_city, u, &tgtx, &tgty, &prio)) {
			return std::make_pair(-1, new primitive_orders(unit_action(action_skip, u)));
		}
	}
	orders_composite* o = new orders_composite();
	o->add_orders(new found_city_orders(myciv, u, param.found_city, tgtx, tgty));
	return std::make_pair(prio, o);
}

class enemy_picker {
	private:
		const civilization* myciv;
	public:
		enemy_picker(const civilization* myciv_) : 
			myciv(myciv_) { }
		bool operator()(const coord& co) {
			const city* c = myciv->m->city_on_spot(co.x, co.y);
			const std::list<unit*>& units = myciv->m->units_on_spot(co.x, co.y);
			int civid = -1;
			if(c)
				civid = c->civ_id;
			else if(!units.empty())
				civid = units.front()->civ_id;
			return civid != -1 && civid != (int)myciv->civ_id &&
				 myciv->get_relationship_to_civ(civid) == relationship_war;
		}
};

bool ai::find_nearest_enemy(const unit* u, int* tgtx, int* tgty) const
{
	enemy_picker picker(myciv);
	std::list<coord> found_path = map_path_to_nearest(*myciv, 
			*u, 
			true,
			coord(u->xpos, u->ypos), 
			picker);
	if(found_path.empty()) {
		return false;
	}
	else {
		*tgtx = found_path.back().x;
		*tgty = found_path.back().y;
		return true;
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
	std::map<unsigned int, city*>::iterator cit = myciv->cities.find(m.msg_data.city_prod_data.building_city_id);
	if(cit != myciv->cities.end()) {
		city* c = cit->second;
		cityordersmap[c] = create_city_orders(c);
	}
}

void ai::handle_new_unit(const msg& m)
{
	std::map<unsigned int, city*>::iterator cit = myciv->cities.find(m.msg_data.city_prod_data.building_city_id);
	if(cit != myciv->cities.end())
		cityordersmap[cit->second] = create_city_orders(cit->second);
}

void ai::handle_unit_disbanded(const msg& m)
{
	if(debug)
		printf("AI: unit disbanded.\n");
}
