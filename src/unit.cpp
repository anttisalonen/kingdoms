#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include "unit.h"

unit::unit(int uid, int uconfid, int x, int y, int civid, 
		const unit_configuration& uconf_,
		unsigned int def_road_moves_)
	: unit_id(uid),
	uconf_id(uconfid),
	civ_id(civid),
	xpos(x),
	ypos(y),
	uconf(uconf_),
	strength(10 * uconf_.max_strength),
	veteran(false),
	fortifying(false),
	fortified(false),
	resting(false),
	sentry(false),
	carrying_unit(NULL),
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
	if(fortifying) {
		fortifying = false;
		fortified = true;
	}
	if(resting || fortified || (moves == uconf.max_moves)) {
		int strdiff = 10 * uconf.max_strength - strength;
		if(strdiff > (int)uconf.max_strength)
			strength += strdiff / 2;
		else if(strdiff > 0)
			strength = 10 * uconf.max_strength;
	}
	moves = uconf.max_moves;
	road_moves = 0;
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
	sentry = false;
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
	if(carrying_unit)
		sentry = true;
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
	return (moves > 0 || road_moves > 0) && 
		!fortified_or_fortifying() && 
		!is_improving() &&
		!sentry;
}

bool unit::is_military_unit() const
{
	return uconf.max_strength > 0;
}

bool unit::load_at(unit* loader)
{
	if(carrying_unit || !idle())
		return false;
	loader->carried_units.push_back(this);
	xpos = loader->xpos;
	ypos = loader->ypos;
	decrement_moves();
	sentry = true;
	carrying_unit = loader;
	return true;
}

bool unit::unload(int x, int y)
{
	if(!carrying_unit)
		return false;
	for(std::list<unit*>::iterator it = carrying_unit->carried_units.begin();
			it != carrying_unit->carried_units.end();
			++it) {
		if(*it == this) {
			it = carrying_unit->carried_units.erase(it);
		}
	}
	xpos = x;
	ypos = y;
	decrement_moves();
	wake_up();
	carrying_unit = NULL;
	return true;
}

bool unit::carried() const
{
	return carrying_unit != NULL;
}

bool unit::carrying() const
{
	return carried_units.size() > 0;
}
bool unit::is_land_unit() const
{
	return uconf.is_land_unit();
}


