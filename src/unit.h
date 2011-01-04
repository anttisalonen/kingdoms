#ifndef UNIT_H
#define UNIT_H

#include <list>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/list.hpp>

#include "unit_configuration.h"
#include "resource_configuration.h"

class unit
{
	public:
		unit(int uid, int uconfid, int x, int y, int civid, 
				const unit_configuration& uconf_, 
				unsigned int def_road_moves_);
		unit(); // for serialization
		~unit();
		void new_round(improvement_type& i);
		bool is_settler() const;
		void fortify();
		void wake_up();
		bool is_fortified() const;
		bool fortified_or_fortifying() const;
		void skip_turn();
		int num_moves() const;
		int num_road_moves() const;
		void move_to(int x, int y, bool road);
		void decrement_moves();
		improvement_type improving_to() const;
		int turns_still_improving() const;
		void start_improving_to(improvement_type i, int turns);
		bool is_improving() const;
		bool idle() const;
		bool is_military_unit() const;
		bool can_load_at(unit* loader) const;
		void load_at(unit* loader);
		void unload();
		bool carried() const;
		bool carrying() const;
		bool is_land_unit() const;
		const int unit_id;
		const int uconf_id;
		const int civ_id;
		int xpos;
		int ypos;
		const unit_configuration* uconf;
		unsigned int strength;
		bool veteran;
		std::list<unit*> carried_units;
	private:
		bool fortifying;
		bool fortified;
		bool resting;
		bool sentry;
		unit* carrying_unit;
		int turns_improving;
		improvement_type improving;
		unsigned int moves;
		unsigned int road_moves;
		const unsigned int def_road_moves;

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & const_cast<int&>(unit_id);
			ar & const_cast<int&>(uconf_id);
			ar & const_cast<int&>(civ_id);
			ar & xpos;
			ar & ypos;
			ar & const_cast<unit_configuration*&>(uconf);
			ar & strength;
			ar & veteran;
			ar & carried_units;
			ar & fortifying;
			ar & fortified;
			ar & resting;
			ar & sentry;
			ar & carrying_unit;
			ar & turns_improving;
			ar & improving;
			ar & moves;
			ar & road_moves;
			ar & const_cast<unsigned int&>(def_road_moves);
		}
};

#endif
