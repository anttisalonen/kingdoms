#include "ai-exploration.h"
#include "map-astar.h"

class explore_picker {
	private:
		const civilization& civ;
	public:
		explore_picker(const civilization& civ_) : civ(civ_) { }
		bool operator()(const coord& c) {
			for(int i = -1; i <= 1; i++) {
				for(int j = -1; j <= 1; j++) {
					if(i == 0 && j == 0)
						continue;
					if(c.x + i < 0 || c.x + i >= civ.m->size_x())
						continue;
					if(c.y + j < 0 || c.y + j >= civ.m->size_y())
						continue;
					if(civ.fog.get_value(c.x + i, c.y + j) == 0) {
						return true;
					}
				}
			}
			return false;
		}
};

std::list<coord> exploration_path(const civilization& civ, const unit& u)
{
	explore_picker picker(civ);
	return map_path_to_nearest(civ, 
			u, 
			false,
			coord(u.xpos, u.ypos), 
			picker);
}

int exploration_distance_to_points(unsigned int dist, int map_dim)
{
	if(dist == 0)
		return -1;
	int drop_coeff = 200;
	return clamp<int>(100, 1000 - dist * drop_coeff, 1000);
}

bool compare_exploration_units(const unit_configuration& lhs,
		const unit_configuration& rhs)
{
	if(lhs.is_water_unit() && rhs.is_land_unit())
		return true;
	if(lhs.is_land_unit() && rhs.is_water_unit())
		return false;
	if(lhs.max_moves > rhs.max_moves)
		return true;
	if(lhs.max_moves < lhs.max_moves)
		return false;
	if(lhs.production_cost < lhs.production_cost)
		return true;
	if(lhs.production_cost > lhs.production_cost)
		return false;
	return lhs.max_strength > lhs.max_strength;
}

bool acceptable_exploration_unit(const unit_configuration& uc)
{
	return !uc.settler && !uc.worker;
}

exploration_objective::exploration_objective(round* r_, civilization* myciv_, const std::string& n)
	: objective(r_, myciv_, n)
{
}

int exploration_objective::get_unit_points(const unit& u) const
{
	if(!acceptable_exploration_unit(u.uconf))
		return -1;
	unsigned int dist = exploration_path(*myciv, u).size();
	int val = exploration_distance_to_points(dist, 
			std::max(myciv->m->size_x(), myciv->m->size_y()));
	ai_debug_printf(myciv->civ_id, "exploration: %d\n", val);
	return val;
}

city_production exploration_objective::get_city_production(const city& c, 
		int* points) const
{
	return best_unit_production(c, points,
			compare_exploration_units,
			acceptable_exploration_unit);
}

bool exploration_objective::add_unit(unit* u)
{
	orders* o = create_exploration_orders(u);
	if(!o)
		return false;
	ordersmap.insert(std::make_pair(u->unit_id, o));
	return true;
}

orders* exploration_objective::create_exploration_orders(unit* u) const
{
	if(u->uconf.settler || u->uconf.worker)
		return NULL;
	else
		return new explore_orders(myciv, u, false);
}

explore_orders::explore_orders(const civilization* civ_, unit* u_,
		bool autocontinue_)
	: goto_orders(civ_, u_, false, u_->xpos, u_->ypos),
	autocontinue(autocontinue_)
{
	get_new_path();
}

void explore_orders::get_new_path()
{
	path.clear();
	std::list<coord> fog_path = exploration_path(*civ, *u);
	if(!fog_path.empty()) {
		tgtx = fog_path.back().x;
		tgty = fog_path.back().y;
		goto_orders::get_new_path();
	}
}

void explore_orders::drop_action()
{
	goto_orders::drop_action();
	if(path.empty() && autocontinue)
		get_new_path();
}

bool explore_orders::replan()
{
	get_new_path();
	return !path.empty();
}

void explore_orders::clear()
{
	path.clear();
}


