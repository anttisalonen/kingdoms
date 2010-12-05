#ifndef AI_H
#define AI_H

#include <utility>
#include <queue>
#include "civ.h"

class orders {
	public:
		virtual ~orders() { }
		virtual action get_action() = 0;
		virtual void drop_action() = 0;
		virtual bool finished() = 0;
		virtual bool replan() = 0;
		virtual void clear() = 0;
};

class primitive_orders : public orders {
	public:
		primitive_orders(const action& a_);
		action get_action();
		void drop_action();
		bool finished();
		bool replan();
		void clear();
	private:
		action a;
		bool finished_flag;
};

class orders_composite : public orders {
	public:
		action get_action();
		void drop_action();
		bool finished();
		void add_orders(orders* o);
		bool replan();
		void clear();
	private:
		std::list<orders*> ord;
};

class goto_orders : public orders {
	public:
		goto_orders(const civilization* civ_, unit* u_, 
				bool ignore_enemy_, int x_, int y_);
		virtual ~goto_orders() { }
		virtual action get_action();
		virtual void drop_action();
		virtual bool finished();
		virtual bool replan();
		void clear();
		int path_length();
	protected:
		int tgtx;
		int tgty;
		const civilization* civ;
		unit* u;
		std::list<coord> path;
		bool ignore_enemy;
	private:
		void get_new_path();
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

class wait_orders : public orders {
	public:
		wait_orders(unit* u_, unsigned int rounds);
		action get_action();
		void drop_action();
		bool finished();
		bool replan();
		void clear();
	private:
		unit* u;
		unsigned int rounds_to_go;
		unsigned int total_rounds;
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
};

struct ai_tunable_parameters {
	ai_tunable_parameters();
	int max_defense_prio;
	int defense_units_prio_coeff;
	int exploration_min_prio;
	int exploration_max_prio;
	int exploration_length_decr_coeff;
	int unit_prodcost_prio_coeff;
	int offense_dist_prio_coeff;
	int max_offense_prio;
	ai_tunables_found_city found_city;
};

class ai {
	typedef std::map<unit*, orders*> ordersmap_t;
	typedef std::map<city*, city_production*> cityordersmap_t;
	typedef std::pair<int, orders*> orderprio_t;
	typedef std::priority_queue<orderprio_t> ordersqueue_t;
	typedef std::priority_queue<std::pair<int, coord> > city_points_t;
	public:
		ai(map& m_, round& r_, civilization* c);
		bool play();
	private:
		city_production* create_city_orders(city* c);
		orderprio_t create_orders(unit* u);
		orderprio_t found_new_city(unit* u);
		orderprio_t military_unit_orders(unit* u);
		city* find_nearest_city(const unit* u, bool own) const;
		bool find_nearest_enemy(const unit* u, int* tgtx, int* tgty) const;
		void get_defense_prio(ordersqueue_t& pq, unit* u);
		void get_offense_prio(ordersqueue_t& pq, unit* u);
		void get_exploration_prio(ordersqueue_t& pq, unit* u);
		orderprio_t get_defense_orders(unit* u);
		void handle_new_advance(unsigned int adv_id);
		void handle_civ_discovery(int civ_id);
		void handle_new_unit(const msg& m);
		void handle_new_improv(const msg& m);
		map& m;
		round& r;
		civilization* myciv;
		ordersmap_t ordersmap;
		cityordersmap_t cityordersmap;
		ai_tunable_parameters param;
		city_points_t cityq;
};

bool find_best_city_pos(const civilization* myciv,
		const ai_tunables_found_city& found_city,
		const unit* u, int* tgtx, int* tgty, int* prio);

#endif

