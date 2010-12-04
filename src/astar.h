#ifndef ASTAR_H
#define ASTAR_H

#include <list>
#include <set>
#include <boost/function.hpp>

#include "coord.h"

typedef boost::function<std::set<coord>(const coord& a)> graphfunc;
typedef boost::function<int(const coord& a, const coord& b)> costfunc;
typedef boost::function<int(const coord& a)> heurfunc;
typedef boost::function<bool(const coord& a)> goaltestfunc;

std::list<coord> astar(graphfunc g, costfunc c, heurfunc h, 
		goaltestfunc gtfunc, const coord& start);

void print_path(FILE* fp, const std::list<coord>& path);

#endif
