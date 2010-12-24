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
				if((*it)->uconf.worker && (*it) != self) {
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

commerce_objective::commerce_objective(round* r_, civilization* myciv_,
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
	return -1;
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
						(*it)->uconf.worker)
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
	if(!usable_unit(u.uconf))
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
	if(!usable_unit(u->uconf))
		return false;
	orders* o = new improve_orders(myciv, u);
	if(o->finished())
		return false;
	ordersmap.insert(std::make_pair(u->unit_id, o));
	return true;
}

improve_orders::improve_orders(const civilization* civ_, unit* u_)
	: goto_orders(civ_, u_, false, u_->xpos, u_->ypos),
	base_city(NULL),
	tgt_imp(improv_none)
{
	replan();
}

action improve_orders::get_action()
{
	if(u->is_improving())
		return action_none;
	if(path.empty()) {
		if(!civ->m->can_improve_terrain(u->xpos, u->ypos,
					civ->civ_id, tgt_imp)) {
			if(replan() && 
				(civ->m->can_improve_terrain(tgtx, tgty,
					civ->civ_id, tgt_imp)))
				return get_action();
			else
				return action_none;
				 
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

bool improve_orders::replan()
{
	path.clear();
	base_city = find_nearest_city(civ, *u, true);
	if(!base_city)
		return false;
	if(get_next_improv_spot(base_city->xpos, base_city->ypos, civ,
				&tgtx, &tgty, &tgt_imp)) {
		bool succ = goto_orders::replan();
		ai_debug_printf(civ->civ_id, "target: (%d, %d) - path: %d - succ: %d\n",
				tgtx, tgty, path.size(), succ);
		return succ;
	}
	else {
		return false;
	}
}

void improve_orders::clear()
{
	tgt_imp = improv_none;
	goto_orders::clear();
}


