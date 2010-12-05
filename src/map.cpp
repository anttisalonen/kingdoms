#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include "map.h"

const std::list<unit*> map::empty_unit_spot = std::list<unit*>();

map::map(int x, int y, const resource_configuration& resconf_)
	: data(buf2d<int>(x, y, 0)),
	unit_map(buf2d<std::list<unit*> >(x, y, std::list<unit*>())),
	city_map(buf2d<city*>(x, y, NULL)),
	land_map(buf2d<int>(x, y, -1)),
	resconf(resconf_)
{
	x_wrap = true;
	y_wrap = false;
	int sea_tile = resconf.get_sea_tile();
	int grass_tile = resconf.get_grass_tile();

	// init to water
	for(int i = 0; i < y; i++) {
		for(int j = 0; j < x; j++) {
			data.set(j, i, sea_tile);
		}
	}

	// create continents
	int max_continent_size = 150; // in tiles
	int num_continents = x * y / max_continent_size;
	for(int i = 0; i < num_continents; i++) {
		int cont_x = rand() % x;
		int cont_y = rand() % y;
		data.set(cont_x, cont_y, grass_tile);
		std::vector<coord> candidates;
		candidates.push_back(coord(cont_x, cont_y));
		int cont_size = rand() % max_continent_size + 1;
		for(int j = 0; j < cont_size && !candidates.empty(); j++) {
			int cand = rand() % candidates.size();
			coord c = candidates[cand];
			candidates.erase(candidates.begin() + cand);
			data.set(c.x, c.y, grass_tile);
			int dx1 = wrap_x(c.x - 1);
			int dx2 = wrap_x(c.x + 1);
			if(get_data(dx1, c.y) == sea_tile)
				candidates.push_back(coord(dx1, c.y));
			if(get_data(dx2, c.y) == sea_tile)
				candidates.push_back(coord(dx2, c.y));
			if(c.y > 0 && get_data(c.x, c.y - 1) == sea_tile)
				candidates.push_back(coord(c.x, c.y - 1));
			if(c.y < y - 1 && get_data(c.x, c.y + 1) == sea_tile)
				candidates.push_back(coord(c.x, c.y + 1));
		}
	}
}

int map::wrap_x(int x) const
{
	if(x_wrap) {
		while(x < 0)
			x += size_x();
		while(x > size_x() - 1)
			x -= size_x();
	}
	return x;
}

int map::wrap_y(int y) const
{
	if(y_wrap) {
		while(y < 0)
			y += size_y();
		while(y > size_y() - 1)
			y -= size_y();
	}
	return y;
}

bool map::x_wrapped() const
{
	return x_wrap;
}

bool map::y_wrapped() const
{
	return y_wrap;
}

int map::get_data(int x, int y) const
{
	const int* v = data.get(x, y);
	if(!v)
		return -1;
	return *v;
}

int map::size_x() const
{
	return data.size_x;
}

int map::size_y() const
{
	return data.size_y;
}

void map::get_resources_by_terrain(int terr, bool city, int* food, int* prod, int* comm) const
{
	*food = *prod = *comm = 0;
	if(terr < 0 || terr >= num_terrain_types)
		return;
	*food = resconf.terrain_food_values[terr] + (city ? resconf.city_food_bonus : 0);
	*prod = resconf.terrain_prod_values[terr] + (city ? resconf.city_prod_bonus : 0);
	*comm = resconf.terrain_comm_values[terr] + (city ? resconf.city_comm_bonus : 0);
}

void map::add_unit(unit* u)
{
	std::list<unit*>* old = unit_map.get_mod(u->xpos, u->ypos);
	if(!old)
		return;
	old->push_back(u);
}

void map::remove_unit(unit* u)
{
	std::list<unit*>* old = unit_map.get_mod(u->xpos, u->ypos);
	if(!old || old->size() == 0)
		return;
	old->remove(u);
}

