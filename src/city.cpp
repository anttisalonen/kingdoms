#include "city.h"

city::city(std::string name, int x, int y, unsigned int civid)
	: cityname(name),
	xpos(x),
	ypos(y),
	civ_id(civid),
	population(1),
	stored_food(0),
	stored_prod(0),
	accum_culture(0),
	culture_level(1),
	city_size(1)
{
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

