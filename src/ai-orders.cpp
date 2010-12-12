#include <queue>

#include "ai-orders.h"
#include "map-astar.h"

class city_picker {
	private:
		const civilization* myciv;
		bool my_city;
	public:
		city_picker(const civilization* myciv_, bool my_city_) : 
			myciv(myciv_), my_city(my_city_) { }
		bool operator()(const coord& co) {
			const city* c = myciv->m->city_on_spot(co.x, co.y);
			if(!c) {
				return false;
			}
			else {
				if(my_city) {
					return c->civ_id == myciv->civ_id;
				}
				else {
					return c->civ_id != myciv->civ_id && 
						myciv->get_relationship_to_civ(c->civ_id) == relationship_war;
				}
			}
		}
};

city* find_nearest_city(const civilization* myciv, const unit* u, bool own)
{
	city_picker picker(myciv, own);
	std::list<coord> path_to_city = map_path_to_nearest(*myciv, 
			*u, 
			false,
			coord(u->xpos, u->ypos), 
			picker);
	if(path_to_city.empty()) {
		return NULL;
	}
	else {
		return myciv->m->city_on_spot(path_to_city.back().x,
				path_to_city.back().y);
	}
}

class found_city_picker {
	private:
		const civilization* myciv;
		const ai_tunables_found_city& found_city;
		// counter acts as a sort of distance-to-origin meter
		int counter;
	public:
		std::priority_queue<std::pair<int, coord> > pq;
		// only inspect 100 nearest locations
		found_city_picker(const civilization* myciv_, 
				const ai_tunables_found_city& found_city_) :
			myciv(myciv_), found_city(found_city_), 
			counter(found_city_.max_search_range) { }
		bool operator()(const coord& co);
};

int points_for_city_founding(const civilization* civ,
		const ai_tunables_found_city& found_city,
		int counter, const coord& co)
{
	if(counter <= 0)
		return -1;

	if(!civ->m->can_found_city_on(co.x, co.y))
		return -1;

	// do not found a city nearer than N squares to a friendly city
	for(int i = -found_city.min_dist_to_friendly_city; 
			i <= found_city.min_dist_to_friendly_city; i++)
		for(int j = -found_city.min_dist_to_friendly_city; 
				j <= -found_city.min_dist_to_friendly_city; j++)
			if(civ->m->has_city_of(co.x + i, co.y + j, civ->civ_id))
				return -1;

	// do not found a city nearer than N squares to any city
	for(int i = -found_city.min_dist_to_city; i <= found_city.min_dist_to_city; i++)
		for(int j = -found_city.min_dist_to_city; j <= found_city.min_dist_to_city; j++)
			if(civ->m->city_on_spot(co.x + i, co.y + j))
				return -1;

	int food_points = 0;
	int prod_points = 0;
	int comm_points = 0;
	civ->m->get_total_city_resources(co.x, co.y, &food_points,
			&prod_points, &comm_points);

	food_points *= found_city.food_coeff;
	prod_points *= found_city.prod_coeff;
	comm_points *= found_city.comm_coeff;

	// filter out very bad locations
	if(food_points < found_city.min_food_points || 
			prod_points < found_city.min_prod_points || 
			comm_points < found_city.min_comm_points)
		return -1;

	return clamp<int>(0,
			found_city.found_city_coeff * 
			(counter + food_points + prod_points + comm_points),
			found_city.max_found_city_prio);
}

bool found_city_picker::operator()(const coord& co)
{
	counter -= found_city.range_coeff;
	if(counter <= 0)
		return true;

	int points = points_for_city_founding(myciv,
			found_city, counter, co);

	pq.push(std::make_pair(points, co));
	return false;
}

bool find_best_city_pos(const civilization* myciv,
		const ai_tunables_found_city& found_city,
		const unit* u, int* tgtx, int* tgty, int* prio)
{
	found_city_picker picker(myciv, found_city);
	boost::function<bool(const coord& a)> testfunc = boost::ref(picker);
	map_path_to_nearest(*myciv, 
			*u, 
			false,
			coord(u->xpos, u->ypos), 
			testfunc);
	if(picker.pq.empty()) {
		*tgtx = u->xpos;
		*tgty = u->ypos;
		if(prio)
			*prio = -1;
		return false;
	}
	std::pair<int, coord> val = picker.pq.top();
	*tgtx = val.second.x;
	*tgty = val.second.y;
	if(prio)
		*prio = val.first;
	return true;
}

