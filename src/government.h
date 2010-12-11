#ifndef GOVERNMENT_H
#define GOVERNMENT_H

#include <string>
#include <map>

class government {
	public:
		government(unsigned int gov_id_,
			const std::string& gov_name_);
		unsigned int gov_id;
		std::string gov_name;
		int needed_advance;
		int free_units;
		int unit_cost;
};

typedef std::map<unsigned int, government> government_map;

#endif

