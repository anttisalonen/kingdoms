#ifndef AI_EXPLORATION_H
#define AI_EXPLORATION_H

#include "round.h"
#include "ai-debug.h"
#include "ai-orders.h"
#include "ai-objective.h"

class exploration_objective : public objective {
	public:
		exploration_objective(round* r_, civilization* myciv_, const std::string& n);
		int get_unit_points(const unit& u) const;
		bool add_unit(unit* u);
	protected:
		bool compare_units(const unit_configuration& lhs,
				const unit_configuration& rhs) const;
		bool usable_unit(const unit_configuration& uc) const;
		int improvement_value(const city_improvement& ci) const;
	private:
		orders* create_exploration_orders(unit* u) const;
};

class explore_orders : public goto_orders {
	public:
		explore_orders(const civilization* civ_, unit* u_, 
				bool autocontinue_);
		void drop_action();
		bool replan();
		void clear();
	private:
		void get_new_path();
		bool autocontinue;
};

#endif

