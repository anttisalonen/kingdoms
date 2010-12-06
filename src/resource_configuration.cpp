#include <string.h>
#include "resource_configuration.h"
#include <stdio.h>

resource_configuration::resource_configuration()
	: city_food_bonus(0),
	city_prod_bonus(0),
	city_comm_bonus(0)
{
	memset(terrain_food_values, 0, sizeof(terrain_food_values));
	memset(terrain_prod_values, 0, sizeof(terrain_prod_values));
	memset(terrain_comm_values, 0, sizeof(terrain_comm_values));
	memset(terrain_type, 0, sizeof(terrain_comm_values));
	memset(temperature, 0, sizeof(terrain_comm_values));
	memset(humidity, 0, sizeof(terrain_comm_values));
	memset(found_city, 0, sizeof(terrain_comm_values));
	sea_tile = -1;
	grass_tile = -1;
	hill_tile = -1;
	mountain_tile = -1;
}

int resource_configuration::get_sea_tile() const
{
	if(sea_tile != -1)
		return sea_tile;
	for(int i = 0; i < num_terrain_types; i++) {
		if(terrain_type[i] == land_type_sea) {
			sea_tile = i;
			return i;
		}
	}
	return 0;
}

int resource_configuration::get_grass_tile() const
{
	if(grass_tile != -1)
		return grass_tile;
	for(int i = 0; i < num_terrain_types; i++) {
		if(terrain_type[i] == land_type_land) {
			grass_tile = i;
			return i;
		}
	}
	return 0;
}

int resource_configuration::get_hill_tile() const
{
	if(hill_tile != -1)
		return hill_tile;
	for(int i = 0; i < num_terrain_types; i++) {
		if(terrain_type[i] == land_type_hill) {
			hill_tile = i;
			return i;
		}
	}
	return 0;
}

int resource_configuration::get_mountain_tile() const
{
	if(mountain_tile != -1)
		return mountain_tile;
	for(int i = 0; i < num_terrain_types; i++) {
		if(terrain_type[i] == land_type_mountain) {
			mountain_tile = i;
			return i;
		}
	}
	return 0;
}

bool resource_configuration::can_found_city(int t) const
{
	if(t < 0 || t >= num_terrain_types)
		return false;
	return found_city[t];
}

bool resource_configuration::is_water_tile(int t) const
{
	if(t < 0 || t >= num_terrain_types)
		return false;
	return terrain_type[t] == land_type_sea;
}

bool resource_configuration::is_hill_tile(int t) const
{
	if(t < 0 || t >= num_terrain_types)
		return false;
	return terrain_type[t] == land_type_hill;
}

bool resource_configuration::is_mountain_tile(int t) const
{
	if(t < 0 || t >= num_terrain_types)
		return false;
	return terrain_type[t] == land_type_mountain;
}

