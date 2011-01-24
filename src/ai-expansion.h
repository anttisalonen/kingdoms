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

typedef std::map<unsigned int, coord> city_plan_map_t;

class expansion_objective : public objective {
	public:
		expansion_objective(pompelmous* r_, civilization* myciv_, const std::string& n);
		int get_unit_points(const unit& u) const;
		bool add_unit(unit* u);
		city_production get_city_production(const city& c, int* points) const;
		void process(std::set<unsigned int>* freed_units);
		virtual void forget_everything();
	protected:
		bool compare_units(const unit_configuration& lhs,
				const unit_configuration& rhs) const;
		bool usable_unit(const unit_configuration& uc) const;
		int improvement_value(const city_improvement& ci) const;
	private:
		ai_tunables_found_city found_city;
		city_plan_map_t planned_cities;
		std::map<coord, unsigned int> escorters;
		std::map<coord, std::list<std::pair<coord, unsigned int> > > transportees;
		mutable bool need_settler;
		mutable bool need_transporter;
};

#endif

