#include <fstream>
#include "serialize.h"

int save_game(const pompelmous& g)
{
	std::ofstream ofs("save.game");
	boost::archive::text_oarchive oa(ofs);
	oa << g;
	return 0;
}

pompelmous load_game()
{
	std::ifstream ifs("save.game");
	boost::archive::text_iarchive ia(ifs);
	pompelmous g;
	ia >> g;
	return g;
}

