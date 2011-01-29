#ifndef AI_OBJECTIVE_H
#define AI_OBJECTIVE_H

#include <string>

#include "pompelmous.h"
#include "ai-orders.h"

typedef std::map<unsigned int, orders*> ordersmap_t;

typedef bool(*unit_comp_func_t)(const unit_configuration& lhs, 
		const unit_configuration& rhs);

typedef bool(*unit_accept_func_t)(const unit_configuration& uc);

class objective {
	public:
		objective(pompelmous* r_, civilization* myciv_, const std::string& obj_name_);
		virtual ~objective();
		virtual int get_unit_points(const unit& u) const = 0;
		virtual city_production get_city_production(const city& c, int* points) const;
		virtual bool add_unit(unit* u) = 0;
		virtual void process(std::set<unsigned int>* freed_units);
		const std::string& get_name() const;
		virtual void forget_everything();
	protected:
		virtual bool compare_units(const unit_configuration& lhs,
				const unit_configuration& rhs) const = 0;
		virtual bool usable_unit(const unit_configuration& uc) const = 0;
		virtual int improvement_value(const city_improvement& ci) const = 0;
		std::list<unsigned int> used_units;
		std::list<objective*> missions;
		pompelmous* r;
		civilization* myciv;
		ordersmap_t ordersmap;
		std::string obj_name;
	private:
		city_production best_unit_production(const city& c,
				int* points) const;
		city_production best_improv_production(const city& c,
				int* points) const;
		unit_configuration_map::const_iterator choose_best_unit(const pompelmous& r, 
				const civilization& myciv, const city& c) const;
};


#endif

