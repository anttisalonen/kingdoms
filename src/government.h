#ifndef GOVERNMENT_H
#define GOVERNMENT_H

#include <map>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/string.hpp>

class government {
	public:
		government(unsigned int gov_id_,
			const std::string& gov_name_);
		government(); // for serialization
		unsigned int gov_id;
		std::string gov_name;
		int needed_advance;
		int free_units;
		int unit_cost;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & gov_id;
			ar & gov_name;
			ar & needed_advance;
			ar & free_units;
			ar & unit_cost;
		}
};

typedef std::map<unsigned int, government> government_map;

#endif

