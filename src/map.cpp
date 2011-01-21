#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include "map.h"
#include "map-astar.h"
#include <stdio.h>

const std::list<unit*> map::empty_unit_spot = std::list<unit*>();

map::map(int x, int y, const resource_configuration& resconf_,
		const resource_map& rmap_)
	: data(buf2d<int>(x, y, 0)),
	unit_map(buf2d<std::list<unit*> >(x, y, std::list<unit*>())),
	city_map(buf2d<city*>(x, y, NULL)),
	land_map(buf2d<int>(x, y, -1)),
	improv_map(buf2d<int>(x, y, 0)),
	res_map(buf2d<int>(x, y, 0)),
	river_map(buf2d<bool>(x, y, false)),
	resconf(resconf_),
	rmap(rmap_),
	x_wrap(true),
	y_wrap(false)
{
	init_to_water();
}

map::map()
{
}

void map::init_to_water()
{
	int x = data.size_x;
	int y = data.size_y;
	int ocean_tile = resconf.get_ocean_tile();
	// init to water
	for(int i = 0; i < y; i++) {
		for(int j = 0; j < x; j++) {
			data.set(j, i, ocean_tile);
		}
	}
}

void map::create()
{
	int x = data.size_x;
	int y = data.size_y;
	init_to_water();

	int sea_tile = resconf.get_sea_tile();
	int grass_tile = resconf.get_grass_tile();

	// create continents
	int num_continents = x * y / 800;
	int max_continent_size = x * y / 10; // in tiles
	for(int i = 0; i < num_continents; i++) {
		int cont_x = rand() % x;
		int cont_y = 10 + rand() % (y - 20);
		data.set(cont_x, cont_y, grass_tile);
		sea_around_land(cont_x, cont_y, sea_tile);

		std::vector<coord> candidates;
		candidates.push_back(coord(cont_x, cont_y));
		int cont_size = rand() % max_continent_size + 1;
		std::set<coord> already_taken;
		for(int j = 0; j < cont_size && !candidates.empty(); j++) {
			int cand = rand() % candidates.size();
			coord c = candidates[cand];
			if(already_taken.find(c) != already_taken.end())
				continue;
			already_taken.insert(c);
			candidates.erase(candidates.begin() + cand);
			data.set(c.x, c.y, grass_tile);
			sea_around_land(c.x, c.y, sea_tile);
			int dx1 = wrap_x(c.x - 1);
			int dx2 = wrap_x(c.x + 1);
			if(already_taken.find(coord(dx1, c.y)) == already_taken.end())
				candidates.push_back(coord(dx1, c.y));
			if(already_taken.find(coord(dx2, c.y)) == already_taken.end())
				candidates.push_back(coord(dx2, c.y));
			if(c.y > 0 && already_taken.find(coord(c.x, c.y - 1)) == already_taken.end())
				candidates.push_back(coord(c.x, c.y - 1));
			if(c.y < y - 1 && already_taken.find(coord(c.x, c.y + 1)) == already_taken.end())
				candidates.push_back(coord(c.x, c.y + 1));
		}
	}

	// create ridges
	int max_ridge_length = 20;
	int num_ridges = x * y / (max_ridge_length * 4);
	for(int i = 0; i < num_ridges; i++) {
		int xpos = rand() % x;
		int ypos = rand() % y;
		int dir = rand() % 8;
		int ridge_width = 3;
		int ridge_size = rand() % max_ridge_length + 4;
		for(int j = 0; j < ridge_size; j++) {
			int realdir = dir % 8;
			if(!resconf.is_water_tile(get_data(xpos, ypos))) {
				create_mountains(xpos, ypos, ridge_width);
			}
			ridge_width += rand() % 3 - 1;
			ridge_width = clamp(2, ridge_width, 5);
			int dx = realdir > 4 ? 1 : realdir < 3 ? -1 : 0;
			int dy = realdir == 0 || realdir == 3 || realdir == 5 ? -1 :
				realdir == 1 || realdir == 6 ? 0 : -1;
			xpos = clamp(0, wrap_x(xpos + dx), x - 1);
			ypos = clamp(0, wrap_y(ypos + dy), y - 1);
			dir += rand() % 3 - 1;
		}
	}

	// create terrain types
	for(int j = 0; j < y; j++) {
		for(int i = 0; i < x; i++) {
			int this_data = get_data(i, j);
			if(resconf.is_water_tile(this_data) ||
			   resconf.is_mountain_tile(this_data))
				continue;
			int temp = get_temperature(i, j);
			std::vector<int> types = get_types_by_temperature(temp);
			if(resconf.is_hill_tile(this_data) &&
			   temp > 3 && temp < 7)
				continue;
			int humidity = get_humidity_at(i, j);
			std::vector<int> candidates = get_terrain_candidates(types, humidity);
			int chosen_type_index = rand() % candidates.size();
			data.set(i, j, candidates[chosen_type_index]);
		}
	}

	// create rivers
	int num_rivers = x * y / 20;
	for(int i = 0; i < num_rivers; i++) {
		int river_x = rand() % x;
		int river_y = rand() % y;
		int terr = get_data(river_x, river_y);
		if(!resconf.is_water_tile(terr)) {
			int gendir = rand() % 4;
			std::vector<coord> river_path;
			river_path.push_back(coord(river_x, river_y));
			bool success = false;
			while(1) {
				if(data.get(river_x, river_y) == NULL) {
					// map border
					break;
				}

				terr = get_data(river_x, river_y);
				if(get_humidity_at(river_x, river_y) < 3) {
					// too dry
					break;
				}
				bool go_right = rand() % 2 == 0;
				bool on_hill = resconf.is_hill_tile(terr);
				bool on_flatland = !on_hill && !resconf.is_mountain_tile(terr);

				if((gendir == 0 && !go_right) || (gendir == 2 && go_right))
					river_x--;
				else if((gendir == 1 && go_right) || (gendir == 3 && !go_right))
					river_x++;
				else if((gendir == 0 && go_right) || (gendir == 1 && !go_right))
					river_y--;
				else // if(gendir == 2 && !go_right || gendir == 3 && go_right)
					river_y++;

				river_x = wrap_x(river_x);
				river_y = wrap_y(river_y);
				int new_terr = get_data(river_x, river_y);
				if(resconf.is_water_tile(new_terr)) {
					success = true;
					break;
				}
				bool new_on_mountain = resconf.is_mountain_tile(new_terr);
				if(on_hill && new_on_mountain) {
					break;
				}
				bool new_on_hill = resconf.is_hill_tile(new_terr);
				if(on_flatland && (new_on_hill || new_on_mountain)) {
					break;
				}
				river_path.push_back(coord(river_x, river_y));
			}
			if(success) {
				for(unsigned int ind = 0; ind < river_path.size(); ind++) {
					river_map.set(river_path[ind].x, river_path[ind].y, true);
				}
			}
		}
	}

	// create resources
	for(int j = 0; j < y; j++) {
		for(int i = 0; i < x; i++) {
			std::vector<unsigned int> selected_resources;
			int this_data = get_data(i, j);
			for(resource_map::const_iterator it = rmap.begin();
					it != rmap.end();
					++it) {
				for(unsigned int k = 0; k < max_num_resource_terrains; k++) {
					if(it->second.terrain[k] &&
						it->second.terrain[k] - 1 == (unsigned int)this_data &&
						it->second.terrain_abundance[k]) {
						if(rand() % it->second.terrain_abundance[k] == 0)
							selected_resources.push_back(it->first);
					}
				}
			}
			if(selected_resources.size() == 1) {
				res_map.set(i, j, selected_resources[0]);
			}
			else if(selected_resources.size() > 1) {
				unsigned int ind = rand() % selected_resources.size();
				res_map.set(i, j, selected_resources[ind]);
			}
		}
	}
}