primitive_orders::primitive_orders(const action& a_)
	: a(a_), finished_flag(false)
{
}

action primitive_orders::get_action()
{
	if(finished())
		return action(action_none);
	return a;
}

void primitive_orders::drop_action()
{
	finished_flag = true;
}

bool primitive_orders::finished()
{
	return finished_flag;
}

bool primitive_orders::replan()
{
	return false;
}

void primitive_orders::clear()
{
	finished_flag = true;
}

action orders_composite::get_action()
{
	if(finished())
		return action(action_none);
	return ord.front()->get_action();
}

void orders_composite::drop_action()
{
	if(ord.size() > 0) {
		if(ord.front()->finished()) {
			delete ord.front();
			ord.pop_front();
		}
		else {
			ord.front()->drop_action();
		}
	}
}

bool orders_composite::finished()
{
	if(ord.empty())
		return true;
	return ord.size() == 1 && ord.front()->finished();
}

void orders_composite::add_orders(orders* o)
{
	ord.push_back(o);
}

bool orders_composite::replan()
{
	if(ord.empty())
		return false;
	return ord.front()->replan();
}

void orders_composite::clear()
{
	ord.clear();
}

goto_orders::goto_orders(const civilization* civ_, unit* u_, 
		bool ignore_enemy_, int x_, int y_)
	: tgtx(x_),
	tgty(y_),
	civ(civ_),
	u(u_),
	ignore_enemy(ignore_enemy_)
{
	get_new_path();
}

void goto_orders::get_new_path()
{
	path = map_astar(*civ, *u, ignore_enemy, coord(u->xpos, u->ypos), coord(tgtx, tgty));
	if(!path.empty())
		path.pop_front();
}

action goto_orders::get_action()
{
	if(finished())
		return action(action_none);
	int chx = path.front().x - u->xpos;
	int chy = path.front().y - u->ypos;
	return move_unit_action(u, chx, chy);
}

void goto_orders::drop_action()
{
	if(!path.empty())
		path.pop_front();
}

bool goto_orders::finished()
{
	return path.empty();
}

int goto_orders::path_length()
{
	return path.size();
}

bool goto_orders::replan()
{
	get_new_path();
	return !path.empty();
}

void goto_orders::clear()
{
	path.clear();
}

explore_orders::explore_orders(const civilization* civ_, unit* u_,
		bool autocontinue_)
	: goto_orders(civ_, u_, false, u_->xpos, u_->ypos),
	autocontinue(autocontinue_)
{
	get_new_path();
}

class explore_picker {
	private:
		const civilization* civ;
	public:
		explore_picker(const civilization* civ_) : civ(civ_) { }
		bool operator()(const coord& c) {
			for(int i = -1; i <= 1; i++) {
				for(int j = -1; j <= 1; j++) {
					if(i == 0 && j == 0)
						continue;
					if(c.x + i < 0 || c.x + i >= civ->m->size_x())
						continue;
					if(c.y + j < 0 || c.y + j >= civ->m->size_y())
						continue;
					if(civ->fog.get_value(c.x + i, c.y + j) == 0) {
						return true;
					}
				}
			}
			return false;
		}
};

