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
	return defend_nearest_city(u);
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

orders* ai::defend_nearest_city(unit* u)
{
	int tgtx, tgty;
	if(myciv->cities.size() == 0) {
		tgtx = u->xpos;
		tgty = u->ypos;
	}
	else {
		find_nearest_own_city(u, &tgtx, &tgty);
	}
	orders_composite* o = new orders_composite();
	o->add_orders(new goto_orders(m, myciv->fog, u, tgtx, tgty));
	o->add_orders(new primitive_orders(unit_action(action_fortify, u)));
	return o;
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

void ai::find_nearest_own_city(const unit* u, int* tgtx, int* tgty) const
{
	*tgtx = u->xpos;
	*tgty = u->ypos;
	city_picker picker(m, myciv->civ_id);
	std::list<coord> path_to_city = map_path_to_nearest(m, 
			myciv->fog, 
			*u, 
			coord(u->xpos, u->ypos), 
			picker);
	if(path_to_city.empty()) {
		printf("AI error: find nearest own city: none found.\n");
	}
	else {
		*tgtx = path_to_city.back().x;
		*tgty = path_to_city.back().y;
	}
	return;
}

