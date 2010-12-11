#ifndef UNIT_H
#define UNIT_H

#include "unit_configuration.h"
#include "resource_configuration.h"

class unit
{
	public:
		unit(int uid, int x, int y, int civid, const unit_configuration& uconf_);
		~unit();
		void new_round(improvement_type& i);
		bool is_settler() const;
		void fortify();
		void wake_up();
		bool is_fortified() const;
		bool fortified_or_fortifying() const;
		void skip_turn();
		int num_moves() const;
		void move_to(int x, int y);
		improvement_type improving_to() const;
		int turns_still_improving() const;
		void start_improving_to(improvement_type i, int turns);
		bool is_improving() const;
		bool idle() const;
		const int unit_id;
		const int civ_id;
		int xpos;
		int ypos;
		unsigned int moves;
		const unit_configuration& uconf;
		unsigned int strength;
		bool veteran;
	private:
		bool fortifying;
		bool fortified;
		bool resting;
		int turns_improving;
		improvement_type improving;
};

#endif
