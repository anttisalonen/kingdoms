#include "map-astar.h"

#include "ai-offense.h"
#include "ai-defense.h"
#include "ai-debug.h"

class enemy_picker {
	private:
		const civilization* myciv;
	public:
		enemy_picker(const civilization* myciv_) : 
			myciv(myciv_) { }
		bool operator()(const coord& co) {
			const city* c = myciv->m->city_on_spot(co.x, co.y);
			const std::list<unit*>& units = myciv->m->units_on_spot(co.x, co.y);
			int civid = -1;
			if(myciv->fog_at(co.x, co.y) != 2)
				return false;
			if(c)
				civid = c->civ_id;
			else if(!units.empty())
				civid = units.front()->civ_id;
			return civid != -1 && civid != (int)myciv->civ_id &&
				 myciv->get_relationship_to_civ(civid) == relationship_war;
		}
};

bool find_nearest_enemy(const civilization* myciv, const unit* u, int* tgtx, int* tgty)
{
	enemy_picker picker(myciv);
	std::list<coord> found_path = map_path_to_nearest(*myciv, 
			*u, 
			true,
			coord(u->xpos, u->ypos), 
			picker);
	if(found_path.empty()) {
		return false;
	}
	else {
		*tgtx = found_path.back().x;
		*tgty = found_path.back().y;
		return true;
	}
}

offense_objective::offense_objective(pompelmous* r_, civilization* myciv_,
		const std::string& n)
	: defense_objective(r_, myciv_, n)
{
	max_offense_prio = 1000;
	unit_strength_prio_coeff = 1;
	offense_dist_prio_coeff = 10;
}

int offense_objective::get_unit_points(const unit& u) const
{
	if(u.uconf->max_strength == 0)
		return -1;
	if(u.uconf->is_water_unit())
		return -1;
	int tgtx, tgty;
	int prio = -1;
	tgtx = u.xpos;
	tgty = u.ypos;
	if(find_nearest_enemy(myciv, &u, &tgtx, &tgty)) {
		prio = std::max<int>(0, max_offense_prio + 
				unit_strength_prio_coeff * u.uconf->max_strength * u.uconf->max_strength - 
				offense_dist_prio_coeff * myciv->m->manhattan_distance(tgtx, tgty,
					u.xpos, u.ypos));
	}
	return prio;
}

bool offense_objective::add_unit(unit* u)
{
	int tgtx, tgty;
	tgtx = u->xpos;
	tgty = u->ypos;
	if(!find_nearest_enemy(myciv, u, &tgtx, &tgty)) {
		return false;
	}
	attack_orders* o = new attack_orders(myciv, u, tgtx, tgty);
	ordersmap.insert(std::make_pair(u->unit_id, o));
	return true;
}

attack_orders::attack_orders(const civilization* civ_, unit* u_, int x_, int y_)
	: goto_orders(civ_, u_, true, x_, y_),
	att_x(0),
	att_y(0)
{
}

void attack_orders::check_for_enemies()
{
	att_x = 0;
	att_y = 0;
	for(int i = -1; i <= 1; i++) {
		for(int j = -1; j <= 1; j++) {
			if(i == 0 && j == 0)
				continue;
			int xp = civ->m->wrap_x(u->xpos + i);
			int yp = civ->m->wrap_y(u->ypos + j);
			int owner = civ->m->get_spot_resident(xp, yp);
			if(owner >= 0 && owner != (int)civ->civ_id &&
					civ->get_relationship_to_civ(owner) == relationship_war) {
				att_x = i;
				att_y = j;
				return;
			}
		}
	}
}

action attack_orders::get_action()
{
	check_for_enemies();
	if(att_x != 0 && att_y != 0) {
		return move_unit_action(u, att_x, att_y);
	}
	else {
		return goto_orders::get_action();
	}
}

void attack_orders::drop_action()
{
	if(att_x == 0 || att_y == 0)
		goto_orders::drop_action();
	else {
		att_x = 0;
		att_y = 0;
	}
}

bool attack_orders::finished()
{
	if(att_x == 0 || att_y == 0)
		return goto_orders::finished();
	else
		return false;
}

bool attack_orders::replan()
{
	if(find_nearest_enemy(civ, u, &tgtx, &tgty)) {
		return goto_orders::replan();
	}
	else {
		return false;
	}
}

void attack_orders::clear()
{
	goto_orders::clear();
}


