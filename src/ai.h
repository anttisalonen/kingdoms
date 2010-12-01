#ifndef AI_H
#define AI_H

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
	private:
		int tgtx;
		int tgty;
		const map& m;
		const fog_of_war& fog;
		unit* u;
		std::list<coord> path;
};

class ai {
	typedef std::map<unit*, orders*> ordersmap_t;
	public:
		ai(map& m_, round& r_, civilization* c);
		bool play();
	private:
		orders* create_orders(unit* u);
		orders* found_new_city(unit* u);
		void find_best_city_pos(const unit* u, int* tgtx, int* tgty) const;
		void find_nearest_own_city(const unit* u, int* tgtx, int* tgty) const;
		orders* defend_nearest_city(unit* u);
		map& m;
		round& r;
		civilization* myciv;
		ordersmap_t ordersmap;
};

#endif

