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
};

class primitive_orders : public orders {
	public:
		primitive_orders(const action& a_);
		action get_action();
		void drop_action();
		bool finished();
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
	private:
		std::list<orders*> ord;
};

class goto_orders : public orders {
	public:
		goto_orders(const map& m_, const fog_of_war& fog_, unit* u_, int x_, int y_);
		virtual ~goto_orders() { }
		virtual action get_action();
		virtual void drop_action();
		virtual bool finished();
		int path_length();
	private:
		int tgtx;
		int tgty;
		const map& m;
		const fog_of_war& fog;
		unit* u;
		std::list<coord> path;
};

class explore_orders : public orders {
	public:
		explore_orders(const map& m_, const fog_of_war& fog_, unit* u_);
		action get_action();
		void drop_action();
		bool finished();
		int path_length();
	private:
		void get_new_path();
		const map& m;
		const fog_of_war& fog;
		unit* u;
		std::list<coord> path;
};

struct ai_tunable_parameters {
	ai_tunable_parameters();
	int def_wanted_units_in_city;
	int def_per_unit_prio;
	int exploration_min_prio;
	int exploration_max_prio;
	int exploration_length_decr_coeff;
	int found_city_base_prio;
	int unit_prodcost_prio_coeff;
};

class ai {
	typedef std::map<unit*, orders*> ordersmap_t;
	typedef std::map<city*, city_production*> cityordersmap_t;
	typedef std::pair<int, orders*> orderprio_t;
	typedef std::priority_queue<orderprio_t> ordersqueue_t;
	public:
		ai(map& m_, round& r_, civilization* c);
		bool play();
	private:
		city_production* create_city_orders(city* c);
		orderprio_t create_orders(unit* u);
		orderprio_t found_new_city(unit* u);
		orderprio_t military_unit_orders(unit* u);
		void find_best_city_pos(const unit* u, int* tgtx, int* tgty) const;
		city* find_nearest_own_city(const unit* u) const;
		void get_defense_prio(ordersqueue_t& pq, unit* u);
		void get_exploration_prio(ordersqueue_t& pq, unit* u);
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
};

#endif

