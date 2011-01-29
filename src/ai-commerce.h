#ifndef AI_COMMERCE_H
#define AI_COMMERCE_H

#include "pompelmous.h"
#include "ai-orders.h"
#include "ai-objective.h"

class commerce_objective : public objective {
	public:
		commerce_objective(pompelmous* r_, civilization* myciv_, const std::string& n);
		~commerce_objective() { }
		int get_unit_points(const unit& u) const;
		bool add_unit(unit* u);
	protected:
		bool compare_units(const unit_configuration& lhs,
				const unit_configuration& rhs) const;
		bool usable_unit(const unit_configuration& uc) const;
		int improvement_value(const city_improvement& ci) const;
	private:
		int worker_prio;
};

class improve_orders : public goto_orders {
	public:
		improve_orders(const civilization* civ_, unit* u_);
		~improve_orders() { }
		virtual action get_action();
		void drop_action();
		virtual bool finished();
		virtual bool replan() = 0;
		void clear();
	protected:
		improvement_type tgt_imp;
};

class improve_city_orders : public improve_orders {
	public:
		improve_city_orders(const civilization* civ_, unit* u_);
		bool replan();
	private:
		city* base_city;
};

class build_road_orders : public improve_orders {
	public:
		build_road_orders(const civilization* civ_, unit* u_);
		~build_road_orders() { }
		bool replan();
	protected:
		std::list<coord> road_path;
};

class connect_cities_orders : public build_road_orders {
	public:
		connect_cities_orders(const civilization* civ_, unit* u_);
	protected:
		void setup_road_path();
};

class connect_resources_orders : public build_road_orders {
	public:
		connect_resources_orders(const civilization* civ_, unit* u_);
	protected:
		void setup_road_path();
};

#endif

