#include "civ.h"
#include <string.h>
#include <algorithm>

void total_resources(const city& c, const map& m, 
		int* food, int* prod, int* comm)
{
	*food = 0; *prod = 0; *comm = 0;
	for(std::list<coord>::const_iterator it = c.resource_coords.begin();
			it != c.resource_coords.end();
			++it) {
		int f, p, cm;
		m.get_resources_by_terrain(m.get_data(c.xpos + it->x,
				c.ypos + it->y), it->x == 0 && it->y == 0, &f,
				&p, &cm);
		*food += f;
		*prod += p;
		*comm += cm;
	}
}

void set_default_city_production(city* c, const unit_configuration_map& uconfmap)
{
	for(unit_configuration_map::const_iterator it = uconfmap.begin();
			it != uconfmap.end();
			++it) {
		if(!it->second->settler) {
			c->current_production_unit_id = it->first;
			return;
		}
	}
}

bool can_attack(const unit& u1, const unit& u2)
{
	if(!in_bounds(u1.xpos - 1, u2.xpos, u1.xpos + 1))
		return false;
	if(!in_bounds(u1.ypos - 1, u2.ypos, u1.ypos + 1))
		return false;
	return u1.uconf.max_strength > 0;
}

void combat(unit* u1, unit* u2)
{
	if(!can_attack(*u1, *u2))
		return;
	if(u1->strength == 0 || u2->strength == 0)
		return;
	if(u2->uconf.max_strength == 0) {
		u2->strength = 0;
		return;
	}
	unsigned int u1chance = u1->strength ^ 2;
	unsigned int u2chance = u2->strength ^ 2;
	unsigned int val = rand() % (u1chance + u2chance);
	if(val < u1chance) {
		u1->strength = u1->strength * (val + 1) / u1chance;
		u2->strength = 0;
	}
	else {
		u1->strength = 0;
		u2->strength = u2->strength * (val + 1 - u1chance) / u2chance;
	}
}

resource_configuration::resource_configuration()
	: city_food_bonus(0),
	city_prod_bonus(0),
	city_comm_bonus(0)
{
	memset(terrain_food_values, 0, sizeof(terrain_food_values));
	memset(terrain_prod_values, 0, sizeof(terrain_prod_values));
	memset(terrain_comm_values, 0, sizeof(terrain_comm_values));
}

unit::unit(int uid, int x, int y, int civid, const unit_configuration& uconf_)
	: unit_id(uid),
	civ_id(civid),
	xpos(x),
	ypos(y),
	moves(0),
	fortified(false),
	uconf(uconf_),
	strength(10 * uconf_.max_strength)
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

const std::vector<unit*> map::empty_unit_spot = std::vector<unit*>();

map::map(int x, int y, const resource_configuration& resconf_)
	: data(buf2d<int>(x, y, 0)),
	unit_map(buf2d<std::vector<unit*> >(x, y, std::vector<unit*>())),
	city_map(buf2d<city*>(x, y, NULL)),
	resconf(resconf_)
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

void map::get_resources_by_terrain(int terr, bool city, int* food, int* prod, int* comm) const
{
	if(terr < 0 || terr >= num_terrain_types)
		return;
	*food = resconf.terrain_food_values[terr] + (city ? resconf.city_food_bonus : 0);
	*prod = resconf.terrain_prod_values[terr] + (city ? resconf.city_prod_bonus : 0);
	*comm = resconf.terrain_comm_values[terr] + (city ? resconf.city_comm_bonus : 0);
}

void map::add_unit(unit* u)
{
	if(!free_spot(u->civ_id, u->xpos, u->ypos))
		return;
	std::vector<unit*>* old = unit_map.get_mod(u->xpos, u->ypos);
	if(!old)
		return;
	old->push_back(u);
}

void map::remove_unit(unit* u)
{
	if(!free_spot(u->civ_id, u->xpos, u->ypos))
		return;
	std::vector<unit*>* old = unit_map.get_mod(u->xpos, u->ypos);
	if(!old || old->size() == 0)
		return;
	std::remove(old->begin(), old->end(), u);
}

bool map::free_spot(unsigned int civ_id, int x, int y) const
{
	int owner = get_spot_owner(x, y);
	return owner == -1 || owner == (int)civ_id;
}

int map::get_spot_owner(int x, int y) const
{
	const std::vector<unit*>* val = unit_map.get(x, y);
	if(!val)
		return -1;
	if(val->size() == 0) {
		city* const* c = city_map.get(x, y);
		if(c == NULL)
			return -1;
		return (*c)->civ_id;
	}
	return (*val)[0]->civ_id;
}

const std::vector<unit*>& map::units_on_spot(int x, int y) const
{
	const std::vector<unit*>* us = unit_map.get(x, y);
	if(us == NULL)
		return map::empty_unit_spot;
	else
		return *us;
}

city* map::city_on_spot(int x, int y) const
{
	city* const* c = city_map.get(x, y);
	if(!c)
		return NULL;
	return *c;
}

