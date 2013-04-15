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
#include "diplomat.h"

#define SETTLER_UNIT_CONFIGURATION_ID	0
#define WARRIOR_UNIT_CONFIGURATION_ID	2

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
	visible_move_action(const unit* un, village_type v);
	const unit* u;
	coord change;
	combat_result combat;
	const unit* opponent;
	village_type village;
};

class action_listener {
	public:
		virtual void handle_action(const visible_move_action& a) = 0;
};

enum victory_type {
	victory_none,
	victory_score,
	victory_elimination,
	victory_domination
};

class pompelmous
{
	public:
		pompelmous(const unit_configuration_map& uconfmap_, 
				const advance_map& amap_, 
				const city_improv_map& cimap_,
				const government_map& govmap_,
				map* m_,
				unsigned int road_moves_,
				unsigned int food_eaten_per_citizen_,
				unsigned int anarchy_period_turns_,
				int num_turns_);
		pompelmous(); // for serialization
		void add_civilization(civilization* civ);
		void add_village(const coord& c);
		void add_diplomat(int civid, diplomat* d);
		bool perform_action(int civid, const action& a);
		const unit_configuration* get_unit_configuration(int uid) const;
		int current_civ_id() const;
		void declare_war_between(unsigned int civ1, unsigned int civ2);
		void peace_between(unsigned int civ1, unsigned int civ2);
		std::vector<civilization*> civs;
		const unit_configuration_map uconfmap;
		const advance_map amap;
		const city_improv_map cimap;
		const government_map govmap;
		bool in_war(unsigned int civ1, unsigned int civ2) const;
		int get_round_number() const;
		unsigned int get_num_road_moves() const;
		int get_num_turns() const;
		const map& get_map() const;
		map& get_map();
		void add_action_listener(action_listener* cb);
		void remove_action_listener(action_listener* cb);
		unsigned int get_city_growth_turns(const city* c) const;
		unsigned int get_city_production_turns(const city* c,
				const city_production& cp) const;
		unsigned int get_city_production_turns(const city* c,
				const unit_configuration& uc) const;
		unsigned int get_city_production_turns(const city* c,
				const city_improvement& ci) const;
		int needed_food_for_growth(int city_size) const;
		int needed_culture_for_growth(int city_culture) const;
		const unsigned int get_food_eaten_per_citizen() const;
		void combat(unit* u1, unit* u2);
		bool combat_chances(const unit* u1, const unit* u2,
				unsigned int* u1chance,
				unsigned int* u2chance) const;
		int get_winning_civ() const;
		victory_type get_victory_type() const;
		bool finished() const;
		bool can_load_unit(unit* u, int x, int y) const;
		void start_revolution(civilization* civ);
		void set_government(civilization* civ, int gov_id);
		bool suggest_peace(int civ_id1, int civ_id2);

	private:
		void broadcast_action(const visible_move_action& a) const;
		bool next_civ();
		void refill_moves();
		void increment_resources();
		bool try_move_unit(unit* u, int chx, int chy);
		void check_city_conquer(int tgtxpos, int tgtypos, int conquering_civid);
		void check_civ_elimination(int civ_id);
		void check_for_city_updates();
		void update_land_owners();
		void destroy_improvements(city* c);
		void load_unit(unit* u, int x, int y);
		bool can_unload_unit(unit* u, int chx, int chy) const;
		void unload_unit(unit* u, int chx, int chy);
		bool try_wakeup_loaded(unit* u);
		void update_civ_points();
		int get_offense_bonus(const unit* off, const unit* def) const;
		int get_defense_bonus(const unit* def, const unit* off) const;
		void check_for_victory_conditions();
		void try_add_random_barbarians(const coord& c, int num);
		void add_gold(int i);
		void add_unit(const coord& c, int uconf_id);
		void add_friendly_mercenary(const coord& c);
		void add_settler(const coord& c);

		std::vector<civilization*>::iterator current_civ;
		map* m;
		int round_number;
		const unsigned int road_moves;
		const unsigned int food_eaten_per_citizen;
		unsigned int anarchy_period_turns;
		int num_turns;
		std::list<action_listener*> action_listeners;
		int winning_civ;
		victory_type victory;
		std::map<int, diplomat*> diplomat_handlers;

		friend class boost::serialization::access;

		template<class Archive>
		void archive_helper(Archive& ar, const unsigned int version)
		{
			ar & const_cast<unit_configuration_map&>(uconfmap);
			ar & const_cast<advance_map&>(amap);
			ar & const_cast<city_improv_map&>(cimap);
			ar & const_cast<government_map&>(govmap);
			ar & m;
			ar & civs;

			ar & round_number;
			ar & const_cast<unsigned int&>(road_moves);
			ar & const_cast<unsigned int&>(food_eaten_per_citizen);
			ar & anarchy_period_turns;
			ar & num_turns;
			ar & winning_civ;
			ar & victory;
			// do not serialize diplomat handlers.
			// This is necessary only when the AI has some
			// intelligence/memory regarding negotiations.
			// After adding diplomat handler serialization,
			// remember to modify run_game() in main to not
			// readd the diplomat handlers.
			// ar & diplomat_handlers;
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

