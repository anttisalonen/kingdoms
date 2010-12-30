#ifndef POMPELMOUS_H
#define POMPELMOUS_H

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/function.hpp>

#include "unit_configuration.h"
#include "advance.h"
#include "city_improvement.h"
#include "civ.h"
#include "map.h"

enum action_type {
	action_give_up,
	action_eot,
	action_unit_action,
	action_city_action,
	action_none,
};

enum unit_action_type {
	action_move_unit,
	action_found_city,
	action_skip,
	action_fortify,
	action_improvement,
	action_load,
	action_unload,
};

struct action {
	action(action_type t);
	std::string to_string() const;
	action_type type;
	union {
		struct {
			unit_action_type uatype;
			unit* u;
			union {
				struct {
					int chx;
					int chy;
				} move_pos;
				improvement_type improv;
			} unit_action_data;

			template<class Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar & uatype;
				switch(uatype) {
					case action_move_unit:
						ar & unit_action_data.move_pos.chx;
						ar & unit_action_data.move_pos.chy;
						break;
					case action_improvement:
						ar & unit_action_data.improv;
						break;
					default:
						break;
				}
				ar & u;
			}
		} unit_data;
	} data;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & type;
		switch(type) {
			case action_unit_action:
				ar & data.unit_data;
				break;
			default:
				break;
		}
	}
};

action unit_action(unit_action_type t, unit* u);
action move_unit_action(unit* u, int chx, int chy);
action improve_unit_action(unit* u, improvement_type i);

enum combat_result {
	combat_result_none,
	combat_result_lost,
	combat_result_won,
};

struct visible_move_action {
	visible_move_action(const unit* un, int chx, int chy,
			combat_result res, const unit* opp);
	const unit* u;
	coord change;
	combat_result combat;
	const unit* opponent;
};

class action_listener {
	public:
		virtual void handle_action(const visible_move_action& a) = 0;
};

class pompelmous
{
	public:
		pompelmous(const unit_configuration_map& uconfmap_, 
				const advance_map& amap_, 
				const city_improv_map& cimap_,
				map* m_,
				unsigned int road_moves_,
				int num_turns_);
		pompelmous(); // for serialization
		void add_civilization(civilization* civ);
		bool perform_action(int civid, const action& a);
		const unit_configuration* get_unit_configuration(int uid) const;
		int current_civ_id() const;
		void declare_war_between(unsigned int civ1, unsigned int civ2);
		void peace_between(unsigned int civ1, unsigned int civ2);
		std::vector<civilization*> civs;
		const unit_configuration_map uconfmap;
		const advance_map amap;
		const city_improv_map cimap;
		bool in_war(unsigned int civ1, unsigned int civ2) const;
		int get_round_number() const;
		unsigned int get_num_road_moves() const;
		int get_num_turns() const;
		const map& get_map() const;
		map& get_map();
		void add_action_listener(action_listener* cb);
		void remove_action_listener(action_listener* cb);
	private:
		void broadcast_action(const visible_move_action& a) const;
		bool next_civ();
		void refill_moves();
		void increment_resources();
		bool try_move_unit(unit* u, int chx, int chy);
		void check_city_conquer(int tgtxpos, int tgtypos, int conquering_civid);
		void check_civ_elimination(int civ_id);
		int needed_food_for_growth(int city_size) const;
		int needed_culture_for_growth(int city_culture) const;
		void check_for_city_updates();
		void update_land_owners();
		void destroy_improvements(city* c);
		bool can_load_unit(unit* u, int x, int y) const;
		void load_unit(unit* u, int x, int y);
		bool try_unload_units(unit* u, int x, int y);
		bool try_wakeup_loaded(unit* u);
		void update_civ_points();
		int get_offense_bonus(const unit* off, const unit* def) const;
		int get_defense_bonus(const unit* def, const unit* off) const;
		std::vector<civilization*>::iterator current_civ;
		map* m;
		int round_number;
		const unsigned int road_moves;
		int num_turns;
		std::list<action_listener*> action_listeners;

		friend class boost::serialization::access;

		template<class Archive>
		void archive_helper(Archive& ar, const unsigned int version)
		{
			ar & const_cast<unit_configuration_map&>(uconfmap);
			ar & const_cast<advance_map&>(amap);
			ar & const_cast<city_improv_map&>(cimap);
			ar & m;
			ar & civs;

			ar & round_number;
			ar & const_cast<unsigned int&>(road_moves);
			ar & num_turns;
		}
		template<class Archive>
		void save(Archive& ar, const unsigned int version) const
		{
			const_cast<pompelmous*>(this)->archive_helper(ar, version);
			int curr_civ;
			if(current_civ != civs.end())
				curr_civ = static_cast<int>((*current_civ)->civ_id);
			else
				curr_civ = -1;
			ar & curr_civ;
		}
		template<class Archive>
		void load(Archive& ar, const unsigned int version)
		{
			archive_helper(ar, version);
			int curr_civ;
			ar & curr_civ;
			current_civ = civs.end();
			if(curr_civ >= 0) {
				for(current_civ = civs.begin(); 
						current_civ != civs.end(); 
						++current_civ) {
					if((*current_civ)->civ_id == (unsigned int)curr_civ)
						break;
				}
			}
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER();
};

#endif

