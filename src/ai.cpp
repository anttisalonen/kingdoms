#include <utility>
#include <stdio.h>

#include "map-astar.h"
#include "ai.h"

// #define AI_DEBUG

class found_city_picker {
	private:
		const civilization* myciv;
		const ai_tunables_found_city& found_city;
		// counter acts as a sort of distance-to-origin meter
		int counter;
	public:
		std::priority_queue<std::pair<int, coord> > pq;
		// only inspect 100 nearest locations
		found_city_picker(const civilization* myciv_, 
				const ai_tunables_found_city& found_city_) :
			myciv(myciv_), found_city(found_city_), 
			counter(found_city_.max_search_range) { }
		bool operator()(const coord& co);
};

int points_for_city_founding(const civilization* civ,
		const ai_tunables_found_city& found_city,
		int counter, const coord& co)
{
	if(counter <= 0)
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

	int food_points = 0;
	int prod_points = 0;
	int comm_points = 0;
	for(int i = -2; i <= 2; i++) {
		for(int j = -2; j <= 2; j++) {
			if(abs(i) == 2 && abs(j) == 2)
				continue;

			int terr = civ->m->get_data(co.x + i, co.y + j);
			int food = 0, prod = 0, comm = 0;
			civ->m->get_resources_by_terrain(terr, false,
					&food, &prod, &comm);
			food_points += food * found_city.food_coeff;
			prod_points += prod * found_city.prod_coeff;
			comm_points += comm * found_city.comm_coeff;
		}
	}

#if 0
	// filter out very bad locations
	if(food_points < found_city.min_food_points || 
			prod_points < found_city.min_prod_points || 
			comm_points < found_city.min_comm_points)
		return -1;
#endif

#ifdef AI_DEBUG
	printf("City prio: %d\n", found_city.found_city_coeff * 
			(counter + food_points + prod_points + comm_points));
#endif
	return clamp<int>(0,
			found_city.found_city_coeff * 
			(counter + food_points + prod_points + comm_points),
			found_city.max_found_city_prio);
}

bool found_city_picker::operator()(const coord& co)
{
	counter -= found_city.range_coeff;
	if(counter <= 0)
		return true;

	int points = points_for_city_founding(myciv,
			found_city, counter, co);

	pq.push(std::make_pair(points, co));
	return false;
}

