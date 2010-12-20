#include <queue>
#include <stdio.h>

#include "ai-orders.h"
#include "map-astar.h"

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

goto_orders::goto_orders(const civilization* civ_, unit* u_, 
		bool ignore_enemy_, int x_, int y_)
	: tgtx(x_),
	tgty(y_),
	civ(civ_),
	u(u_),
	ignore_enemy(ignore_enemy_)
{
	get_new_path();
}

void goto_orders::get_new_path()
{
	path = map_astar(*civ, *u, ignore_enemy, coord(u->xpos, u->ypos), coord(tgtx, tgty));
	if(!path.empty())
		path.pop_front();
}

action goto_orders::get_action()
{
	if(finished())
		return action(action_none);
	if(tgtx == u->xpos && tgty == u->ypos) {
		path.clear();
		return action(action_none);
	}
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

coord goto_orders::get_target_position()
{
	return coord(tgtx, tgty);
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

city* find_nearest_city(const civilization* myciv, const unit& u, bool own)
{
	city_picker picker(myciv, own);
	std::list<coord> path_to_city = map_path_to_nearest(*myciv, 
			u, 
			false,
			coord(u.xpos, u.ypos), 
			picker);
	if(path_to_city.empty()) {
		return NULL;
	}
	else {
		return myciv->m->city_on_spot(path_to_city.back().x,
				path_to_city.back().y);
	}
}


