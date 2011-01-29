#ifndef GUI_RESOURCES_H
#define GUI_RESOURCES_H

#include <vector>
#include <string>

struct gui_resource_files {
	std::vector<std::string> terrains;
	std::vector<std::string> units;
	std::vector<std::string> resources;
	std::vector<std::string> rivers;
	std::string empty_unit;
	std::string city;
	std::string food_icon;
	std::string prod_icon;
	std::string comm_icon;
	std::string irrigation;
	std::string mine;
	std::vector<std::string> roads;
};

void fetch_gui_resource_files(const std::string& ruleset_name, gui_resource_files* r);
std::string get_beige_bg_file();

#endif

