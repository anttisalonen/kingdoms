#include <algorithm>
#include "city.h"

city::city(std::string name, int x, int y, unsigned int civid)
	: cityname(name),
	xpos(x),
	ypos(y),
	civ_id(civid),
	stored_food(0),
	stored_prod(0),
	accum_culture(0),
	culture_level(1),
	city_size(1),
	entertainers(1)
{
	resource_coords.push_back(coord(0, 0));
	production.producing_unit = true;
	production.current_production_id = -1;
}

bool city::producing_something() const
{
	return production.current_production_id != -1;
}

void city::set_unit_production(int uid)
{
	production.producing_unit = true;
	production.current_production_id = uid;
}

void city::set_improv_production(int ciid)
{
	production.producing_unit = false;
	production.current_production_id = ciid;
}

void city::set_production(const city_production& c)
{
	production.producing_unit = c.producing_unit;
	production.current_production_id = c.current_production_id;
}

bool city::has_barracks(const city_improv_map& cimap) const
{
	for(std::set<unsigned int>::const_iterator it = built_improvements.begin();
			it != built_improvements.end();
			++it) {
		city_improv_map::const_iterator cit = cimap.find(*it);
		if(cit != cimap.end()) {
			if(cit->second.barracks)
				return true;
		}
	}
	return false;
}

bool city::has_granary(const city_improv_map& cimap) const
{
	for(std::set<unsigned int>::const_iterator it = built_improvements.begin();
			it != built_improvements.end();
			++it) {
		city_improv_map::const_iterator cit = cimap.find(*it);
		if(cit != cimap.end()) {
			if(cit->second.granary)
				return true;
		}
	}
	return false;
}
void city::decrement_city_size()
{
	if(city_size > 0) {
		city_size--;
		if(entertainers == 0)
			pop_resource_worker();
		else
			entertainers--;
	}
}

void city::increment_city_size()
{
	city_size++;
	entertainers++;
}

int city::get_city_size() const
{
	return city_size;
}

const std::list<coord>& city::get_resource_coords() const
{
	return resource_coords;
}

void city::pop_resource_worker()
{
	if(resource_coords.size() > 1) {
		entertainers++;
		resource_coords.pop_back();
	}
}

void city::drop_resource_worker(const coord& c)
{
	if((c.x == 0 && c.y == 0) || abs(c.x) > 2 ||
		abs(c.y) > 2 || (abs(c.x) == 2 && abs(c.y) == 2))
		return;
	std::list<coord>::iterator it = std::find(resource_coords.begin(), 
			resource_coords.end(), c);
	if(it != resource_coords.end()) {
		resource_coords.erase(it);
		entertainers--;
	}
}

void city::add_resource_worker(const coord& c)
{
	if((c.x == 0 && c.y == 0) || abs(c.x) > 2 ||
		abs(c.y) > 2 || (abs(c.x) == 2 && abs(c.y) == 2))
		return;
	if(entertainers > 0 && std::find(resource_coords.begin(),
				resource_coords.end(), c) == resource_coords.end()) {
		entertainers--;
		resource_coords.push_back(c);
	}
}

int city::get_num_entertainers() const
{
	return entertainers;
}

