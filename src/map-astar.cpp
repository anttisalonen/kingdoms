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
		const unit& u, bool ignore_enemy, bool only_roads,
		int x, int y)
{
	x = civ.m->wrap_x(x);
	y = civ.m->wrap_y(y);
	if(x >= 0 && y >= 0 && x < civ.m->size_x() && y < civ.m->size_y()) {
		if(terrain_allowed(*civ.m, u, x, y)) {
			int fogval = civ.fog.get_value(x, y);
			if(fogval) { // known terrain
				if((!civ.blocked_by_land(x, y) && 
					(fogval == 1 || civ.move_acceptable_by_land_and_units(x, y))) || ignore_enemy) { 
					// terrain visible and no enemy on it
					if(!only_roads || (civ.m->get_improvements_on(x, y) & improv_road)) {
						s.insert(coord(x, y));
					}
				}
			}
		}
	}
}

std::set<coord> map_graph(const civilization& civ, const unit& u, 
		boost::function<bool(const coord& a)> filterfunc,
		bool ignore_enemy, bool only_roads,
		const coord& a, const coord* coastal_goal)
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
					if(filterfunc(a))
						check_insert(ret, civ, u, ignore_enemy,
								only_roads,
								ax, ay);
				}
			}
		}
	}
	return ret;
}

std::set<coord> map_graph(const civilization& civ, const unit& u, 
		bool ignore_enemy, bool only_roads,
		const coord& a, const coord* coastal_goal)
{
	return map_graph(civ, u, 
			boost::lambda::constant(true),
			ignore_enemy, only_roads,
			a, coastal_goal);
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

std::set<coord> graph_along_roads(const civilization& civ,
		bool no_enemy_territory, bool known_territory,
		const coord& a)
{
	std::set<coord> ret;
	for(int i = -1; i <= 1; i++) {
		for(int j = -1; j <= 1; j++) {
			if(i || j) {
				int x = a.x + i;
				int y = a.y + j;
				if(civ.m->get_improvements_on(x, y) & improv_road) {
					if(known_territory && civ.fog_at(x, y) == 0)
						continue;
					if(no_enemy_territory) {
						int civid = civ.m->get_land_owner(x, y);
						if(civid != -1 &&
							civid != (int)civ.civ_id &&
							civ.get_relationship_to_civ(civid) == relationship_war) {
							continue;
						}
					}
					ret.insert(coord(x, y));
				}
			}
		}
	}
	return ret;
}

int map_cost(const map& m, const unit& u, bool coastal, const coord& a, const coord& b)
{
	bool road;
	int cost = m.get_move_cost(u, a.x, a.y, b.x, b.y, &road);
	if(cost < 0 && coastal)
		cost = 1;
	if(!road)
		return cost * 10;
	else
		return 4; // TODO: make this dependent of road_moves in pompelmous
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
		bool coastal, bool only_roads)
{
	return map_astar(civ, u, ignore_enemy, start, goal,
			coastal, only_roads, 
			boost::lambda::constant(true));
}

std::list<coord> map_astar(const civilization& civ,
		const unit& u, bool ignore_enemy,
		const coord& start, const coord& goal,
		bool coastal, bool only_roads,
		boost::function<bool(const coord& a)> filterfunc)
{
	using boost::bind;
	using boost::lambda::_1;
	using boost::lambda::_2;
	using boost::ref;
	const coord* coastal_goal = coastal ? &goal : NULL;
	return astar(bind(map_graph, ref(civ), ref(u), filterfunc, ignore_enemy,
				only_roads, _1, 
				coastal_goal),
			bind(map_cost, ref(*civ.m), ref(u), coastal, _1, _2),
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
				false, _1, coastal_goal),
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

std::list<coord> map_birds_path_to_nearest(const coord& start, const coord& goal)
{
	return map_birds_path_to_nearest(start,
			[&](const coord& a) -> bool { return a == goal; });
}

std::list<coord> map_along_roads(const coord& start,
		const civilization& civ,
		bool no_enemy_territory, bool known_territory,
		boost::function<bool(const coord& a)> goaltestfunc)
{
	using boost::bind;
	using boost::lambda::_1;
	using boost::lambda::_2;
	using boost::ref;
	return astar(bind(graph_along_roads, ref(civ),
				no_enemy_territory,
				known_territory,
				_1),
			boost::lambda::constant(1),
			boost::lambda::constant(0),
			goaltestfunc, start);
}

std::list<coord> map_along_roads(const coord& start,
		const civilization& civ,
		bool no_enemy_territory, bool known_territory,
		const coord& goal)
{
	return map_along_roads(start, civ, no_enemy_territory, known_territory,
			[&](const coord& a) -> bool { return a == goal; });
}


