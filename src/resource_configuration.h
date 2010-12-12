#ifndef RESOURCE_CONFIGURATION_H
#define RESOURCE_CONFIGURATION_H

#include <string>

const int num_terrain_types = 16;

enum land_type {
	land_type_sea,
	land_type_land,
	land_type_hill,
	land_type_mountain,
	land_type_ocean,
};

enum improvement_type {
	improv_none        = 0x00,
	improv_road        = 0x01,
	improv_irrigation  = 0x02,
	improv_mine        = 0x04,
};

class resource_configuration {
	public:
		resource_configuration();
		int get_sea_tile() const;
		int get_ocean_tile() const;
		int get_grass_tile() const;
		int get_hill_tile() const;
		int get_mountain_tile() const;
		bool can_found_city_on(int t) const;
		bool is_water_tile(int t) const;
		bool is_sea_tile(int t) const;
		bool is_ocean_tile(int t) const;
		bool is_hill_tile(int t) const;
		bool is_mountain_tile(int t) const;
		bool is_irrigatable(int t) const;
		bool is_mineable(int t) const;
		bool is_roadable(int t) const;
		bool can_have_improvement(int t, improvement_type i) const;
		int get_needed_turns_for_improvement(improvement_type i) const;
		std::string resource_name[num_terrain_types];
		int terrain_food_values[num_terrain_types];
		int terrain_prod_values[num_terrain_types];
		int terrain_comm_values[num_terrain_types];
		int terrain_type[num_terrain_types];
		int temperature[num_terrain_types];
		int humidity[num_terrain_types];
		bool found_city[num_terrain_types];
		bool irrigatable[num_terrain_types];
		bool mineable[num_terrain_types];
		bool roadable[num_terrain_types];
		int city_food_bonus;
		int city_prod_bonus;
		int city_comm_bonus;
		int irrigation_needed_turns;
		int mine_needed_turns;
		int road_needed_turns;
	private:
		template<typename T>
			T get_value(int t, const T& def, const T ar[]) const;
		mutable int sea_tile;
		mutable int grass_tile;
		mutable int hill_tile;
		mutable int mountain_tile;
		mutable int ocean_tile;
};


#endif
