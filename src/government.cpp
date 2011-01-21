#include "government.h"

government::government(unsigned int gov_id_,
		const std::string& gov_name_)
	: gov_id(gov_id_),
	gov_name(gov_name_),
	needed_advance(0),
	production_cap(0),
	free_units(0),
	unit_cost(1)
{
}

government::government()
{
}

