#include <algorithm>

#include "ai-defense.h"
#include "ai-debug.h"

defense_objective::defense_objective(round* r_, civilization* myciv_,
		const std::string& n)
	: objective(r_, myciv_, n)
{
	unit_strength_prio_coeff = 1;
	defense_units_prio_coeff = 400;
}

int defense_objective::get_unit_points(const unit& u) const
{
	if(!usable_unit(u.uconf))
		return -1;
	int tgtx, tgty;
	int prio = 1;
	tgtx = u.xpos;
	tgty = u.ypos;
	if(myciv->cities.size() != 0) {
		city* c = find_nearest_city(myciv, u, true);
		if(c) {
			tgtx = c->xpos;
			tgty = c->ypos;
			const std::list<unit*>& units = myciv->m->units_on_spot(tgtx, tgty);
			int num_units = std::count_if(units.begin(),
					units.end(),
					std::mem_fun(&unit::is_military_unit));
			if(tgtx == u.xpos && tgty == u.ypos)
				num_units--;
			prio = clamp<int>(1, 1000 + 
					unit_strength_prio_coeff * u.uconf.max_strength * u.uconf.max_strength - 
					defense_units_prio_coeff * num_units,
					1000);
		}
	}
	return prio;
}

bool defense_objective::add_unit(unit* u)
{
	if(!usable_unit(u->uconf))
		return false;
	int tgtx, tgty;
	tgtx = u->xpos;
	tgty = u->ypos;
	city* c = find_nearest_city(myciv, *u, true);
	if(c) {
		tgtx = c->xpos;
		tgty = c->ypos;
	}
	orders* o = new defend_orders(myciv, u, tgtx, tgty, 50);
	ordersmap.insert(std::make_pair(u->unit_id, o));
	return true;
}

bool defense_objective::compare_units(const unit_configuration& lhs,
		const unit_configuration& rhs) const
{
	if(lhs.is_water_unit())
		return false;
	if(lhs.max_strength > rhs.max_strength)
		return true;
	if(lhs.max_strength < lhs.max_strength)
		return false;
	if(lhs.production_cost < lhs.production_cost)
		return true;
	if(lhs.production_cost > lhs.production_cost)
		return false;
	return lhs.max_moves > lhs.max_moves;
}

int defense_objective::improvement_value(const city_improvement& ci) const
{
	return -1;
}

bool defense_objective::usable_unit(const unit_configuration& uc) const
{
	return uc.max_strength > 0 && uc.is_land_unit();
}

defend_orders::defend_orders(const civilization* civ_, unit* u_, 
		int x_, int y_, int waittime_)
	: goto_orders(civ_, u_, true, x_, y_),
	waittime(waittime_),
	max_waittime(waittime_)
{
}

action defend_orders::get_action()
{
	if(path.empty())
		return unit_action(action_fortify, u);
	else
		return goto_orders::get_action();
}

void defend_orders::drop_action()
{
	if(!path.empty())
		goto_orders::drop_action();
	else if(waittime > 0)
		waittime--;
}

bool defend_orders::finished()
{
	return path.empty() && waittime <= 0;
}

bool defend_orders::replan()
{
	waittime = max_waittime;
	return goto_orders::replan();
}

void defend_orders::clear()
{
	goto_orders::clear();
	waittime = 0;
}


