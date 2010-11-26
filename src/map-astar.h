#ifndef MAP_ATAR_H
#define MAP_ASTAR_H

#include <list>
#include "civ.h"

std::list<coord> map_astar(const map& m, const unit& u, const coord& start, const coord& goal);

#endif
