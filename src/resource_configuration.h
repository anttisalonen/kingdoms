#ifndef RESOURCE_CONFIGURATION_H
#define RESOURCE_CONFIGURATION_H

#include <string>

const int num_terrain_types = 16;

enum land_type {
	land_type_sea,
	land_type_land,
	land_type_hill,
	land_type_mountain,
};

class resource_configuration {
	public:
		resource_configuration();
		int get_sea_tile() const;
		int get_grass_tile() const;
		bool can_found_city(int t) const;
		std::string resource_name[num_terrain_types];
		int terrain_food_values[num_terrain_types];
		int terrain_prod_values[num_terrain_types];
		int terrain_comm_values[num_terrain_types];
		int terrain_type[num_terrain_types];
		int temperature[num_terrain_types];
		int humidity[num_terrain_types];
		bool found_city[num_terrain_types];
		int city_food_bonus;
		int city_prod_bonus;
		int city_comm_bonus;
	private:
		mutable int sea_tile;
		mutable int grass_tile;
};


#endif
