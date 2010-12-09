#include <stdlib.h>
#include "unit.h"

unit::unit(int uid, int x, int y, int civid, const unit_configuration& uconf_)
	: unit_id(uid),
	civ_id(civid),
	xpos(x),
	ypos(y),
	moves(0),
	uconf(uconf_),
	strength(10 * uconf_.max_strength),
	veteran(false),
	fortifying(false),
	fortified(false),
	resting(false)
{
}

unit::~unit()
{
}

void unit::new_round()
{
	moves = uconf.max_moves;
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
	if(!fortifying)
		fortifying = true;
}

void unit::wake_up()
{
	fortified = fortifying = false;
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
	resting = true;
}

int unit::num_moves() const
{
	return moves;
}

void unit::move_to(int x, int y)
{
	if(moves < 1)
		return;
	if(abs(xpos - x) > 1 || abs(ypos - y) > 1)
		return;
	xpos = x;
	ypos = y;
	moves--;
	return;
}