bool find_best_city_pos(const civilization* myciv,
		const ai_tunables_found_city& found_city,
		const unit* u, int* tgtx, int* tgty, int* prio)
{
	found_city_picker picker(myciv, found_city);
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

primitive_orders::primitive_orders(const action& a_)
	: a(a_), finished_flag(false)
{
}

action primitive_orders::get_action()
{
	if(finished())
		return action(action_none);
	return a;
}

void primitive_orders::drop_action()
{
	finished_flag = true;
}

bool primitive_orders::finished()
{
	return finished_flag;
}

bool primitive_orders::replan()
{
	return false;
}

void primitive_orders::clear()
{
	finished_flag = true;
}

action orders_composite::get_action()
{
	if(finished())
		return action(action_none);
	return ord.front()->get_action();
}

void orders_composite::drop_action()
{
	if(ord.size() > 0) {
		if(ord.front()->finished()) {
			delete ord.front();
			ord.pop_front();
		}
		else {
			ord.front()->drop_action();
		}
	}
}

bool orders_composite::finished()
{
	if(ord.empty())
		return true;
	return ord.size() == 1 && ord.front()->finished();
}

void orders_composite::add_orders(orders* o)
{
	ord.push_back(o);
}

bool orders_composite::replan()
{
	if(ord.empty())
		return false;
	return ord.front()->replan();
}

void orders_composite::clear()
{
	ord.clear();
}

goto_orders::goto_orders(const civilization* civ_, unit* u_, 
		bool ignore_enemy_, int x_, int y_)
	: tgtx(x_),
	tgty(y_),
	civ(civ_),
	u(u_),
	ignore_enemy(ignore_enemy_)
{
	get_new_path();
	if(!path.empty())
		path.pop_front();
}

void goto_orders::get_new_path()
{
	path = map_astar(*civ, *u, ignore_enemy, coord(u->xpos, u->ypos), coord(tgtx, tgty));
}

action goto_orders::get_action()
{
	if(finished())
		return action(action_none);
	int chx = path.front().x - u->xpos;
	int chy = path.front().y - u->ypos;
	return move_unit_action(u, chx, chy);
}

void goto_orders::drop_action()
{
	if(!path.empty())
		path.pop_front();
}

bool goto_orders::finished()
{
	return path.empty();
}

int goto_orders::path_length()
{
	return path.size();
}

bool goto_orders::replan()
{
	get_new_path();
	return !path.empty();
}

void goto_orders::clear()
{
	path.clear();
}

explore_orders::explore_orders(const civilization* civ_, unit* u_,
		bool autocontinue_)
	: goto_orders(civ_, u_, false, u_->xpos, u_->ypos),
	autocontinue(autocontinue_)
{
	get_new_path();
}

class explore_picker {
	private:
		const civilization* civ;
	public:
		explore_picker(const civilization* civ_) : civ(civ_) { }
		bool operator()(const coord& c) {
			for(int i = -1; i <= 1; i++) {
				for(int j = -1; j <= 1; j++) {
					if(i == 0 && j == 0)
						continue;
					if(c.x + i < 0 || c.x + i >= civ->m->size_x())
						continue;
					if(c.y + j < 0 || c.y + j >= civ->m->size_y())
						continue;
					if(civ->fog.get_value(c.x + i, c.y + j) == 0) {
						return true;
					}
				}
			}
			return false;
		}
};

void explore_orders::get_new_path()
{
	path.clear();
	explore_picker picker(civ);
	std::list<coord> fog_path = map_path_to_nearest(*civ, 
			*u, 
			false,
			coord(u->xpos, u->ypos), 
			picker);
	if(!fog_path.empty()) {
		path = map_astar(*civ, *u, false, coord(u->xpos, u->ypos), coord(fog_path.back().x, fog_path.back().y));
		if(!path.empty())
			path.pop_front();
	}
}

void explore_orders::drop_action()
{
	goto_orders::drop_action();
	if(path.empty() && autocontinue)
		get_new_path();
}

bool explore_orders::replan()
{
	get_new_path();
	return !path.empty();
}

void explore_orders::clear()
{
	path.clear();
}

wait_orders::wait_orders(unit* u_, unsigned int rounds)
	: u(u_),
	rounds_to_go(rounds),
	total_rounds(rounds)
{
}

action wait_orders::get_action()
{
	if(finished())
		return action(action_none);
	return unit_action(action_skip, u);
}

void wait_orders::drop_action()
{
	if(rounds_to_go > 0)
		rounds_to_go--;
}

bool wait_orders::finished()
{
	return rounds_to_go == 0;
}

bool wait_orders::replan()
{
	rounds_to_go = total_rounds;
	return true;
}

void wait_orders::clear()
{
	rounds_to_go = 0;
}

attack_orders::attack_orders(const civilization* civ_, unit* u_, int x_, int y_)
	: goto_orders(civ_, u_, true, x_, y_),
	att_x(-1),
	att_y(-1)
{
}

void attack_orders::check_for_enemies()
{
	att_x = -1;
	att_y = -1;
	for(int i = -1; i <= 1; i++) {
		for(int j = -1; j <= 1; j++) {
			if(i == 0 && j == 0)
				continue;
			int xp = u->xpos + i;
			int yp = u->ypos + j;
			if(xp < 0 || xp >= civ->m->size_x())
				continue;
			if(yp < 0 || yp >= civ->m->size_y())
				continue;
			int owner = civ->m->get_spot_resident(xp, yp);
			if(owner >= 0 && owner != (int)civ->civ_id &&
					civ->get_relationship_to_civ(owner) == relationship_war) {
				att_x = xp;
				att_y = yp;
				return;
			}
		}
	}
}

action attack_orders::get_action()
{
	check_for_enemies();
	if(att_x != -1 && att_y != -1) {
		return move_unit_action(u, att_x - u->xpos,
			att_y - u->ypos);
	}
	else {
		return goto_orders::get_action();
	}
}

void attack_orders::drop_action()
{
	if(att_x == -1 || att_y == -1)
		goto_orders::drop_action();
	else {
		att_x = -1;
		att_y = -1;
	}
}

bool attack_orders::finished()
{
	if(att_x == -1 || att_y == -1)
		return goto_orders::finished();
	else
		return false;
}

bool attack_orders::replan()
{
	return goto_orders::replan();
}

void attack_orders::clear()
{
	goto_orders::clear();
}

found_city_orders::found_city_orders(const civilization* civ_,
		unit* u_, const ai_tunables_found_city& found_city_, 
		int x_, int y_)
	: goto_orders(civ_, u_, false, x_, y_),
	found_city(found_city_)
{
	city_points = points_for_city_founding(civ, found_city,
			1, coord(tgtx, tgty));
}

action found_city_orders::get_action()
{
	if(path.empty()) {
		int new_city_points = points_for_city_founding(civ,
				found_city, 1, coord(tgtx, tgty));
		printf("old: %d - new: %d\n", city_points,
				new_city_points);
		if(new_city_points < 1 || new_city_points < city_points) {
			replan();
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
	return false;
}

bool found_city_orders::replan()
{
	bool succ = find_best_city_pos(civ, found_city,
			u, &tgtx, &tgty, NULL);
	if(succ)
		city_points = points_for_city_founding(civ, found_city,
				0, coord(tgtx, tgty));
	return succ && city_points > 0;
}

void found_city_orders::clear()
{
	goto_orders::clear();
}

ai_tunables_found_city::ai_tunables_found_city()
	: min_dist_to_city(3),
	min_dist_to_friendly_city(4),
	food_coeff(1),
	prod_coeff(1),
	comm_coeff(0),
	min_food_points(20),
	min_prod_points(10),
	min_comm_points(5),
	max_search_range(50),
	range_coeff(1),
	max_found_city_prio(1000),
	found_city_coeff(10)
{
}

ai_tunable_parameters::ai_tunable_parameters()
	: max_defense_prio(1000),
	defense_units_prio_coeff(600),
	exploration_min_prio(100),
	exploration_max_prio(600),
	exploration_length_decr_coeff(50),
	unit_prodcost_prio_coeff(1),
	offense_dist_prio_coeff(50),
	max_offense_prio(1000)
{
}

ai::ai(map& m_, round& r_, civilization* c)
	: m(m_),
	r(r_),
	myciv(c)
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
			default:
				break;
		}
		myciv->messages.pop_front();
	}

	// assign orders to cities not producing anything
	for(std::list<city*>::iterator it = myciv->cities.begin();
			it != myciv->cities.end();
			++it) {
		if(!(*it)->producing_something()) {
			cityordersmap[*it] = create_city_orders(*it);
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
		int success = r.perform_action(myciv->civ_id, a, &m);
		if(!success) {
#ifdef AI_DEBUG
			printf("AI error: could not perform action.\n");
#endif
			oit->second->replan();
			action a = oit->second->get_action();
			success = r.perform_action(myciv->civ_id, a, &m);
			if(!success) {
#ifdef AI_DEBUG
				printf("AI error: still could not perform action.\n");
#endif
				oit->second->clear();
			}
		}
		else {
			oit->second->drop_action();
		}
	}

	// send end of turn
	int success = r.perform_action(myciv->civ_id, action(action_eot), &m);
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
				it->second);
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
	return military_unit_orders(u);
}

