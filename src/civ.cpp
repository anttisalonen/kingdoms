#include "civ.h"
#include <string.h>
#include <algorithm>
#include <math.h>
#include <stdio.h>

void total_resources(const city& c, const map& m, 
		int* food, int* prod, int* comm)
{
	*food = 0; *prod = 0; *comm = 0;
	const std::list<coord>& resource_coords = c.get_resource_coords();
	for(std::list<coord>::const_iterator it = resource_coords.begin();
			it != resource_coords.end();
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

bool can_attack(const unit& u1, const unit& u2)
{
	if(!in_bounds(u1.xpos - 1, u2.xpos, u1.xpos + 1))
		return false;
	if(!in_bounds(u1.ypos - 1, u2.ypos, u1.ypos + 1))
		return false;
	return u1.uconf.max_strength > 0;
}

coord next_good_resource_spot(const city* c, const map* m)
{
	int curr_food, curr_prod, curr_comm;
	total_resources(*c, *m, &curr_food, &curr_prod, &curr_comm);
	int req_food = c->get_city_size() * 2 + 2 - curr_food;
	coord ret(0, 0);
	int opt_food = -1;
	int opt_prod = -1;
	int opt_comm = -1;
	for(int i = -2; i <= 2; i++) {
		for(int j = -2; j <= 2; j++) {
			if(abs(i) == 2 && abs(j) == 2)
				continue;
			if(!i && !j)
				continue;
			if(std::find(c->get_resource_coords().begin(),
					c->get_resource_coords().end(),
					coord(i, j)) != c->get_resource_coords().end())
				continue;
			int terr = m->get_data(c->xpos + i, c->ypos + j);
			if(terr == -1)
				continue;
			if(m->get_land_owner(c->xpos + i, c->ypos + j) != (int)c->civ_id)
				continue;
			int tf, tp, tc;
			m->get_resources_by_terrain(terr, false,
					&tf, &tp, &tc);
			if((tf >= opt_food && opt_food < req_food) || (tf >= req_food &&
			   (tp > opt_prod || (tp == opt_prod && tc > opt_comm)))) {
				ret.x = i;
				ret.y = j;
				opt_food = tf;
				opt_prod = tp;
				opt_comm = tc;
			}
		}
	}
	return ret;
}

civilization::civilization(std::string name, unsigned int civid, 
		const color& c_, map* m_, bool ai_,
		const std::vector<std::string>::iterator& names_start,
		const std::vector<std::string>::iterator& names_end)
	: civname(name),
	civ_id(civid),
	col(c_),
	m(m_),
	fog(fog_of_war(m_)),
	gold(0),
	science(0),
	alloc_gold(5),
	alloc_science(5),
	research_goal_id(0),
	ai(ai_),
	relationships(civid + 1, relationship_unknown),
	known_land_map(buf2d<int>(0, 0, -1)),
	curr_city_name_index(0)
{
	for(std::vector<std::string>::const_iterator it = names_start;
			it != names_end;
			++it) {
		city_names.push_back(*it);
	}
	relationships[civid] = relationship_peace;
	if(m) {
		fog = fog_of_war(m);
		known_land_map = buf2d<int>(m->size_x(),
				m->size_y(), -1);
	}
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

void civilization::reveal_land(int x, int y, int r)
{
	for(int i = x - r; i <= x + r; i++) {
		for(int j = y - r; j <= y + r; j++) {
			int di = m->wrap_x(i);
			int dj = m->wrap_y(j);
			known_land_map.set(di, dj, m->get_land_owner(di, dj));
		}
	}
}

unit* civilization::add_unit(int uid, int x, int y, const unit_configuration& uconf)
{
	unit* u = new unit(uid, x, y, civ_id, uconf);
	units.push_back(u);
	m->add_unit(u);
	fog.reveal(x, y, 1);
	reveal_land(x, y, 1);
	return u;
}

void civilization::remove_unit(unit* u)
{
	fog.shade(u->xpos, u->ypos, 1);
	m->remove_unit(u);
	units.remove(u);
	delete u;
}

void civilization::eliminate()
{
	while(!units.empty()) {
		remove_unit(units.back());
	}
	while(!cities.empty()) {
		remove_city(cities.back());
	}
	m->remove_civ_land(civ_id);
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
			max_moves = max_moves_it->second.max_moves;
		(*uit)->refill_moves(max_moves);
	}
}

void civilization::add_message(const msg& m)
{
	messages.push_back(m);
}

msg new_unit_msg(unit* u, city* c)
{
	msg m;
	m.type = msg_new_unit;
	m.msg_data.city_prod_data.building_city = c;
	m.msg_data.city_prod_data.prod_id = u->unit_id;
	return m;
}

msg new_advance_discovered(unsigned int adv_id)
{
	msg m;
	m.type = msg_new_advance;
	m.msg_data.new_advance_id = adv_id;
	return m;
}


msg discovered_civ(int civid)
{
	msg m;
	m.type = msg_civ_discovery;
	m.msg_data.discovered_civ_id = civid;
	return m;
}

msg new_improv_msg(city* c, unsigned int ciid)
{
	msg m;
	m.type = msg_new_city_improv;
	m.msg_data.city_prod_data.building_city = c;
	m.msg_data.city_prod_data.prod_id = ciid;
	return m;
}

void civilization::increment_resources(const unit_configuration_map& uconfmap,
		const advance_map& amap, const city_improv_map& cimap)
{
	int total_commerce = 0;
	for(std::list<city*>::iterator cit = cities.begin();
			cit != cities.end();
			++cit) {
		int food, prod, comm;
		total_resources(**cit, *m, &food, &prod, &comm);
		(*cit)->stored_food += food - (*cit)->get_city_size() * 2;
		total_commerce += comm;
		(*cit)->stored_prod += prod;
		if((*cit)->production.current_production_id > -1) {
			if((*cit)->production.producing_unit) {
				unit_configuration_map::const_iterator prod_unit = uconfmap.find((*cit)->production.current_production_id);
				if(prod_unit != uconfmap.end()) {
					if((int)prod_unit->second.production_cost <= (*cit)->stored_prod) {
						unit* u = add_unit((*cit)->production.current_production_id, 
								(*cit)->xpos, (*cit)->ypos, prod_unit->second);
						if((*cit)->has_barracks(cimap))
							u->veteran = true;
						(*cit)->stored_prod -= prod_unit->second.production_cost;
						add_message(new_unit_msg(u, (*cit)));
					}
				}
			}
			else {
				city_improv_map::const_iterator prod_improv = cimap.find((*cit)->production.current_production_id);
				if(prod_improv != cimap.end()) {
					if((int)prod_improv->second.cost <= (*cit)->stored_prod) {
						if((*cit)->built_improvements.find(prod_improv->first) ==
								(*cit)->built_improvements.end()) {
							(*cit)->built_improvements.insert(prod_improv->first);
							(*cit)->stored_prod -= prod_improv->second.cost;
						}
						add_message(new_improv_msg(*cit, prod_improv->first));
						(*cit)->production.current_production_id = -1;
					}
				}
			}
		}

		for(std::set<unsigned int>::const_iterator ciit = (*cit)->built_improvements.begin();
				ciit != (*cit)->built_improvements.end();
				++ciit) {
			city_improv_map::const_iterator cnit = cimap.find(*ciit);
			if(cnit != cimap.end())
				(*cit)->accum_culture += cnit->second.culture;
		}
	}
	int gold_add = total_commerce * alloc_gold / 10;
	int science_add = alloc_gold != 10 ? 
		(total_commerce - gold_add) * alloc_science / (10 - alloc_gold) : 
		0;

#if 0
	int luxury_add = alloc_gold + alloc_science != 10 ? 
		(total_commerce - gold_add - science_add) * 
		(10 - alloc_science - alloc_gold) / 
		(10 - alloc_gold - alloc_science) : 
		0;
#endif
	gold += gold_add;
	science += science_add;
	advance_map::const_iterator adv = amap.find(research_goal_id);
	if(adv == amap.end()) {
		research_goal_id = 0;
		add_message(new_advance_discovered(0));
	}
	else if(adv->second.cost <= science) {
		science -= adv->second.cost;
		add_message(new_advance_discovered(research_goal_id));
		researched_advances.insert(research_goal_id);
		research_goal_id = 0;
	}
}

int civilization::try_move_unit(unit* u, int chx, int chy)
{
	if(!u->moves || !(chx || chy))
		return 0;
	int newx = m->wrap_x(u->xpos + chx);
	int newy = m->wrap_y(u->ypos + chy);
	if(m->get_data(newx, newy) > 0 && can_move_to(newx, newy)) {
		m->remove_unit(u);
		fog.shade(u->xpos, u->ypos, 1);
		u->xpos = newx;
		u->ypos = newy;
		u->moves--;
		fog.reveal(u->xpos, u->ypos, 1);
		reveal_land(u->xpos, u->ypos, 1);
		m->add_unit(u);
		return 1;
	}
	return 0;
}

char civilization::fog_at(int x, int y) const
{
	return fog.get_value(m->wrap_x(x), m->wrap_y(y));
}

city* civilization::add_city(int x, int y)
{
	city* c = new city(city_names[curr_city_name_index++], x, y, civ_id);
	if(curr_city_name_index >= city_names.size()) {
		for(unsigned int i = 0; i < city_names.size(); i++) {
			city_names[i] = std::string("New ") + city_names[i];
		}
		curr_city_name_index = 0;
	}
	fog.reveal(c->xpos, c->ypos, 2);
	reveal_land(c->xpos, c->ypos, 2);
	fog.shade(c->xpos, c->ypos, 2);
	fog.reveal(c->xpos, c->ypos, 1);
	cities.push_back(c);
	m->add_city(c, x, y);
	coord rescoord = next_good_resource_spot(c, m);
	c->add_resource_worker(rescoord);
	return c;
}

void civilization::remove_city(city* c)
{
	fog.shade(c->xpos, c->ypos, 2);
	m->remove_city(c);
	cities.remove(c);
	delete c;
}

relationship civilization::get_relationship_to_civ(unsigned int civid) const
{
	if(civid == civ_id)
		return relationship_peace;
	if(relationships.size() <= civid)
		return relationship_unknown;
	return relationships[civid];
}

void civilization::set_relationship_to_civ(unsigned int civid, relationship val)
{
	if(civid == civ_id)
		return;
	if(relationships.size() <= civid) {
		relationships.resize(civid + 1, relationship_unknown);
	}
	relationships[civid] = val;
}

bool civilization::discover(unsigned int civid)
{
	if(civid != civ_id && get_relationship_to_civ(civid) == relationship_unknown) {
		set_relationship_to_civ(civid, relationship_peace);
		add_message(discovered_civ(civid));
		return 1;
	}
	return 0;
}

void civilization::undiscover(unsigned int civid)
{
	if(civid != civ_id) {
		set_relationship_to_civ(civid, relationship_unknown);
	}
}

void civilization::set_war(unsigned int civid)
{
	if(civid != civ_id) {
		set_relationship_to_civ(civid, relationship_war);
	}
}

void civilization::set_peace(unsigned int civid)
{
	if(civid != civ_id) {
		set_relationship_to_civ(civid, relationship_peace);
	}
}

std::vector<unsigned int> civilization::check_discoveries(int x, int y, int radius)
{
	std::vector<unsigned int> discs;
	for(int i = x - radius; i <= x + radius; i++) {
		for(int j = y - radius; j <= y + radius; j++) {
			int owner = m->get_spot_owner(i, j);
			if(owner != -1 && owner != (int)civ_id) {
				discover(owner);
				discs.push_back(owner);
			}
		}
	}
	return discs;
}

bool civilization::improv_discovered(const city_improvement& uconf) const
{
	return researched_advances.find(uconf.needed_advance) != researched_advances.end() || 
			uconf.needed_advance == 0;
}

bool civilization::unit_discovered(const unit_configuration& uconf) const
{
	return researched_advances.find(uconf.needed_advance) != researched_advances.end() || 
			uconf.needed_advance == 0;
}

void civilization::set_map(map* m_)
{
	if(m_ && !m) {
		m = m_;
		fog = fog_of_war(m);
		known_land_map = buf2d<int>(m->size_x(),
				m->size_y(), -1);
	}
}

bool civilization::can_move_to(int x, int y) const
{
	const std::list<unit*>& units = m->units_on_spot(x, y);
	int land_owner = m->get_land_owner(x, y);
	int unit_owner = -1;
	if(!units.empty())
		unit_owner = units.front()->civ_id;
	bool acceptable_unit_owner = unit_owner == -1 ||
		unit_owner == (int)civ_id ||
		get_relationship_to_civ(unit_owner) != relationship_peace;
	bool acceptable_land_owner = land_owner == -1 ||
		land_owner == (int)civ_id ||
		get_relationship_to_civ(land_owner) != relationship_peace;
	return acceptable_unit_owner && acceptable_land_owner;
}

int civilization::get_known_land_owner(int x, int y) const
{
	const int* v = known_land_map.get(m->wrap_x(x), m->wrap_y(y));
	if(!v)
		return -1;
	return *v;
}

bool civilization::blocked_by_land(int x, int y) const
{
	int land_owner = get_known_land_owner(x, y);
	if(land_owner < 0 || land_owner == (int)civ_id) {
		return false;
	}
	else {
		return get_relationship_to_civ(land_owner) == relationship_peace;
	}
}


