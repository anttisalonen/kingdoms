#ifndef AI_COMMERCE_H
#define AI_COMMERCE_H

#include "round.h"
#include "ai-orders.h"
#include "ai-objective.h"

class commerce_objective : public objective {
	public:
		commerce_objective(round* r_, civilization* myciv_, const std::string& n);
		int get_unit_points(const unit& u) const;
		city_production get_city_production(const city& c, int* points) const;
		bool add_unit(unit* u);
	private:
		int worker_prio;
};

class improve_orders : public goto_orders {
	public:
		improve_orders(const civilization* civ_, unit* u_);
		action get_action();
		void drop_action();
		bool finished();
		bool replan();
		void clear();
	private:
		city* base_city;
		improvement_type tgt_imp;
};

#endif

