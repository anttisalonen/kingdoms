#ifndef MAP_ATAR_H
#define MAP_ASTAR_H

#include <list>
#include <boost/function.hpp>
#include "civ.h"

std::list<coord> map_astar(const civilization& civ,
		const unit& u, bool ignore_enemy,
		const coord& start, const coord& goal);

// simple BFS, but respecting whether the terrain is allowed for the unit
std::list<coord> map_path_to_nearest(const civilization& civ, 
		const unit& u, bool ignore_enemy, const coord& start, 
		boost::function<bool(const coord& a)> goaltestfunc);

#endif