void explore_orders::get_new_path()
{
	path.clear();
	explore_picker picker(civ);
	std::list<coord> fog_path = map_path_to_nearest(*civ, 
			*u, 
			false,
			coord(u->xpos, u->ypos), 
			picker);
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

wait_orders::wait_orders(unit* u_, unsigned int rounds)
	: u(u_),
	rounds_to_go(rounds),
	total_rounds(rounds)
{
}

action wait_orders::get_action()
{
	if(finished())
		return action(action_none);
	return unit_action(action_skip, u);
}

void wait_orders::drop_action()
{
	if(rounds_to_go > 0)
		rounds_to_go--;
}

bool wait_orders::finished()
{
	return rounds_to_go == 0;
}

bool wait_orders::replan()
{
	rounds_to_go = total_rounds;
	return true;
}

void wait_orders::clear()
{
	rounds_to_go = 0;
}

attack_orders::attack_orders(const civilization* civ_, unit* u_, int x_, int y_)
	: goto_orders(civ_, u_, true, x_, y_),
	att_x(-1),
	att_y(-1)
{
}

void attack_orders::check_for_enemies()
{
	att_x = -1;
	att_y = -1;
	for(int i = -1; i <= 1; i++) {
		for(int j = -1; j <= 1; j++) {
			if(i == 0 && j == 0)
				continue;
			int xp = u->xpos + i;
			int yp = u->ypos + j;
			if(xp < 0 || xp >= civ->m->size_x())
				continue;
			if(yp < 0 || yp >= civ->m->size_y())
				continue;
			int owner = civ->m->get_spot_resident(xp, yp);
			if(owner >= 0 && owner != (int)civ->civ_id &&
					civ->get_relationship_to_civ(owner) == relationship_war) {
				att_x = xp;
				att_y = yp;
				return;
			}
		}
	}
}

action attack_orders::get_action()
{
	check_for_enemies();
	if(att_x != -1 && att_y != -1) {
		return move_unit_action(u, att_x - u->xpos,
			att_y - u->ypos);
	}
	else {
		return goto_orders::get_action();
	}
}

void attack_orders::drop_action()
{
	if(att_x == -1 || att_y == -1)
		goto_orders::drop_action();
	else {
		att_x = -1;
		att_y = -1;
	}
}

bool attack_orders::finished()
{
	if(att_x == -1 || att_y == -1)
		return goto_orders::finished();
	else
		return false;
}

bool attack_orders::replan()
{
	return goto_orders::replan();
}

void attack_orders::clear()
{
	goto_orders::clear();
}

found_city_orders::found_city_orders(const civilization* civ_,
		unit* u_, const ai_tunables_found_city& found_city_, 
		int x_, int y_)
	: goto_orders(civ_, u_, false, x_, y_),
	found_city(found_city_)
{
	city_points = points_for_city_founding(civ, found_city,
			1, coord(tgtx, tgty));
}

action found_city_orders::get_action()
{
	if(path.empty()) {
		int new_city_points = points_for_city_founding(civ,
				found_city, 1, coord(tgtx, tgty));
		if(new_city_points < 1 || new_city_points < city_points) {
			replan();
			return action_none;
		}
		else {
			return unit_action(action_found_city, u);
		}
	}
	else {
		return goto_orders::get_action();
	}
}

void found_city_orders::drop_action()
{
	goto_orders::drop_action();
}

bool found_city_orders::finished()
{
	return false;
}

bool found_city_orders::replan()
{
	bool succ = find_best_city_pos(civ, found_city,
			u, &tgtx, &tgty, NULL);
	if(succ)
		city_points = points_for_city_founding(civ, found_city,
				0, coord(tgtx, tgty));
	return succ && city_points > 0;
}

void found_city_orders::clear()
{
	goto_orders::clear();
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
		return improve_unit_action(u, tgt_imp);
	}
	else {
		return goto_orders::get_action();
	}
}

void improve_orders::drop_action()
{
	if(u->is_improving())
		return;
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
	base_city = find_nearest_city(civ, u, true);
	if(!base_city)
		return false;
	for(int i = -2; i <= 2; i++) {
		for(int j = -2; j <= 2; j++) {
			int xp = base_city->xpos + i;
			int yp = base_city->ypos + j;
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
						tgtx = xp;
						tgty = yp;
						tgt_imp = improv_irrigation;
						return goto_orders::replan();
					}
					else if(civ->m->can_improve_terrain(xp,
								yp, civ->civ_id, improv_mine)) {
						tgtx = xp;
						tgty = yp;
						tgt_imp = improv_mine;
						return goto_orders::replan();
					}
				}
				if(civ->m->can_improve_terrain(xp,
							yp, civ->civ_id, improv_road)) {
					tgtx = xp;
					tgty = yp;
					tgt_imp = improv_road;
					return goto_orders::replan();
				}
			}
		}
	}
	return false;
}

void improve_orders::clear()
{
	tgt_imp = improv_none;
	goto_orders::clear();
}

ai_tunables_found_city::ai_tunables_found_city()
	: min_dist_to_city(3),
	min_dist_to_friendly_city(4),
	food_coeff(1),
	prod_coeff(1),
	comm_coeff(0),
	min_food_points(2),
	min_prod_points(2),
	min_comm_points(0),
	max_search_range(50),
	range_coeff(1),
	max_found_city_prio(1000),
	found_city_coeff(10)
{
}


