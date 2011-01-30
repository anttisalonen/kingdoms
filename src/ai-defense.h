#ifndef AI_DEFENSE_H
#define AI_DEFENSE_H

#include "ai-orders.h"
#include "ai-objective.h"
#include "pompelmous.h"

class defense_objective : public objective {
	public:
		defense_objective(pompelmous* r_, civilization* myciv_, const std::string& n);
		virtual ~defense_objective() {}
		virtual int get_unit_points(const unit& u) const;
		virtual int improvement_value(const city_improvement& ci) const;
		virtual bool add_unit(unit* u);
	protected:
		virtual bool compare_units(const unit_configuration& lhs,
				const unit_configuration& rhs) const;
		virtual bool usable_unit(const unit_configuration& uc) const;
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
bool usable_defense_unit(const unit_configuration& uc);

#endif
