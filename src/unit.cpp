#include <stdlib.h>
#include "unit.h"

unit::unit(int uid, int x, int y, int civid, 
		const unit_configuration& uconf_,
		unsigned int def_road_moves_)
	: unit_id(uid),
	civ_id(civid),
	xpos(x),
	ypos(y),
	uconf(uconf_),
	strength(10 * uconf_.max_strength),
	veteran(false),
	fortifying(false),
	fortified(false),
	resting(false),
	turns_improving(0),
	improving(improv_none),
	moves(0),
	road_moves(0),
	def_road_moves(def_road_moves_)
{
}

unit::~unit()
{
}

void unit::new_round(improvement_type& i)
{
	i = improv_none;
	if(is_improving()) {
		turns_improving--;
		if(turns_improving == 0) {
			i = improving;
			improving = improv_none;
		}
	}
	moves = uconf.max_moves;
	road_moves = 0;
	if(fortifying) {
		fortifying = false;
		fortified = true;
	}
	if(resting || fortified) {
		int strdiff = 10 * uconf.max_strength - strength;
		if(strdiff > (int)uconf.max_strength)
			strength += strdiff / 2;
		else if(strdiff > 0)
			strength = 10 * uconf.max_strength;
	}
	resting = false;
}

bool unit::is_settler() const
{
	return uconf.settler;
}

void unit::fortify()
{
	if(!fortifying && !is_improving())
		fortifying = true;
}

void unit::wake_up()
{
	fortified = fortifying = false;
	turns_improving = 0;
	improving = improv_none;
}

bool unit::is_fortified() const
{
	return fortified;
}

bool unit::fortified_or_fortifying() const
{
	return fortified || fortifying;
}

void unit::skip_turn()
{
	moves = 0;
	road_moves = 0;
	resting = true;
	turns_improving = 0;
	improving = improv_none;
}

int unit::num_moves() const
{
	return moves;
}

int unit::num_road_moves() const
{
	return road_moves;
}

bool unit::move_to(int x, int y, bool road)
{
	turns_improving = 0;
	improving = improv_none;
	if(moves < 1 && road_moves < 1)
		return false;
	xpos = x;
	ypos = y;
	if(!road) {
		if(moves) {
			moves--;
		}
		else {
			road_moves = 0;
		}
	}
	else {
		if(road_moves) {
			road_moves--;
		}
		else {
			moves--;
			road_moves = def_road_moves - 1;
		}
	}
	return true;
}

void unit::decrement_moves()
{
	if(moves)
		moves--;
}

improvement_type unit::improving_to() const
{
	return improving;
}

int unit::turns_still_improving() const
{
	return turns_improving;
}

void unit::start_improving_to(improvement_type i, int turns)
{
	if(i != improving) {
		improving = i;
		turns_improving = turns;
	}
}

bool unit::is_improving() const
{
	return turns_improving > 0 && improving != improv_none;
}

bool unit::idle() const
{
	return (moves > 0 || road_moves > 0) && !fortified_or_fortifying() && !is_improving();
}

bool unit::is_military_unit() const
{
	return uconf.max_strength > 0;
}


