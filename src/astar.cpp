#include "astar.h"

#include <queue>
#include <set>
#include <map>
#include <utility>

bool comp_func(const std::pair<int, const coord&>& lhs, const std::pair<int, const coord&>& rhs)
{
	return lhs.first < rhs.first;
}

std::list<coord> astar(graphfunc g, costfunc c, heurfunc h, 
		goaltestfunc gtfunc, const coord& start)
{
	using namespace std;

	set<coord> visited;
	std::map<coord, int> cost_here; // real (g) cost
	list<coord> path;
	std::map<coord, coord> parents;
	priority_queue<pair<int, coord> > open_nodes; // key is the total (f) cost

	open_nodes.push(make_pair(0, start));
	do {
		// current node is the parent
		pair<int, coord> top_node(open_nodes.top());
		coord current(open_nodes.top().second);
		open_nodes.pop();

		// when relaxing the edges, previous edges are left in the queue
		// so check if already visited
		if(visited.find(current) != visited.end())
			continue;

		int cost_to_parent(open_nodes.top().first);
		visited.insert(current);
		set<coord> children = g(current);

		// check for goal
		if(gtfunc(current)) {
			path.push_front(current);
			break;
		}
		for(set<coord>::const_iterator children_it = children.begin();
				children_it != children.end();
				++children_it) {
			// check if already visited
			if(visited.find(*children_it) != visited.end())
				continue;

			int this_g_cost = cost_to_parent + c(current, *children_it);
			int this_f_cost = this_g_cost + h(*children_it);

			// check if already in open list
			bool add_this_as_parent = true;
			std::map<coord, int>::const_iterator already_open_child = cost_here.find(*children_it);
			if(already_open_child != cost_here.end()) {
				// already in open list => check if the cost is less than previous
				int cost_prev = already_open_child->second;
				if(cost_prev <= this_g_cost)
					add_this_as_parent = false;
			}
			if(add_this_as_parent) {
				parents[*children_it] = current;
				open_nodes.push(make_pair(this_f_cost, *children_it));
				cost_here[*children_it] = this_g_cost;
			}
		}
	} while(!open_nodes.empty());
	if(path.empty())
		return path;
	coord& curr_node = path.front();
	while(1) {
		std::map<coord, coord>::const_iterator par_it = parents.find(curr_node);
		if(par_it == parents.end())
			break;
		path.push_front(par_it->second);
		curr_node = par_it->second;
	}
	return path;
}

