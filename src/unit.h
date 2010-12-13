#ifndef UNIT_H
#define UNIT_H

#include <vector>
#include "unit_configuration.h"
#include "resource_configuration.h"

class unit
{
	public:
		unit(int uid, int x, int y, int civid, 
				const unit_configuration& uconf_, 
				unsigned int def_road_moves_);
		~unit();
		void new_round(improvement_type& i);
		bool is_settler() const;
		void fortify();
		void wake_up();
		bool is_fortified() const;
		bool fortified_or_fortifying() const;
		void skip_turn();
		int num_moves() const;
		int num_road_moves() const;
		bool move_to(int x, int y, bool road);
		void decrement_moves();
		improvement_type improving_to() const;
		int turns_still_improving() const;
		void start_improving_to(improvement_type i, int turns);
		bool is_improving() const;
		bool idle() const;
		bool is_military_unit() const;
		bool load_at(unit* loader);
		bool unload(int x, int y);
		bool carried() const;
		bool carrying() const;
		const int unit_id;
		const int civ_id;
		int xpos;
		int ypos;
		const unit_configuration& uconf;
		unsigned int strength;
		bool veteran;
		std::vector<unit*> carried_units;
	private:
		bool fortifying;
		bool fortified;
		bool resting;
		bool sentry;
		unit* carrying_unit;
		int turns_improving;
		improvement_type improving;
		unsigned int moves;
		unsigned int road_moves;
		const unsigned int def_road_moves;
};

#endif