void map::add_city(city* c, int x, int y)
{
	if(city_on_spot(x, y))
		return;
	city_map.set(x, y, c);
}

void map::remove_city(const city* c)
{
	city_map.set(c->xpos, c->ypos, NULL);
}

coord::coord(int x_, int y_)
	: x(x_),
	y(y_)
{
}

city::city(const char* name, int x, int y, unsigned int civid)
	: cityname(name),
	xpos(x),
	ypos(y),
	civ_id(civid),
	population(1),
	stored_food(0),
	stored_prod(0),
	current_production_unit_id(-1)
{
}

civilization::civilization(const char* name, unsigned int civid, const color& c_, map& m_)
	: civname(name),
	civ_id(civid),
	col(c_),
	m(m_),
	fog(fog_of_war(m_.size_x(), m_.size_y())),
	gold(0),
	relationships(civid + 1, 0)
{
	relationships[civid] = 1;
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

unit* civilization::add_unit(int uid, int x, int y, const unit_configuration& uconf)
{
	unit* u = new unit(uid, x, y, civ_id, uconf);
	units.push_back(u);
	fog.reveal(x, y, 1);
	return u;
}

void civilization::remove_unit(unit* u)
{
	m.remove_unit(u);
	std::remove(units.begin(), units.end(), u);
	delete u;
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

void civilization::add_message(const msg& m)
{
	messages.push_back(m);
}

msg new_unit_msg(unit* u)
{
	msg m;
	m.type = msg_new_unit;
	m.msg_data.new_unit = u;
	return m;
}

msg discovered_civ(int civid)
{
	msg m;
	m.type = msg_civ_discovery;
	m.msg_data.discovered_civ_id = civid;
	return m;
}

void civilization::increment_resources(const unit_configuration_map& uconfmap)
{
	for(std::list<city*>::iterator cit = cities.begin();
			cit != cities.end();
			++cit) {
		int food, prod, comm;
		total_resources(**cit, m, &food, &prod, &comm);
		(*cit)->stored_food += food - (*cit)->population * 2;
		gold += comm;
		(*cit)->stored_prod += prod;
		if((*cit)->current_production_unit_id > -1) {
			unit_configuration_map::const_iterator prod_unit = uconfmap.find((*cit)->current_production_unit_id);
			if(prod_unit != uconfmap.end()) {
				if((int)prod_unit->second->production_cost <= (*cit)->stored_prod) {
					unit* u = add_unit((*cit)->current_production_unit_id, 
							(*cit)->xpos, (*cit)->ypos, *(prod_unit->second));
					(*cit)->stored_prod -= prod_unit->second->production_cost;
					add_message(new_unit_msg(u));
				}
			}
		}
	}
}

int civilization::try_move_unit(unit* u, int chx, int chy)
{
	if((!chx && !chy) || !u || !u->moves)
		return 0;
	int newx = u->xpos + chx;
	int newy = u->ypos + chy;
	if(m.get_data(newx, newy) > 0 && m.free_spot(civ_id, newx, newy)) {
		m.remove_unit(u);
		fog.shade(u->xpos, u->ypos, 1);
		u->xpos += chx;
		u->ypos += chy;
		u->moves--;
		fog.reveal(u->xpos, u->ypos, 1);
		m.add_unit(u);
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
	m.add_city(c, x, y);
	return c;
}

int civilization::get_relationship_to_civ(unsigned int civid) const
{
	if(civid == civ_id)
		return 1;
	if(relationships.size() <= civid)
		return 0;
	return relationships[civid];
}

void civilization::set_relationship_to_civ(unsigned int civid, int val)
{
	if(civid == civ_id)
		return;
	if(relationships.size() <= civid) {
		relationships.resize(civid + 1, 0);
	}
	relationships[civid] = val;
}

bool civilization::discover(unsigned int civid)
{
	if(civid != civ_id && get_relationship_to_civ(civid) == 0) {
		set_relationship_to_civ(civid, 1);
		add_message(discovered_civ(civid));
		return 1;
	}
	return 0;
}

std::vector<unsigned int> civilization::check_discoveries(int x, int y, int radius)
{
	std::vector<unsigned int> discs;
	for(int i = x - radius; i <= x + radius; i++) {
		for(int j = y - radius; j <= y + radius; j++) {
			int owner = m.get_spot_owner(i, j);
			if(owner != -1 && owner != (int)civ_id) {
				discover(owner);
				discs.push_back(owner);
			}
		}
	}
	return discs;
}

round::round(const unit_configuration_map& uconfmap_)
	: uconfmap(uconfmap_)
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

void round::increment_resources()
{
	for(std::vector<civilization*>::iterator it = civs.begin();
	    it != civs.end();
	    ++it) {
		(*it)->increment_resources(uconfmap);
	}
}

bool round::next_civ()
{
	if(civs.empty())
		return false;
	current_civ++;
	if(current_civ == civs.end()) {
		current_civ = civs.begin();
		increment_resources();
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


