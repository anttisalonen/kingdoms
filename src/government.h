#ifndef GOVERNMENT_H
#define GOVERNMENT_H

#include <map>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/string.hpp>

#define INITIAL_GOVERNMENT_INDEX	1
#define ANARCHY_INDEX			2

class government {
	public:
		government(unsigned int gov_id_,
			const std::string& gov_name_);
		government(); // for serialization
		unsigned int gov_id;
		std::string gov_name;
		unsigned int needed_advance;
		int production_cap;
		int city_units;
		int free_units;
		int unit_cost;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & gov_id;
			ar & gov_name;
			ar & needed_advance;
			ar & production_cap;
			ar & city_units;
			ar & free_units;
			ar & unit_cost;
		}
};

typedef std::map<unsigned int, government> government_map;

#endif

