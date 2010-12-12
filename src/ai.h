#ifndef AI_H
#define AI_H

#include <utility>
#include <queue>
#include "round.h"

#include "ai-orders.h"

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
};

class ai {
	typedef std::map<unit*, orders*> ordersmap_t;
	typedef std::map<city*, city_production*> cityordersmap_t;
	typedef std::pair<int, orders*> orderprio_t;
	typedef std::priority_queue<orderprio_t> ordersqueue_t;
	typedef std::priority_queue<std::pair<int, coord> > city_points_t;
	public:
		ai(map& m_, round& r_, civilization* c, bool debug_);
		bool play();
	private:
		city_production* create_city_orders(city* c);
		orderprio_t create_orders(unit* u);
		orderprio_t found_new_city(unit* u);
		orderprio_t military_unit_orders(unit* u);
		orderprio_t workers_orders(unit* u);
		bool find_nearest_enemy(const unit* u, int* tgtx, int* tgty) const;
		void get_defense_prio(ordersqueue_t& pq, unit* u);
		void get_offense_prio(ordersqueue_t& pq, unit* u);
		void get_exploration_prio(ordersqueue_t& pq, unit* u);
		orderprio_t get_defense_orders(unit* u);
		void handle_new_advance(unsigned int adv_id);
		void handle_civ_discovery(int civ_id);
		void handle_new_unit(const msg& m);
		void handle_new_improv(const msg& m);
		void handle_unit_disbanded(const msg& m);
		map& m;
		round& r;
		civilization* myciv;
		ordersmap_t ordersmap;
		cityordersmap_t cityordersmap;
		ai_tunable_parameters param;
		city_points_t cityq;
		bool debug;
};

#endif

