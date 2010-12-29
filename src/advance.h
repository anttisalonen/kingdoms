#ifndef ADVANCE_H
#define ADVANCE_H

#include <map>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/string.hpp>

const int max_num_needed_advances = 4;

class advance {
	public:
		advance();
		unsigned int advance_id;
		std::string advance_name;
		int cost;
		unsigned int needed_advances[max_num_needed_advances];

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & advance_id;
			ar & advance_name;
			ar & cost;
			ar & needed_advances;
		}
};

typedef std::map<unsigned int, advance> advance_map;


#endif