void map::sea_around_land(int x, int y, int sea_tile)
{
	for(int j = -1; j <= 1; j++) {
		for(int k = -1; k <= 1; k++) {
			if(!j && !k)
				continue;
			if(resconf.is_ocean_tile(get_data(x + j, y + k))) {
				data.set(wrap_x(x + j), wrap_y(y + k), sea_tile);
			}
		}
	}
}

int map::get_temperature(int x, int y) const
{
	float dist_to_eq = fabsf(get_latitude(y));
	int dist_to_sea = dist_to_sea_incl_mountains(x, y);
	int t1 = 10 - clamp(1, (int)(dist_to_eq * 10.0f), 9);
	if(dist_to_sea <= 2) {
		if(t1 <= 3)
			t1 += dist_to_sea;
		else if(t1 >= 7)
			t1 -= dist_to_sea;
	}
	return clamp(1, t1, 9);
}

std::vector<int> map::get_types_by_temperature(int temp) const
{
	std::vector<int> res;
	for(int i = 0; i < num_terrain_types; i++) {
		if(resconf.terrain_type[i] == land_type_land && 
			(resconf.temperature[i] == temp || 
			 resconf.temperature[i] == 0))
			res.push_back(i);
	}
	if(res.size() >= 3)
		return res;
	for(int i = 0; i < num_terrain_types; i++) {
		if(resconf.terrain_type[i] == land_type_land && 
			((temp > 1 && resconf.temperature[i] == temp - 1) ||
			(temp < 9 && resconf.temperature[i] == temp + 1)))
			res.push_back(i);
	}
	return res;
}

