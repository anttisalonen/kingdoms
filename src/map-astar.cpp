#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "astar.h"
#include "map-astar.h"

#include <stdio.h>

bool terrain_allowed(const map& m, const unit& u, int x, int y)
{
	return m.terrain_allowed(u, x, y);
}

void check_insert(std::set<coord>& s, const civilization& civ,
		const unit& u, bool ignore_enemy, int x, int y)
{
	x = civ.m->wrap_x(x);
	y = civ.m->wrap_y(y);
	if(x >= 0 && y >= 0 && x < civ.m->size_x() && y < civ.m->size_y()) {
		if(terrain_allowed(*civ.m, u, x, y)) {
			int fogval = civ.fog.get_value(x, y);
			if(fogval) { // known terrain
				if((!civ.blocked_by_land(x, y) && 
					(fogval == 1 || civ.can_move_to(x, y))) || ignore_enemy) { 
					// terrain visible and no enemy on it
					s.insert(coord(x, y));
				}
			}
		}
	}
}

std::set<coord> map_graph(const civilization& civ, const unit& u, 
		bool ignore_enemy, const coord& a, const coord* coastal_goal)
{
	std::set<coord> ret;
	for(int i = -1; i <= 1; i++) {
		for(int j = -1; j <= 1; j++) {
			if(i || j) {
				int ax = a.x + i;
				int ay = a.y + j;
				if(coastal_goal && coastal_goal->x == ax &&
					coastal_goal->y == ay) {
					ret.insert(coord(ax, ay));
				}
				else {
					check_insert(ret, civ, u, ignore_enemy,
							ax, ay);
				}
			}
		}
	}
	return ret;
}

std::set<coord> map_bird_graph(const coord& a)
{
	std::set<coord> ret;
	for(int i = -1; i <= 1; i++) {
		for(int j = -1; j <= 1; j++) {
			if(i || j) {
				ret.insert(coord(a.x + i, a.y + j));
			}
		}
	}
	return ret;
}

int map_cost(const map& m, const unit& u, const coord& a, const coord& b)
{
	bool road;
	int cost = m.get_move_cost(u, a.x, a.y, b.x, b.y, &road);
	if(cost < 0) // for the coastal case
		cost = 100000000;
	if(!road)
		return cost * 10;
	else
		return 4;
}

int map_heur(const coord& b, const coord& a)
{
	return abs(b.x - a.x) + abs(b.y - a.y);
}

bool map_goaltest(const coord& b, const coord& a)
{
	return b == a;
}

std::list<coord> map_astar(const civilization& civ,
		const unit& u, bool ignore_enemy,
		const coord& start, const coord& goal,
		bool coastal)
{
	using boost::bind;
	using boost::lambda::_1;
	using boost::lambda::_2;
	using boost::ref;
	const coord* coastal_goal = coastal ? &goal : NULL;
	return astar(bind(map_graph, ref(civ), ref(u), ignore_enemy, _1, 
				coastal_goal),
			bind(map_cost, ref(*civ.m), ref(u), _1, _2),
			bind(map_heur, ref(goal), _1),
			bind(map_goaltest, ref(goal), _1), start);
}

std::list<coord> map_path_to_nearest(const civilization& civ,
		const unit& u, bool ignore_enemy,
		const coord& start, 
		boost::function<bool(const coord& a)> goaltestfunc)
{
	using boost::bind;
	using boost::lambda::_1;
	using boost::lambda::_2;
	using boost::ref;
	const coord* coastal_goal = NULL;
	return astar(bind(map_graph, ref(civ), ref(u), ignore_enemy, 
				_1, coastal_goal),
			boost::lambda::constant(1),
			boost::lambda::constant(0),
			goaltestfunc, start);
}

std::list<coord> map_birds_path_to_nearest(const coord& start,
		boost::function<bool(const coord& a)> goaltestfunc)
{
	using boost::bind;
	using boost::lambda::_1;
	using boost::lambda::_2;
	using boost::ref;
	return astar(bind(map_bird_graph, _1),
			boost::lambda::constant(1),
			boost::lambda::constant(0),
			goaltestfunc, start);
}

