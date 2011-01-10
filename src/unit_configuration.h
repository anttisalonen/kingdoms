#ifndef UNIT_CONFIGURATION_H
#define UNIT_CONFIGURATION_H

#include <map>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/string.hpp>

const unsigned int max_num_unit_bonuses = 4;

enum unit_bonus_type {
	unit_bonus_group,
	unit_bonus_city, // attack only
	unit_bonus_none
};

struct unit_bonus {
	unit_bonus_type type;
	union {
		int group_mask;
	} bonus_data;
	int bonus_amount;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & type;
		switch(type) {
			case unit_bonus_group:
				ar & bonus_data.group_mask;
				break;
			case unit_bonus_city:
			case unit_bonus_none:
			default:
				break;
		}
		ar & bonus_amount;
	}
};

class unit_configuration {
	public:
		std::string unit_name;
		unsigned int max_moves;
		bool settler;
		bool worker;
		unsigned int production_cost;
		unsigned int max_strength;
		unsigned int needed_advance;
		bool sea_unit;
		bool ocean_unit;
		unsigned int carry_units;
		int unit_group_mask;
		unit_bonus unit_bonuses[max_num_unit_bonuses];
		bool is_land_unit() const;
		bool is_water_unit() const;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & unit_name;
			ar & max_moves;
			ar & settler;
			ar & worker;
			ar & production_cost;
			ar & max_strength;
			ar & needed_advance;
			ar & sea_unit;
			ar & ocean_unit;
			ar & carry_units;
			ar & unit_bonuses;
		}
};

inline bool unit_configuration::is_land_unit() const
{
	return !sea_unit && !ocean_unit;
}

inline bool unit_configuration::is_water_unit() const
{
	return sea_unit || ocean_unit;
}

typedef std::map<int, unit_configuration> unit_configuration_map;

#endif