int map::get_spot_resident(int x, int y) const
{
	x = wrap_x(x);
	y = wrap_y(y);
	const std::list<unit*>* val = unit_map.get(x, y);
	if(!val)
		return -1;
	if(val->size() == 0) {
		city* const* c = city_map.get(x, y);
		if(c == NULL || *c == NULL)
			return -1;
		return (*c)->civ_id;
	}
	return val->front()->civ_id;
}

int map::get_move_cost(const unit& u, int x, int y) const
{
	int t = get_data(wrap_x(x), wrap_y(y));
	if(t == -1)
		return -1;
	if(t == 0)
		return -1;
	return 1;
}

const std::list<unit*>& map::units_on_spot(int x, int y) const
{
	const std::list<unit*>* us = unit_map.get(wrap_x(x), wrap_y(y));
	if(us == NULL)
		return map::empty_unit_spot;
	else
		return *us;
}

city* map::city_on_spot(int x, int y) const
{
	city* const* c = city_map.get(wrap_x(x), wrap_y(y));
	if(!c)
		return NULL;
	return *c;
}

class land_grabber {
	int x;
	int y;
	int lr;
	int civid;
	const map* m;
	public:
		land_grabber(int x_, int y_, int lr_, int civid_, const map* m_) 
			: x(x_), y(y_), lr(lr_), civid(civid_), m(m_) { }
		void operator()(buf2d<int>& land_map, int xp, int yp) {
			if(xp == x && yp == y) {
				land_map.set(xp, yp, civid);
				return;
			}
			int xd = xp - x;
			int yd = yp - y;
			float dist = sqrt(xd * xd + yd * yd);
			if(static_cast<int>(dist) > lr)
				return;
			const int* v = land_map.get(m->wrap_x(xp), m->wrap_y(yp));
			if(v && *v == -1)
				land_map.set(m->wrap_x(xp), m->wrap_y(yp), civid);
		}
};

void map::add_city(city* c, int x, int y)
{
	x = wrap_x(x);
	y = wrap_y(y);
	if(city_on_spot(x, y))
		return;
	city_map.set(x, y, c);
	int land_radius = static_cast<int>(c->culture + 1.5f);
	land_grabber grab(x, y, land_radius, c->civ_id, this);
	mod_rectangle(land_map, x, y, land_radius, x_wrap, y_wrap, grab);
}

void map::remove_city(const city* c)
{
	city_map.set(c->xpos, c->ypos, NULL);
}

bool map::has_city_of(int x, int y, unsigned int civ_id) const
{
	city* c = city_on_spot(wrap_x(x), wrap_y(y));
	if(c)
		return c->civ_id == civ_id;
	else
		return false;
}

int map::city_owner_on_spot(int x, int y) const
{
	city* c = city_on_spot(wrap_x(x), wrap_y(y));
	if(!c)
		return -1;
	return c->civ_id;
}

void map::set_land_owner(int civ_id, int x, int y)
{
	land_map.set(wrap_x(x), wrap_y(y), civ_id);
}

int map::get_land_owner(int x, int y) const
{
	const int* v = land_map.get(wrap_x(x), wrap_y(y));
	if(!v)
		return -1;
	return *v;
}

int map::get_spot_owner(int x, int y) const
{
	x = wrap_x(x);
	y = wrap_y(y);
	int res = get_spot_resident(x, y);
	if(res >= 0)
		return res;
	else
		return get_land_owner(x, y);

}

void map::remove_civ_land(unsigned int civ_id)
{
	for(int i = 0; i < land_map.size_x; i++) {
		for(int j = 0; j < land_map.size_y; j++) {
			if(get_land_owner(i, j) == (int)civ_id) {
				set_land_owner(-1, i, j);
			}
		}
	}
}

std::vector<coord> map::get_starting_places(int num) const
{
	std::vector<coord> retval;
	while((int)retval.size() < num) {
		int xp = rand() % size_x();
		int yp = rand() % size_y();
		if(resconf.can_found_city(get_data(xp, yp))) {
			coord v(xp, yp);
			if(std::find(retval.begin(), retval.end(), v) == retval.end())
				retval.push_back(v);
		}
	}
	return retval;
}


