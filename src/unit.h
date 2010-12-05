#ifndef UNIT_H
#define UNIT_H

#include "unit_configuration.h"

class unit
{
	public:
		unit(int uid, int x, int y, int civid, const unit_configuration& uconf_);
		~unit();
		void refill_moves(unsigned int m);
		bool is_settler() const;
		const int unit_id;
		const int civ_id;
		int xpos;
		int ypos;
		unsigned int moves;
		bool fortified;
		const unit_configuration& uconf;
		unsigned int strength;
		bool veteran;
};

#endif
