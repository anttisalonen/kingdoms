#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "map-astar.h"
#include "ai-commerce.h"
#include "ai-debug.h"

class worker_searcher {
	private:
		const civilization* myciv;
		int maxrange;
		unit* found_unit;
		const unit* self;
	public:
		worker_searcher(const civilization* myciv_, const unit* self_, int maxrange_) : 
			myciv(myciv_), maxrange(maxrange_), found_unit(NULL), self(self_) { }
		const unit* get_found_unit() const { return found_unit; }
		bool operator()(const coord& co) {
			const std::list<unit*>& units = myciv->m->units_on_spot(co.x, co.y);
			for(std::list<unit*>::const_iterator it = units.begin();
					it != units.end();
					++it) {
				if((*it)->uconf->worker && (*it) != self) {
					found_unit = *it;
					return true;
				}
			}
			maxrange--;
			if(maxrange < 0) {
				return true;
			}
			return false;
		}
};

commerce_objective::commerce_objective(pompelmous* r_, civilization* myciv_,
		const std::string& n)
	: objective(r_, myciv_, n)
{
	worker_prio = 700;
}

bool commerce_objective::compare_units(const unit_configuration& lhs,
		const unit_configuration& rhs) const
{
	if(lhs.is_water_unit())
		return false;
	if(lhs.production_cost < lhs.production_cost)
		return true;
	if(lhs.production_cost > lhs.production_cost)
		return false;
	return lhs.max_moves > lhs.max_moves;
}

bool commerce_objective::usable_unit(const unit_configuration& uc) const
{
	return uc.worker;
}

int commerce_objective::improvement_value(const city_improvement& ci) const
{
	int points = -1;
	if(ci.granary)
		points += 100;
	points += ci.comm_bonus * 10;
	points += ci.science_bonus * 10;
	points += ci.culture * 50;
	return points;
}

bool get_next_improv_spot(int startx, int starty, const civilization* civ,
		int* tgtx, int *tgty,
		improvement_type* tgt_imp)
{
	for(int i = -2; i <= 2; i++) {
		for(int j = -2; j <= 2; j++) {
			if(abs(i) == 2 && abs(j) == 2)
				continue;
			int xp = startx + i;
			int yp = starty + j;
			if(civ->m->get_land_owner(xp, yp) == (int)civ->civ_id) {
				if(civ->m->city_on_spot(xp, yp) != NULL)
					continue;
				const std::list<unit*>& units = civ->m->units_on_spot(xp, yp);
				for(std::list<unit*>::const_iterator it = units.begin();
						it != units.end(); ++it) {
					if((*it)->civ_id == (int)civ->civ_id &&
						(*it)->uconf->worker)
						continue;
				}
				if((civ->m->get_improvements_on(xp, yp) & ~improv_road) == 0) {
					if(civ->m->can_improve_terrain(xp,
								yp, civ->civ_id, improv_irrigation)) {
						*tgtx = xp;
						*tgty = yp;
						*tgt_imp = improv_irrigation;
						return true;
					}
					else if(civ->m->can_improve_terrain(xp,
								yp, civ->civ_id, improv_mine)) {
						*tgtx = xp;
						*tgty = yp;
						*tgt_imp = improv_mine;
						return true;
					}
				}
				if(civ->m->can_improve_terrain(xp,
							yp, civ->civ_id, improv_road)) {
					*tgtx = xp;
					*tgty = yp;
					*tgt_imp = improv_road;
					return true;
				}
			}
		}
	}
	return false;
}

int commerce_objective::get_unit_points(const unit& u) const
{
	if(!usable_unit(*u.uconf))
		return -1;
	int prio = worker_prio;
	worker_searcher searcher(myciv, &u, 25);
	boost::function<bool(const coord& a)> testfunc = boost::ref(searcher);
	map_path_to_nearest(*myciv, 
			u, 
			false,
			coord(u.xpos, u.ypos), 
			testfunc);
	if(searcher.get_found_unit() != NULL) {
		prio /= 4;
	}
	int tgtx, tgty;
	improvement_type tgt_imp;
	if(!get_next_improv_spot(u.xpos, u.ypos, myciv, &tgtx, &tgty,
				&tgt_imp)) {
		prio = -1;
	}
	return prio;
}

