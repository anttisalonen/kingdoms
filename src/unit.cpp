#include "unit.h"

unit::unit(int uid, int x, int y, int civid, const unit_configuration& uconf_)
	: unit_id(uid),
	civ_id(civid),
	xpos(x),
	ypos(y),
	moves(0),
	fortified(false),
	uconf(uconf_),
	strength(10 * uconf_.max_strength),
	veteran(false)
{
}

unit::~unit()
{
}

void unit::refill_moves(unsigned int m)
{
	moves = m;
}

bool unit::is_settler() const
{
	return uconf.settler;
}


