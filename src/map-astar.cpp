#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "astar.h"
#include "map-astar.h"

void check_insert(std::set<coord>& s, const map& m, const unit& u, int x, int y)
{
	if(x >= 0 && y >= 0 && x < m.size_x() && y < m.size_y()) {
		if(terrain_allowed(m, u, x, y))
			s.insert(coord(x, y));
	}
}

std::set<coord> map_graph(const map& m, const unit& u, const coord& a)
{
	std::set<coord> ret;
	for(int i = -1; i <= 1; i++) {
		for(int j = -1; j <= 1; j++) {
			if(i && j) {
				check_insert(ret, m, u, a.x + i, a.y + j);
			}
		}
	}
	return ret;
}

int map_cost(const map& m, const unit& u, const coord& a, const coord& b)
{
	return m.get_move_cost(u, b.x, b.y);
}

int map_heur(const coord& b, const coord& a)
{
	return abs(b.x - a.x) + abs(b.y - a.y);
}

bool map_goaltest(const coord& b, const coord& a)
{
	return b == a;
}

std::list<coord> map_astar(const map& m, const unit& u, const coord& start, const coord& goal)
{
	using boost::bind;
	using boost::lambda::_1;
	using boost::lambda::_2;
	return astar(bind(map_graph, m, u, _1),
			bind(map_cost, m, u, _1, _2),
			bind(map_heur, goal, _1),
			bind(map_goaltest, goal, _1), start);
}


