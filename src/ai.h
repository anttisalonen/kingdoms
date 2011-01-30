#ifndef AI_H
#define AI_H

#include <utility>
#include <queue>
#include "pompelmous.h"
#include "utils.h"
#include "diplomat.h"

#include "ai-objective.h"
#include "ai-orders.h"
#include "ai-exploration.h"
#include "ai-expansion.h"
#include "ai-defense.h"
#include "ai-offense.h"
#include "ai-commerce.h"

struct ai_tunable_parameters {
	ai_tunable_parameters();
	int max_defense_prio;
	int defense_units_prio_coeff;
	int exploration_min_prio;
	int exploration_max_prio;
	int exploration_length_decr_coeff;
	int unit_prodcost_prio_coeff;
	int offense_dist_prio_coeff;
	int unit_strength_prio_coeff;
	int max_offense_prio;
	ai_tunables_found_city found_city;
	int worker_prio;
	int ci_barracks_value;
	int ci_granary_value;
	int ci_comm_bonus_coeff;
	int ci_culture_coeff;
	int ci_happiness_coeff;
	int ci_cost_coeff;
};

class ai : public diplomat {
	public:
		ai(map& m_, pompelmous& r_, civilization* c);
		~ai();
		bool play();
		bool peace_suggested(int civ_id);
	private:
		void create_city_orders(city* c);
		bool assign_free_unit(unit* u);
		void handle_new_advance(unsigned int adv_id);
		void handle_civ_discovery(int civ_id);
		void handle_new_unit(const msg& m);
		void handle_new_improv(const msg& m);
		void handle_unit_disbanded(const msg& m);
		void handle_anarchy_over(const msg& m);
		void check_for_revolution(unsigned int adv_id);
		bool want_peace() const;
		void forget_all_unit_plans();
		void setup_research_goal();
		int get_research_goal_points(const advance& a, int levels) const;
		int get_research_goal_points_worker(const advance& a, int levels, int total_levels) const;
		int get_government_value(const government& gov) const;
		std::set<unsigned int> free_units;
		std::list<std::pair<objective*, int> > objectives;
		std::set<unsigned int> handled_units;
		std::map<unsigned int, objective*> building_cities;
		map& m;
		pompelmous& r;
		civilization* myciv;
		unsigned int planned_new_government_form;
};

#endif