std::vector<int> map::get_terrain_candidates(const std::vector<int>& types, 
		int humidity) const
{
	std::vector<int> res;
	for(unsigned int i = 0; i < types.size(); i++) {
		if((resconf.humidity[types[i]] == humidity) ||
		   (resconf.humidity[types[i]] == 0 && resconf.terrain_type[types[i]] == land_type_land))
			res.push_back(types[i]);
	}
	if(res.size() >= 1)
		return res;
	for(int j = 1; j <= 9; j++) {
		for(unsigned int i = 0; i < types.size(); i++) {
			if(resconf.humidity[types[i]] == 0)
				continue;
			if(abs(resconf.humidity[types[i]] - humidity) == j)
				res.push_back(types[i]);
		}
		if(res.size() >= 1)
			return res;
	}
	return res;
}

// between -1.0 and 1.0; 0.0 = equator
float map::get_latitude(int y) const
{
	float max_dist = size_y() / 2.0f;
	return 2.0f * (0.5f * y / max_dist) - 1.0f;
}

class sea_picker {
	private:
		const map& m;
	public:
		sea_picker(const map& m_) : m(m_) { }
		bool operator()(const coord& a) {
			return m.resconf.is_water_tile(m.get_data(a.x, a.y));
		}
};

int map::get_humidity_at(int x, int y) const
{
	float lat = fabsf(get_latitude(y));
	int lat_bonus = lat < 0.10f ? 3 : 0;
	int dist_coeff = lat > 0.40f ? 3 : lat > 0.25f ? 2 : 1;

	int dist_to_sea = dist_to_sea_incl_mountains(x, y);
	return clamp(1, lat_bonus + dist_to_sea * dist_coeff - 2, 9);
}

int map::dist_to_sea_incl_mountains(int x, int y) const
{
	std::list<coord> path_to_sea = map_birds_path_to_nearest(coord(x, y),
			sea_picker(*this));
	int dist_to_sea = path_to_sea.size();
	for(std::list<coord>::const_iterator it = path_to_sea.begin();
			it != path_to_sea.end();
			++it) {
		if(resconf.is_mountain_tile(get_data(it->x, it->y)))
			dist_to_sea += 2;
	}
	return dist_to_sea;
}

