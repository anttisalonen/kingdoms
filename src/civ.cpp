#include "civ.h"
#include <string.h>

resource_configuration::resource_configuration()
	: city_food_bonus(0),
	city_prod_bonus(0),
	city_comm_bonus(0)
{
	memset(terrain_food_values, 0, sizeof(terrain_food_values));
	memset(terrain_prod_values, 0, sizeof(terrain_prod_values));
	memset(terrain_comm_values, 0, sizeof(terrain_comm_values));
}

unit::unit(int uid, int x, int y, int civid)
	: unit_id(uid),
	civ_id(civid),
	xpos(x),
	ypos(y),
	moves(0)
{
}

unit::~unit()
{
}

void unit::refill_moves(unsigned int m)
{
	moves = m;
}

fog_of_war::fog_of_war(int x, int y)
	: fog(buf2d<int>(x, y, 0))
{
}

void fog_of_war::reveal(int x, int y, int radius)
{
	for(int i = x - radius; i <= x + radius; i++) {
		for(int j = y - radius; j <= y + radius; j++) {
			up_refcount(i, j);
			set_value(i, j, 2);
		}
	}
}

void fog_of_war::shade(int x, int y, int radius)
{
	for(int i = x - radius; i <= x + radius; i++) {
		for(int j = y - radius; j <= y + radius; j++) {
			down_refcount(i, j);
			if(get_refcount(i, j) == 0)
				set_value(i, j, 1);
		}
	}
}

char fog_of_war::get_value(int x, int y) const
{
	return get_raw(x, y) & 3;
}

int fog_of_war::get_refcount(int x, int y) const
{
	return get_raw(x, y) >> 2;
}

int fog_of_war::get_raw(int x, int y) const
{
	const int* i = fog.get(x, y);
	if(!i)
		return 0;
	return *i;
}

void fog_of_war::up_refcount(int x, int y)
{
	int i = get_refcount(x, y);
	int v = get_value(x, y);
	fog.set(x, y, ((i + 1) << 2) | v);
}

void fog_of_war::down_refcount(int x, int y)
{
	int i = get_refcount(x, y);
	if(i == 0)
		return;
	int v = get_value(x, y);
	fog.set(x, y, ((i - 1) << 2) | v);
}

void fog_of_war::set_value(int x, int y, int val)
{
	int i = get_raw(x, y);
	i &= ~3;
	fog.set(x, y, i | val); 
}

map::map(int x, int y)
	: data(buf2d<int>(x, y, 0))
{
	for(int i = 0; i < y; i++) {
		for(int j = 0; j < x; j++) {
			data.set(j, i, rand() % 2);
		}
	}
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

coord::coord(int x_, int y_)
	: x(x_),
	y(y_)
{
}

city::city(const char* name, int x, int y, int civid)
	: cityname(name),
	xpos(x),
	ypos(y),
	civ_id(civid)
{
}

civilization::civilization(const char* name, int civid, const color& c_, const map& m_)
	: civname(name),
	civ_id(civid),
	col(c_),
	m(m_),
	fog(fog_of_war(m_.size_x(), m_.size_y()))
{
}

civilization::~civilization()
{
	while(!units.empty()) {
		delete units.back();
		units.pop_back();
	}
	while(!cities.empty()) {
		delete cities.back();
		cities.pop_back();
	}
}

void civilization::add_unit(int uid, int x, int y)
{
	units.push_back(new unit(uid, x, y, civ_id));
	fog.reveal(x, y, 1);
}

void civilization::refill_moves(const unit_configuration_map& uconfmap)
{
	for(std::list<unit*>::iterator uit = units.begin();
		uit != units.end();
		++uit) {
		unit_configuration_map::const_iterator max_moves_it = uconfmap.find((*uit)->unit_id);
		int max_moves;
		if(max_moves_it == uconfmap.end())
			max_moves = 0;
		else
			max_moves = max_moves_it->second->max_moves;
		(*uit)->refill_moves(max_moves);
	}
}

int civilization::try_move_unit(unit* u, int chx, int chy)
{
	if((!chx && !chy) || !u || !u->moves)
		return 0;
	if(m.get_data(u->xpos + chx, u->ypos + chy) > 0) {
		fog.shade(u->xpos, u->ypos, 1);
		u->xpos += chx;
		u->ypos += chy;
		u->moves--;
		fog.reveal(u->xpos, u->ypos, 1);
		return 1;
	}
	return 0;
}

char civilization::fog_at(int x, int y) const
{
	return fog.get_value(x, y);
}

city* civilization::add_city(const char* name, int x, int y)
{
	city* c = new city(name, x, y, civ_id);
	cities.push_back(c);
		c->resource_coords.push_back(coord(0, 0));
	if(y != 0)
		c->resource_coords.push_back(coord(0, -1));
	else
		c->resource_coords.push_back(coord(0, 1));
	return c;
}

round::round(const unit_configuration_map& uconfmap_, const resource_configuration& resconf_)
	: uconfmap(uconfmap_),
	resconf(resconf_)
{
	current_civ = civs.begin();
}

void round::add_civilization(civilization* civ)
{
	civs.push_back(civ);
	current_civ = civs.begin();
	refill_moves();
}

void round::refill_moves()
{
	for(std::vector<civilization*>::iterator it = civs.begin();
	    it != civs.end();
	    ++it) {
		(*it)->refill_moves(uconfmap);
	}
}

bool round::next_civ()
{
	if(civs.empty())
		return false;
	current_civ++;
	if(current_civ == civs.end()) {
		current_civ = civs.begin();
		refill_moves();
		return true;
	}
	return false;
}

const unit_configuration* round::get_unit_configuration(int uid) const
{
	unit_configuration_map::const_iterator it = uconfmap.find(uid);
	if(it == uconfmap.end())
		return NULL;
	return it->second;
}

void round::get_resources_by_terrain(int terr, bool city, int* food, int* prod, int* comm) const
{
	if(terr < 0 || terr >= num_terrain_types)
		return;
	*food = resconf.terrain_food_values[terr] + (city ? resconf.city_food_bonus : 0);
	*prod = resconf.terrain_prod_values[terr] + (city ? resconf.city_prod_bonus : 0);
	*comm = resconf.terrain_comm_values[terr] + (city ? resconf.city_comm_bonus : 0);
}

