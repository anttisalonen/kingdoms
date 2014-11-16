#ifndef MAP_ASTAR_H
#define MAP_ASTAR_H

#include <list>
#include <boost/function.hpp>
#include "civ.h"

// "coastal": set to true if the unit is a transporter ship
// looking to unload the transportees at goal
std::list<coord> map_astar(const civilization& civ,
		const unit& u, bool ignore_enemy,
		const coord& start, const coord& goal,
		bool coastal = false, bool only_roads = false);

std::list<coord> map_astar(const civilization& civ,
		const unit& u, bool ignore_enemy,
		const coord& start, const coord& goal,
		bool coastal, bool only_roads,
		boost::function<bool(const coord& a)> filterfunc);

// simple BFS, but respecting whether the terrain is allowed for the unit
std::list<coord> map_path_to_nearest(const civilization& civ, 
		const unit& u, bool ignore_enemy, const coord& start, 
		boost::function<bool(const coord& a)> goaltestfunc);

// BFS, as the crow flies
std::list<coord> map_birds_path_to_nearest(const coord& start,
		boost::function<bool(const coord& a)> goaltestfunc);
std::list<coord> map_birds_path_to_nearest(const coord& start, const coord& goal);

// BFS
std::list<coord> map_along_roads(const coord& start,
		const civilization& civ,
		bool no_enemy_territory, bool known_territory,
		boost::function<bool(const coord& a)> goaltestfunc);
std::list<coord> map_along_roads(const coord& start,
		const civilization& civ,
		bool no_enemy_territory, bool known_territory,
		const coord& goal);

#endif