void map::create_mountains(int x, int y, int width)
{
	int rad = width / 2;
	int skip = width % 2;
	for(int i = -rad; i < rad + skip; i++) {
		for(int j = -rad; j < rad + skip; j++) {
			int manh = abs(i) + abs(j);
			if(resconf.is_water_tile(get_data(x + i, y + j)))
				continue;
			if(manh == rad)
				data.set(x + i, y + j, resconf.get_hill_tile());
			else if(manh < rad)
				data.set(x + i, y + j, resconf.get_mountain_tile());
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

void map::set_x_wrap(bool w)
{
	x_wrap = w;
}

void map::set_y_wrap(bool w)
{
	y_wrap = w;
}

int map::get_data(int x, int y) const
{
	const int* v = data.get(wrap_x(x), wrap_y(y));
	if(!v)
		return -1;
	return *v;
}

void map::set_data(int x, int y, int terr)
{
	data.set(wrap_x(x), wrap_y(y), terr);
}

unsigned int map::get_resource(int x, int y) const
{
	const int* v = res_map.get(wrap_x(x), wrap_y(y));
	if(!v)
		return 0;
	return *v;
}

void map::set_resource(int x, int y, unsigned int res)
{
	res_map.set(wrap_x(x), wrap_y(y), res);
}

bool map::has_river(int x, int y) const
{
	const bool* v = river_map.get(wrap_x(x), wrap_y(y));
	if(!v)
		return 0;
	return *v;
}

void map::set_river(int x, int y, bool riv)
{
	river_map.set(wrap_x(x), wrap_y(y), riv);
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

void map::get_resources_on_spot(int x, int y, int* food, int* prod, int* comm,
		const std::set<unsigned int>* advances) const
{
	get_resources_by_terrain(get_data(x, y), city_on_spot(x, y) != NULL, 
			food, prod, comm);
	int imps = get_improvements_on(x, y);
	if(imps & improv_irrigation)
		(*food)++;
	if(imps & improv_mine)
		(*prod)++;
	if(imps & improv_road)
		(*comm)++;
	if(has_river(x, y))
		(*comm)++;
	if(advances) {
		int res = get_resource(x, y);
		if(res) {
			resource_map::const_iterator rit = rmap.find(res);
			if(rit != rmap.end()) {
				std::set<unsigned int>::const_iterator it =
					advances->find(rit->second.needed_advance);
				if(rit->second.needed_advance == 0 ||
						it != advances->end()) {
					*food = *food + rit->second.food_bonus;
					*prod = *prod + rit->second.prod_bonus;
					*comm = *comm + rit->second.comm_bonus;
				}
			}
		}
	}
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

int map::get_move_cost(const unit& u, int x1, int y1, int x2, int y2, bool* road) const
{
	int t = get_data(wrap_x(x2), wrap_y(y2));
	*road = false;
	if(t == -1)
		return -1;
	if(!terrain_allowed(u, x2, y2))
		return -1;
	if(road_between(x1, y1, x2, y2)) {
		*road = true;
		return 1;
	}
	return 1;
}

bool map::terrain_allowed(const unit& u, int x, int y) const
{
	int t = get_data(wrap_x(x), wrap_y(y));
	if(t == -1)
		return false;
	city* c = city_on_spot(x, y);
	if(c && c->civ_id == (unsigned int)u.civ_id)
		return true;
	if(u.is_land_unit()) {
		return !resconf.is_water_tile(t);
	}
	else {
		if(u.uconf->ocean_unit) {
			return resconf.is_water_tile(t);
		}
		else {
			return resconf.is_sea_tile(t);
		}
	}
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
	float lr;
	int civid;
	const map* m;
	public:
		land_grabber(int x_, int y_, float lr_, int civid_, const map* m_) 
			: x(x_), y(y_), lr(lr_), civid(civid_), m(m_) { }
		void operator()(buf2d<int>& land_map, int xp, int yp) {
			if(xp == x && yp == y) {
				land_map.set(xp, yp, civid);
				return;
			}
			int xd = xp - x;
			int yd = yp - y;
			float dist = sqrt(xd * xd + yd * yd);
			if(dist > lr)
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
	improv_map.set(x, y, 0x01);
	grab_land(c);
}

void map::grab_land(city* c)
{
	float land_radius = c->culture_level + 0.5f;
	land_grabber grab(c->xpos, c->ypos, land_radius, c->civ_id, this);
	mod_rectangle(land_map, c->xpos, c->ypos, land_radius, x_wrap, y_wrap, grab);
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

std::vector<coord> map::random_starting_places(int num) const
{
	std::vector<coord> retval;
	int tries = 0;
	while(tries < num * 1000 && (int)retval.size() < num) {
		tries++;
		int xp = rand() % size_x();
		int yp = rand() % size_y();
		if(resconf.can_found_city_on(get_data(xp, yp))) {
			coord v(xp, yp);
			int f, p, c;
			get_total_city_resources(xp, yp, &f, &p, &c, NULL);
			if(f < 10 || p < 4 || c < 3)
				continue;
			int at_least_two_food = 0;
			int at_least_one_prod = 0;
			for(int i = -2; i <= 2; i++) {
				for(int j = -2; j <= 2; j++) {
					if(abs(i) == 2 && abs(j) == 2)
						continue;

					int terr = get_data(xp + i, yp + j);
					int food = 0, prod = 0, comm = 0;
					get_resources_by_terrain(terr, false,
							&food, &prod, &comm);
					if(food >= 2)
						at_least_two_food++;
					if(prod >= 1)
						at_least_one_prod++;
				}
			}
			if(at_least_two_food < 2)
				continue;
			if(at_least_one_prod < 2)
				continue;
			bool too_close = false;
			for(std::vector<coord>::const_iterator it = retval.begin();
					it != retval.end();
					++it) {
				int manh = manhattan_distance(it->x, it->y, xp, yp);
				if(manh < 10) {
					too_close = true;
					break;
				}
			}
			if(!too_close)
				retval.push_back(v);
		}
	}
	return retval;
}

std::map<int, coord> map::get_starting_places() const
{
	return starting_places;
}

int map::get_starter_at(int x, int y) const
{
	coord pos(wrap_x(x), wrap_y(y));
	for(std::map<int, coord>::const_iterator it = starting_places.begin();
			it != starting_places.end();
			++it) {
		if(it->second == pos)
			return it->first;
	}
	return -1;
}

void map::add_starting_place(const coord& c, int civid)
{
	coord c2;
	c2.x = wrap_x(c.x);
	c2.y = wrap_y(c.y);
	if(get_starter_at(c2.x, c2.y) != -1)
		return;
	starting_places[civid] = c2;
}

void map::remove_starting_place_of(int civid)
{
	starting_places.erase(civid);
}

void map::remove_starting_place(const coord& c)
{
	coord c2;
	c2.x = wrap_x(c.x);
	c2.y = wrap_y(c.y);
	for(std::map<int, coord>::iterator it = starting_places.begin();
			it != starting_places.end();
			++it) {
		if(it->second == c2) {
			starting_places.erase(it);
			return;
		}
	}
}

coord map::get_starting_place_of(int civid) const
{
	std::map<int, coord>::const_iterator it = starting_places.find(civid);
	if(it == starting_places.end())
		return coord(-1, -1);
	else
		return it->second;
}

void map::get_total_city_resources(int x, int y, int* food_points, 
		int* prod_points, int* comm_points,
		const std::set<unsigned int>* advances) const
{
	*food_points = *prod_points = *comm_points = 0;
	for(int i = -2; i <= 2; i++) {
		for(int j = -2; j <= 2; j++) {
			if(abs(i) == 2 && abs(j) == 2)
				continue;

			int food = 0, prod = 0, comm = 0;
			get_resources_on_spot(x + i, y + j,
					&food, &prod, &comm,
					advances);
			*food_points += food;
			*prod_points += prod;
			*comm_points += comm;
		}
	}
}

bool map::can_found_city_on(int x, int y) const
{
	x = wrap_x(x);
	y = wrap_y(y);
	if(city_on_spot(x, y) != NULL)
		return false;
	if(!resconf.can_found_city_on(get_data(x, y)))
		return false;
	for(int i = -1; i <= 1; i++) {
		for(int j = -1; j <= 1; j++) {
			if(city_on_spot(x + i, y + j))
				return false;
		}
	}
	return true;
}

bool map::can_improve_terrain(int x, int y, unsigned int civ_id, 
		improvement_type i) const
{
	x = wrap_x(x);
	y = wrap_y(y);
	int own = get_land_owner(x, y);
	return resconf.can_have_improvement(get_data(x, y), i) &&
	   ((get_improvements_on(x, y) & i) == 0) &&
	   city_on_spot(x, y) == NULL && 
	   (own == -1 || (int)civ_id == own);
}

bool map::try_improve_terrain(int x, int y, 
		unsigned int civ_id, improvement_type i)
{
	x = wrap_x(x);
	y = wrap_y(y);
	if(!can_improve_terrain(x, y, civ_id, i))
		return false;
	int old = get_improvements_on(x, y);
	if(i != improv_road)
		old &= 0x01; // leave road, destroy rest
	improv_map.set(x, y, old | i);
	return true;
}

int map::get_improvements_on(int x, int y) const
{
	x = wrap_x(x);
	y = wrap_y(y);
	const int* v = improv_map.get(x, y);
	if(!v)
		return 0;
	return *v;
}

int map::get_needed_turns_for_improvement(improvement_type i) const
{
	return resconf.get_needed_turns_for_improvement(i);
}

bool map::road_between(int x1, int y1, int x2, int y2) const
{
	return (get_improvements_on(x1, y1) & improv_road) &&
			(get_improvements_on(x2, y2) & improv_road);
}

bool map::connected_to_sea(int x, int y) const
{
	for(int i = -1; i <= 1; i++) {
		for(int j = -1; j <= 1; j++) {
			if(!i && !j)
				continue;
			if(resconf.is_water_tile(get_data(x + i, y + j)))
				return true;
		}
	}
	return false;
}
int map::manhattan_distance(int x1, int y1, int x2, int y2) const
{
	return manhattan_distance_x(x1, x2) + manhattan_distance_y(y1, y2);
}

int map::manhattan_distance_x(int x1, int x2) const
{
	return abs(vector_from_to_x(x1, x2));
}

int map::manhattan_distance_y(int y1, int y2) const
{
	return abs(vector_from_to_y(y1, y2));
}

int map::vector_from_to_x(int x1, int x2) const
{
	if(x_wrap) {
		x1 = wrap_x(x1);
		x2 = wrap_x(x2);
		if(x1 > data.size_x * 3 / 4 && x2 < data.size_x / 4) {
			x2 += data.size_x;
		}
		else if(x2 > data.size_x * 3 / 4 && x1 < data.size_x / 4) {
			x1 += data.size_x;
		}
	}
	return x1 - x2;
}

int map::vector_from_to_y(int y1, int y2) const
{
	if(y_wrap) {
		y1 = wrap_y(y1);
		y2 = wrap_y(y2);
		if(y1 > data.size_y * 3 / 4 && y2 < data.size_y / 4) {
			y2 += data.size_y;
		}
		else if(y2 > data.size_y * 3 / 4 && y1 < data.size_y / 4) {
			y1 += data.size_y;
		}
	}
	return y1 - y2;
}

void map::resize(int x, int y)
{
	data = buf2d<int>(x, y, 0);
	unit_map = buf2d<std::list<unit*> >(x, y, std::list<unit*>());
	city_map = buf2d<city*>(x, y, NULL);
	land_map = buf2d<int>(x, y, -1);
	improv_map = buf2d<int>(x, y, 0);
	res_map = buf2d<int>(x, y, 0);
	river_map = buf2d<bool>(x, y, false);
	init_to_water();
}

