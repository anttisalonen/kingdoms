#ifndef CITY_IMPROVEMENT_H
#define CITY_IMPROVEMENT_H

#include <map>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/string.hpp>

class city_improvement {
	public:
		city_improvement();
		unsigned int improv_id;
		std::string improv_name;
		int cost;
		bool barracks;
		bool granary;
		bool palace;
		int comm_bonus;
		int culture;
		int happiness;
		unsigned int needed_advance;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & improv_id;
			ar & improv_name;
			ar & cost;
			ar & barracks;
			ar & granary;
			ar & palace;
			ar & comm_bonus;
			ar & culture;
			ar & happiness;
			ar & needed_advance;
		}
};

typedef std::map<unsigned int, city_improvement> city_improv_map;

#endif
