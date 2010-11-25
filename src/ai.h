#ifndef AI_H
#define AI_H

#include "civ.h"

class ai {
	public:
		ai(map& m_, round& r_, civilization* c);
		bool process();
	private:
		map& m;
		round& r;
		civilization* myciv;
};

#endif

