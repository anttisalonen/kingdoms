#ifndef AI_OFFENSE_H
#define AI_OFFENSE_H

#include "ai-orders.h"
#include "ai-defense.h"
#include "pompelmous.h"

class offense_objective : public defense_objective {
	public:
		offense_objective(pompelmous* r_, civilization* myciv_, const std::string& n);
		int get_unit_points(const unit& u) const;
		bool add_unit(unit* u);
	private:
		int max_offense_prio;
		int unit_strength_prio_coeff;
		int offense_dist_prio_coeff;
};

class attack_orders : public goto_orders {
	public:
		attack_orders(const civilization* civ_, unit* u_, int x_, int y_);
		action get_action();
		void drop_action();
		bool finished();
		bool replan();
		void clear();
	private:
		void check_for_enemies();
		int att_x;
		int att_y;
};

#endif

