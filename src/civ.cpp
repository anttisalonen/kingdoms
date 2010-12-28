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
		m.get_resources_on_spot(c.xpos + it->x,
				c.ypos + it->y, &f, &p, &cm);
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
	return u1.uconf->max_strength > 0;
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
			m->get_resources_on_spot(c->xpos + i, c->ypos + j, &tf, &tp, &tc);
			if((tf >= opt_food && opt_food < req_food) || 
				 (tf >= req_food &&
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
		const std::vector<std::string>::iterator& names_end,
		const government* gov_)
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
	curr_city_name_index(0),
	next_city_id(1),
	next_unit_id(1),
	gov(gov_),
	points(0),
	cross_oceans(false)
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

civilization::civilization()
	: civ_id(1337),
	m(NULL)
{
}

civilization::~civilization()
{
	for(std::map<unsigned int, unit*>::iterator it = units.begin();
			it != units.end();
			++it) {
		delete it->second;
	}
	for(std::map<unsigned int, city*>::iterator cit = cities.begin();
			cit != cities.end();
			++cit) {
		delete cit->second;
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

unit* civilization::add_unit(int uid, int x, int y, 
		const unit_configuration& uconf,
		unsigned int road_moves)
{
	unit* u = new unit(next_unit_id, uid, x, y, civ_id, uconf, road_moves);
	units.insert(std::make_pair(next_unit_id++, u));
	built_units[uid]++;
	m->add_unit(u);
	fog.reveal(x, y, 1);
	reveal_land(x, y, 1);
	return u;
}

void civilization::remove_unit(unit* u)
{
	std::map<unsigned int, unit*>::iterator uit = units.find(u->unit_id);
	if(uit != units.end()) {
		if(u->carried()) {
			u->unload(u->xpos, u->ypos);
		}
		std::list<unit*>::iterator it = u->carried_units.begin();
		while(it != u->carried_units.end()) {
			remove_unit(*it);
			it = u->carried_units.begin();
		}
		fog.shade(u->xpos, u->ypos, 1);
		m->remove_unit(u);
		lost_units[uit->second->uconf_id]++;
		units.erase(uit);
		delete u;
	}
}

void civilization::eliminate()
{
	std::map<unsigned int, unit*>::iterator uit;
	while((uit = units.begin()) != units.end()) {
		remove_unit(uit->second);
	}
	std::map<unsigned int, city*>::iterator cit;
	while((cit = cities.begin()) != cities.end()) {
		remove_city(cit->second, true);
	}
	m->remove_civ_land(civ_id);
}

void civilization::refill_moves(const unit_configuration_map& uconfmap)
{
	for(std::map<unsigned int, unit*>::iterator uit = units.begin();
		uit != units.end();
		++uit) {
		improvement_type i = improv_none;
		unit* u = uit->second;
		u->new_round(i);
		if(i != improv_none) {
			m->try_improve_terrain(u->xpos,
					u->ypos, civ_id, i);
		}
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
	m.msg_data.city_prod_data.building_city_id = c->city_id;
	m.msg_data.city_prod_data.prod_id = u->uconf_id;
	m.msg_data.city_prod_data.unit_id = u->unit_id;
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
	m.msg_data.city_prod_data.building_city_id = c->city_id;
	m.msg_data.city_prod_data.prod_id = ciid;
	return m;
}

msg unit_disbanded(unsigned int uit)
{
	msg m;
	m.type = msg_unit_disbanded;
	m.msg_data.disbanded_unit_id = uit;
	return m;
}

void civilization::update_national_income()
{
	int total_commerce = 0;
	for(std::map<unsigned int, city*>::iterator cit = cities.begin();
			cit != cities.end();
			++cit) {
		int food, prod, comm;
		city* this_city = cit->second;
		total_resources(*this_city, *m, &food, &prod, &comm);
		total_commerce += comm;
	}
	national_income = total_commerce * alloc_gold / 10;
}

void civilization::update_military_expenses()
{
	military_expenses = 0;
	int free_units_togo = gov->free_units;
	for(std::map<unsigned int, unit*>::const_iterator it = units.begin();
			it != units.end(); ++it) {
		if(it->second->uconf->max_strength > 0) {
			if(free_units_togo > 0)
				free_units_togo--;
			else
				military_expenses += gov->unit_cost;
		}
	}
}

void civilization::increment_resources(const unit_configuration_map& uconfmap,
		const advance_map& amap, const city_improv_map& cimap,
		unsigned int road_moves)
{
	int total_commerce = 0;
	for(std::map<unsigned int, city*>::iterator cit = cities.begin();
			cit != cities.end();
			++cit) {
		int food, prod, comm;
		city* this_city = cit->second;
		total_resources(*this_city, *m, &food, &prod, &comm);
		this_city->stored_food += food - this_city->get_city_size() * 2;
		total_commerce += comm;
		this_city->stored_prod += prod;
		if(this_city->production.current_production_id > -1) {
			if(this_city->production.producing_unit) {
				unit_configuration_map::const_iterator prod_unit = uconfmap.find(this_city->production.current_production_id);
				if(prod_unit != uconfmap.end()) {
					if((int)prod_unit->second.production_cost <= this_city->stored_prod) {
						unit* u = add_unit(this_city->production.current_production_id, 
								this_city->xpos, this_city->ypos, prod_unit->second,
								road_moves);
						if(this_city->has_barracks(cimap))
							u->veteran = true;
						this_city->stored_prod -= prod_unit->second.production_cost;
						add_message(new_unit_msg(u, this_city));
					}
				}
			}
			else {
				city_improv_map::const_iterator prod_improv = cimap.find(this_city->production.current_production_id);
				if(prod_improv != cimap.end()) {
					if((int)prod_improv->second.cost <= this_city->stored_prod) {
						if(this_city->built_improvements.find(prod_improv->first) ==
								this_city->built_improvements.end()) {
							this_city->built_improvements.insert(prod_improv->first);
							this_city->stored_prod -= prod_improv->second.cost;
							if(prod_improv->second.palace) {
								destroy_old_palace(this_city, cimap);
							}
						}
						add_message(new_improv_msg(this_city, prod_improv->first));
						this_city->production.current_production_id = -1;
					}
				}
			}
		}

		for(std::set<unsigned int>::const_iterator ciit = this_city->built_improvements.begin();
				ciit != this_city->built_improvements.end();
				++ciit) {
			city_improv_map::const_iterator cnit = cimap.find(*ciit);
			if(cnit != cimap.end())
				this_city->accum_culture += cnit->second.culture;
		}
	}
	national_income = total_commerce * alloc_gold / 10;
	int science_add = alloc_gold != 10 ? 
		(total_commerce - national_income) * alloc_science / (10 - alloc_gold) : 
		0;

#if 0
	int luxury_add = alloc_gold + alloc_science != 10 ? 
		(total_commerce - national_income - science_add) * 
		(10 - alloc_science - alloc_gold) / 
		(10 - alloc_gold - alloc_science) : 
		0;
#endif

	update_military_expenses();
	gold += national_income - military_expenses;
	{
		std::map<unsigned int, unit*>::iterator uit = units.begin();
		while(uit != units.end() && gold < 0) {
			std::map<unsigned int, unit*>::iterator uit2(uit);
			uit++;
			if(uit2->second->uconf->max_strength > 0) {
				gold += gov->unit_cost;
				add_message(unit_disbanded(uit2->second->unit_id));
				remove_unit(uit2->second);
			}
		}
	}
	science += science_add;
	advance_map::const_iterator adv = amap.find(research_goal_id);
	if(adv == amap.end()) {
		add_message(new_advance_discovered(0));
		update_ocean_crossing(uconfmap, amap, 0);
		setup_default_research_goal(amap);
	}
	else if(adv->second.cost <= science) {
		science -= adv->second.cost;
		add_message(new_advance_discovered(research_goal_id));
		researched_advances.insert(research_goal_id);
		update_ocean_crossing(uconfmap, amap, research_goal_id);
		setup_default_research_goal(amap);
	}
}

class not_in_set {
	private:
		const std::set<unsigned int>& u;
	public:
		not_in_set(const std::set<unsigned int>& u_) : u(u_) { }
		bool operator()(const std::pair<unsigned int, advance>& i) {
			return u.find(i.first) == u.end();
		}
};

void civilization::setup_default_research_goal(const advance_map& amap)
{
	advance_map::const_iterator it = std::find_if(amap.begin(),
			amap.end(),
			not_in_set(researched_advances));
	if(it != amap.end()) {
		research_goal_id = it->first;
	}
	else {
		research_goal_id = 0;
	}
}

int civilization::try_move_unit(unit* u, int chx, int chy)
{
	if((!u->num_moves() && !u->num_road_moves()) || !(chx || chy)) {
		return 0;
	}
	int newx = m->wrap_x(u->xpos + chx);
	int newy = m->wrap_y(u->ypos + chy);
	if(m->terrain_allowed(*u, newx, newy) && can_move_to(newx, newy)) {
		m->remove_unit(u);
		if(!u->carried())
			fog.shade(u->xpos, u->ypos, 1);
		bool succ = u->move_to(newx, newy, 
				m->road_between(u->xpos, u->ypos, newx, newy));
		if(!u->carried())
			fog.reveal(u->xpos, u->ypos, 1);
		reveal_land(u->xpos, u->ypos, 1);
		m->add_unit(u);
		for(std::list<unit*>::iterator it = u->carried_units.begin();
				it != u->carried_units.end();
				++it) {
			(*it)->xpos = u->xpos;
			(*it)->ypos = u->ypos;
		}
		if(succ && u->carried()) {
			unload_unit(u, newx, newy);
		}
		return succ;
	}
	return 0;
}

char civilization::fog_at(int x, int y) const
{
	return fog.get_value(m->wrap_x(x), m->wrap_y(y));
}

city* civilization::add_city(int x, int y)
{
	city* c = new city(city_names[curr_city_name_index++], x, y, civ_id,
			next_city_id);
	if(curr_city_name_index >= city_names.size()) {
		for(unsigned int i = 0; i < city_names.size(); i++) {
			city_names[i] = std::string("New ") + city_names[i];
		}
		curr_city_name_index = 0;
	}
	m->add_city(c, x, y);
	add_city(c);
	update_city_resource_workers(c);
	return c;
}

void civilization::update_city_resource_workers(city* c)
{
	c->clear_resource_workers();
	while(c->add_resource_worker(next_good_resource_spot(c, m)))
		;
}

void civilization::add_city(city* c)
{
	fog.reveal(c->xpos, c->ypos, 2);
	reveal_land(c->xpos, c->ypos, 2);
	fog.shade(c->xpos, c->ypos, 2);
	fog.reveal(c->xpos, c->ypos, 1);
	c->set_city_id(next_city_id);
	c->set_civ_id(civ_id);
	cities.insert(std::make_pair(next_city_id++, c));
}

void civilization::remove_city(city* c, bool del)
{
	std::map<unsigned int, city*>::iterator cit = cities.find(c->city_id);
	if(cit != cities.end()) {
		fog.shade(c->xpos, c->ypos, 2);
		cities.erase(cit);
		if(del) {
			m->remove_city(c);
			delete c;
		}
	}
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
		unit_owner == (int)civ_id;
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

void civilization::set_government(const government* g)
{
	gov = g;
}

void civilization::destroy_old_palace(const city* c, const city_improv_map& cimap)
{
	// go through all cities
	for(std::map<unsigned int, city*>::iterator it = cities.begin();
			it != cities.end();
			++it) {
		if(it->second != c) {
			// all improvements in the city
			for(std::set<unsigned int>::iterator cit = it->second->built_improvements.begin();
					cit != it->second->built_improvements.end();
					++cit) {
				// see if this improvement is the palace
				city_improv_map::const_iterator ciit = cimap.find(*cit);
				if(ciit != cimap.end()) {
					if(ciit->second.palace) {
						it->second->built_improvements.erase(cit);
						return;
					}
				}
			}
		}
	}
}

bool civilization::can_build_unit(const unit_configuration& uc, const city& c) const
{
	if(!unit_discovered(uc))
		return false;
	if(uc.is_water_unit() && !m->connected_to_sea(c.xpos, c.ypos))
		return false;
	return true;
}

bool civilization::can_build_improvement(const city_improvement& ci, const city& c) const
{
	if(!improv_discovered(ci))
		return false;
	if(c.built_improvements.find(ci.improv_id) != c.built_improvements.end())
		return false;
	return true;
}

bool civilization::load_unit(unit* loadee, unit* loader)
{
	int oldx = loadee->xpos;
	int oldy = loadee->ypos;
	if(loadee->load_at(loader)) {
		m->remove_unit(loadee);
		fog.shade(oldx, oldy, 1);
		printf("loaded unit\n");
		return true;
	}
	return false;
}

bool civilization::unload_unit(unit* unloadee, int x, int y)
{
	if(!unloadee->unload(x, y))
		return false;
	m->add_unit(unloadee);
	fog.reveal(x, y, 1);
	reveal_land(x, y, 1);
	printf("unloaded unit\n");
	return true;
}

const std::map<unsigned int, int>& civilization::get_built_units() const
{
	return built_units;
}

const std::map<unsigned int, int>& civilization::get_lost_units() const
{
	return lost_units;
}

void civilization::add_points(unsigned int num)
{
	points += num;
}

void civilization::reset_points()
{
	points = 0;
}

int civilization::get_points() const
{
	return points;
}

void civilization::update_ocean_crossing(const unit_configuration_map& uconfmap,
		const advance_map& amap, int adv_id)
{
	// NOTE: if units are ever obsoleted, it has to be checked here too.
	if(!cross_oceans) {
		for(unit_configuration_map::const_iterator uit = uconfmap.begin();
				uit != uconfmap.end();
				++uit) {
			if((int)uit->second.needed_advance == adv_id && 
				uit->second.ocean_unit) {
				cross_oceans = true;
				return;
			}
		}
	}
}

bool civilization::can_cross_oceans() const
{
	return cross_oceans;
}

