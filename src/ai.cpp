#include <utility>
#include <stdio.h>

#include "map-astar.h"
#include "ai.h"

primitive_orders::primitive_orders(const action& a_)
	: a(a_), finished_flag(false)
{
}

action primitive_orders::get_action()
{
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

goto_orders::goto_orders(const map& m_, const fog_of_war& fog_, unit* u_, int x_, int y_)
	: tgtx(x_),
	tgty(y_),
	m(m_),
	fog(fog_),
	u(u_)
{
	path = map_astar(m, fog, *u, coord(u->xpos, u->ypos), coord(tgtx, tgty));
	if(!path.empty())
		path.pop_front();
}

action goto_orders::get_action()
{
	if(path.empty())
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

explore_orders::explore_orders(const map& m_, const fog_of_war& fog_, unit* u_)
	: m(m_),
	fog(fog_),
	u(u_)
{
	get_new_path();
}

class explore_picker {
	private:
		const fog_of_war& fog;
		const map& m;
	public:
		explore_picker(const map& m_, const fog_of_war& fog_) : fog(fog_), m(m_) { };
		bool operator()(const coord& c) {
			for(int i = -1; i <= 1; i++) {
				for(int j = -1; j <= 1; j++) {
					if(i == 0 && j == 0)
						continue;
					if(c.x + i < 0 || c.x + i >= m.size_x())
						continue;
					if(c.y + j < 0 || c.y + j >= m.size_y())
						continue;
					if(fog.get_value(c.x + i, c.y + j) == 0) {
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
	explore_picker picker(m, fog);
	std::list<coord> fog_path = map_path_to_nearest(m, 
			fog, 
			*u, 
			coord(u->xpos, u->ypos), 
			picker);
	for(std::list<coord>::const_iterator it = fog_path.begin();
			it != fog_path.end();
			++it) {
		printf("(%d, %d) ", it->x, it->y);
	}
	printf("\n(%d, %d)\n", u->xpos, u->ypos);
	if(!fog_path.empty()) {
		path = map_astar(m, fog, *u, coord(u->xpos, u->ypos), coord(fog_path.back().x, fog_path.back().y));
		if(!path.empty())
			path.pop_front();
	}
}

action explore_orders::get_action()
{
	if(path.empty())
		return action(action_none);
	int chx = path.front().x - u->xpos;
	int chy = path.front().y - u->ypos;
	return move_unit_action(u, chx, chy);
}

void explore_orders::drop_action()
{
	if(!path.empty())
		path.pop_front();
	if(path.empty())
		get_new_path();
}

bool explore_orders::finished()
{
	return path.empty();
}

int explore_orders::path_length()
{
	return path.size();
}

ai_tunable_parameters::ai_tunable_parameters()
	: def_wanted_units_in_city(3),
	def_per_unit_prio(100),
	exploration_min_prio(100),
	exploration_max_prio(1000),
	exploration_length_decr_coeff(10)
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
	for(std::list<unit*>::iterator it = myciv->units.begin();
			it != myciv->units.end();
			++it) {
		ordersmap_t::iterator oit = ordersmap.find(*it);
		// check whether previous orders were given
		if(oit == ordersmap.end())
			oit = ordersmap.insert(std::make_pair(*it, create_orders(*it))).first;
	}
	for(ordersmap_t::iterator oit = ordersmap.begin();
			oit != ordersmap.end();
			++oit) {
		action a = oit->second->get_action();
		int success = r.perform_action(myciv->civ_id, a, &m);
		if(!success) {
			printf("AI error: could not perform action.\n");
		}
		else {
			oit->second->drop_action();
			if(oit->second->finished()) {
				delete oit->second;
				ordersmap.erase(oit--);
			}
		}
	}
	int success = r.perform_action(myciv->civ_id, action(action_eot), &m);
	return !success;
}

orders* ai::create_orders(unit* u)
{
	if(u->uconf.settler) {
		return found_new_city(u);
	}
	return military_unit_orders(u);
}

orders* ai::military_unit_orders(unit* u)
{
	ordersqueue_t ordersq;
	get_exploration_prio(ordersq, u);
	get_defense_prio(ordersq, u);
	std::pair<int, orders*> best = ordersq.top();
	ordersq.pop();
	while(!ordersq.empty()) {
		delete ordersq.top().second;
		ordersq.pop();
	}
	return best.second;
}

void ai::get_exploration_prio(ordersqueue_t& pq, unit* u)
{
	orders_composite* o = new orders_composite();
	explore_orders* e = new explore_orders(m, myciv->fog, u);
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
	pq.push(std::make_pair(prio, o));
}

void ai::get_defense_prio(ordersqueue_t& pq, unit* u)
{
	int tgtx, tgty;
	int prio = 1;
	tgtx = u->xpos;
	tgty = u->ypos;
	if(myciv->cities.size() != 0) {
		city* c = find_nearest_own_city(u);
		if(c) {
			tgtx = c->xpos;
			tgty = c->ypos;
			const std::list<unit*>& units = m.units_on_spot(tgtx, tgty);
			prio = std::max<int>(0, param.def_wanted_units_in_city - units.size()) * 
				param.def_per_unit_prio;
		}
	}
	orders_composite* o = new orders_composite();
	o->add_orders(new goto_orders(m, myciv->fog, u, tgtx, tgty));
	o->add_orders(new primitive_orders(unit_action(action_fortify, u)));
	pq.push(std::make_pair(prio, o));
}

orders* ai::found_new_city(unit* u)
{
	int tgtx, tgty;
	if(myciv->cities.size() == 0) {
		tgtx = u->xpos;
		tgty = u->ypos;
	}
	else {
		find_best_city_pos(u, &tgtx, &tgty);
	}
	orders_composite* o = new orders_composite();
	o->add_orders(new goto_orders(m, myciv->fog, u, tgtx, tgty));
	o->add_orders(new primitive_orders(unit_action(action_found_city, u)));
	return o;
}

void ai::find_best_city_pos(const unit* u, int* tgtx, int* tgty) const
{
	// TODO
	*tgtx = u->xpos;
	*tgty = u->ypos;
}

class city_picker {
	private:
		unsigned int civ_id;
		const map& m;
	public:
		city_picker(const map& m_, unsigned int civ_id_) : civ_id(civ_id_), m(m_) { };
		bool operator()(const coord& c) {
			return m.has_city_of(c, civ_id);
		}
};

city* ai::find_nearest_own_city(const unit* u) const
{
	city_picker picker(m, myciv->civ_id);
	std::list<coord> path_to_city = map_path_to_nearest(m, 
			myciv->fog, 
			*u, 
			coord(u->xpos, u->ypos), 
			picker);
	if(path_to_city.empty()) {
		printf("AI error: find nearest own city: none found.\n");
		return NULL;
	}
	else {
		return m.city_on_spot(path_to_city.back().x,
				path_to_city.back().y);
	}
}