ai::orderprio_t ai::military_unit_orders(unit* u)
{
	ordersqueue_t ordersq;
#ifdef AI_DEBUG
	printf("AI: ");
#endif
	get_exploration_prio(ordersq, u);
	get_defense_prio(ordersq, u);
	get_offense_prio(ordersq, u);
#ifdef AI_DEBUG
	printf("\n");
#endif
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
				param.exploration_max_prio - 
					e->path_length() * param.exploration_length_decr_coeff, 
				param.exploration_max_prio);
#ifdef AI_DEBUG
	printf("exploration: %d; ", prio);
#endif
	pq.push(std::make_pair(prio, o));
}

ai::orderprio_t ai::get_defense_orders(unit* u)
{
	int tgtx, tgty;
	int prio = 1;
	tgtx = u->xpos;
	tgty = u->ypos;
	if(myciv->cities.size() != 0) {
		city* c = find_nearest_city(u, true);
		if(c) {
			tgtx = c->xpos;
			tgty = c->ypos;
			const std::list<unit*>& units = m.units_on_spot(tgtx, tgty);
			int num_units = units.size();
			if(tgtx == u->xpos && tgty == u->ypos)
				num_units--;
			prio = clamp<int>(1, param.max_defense_prio - 
					param.defense_units_prio_coeff * num_units,
					param.max_defense_prio);
		}
	}
	orders_composite* o = new orders_composite();
	o->add_orders(new goto_orders(myciv, u, false, tgtx, tgty));
	o->add_orders(new primitive_orders(unit_action(action_fortify, u)));
	o->add_orders(new wait_orders(u, 50)); // time not updating orders
#ifdef AI_DEBUG
	printf("defense: %d; ", prio);
#endif
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
		prio = std::max<int>(0, param.max_offense_prio - 
				param.offense_dist_prio_coeff * abs(tgtx - u->xpos) + abs(tgty - u->ypos));
	}
	orders_composite* o = new orders_composite();
	o->add_orders(new attack_orders(myciv, u, tgtx, tgty));