bool commerce_objective::add_unit(unit* u)
{
	if(!usable_unit(*u->uconf)) {
		return false;
	}
	orders* o = new connect_resources_orders(myciv, u);
	if(o->finished()) {
		ai_debug_printf(myciv->civ_id, "No resources to connect.\n");
		delete o;
		o = new connect_cities_orders(myciv, u);
		if(o->finished()) {
			ai_debug_printf(myciv->civ_id, "No cities to connect.\n");
			delete o;
			o = new improve_city_orders(myciv, u);
			if(o->finished()) {
				ai_debug_printf(myciv->civ_id, "No improvements to build.\n");
				delete o;
				return false;
			}
		}
	}
	ordersmap.insert(std::make_pair(u->unit_id, o));
	return true;
}

improve_orders::improve_orders(const civilization* civ_, unit* u_)
	: goto_orders(civ_, u_, false, u_->xpos, u_->ypos),
	tgt_imp(improv_none)
{
}

action improve_orders::get_action()
{
	if(u->is_improving()) {
		return action_none;
	}
	if(path.empty()) {
		if(!civ->m->can_improve_terrain(u->xpos, u->ypos,
					civ->civ_id, tgt_imp)) {
			if(replan() && 
				(civ->m->can_improve_terrain(tgtx, tgty,
					civ->civ_id, tgt_imp))) {
				if(u->xpos == tgtx && u->ypos == tgty)
					return improve_unit_action(u, tgt_imp);
				else
					return goto_orders::get_action();
			}
			else {
				clear();
				return action_none;
			}
				 
		}
		else {
			ai_debug_printf(civ->civ_id, "improving at (%d, %d)\n",
					u->xpos, u->ypos);
			return improve_unit_action(u, tgt_imp);
		}
	}
	else {
		return goto_orders::get_action();
	}
}

void improve_orders::drop_action()
{
	if(u->is_improving()) {
		return;
	}
	if(path.empty()) {
		tgt_imp = improv_none;
	}
	else {
		goto_orders::drop_action();
	}
}

bool improve_orders::finished()
{
	return path.empty() && tgt_imp == improv_none;
}

void improve_orders::clear()
{
	tgt_imp = improv_none;
	goto_orders::clear();
}

improve_city_orders::improve_city_orders(const civilization* civ_, unit* u_)
	: improve_orders(civ_, u_),
	base_city(NULL)
{
	replan();
}

bool improve_city_orders::replan()
{
	path.clear();
	base_city = find_nearest_city(civ, *u, true);
	if(!base_city)
		return false;
	if(get_next_improv_spot(base_city->xpos, base_city->ypos, civ,
				&tgtx, &tgty, &tgt_imp)) {
		bool succ;
		if(tgtx == u->xpos && tgty == u->ypos)
			succ = true;
		else
			succ = goto_orders::replan();
		ai_debug_printf(civ->civ_id, "target: (%d, %d) - path: %zu - succ: %d\n",
				tgtx, tgty, path.size(), succ);
		return succ;
	}
	else {
		return false;
	}
}

class city_collector {
	public:
		city_collector(const civilization& civ_, int num_cities_)
			: civ(civ_), num_cities(num_cities_) { }
		bool operator()(const coord& a)
		{
			city* c = civ.m->city_on_spot(a.x, a.y);
			if(c && c->civ_id == civ.civ_id) {
				cities.push_back(c);
				if(cities.size() >= num_cities)
					return true;
			}
			return false;
		}
		const std::vector<city*>& get_found_cities() const
		{
			return cities;
		}
	private:
		const civilization& civ;
		std::vector<city*> cities;
		unsigned int num_cities;
};

bool compare_lengths(const std::list<coord>& a, const std::list<coord>& b)
{
	return a.size() < b.size();
}

bool is_roadable(const map* m, const coord& c)
{
	int terr = m->get_data(c.x, c.y);
	if(terr < 0 || terr >= num_terrain_types)
		return false;
	return m->resconf.roadable[terr];
}

build_road_orders::build_road_orders(const civilization* civ_, unit* u_)
	: improve_orders(civ_, u_)
{
}

bool build_road_orders::replan()
{
	ai_debug_printf(civ->civ_id, "Road path has size %zu\n",
			road_path.size());

	if(!road_path.empty()) {
		coord p = road_path.front();
		if(p.x == u->xpos && p.y == u->ypos)
			road_path.pop_front();
	}

	tgt_imp = improv_road;
	// NOTE: the unimprovable spots (which shouldn't exist)
	// must be deleted here, because improve_orders will check
	// whether the terrain improvement can be built.
	while(!road_path.empty()) {
		tgtx = road_path.begin()->x;
		tgty = road_path.begin()->y;
		if(!civ->m->can_improve_terrain(tgtx, tgty,
					     civ->civ_id, tgt_imp)) {
			road_path.pop_front();
		}
		else {
			break;
		}
	}

	if(!road_path.empty()) {
		tgtx = road_path.begin()->x;
		tgty = road_path.begin()->y;
		return goto_orders::replan();
	}
	else {
		improve_orders::clear();
		return false;
	}
}

