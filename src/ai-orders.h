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
		coord get_target_position();
	protected:
		void get_new_path();
		int tgtx;
		int tgty;
		const civilization* civ;
		unit* u;
		std::list<coord> path;
		bool ignore_enemy;
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

city* find_nearest_city(const civilization* myciv, const unit& u, bool own);

#endif

