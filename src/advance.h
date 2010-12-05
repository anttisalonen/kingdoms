#ifndef ADVANCE_H
#define ADVANCE_H

#include <string>
#include <map>

class advance {
	public:
		advance();
		unsigned int advance_id;
		std::string advance_name;
		int cost;
		unsigned int needed_advances[4];
};

typedef std::map<unsigned int, advance> advance_map;


#endif
