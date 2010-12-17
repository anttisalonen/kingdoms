#ifndef AI_ORDERS_H
#define AI_ORDERS_H

#include <list>

#include "round.h"

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
		void get_new_path();
		int tgtx;
		int tgty;
		const civilization* civ;
		unit* u;
		std::list<coord> path;
		bool ignore_enemy;
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
		bool failed;
};

bool find_best_city_pos(const civilization* myciv,
		const ai_tunables_found_city& found_city,
		const unit* u, int* tgtx, int* tgty, int* prio);
city* find_nearest_city(const civilization* myciv, const unit* u, bool own);

#endif