#ifdef AI_DEBUG
	printf("offense: %d; ", prio);
#endif
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
		find_best_city_pos(myciv, param.found_city, u, &tgtx, &tgty, &prio);
	}
	orders_composite* o = new orders_composite();
	o->add_orders(new found_city_orders(myciv, u, param.found_city, tgtx, tgty));
	return std::make_pair(prio, o);
}

class city_picker {
	private:
		const civilization* myciv;
		bool my_city;
	public:
		city_picker(const civilization* myciv_, bool my_city_) : 
			myciv(myciv_), my_city(my_city_) { }
		bool operator()(const coord& co) {
			const city* c = myciv->m->city_on_spot(co.x, co.y);
			if(!c) {
				return false;
			}
			else {
				if(my_city) {
					return c->civ_id == myciv->civ_id;
				}
				else {
					return c->civ_id != myciv->civ_id && 
						myciv->get_relationship_to_civ(c->civ_id) == relationship_war;
				}
			}
		}
};

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

city* ai::find_nearest_city(const unit* u, bool own) const
{
	city_picker picker(myciv, own);
	std::list<coord> path_to_city = map_path_to_nearest(*myciv, 
			*u, 
			false,
			coord(u->xpos, u->ypos), 
			picker);
	if(path_to_city.empty()) {
		return NULL;
	}
	else {
		return m.city_on_spot(path_to_city.back().x,
				path_to_city.back().y);
	}
}

void ai::handle_new_advance(unsigned int adv_id)
{
}

void ai::handle_civ_discovery(int civ_id)
{
}

void ai::handle_new_improv(const msg& m)
{
	city* c = m.msg_data.city_prod_data.building_city;
	cityordersmap[c] = create_city_orders(c);
}

void ai::handle_new_unit(const msg& m)
{
	city* c = m.msg_data.city_prod_data.building_city;
	cityordersmap[c] = create_city_orders(c);
}

