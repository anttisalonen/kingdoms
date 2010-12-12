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
	memset(temperature, 0, sizeof(temperature));
	memset(humidity, 0, sizeof(humidity));
	memset(found_city, 0, sizeof(found_city));
	memset(irrigatable, 0, sizeof(irrigatable));
	memset(mineable, 0, sizeof(mineable));
	memset(roadable, 0, sizeof(roadable));
	sea_tile = -1;
	grass_tile = -1;
	hill_tile = -1;
	mountain_tile = -1;
	ocean_tile = -1;
	irrigation_needed_turns = 1;
	mine_needed_turns = 1;
	road_needed_turns = 1;
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

int resource_configuration::get_ocean_tile() const
{
	if(ocean_tile != -1)
		return ocean_tile;
	for(int i = 0; i < num_terrain_types; i++) {
		if(terrain_type[i] == land_type_ocean) {
			ocean_tile = i;
			return i;
		}
	}
	return 0;
}

template<typename T>
T resource_configuration::get_value(int t, const T& def, const T ar[]) const
{
	if(t < 0 || t >= num_terrain_types)
		return def;
	return ar[t];
}

bool resource_configuration::can_found_city_on(int t) const
{
	return get_value(t, false, found_city);
}

bool resource_configuration::is_water_tile(int t) const
{
	if(t < 0 || t >= num_terrain_types)
		return false;
	return terrain_type[t] == land_type_sea ||
		terrain_type[t] == land_type_ocean;
}

bool resource_configuration::is_sea_tile(int t) const
{
	if(t < 0 || t >= num_terrain_types)
		return false;
	return terrain_type[t] == land_type_sea;
}

bool resource_configuration::is_ocean_tile(int t) const
{
	if(t < 0 || t >= num_terrain_types)
		return false;
	return terrain_type[t] == land_type_ocean;
}

bool resource_configuration::is_hill_tile(int t) const
{
	if(t < 0 || t >= num_terrain_types)
		return false;
	return terrain_type[t] == land_type_hill;
}

bool resource_configuration::is_irrigatable(int t) const
{
	return get_value(t, false, irrigatable);
}

bool resource_configuration::is_mineable(int t) const
{
	return get_value(t, false, mineable);
}

bool resource_configuration::is_roadable(int t) const
{
	return get_value(t, false, roadable);
}

bool resource_configuration::is_mountain_tile(int t) const
{
	if(t < 0 || t >= num_terrain_types)
		return false;
	return terrain_type[t] == land_type_mountain;
}

bool resource_configuration::can_have_improvement(int t, improvement_type i) const
{
	switch(i) {
		case improv_none:
			return true;
		case improv_irrigation:
			return is_irrigatable(t);
		case improv_mine:
			return is_mineable(t);
		case improv_road:
			return is_roadable(t);
		default:
			return false;
	}
}

int resource_configuration::get_needed_turns_for_improvement(improvement_type i) const
{
	switch(i) {
		case improv_none:
			return 0;
		case improv_irrigation:
			return irrigation_needed_turns;
		case improv_mine:
			return mine_needed_turns;
		case improv_road:
			return road_needed_turns;
		default:
			return 0;
	}
}

