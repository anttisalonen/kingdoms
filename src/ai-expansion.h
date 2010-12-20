#ifndef AI_EXPANSION_H
#define AI_EXPANSION_H

#include "ai-orders.h"
#include "ai-objective.h"

struct ai_tunables_found_city {
	ai_tunables_found_city();
	int min_dist_to_city;
	int min_dist_to_friendly_city;
	int food_coeff;
	int prod_coeff;
	int comm_coeff;
	int min_food_points;
	int min_prod_points;
	int min_comm_points;
	int max_search_range;
	int range_coeff;
	int max_found_city_prio;
	int found_city_coeff;
};

class expansion_objective : public objective {
	public:
		expansion_objective(round* r_, civilization* myciv_, const std::string& n);
		int get_unit_points(const unit& u) const;
		bool add_unit(unit* u);
	protected:
		bool compare_units(const unit_configuration& lhs,
				const unit_configuration& rhs) const;
		bool usable_unit(const unit_configuration& uc) const;
		int improvement_value(const city_improvement& ci) const;
	private:
		ai_tunables_found_city found_city;
};

class found_city_orders : public goto_orders {
	public:
		found_city_orders(const civilization* civ_, unit* u_, 
				const ai_tunables_found_city& found_city_, 
				int x_, int y_);
		action get_action();
		void drop_action();
		bool finished();
		bool replan();
		void clear();
	private:
		const ai_tunables_found_city& found_city;
		int city_points;
		bool failed;
};

bool find_best_city_pos(const civilization* myciv,
		const ai_tunables_found_city& found_city,
		const unit* u, int* tgtx, int* tgty, int* prio);
city* find_nearest_city(const civilization* myciv, const unit* u, bool own);

#endif