connect_cities_orders::connect_cities_orders(const civilization* civ_, unit* u_)
	: build_road_orders(civ_, u_)
{
	city_collector coll(*civ, 5);

	// get an array of nearby cities
	boost::function<bool(const coord& a)> testfunc = boost::ref(coll);
	map_path_to_nearest(*civ, *u, false, coord(u->xpos, u->ypos),
			testfunc);
	const std::vector<city*>& found = coll.get_found_cities();

	road_path.clear();
	// find unconnected cities
	if(!found.empty()) {
		std::vector<std::list<coord> > edges;
		for(unsigned int i = 0; i < found.size() - 1; i++) {
			for(unsigned int j = i + 1; j < found.size(); j++) {
				// check if a road exists between cities
				road_path = map_astar(*civ,
						*u, false,
						coord(found[i]->xpos,
							found[i]->ypos),
						coord(found[j]->xpos,
							found[j]->ypos),
						false, true);
				if(!road_path.empty()) {
					continue;
				}

				// plan the road between cities
				std::list<coord> road_path = map_astar(*civ,
						*u, false,
						coord(found[i]->xpos,
							found[i]->ypos),
						coord(found[j]->xpos,
							found[j]->ypos),
						false, false,
						boost::bind(is_roadable, civ->m, boost::lambda::_1));
				if(!road_path.empty()) {
					edges.push_back(road_path);
				}
				else {
					ai_debug_printf(civ->civ_id, "no path found between cities %d and %d\n",
							i, j);
				}
			}
		}

		if(!edges.empty()) {
			// at least one edge found
			std::sort(edges.begin(), edges.end(), compare_lengths);
			road_path = *edges.begin();
		}
	}
	build_road_orders::replan();
}

class resource_collector {
	public:
		resource_collector(const civilization& civ_, const unit* u_)
			: civ(civ_), u(u_) { }
		bool operator()(const coord& co)
		{
			int res = civ.m->get_resource(co.x, co.y);
			if(res && civ.m->get_land_owner(co.x, co.y) == (int)civ.civ_id) {
				city* c = find_nearest_city(&civ, *u, true);
				if(!c) {
					// no nearest city
					ai_debug_printf(civ.civ_id, "No nearest city.\n");
					return true;
				}
				else {
					// check if a road from the city to the resource exists
					std::list<coord> road_path = map_astar(civ, *u,
							false, coord(c->xpos, c->ypos),
							co,
							false, true);
#if 0
					ai_debug_printf(civ.civ_id, "Searching for path on roads from (%d, %d) to (%d, %d): %d\n",
							c->xpos, c->ypos, co.x, co.y,
							!road_path.empty());
#endif
					if(!road_path.empty()) {
						// path on road found
						return false;
					}
					else {
						// plan road from the city to the resource
						path = map_astar(civ, *u,
								false, coord(c->xpos, c->ypos),
								co,
								false, false,
								boost::bind(is_roadable, civ.m, boost::lambda::_1));
						ai_debug_printf(civ.civ_id, "Searching for path from (%d, %d) to (%d, %d): %d\n",
								c->xpos, c->ypos, co.x, co.y,
								!path.empty());
						if(!path.empty()) {
							return true;
						}
					}
				}
			}
			return false;
		}

		const std::list<coord>& get_path() const
		{
			return path;
		}

	private:
		const civilization& civ;
		const unit* u;
		std::list<coord> path;
};

connect_resources_orders::connect_resources_orders(const civilization* civ_, unit* u_)
	: build_road_orders(civ_, u_)
{
	ai_debug_printf(civ->civ_id, "Searching for resources to connect.\n");
	resource_collector coll(*civ, u);
	// search for available resources and
	// check whether the resource is connected to the nearest friendly city
	boost::function<bool(const coord& a)> testfunc = boost::ref(coll);
	map_path_to_nearest(*civ, *u, false, coord(u->xpos, u->ypos),
			testfunc);
	road_path = coll.get_path();
	build_road_orders::replan();
}


