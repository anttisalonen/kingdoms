#include "ai.h"

ai::ai(map& m_, round& r_, civilization* c)
	: m(m_),
	r(r_),
	myciv(c)
{
}

bool ai::process()
{
	int success = r.perform_action(myciv->civ_id, action(action_eot), &m);
	return !success;
}

