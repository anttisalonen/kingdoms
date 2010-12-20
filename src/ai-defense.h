#ifndef AI_DEFENSE_H
#define AI_DEFENSE_H

#include "ai-orders.h"
#include "ai-objective.h"
#include "round.h"

class defense_objective : public objective {
	public:
		defense_objective(round* r_, civilization* myciv_, const std::string& n);
		int get_unit_points(const unit& u) const;
		city_production get_city_production(const city& c, int* points) const;
		bool add_unit(unit* u);
	private:
		int unit_strength_prio_coeff;
		int defense_units_prio_coeff;
};

class defend_orders : public goto_orders {
	public:
		defend_orders(const civilization* civ_, unit* u_, 
				int x_, int y_, int waittime_);
		action get_action();
		void drop_action();
		bool finished();
		bool replan();
		void clear();
	private:
		int waittime;
		int max_waittime;
};

bool compare_defense_units(const unit_configuration& lhs,
		const unit_configuration& rhs);
bool acceptable_defense_unit(const unit_configuration& uc);

#endif
