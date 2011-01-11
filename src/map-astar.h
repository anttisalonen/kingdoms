#ifndef MAP_ATAR_H
#define MAP_ASTAR_H

#include <list>
#include <boost/function.hpp>
#include "civ.h"

std::list<coord> map_astar(const civilization& civ,
		const unit& u, bool ignore_enemy,
		const coord& start, const coord& goal,
		bool coastal = false);

// simple BFS, but respecting whether the terrain is allowed for the unit
std::list<coord> map_path_to_nearest(const civilization& civ, 
		const unit& u, bool ignore_enemy, const coord& start, 
		boost::function<bool(const coord& a)> goaltestfunc);

// BFS, as the crow flies
std::list<coord> map_birds_path_to_nearest(const coord& start,
		boost::function<bool(const coord& a)> goaltestfunc);

// BFS
std::list<coord> map_along_roads(const coord& start,
		const civilization& civ,
		bool no_enemy_territory, bool known_territory,
		boost::function<bool(const coord& a)> goaltestfunc);

#endif
