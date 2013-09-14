#include <fstream>
#include <boost/algorithm/string.hpp>
#include "parse_rules.h"
#include "gui-resources.h"
#include "paths.h"

std::vector<std::string> get_file_list(const std::string& filepath)
{
	using namespace std;
	ifstream ifs(filepath.c_str(), ifstream::in);
	int linenum = 0;
	std::vector<std::string> results;
	while(ifs.good()) {
		linenum++;
		string line;
		getline(ifs, line);
		boost::trim(line);
		if(line.length())
			results.push_back(get_graphics_path(line));
	}
	ifs.close();
	return results;
}

void fetch_gui_resource_files(const std::string& ruleset_name, gui_resource_files* r)
{
	std::string prefix = get_rules_path(ruleset_name);
	r->terrains = get_file_list(prefix + "/terrain-gfx.txt");
	r->units = get_file_list(prefix + "/units-gfx.txt");
	r->resources = get_file_list(prefix + "/resources-gfx.txt");

	r->roads.clear();
	r->roads.push_back(get_graphics_path("road_nw.png"));
	r->roads.push_back(get_graphics_path("road_w.png"));
	r->roads.push_back(get_graphics_path("road_sw.png"));
	r->roads.push_back(get_graphics_path("road_n.png"));
	r->roads.push_back(get_graphics_path("road.png"));
	r->roads.push_back(get_graphics_path("road_s.png"));
	r->roads.push_back(get_graphics_path("road_ne.png"));
	r->roads.push_back(get_graphics_path("road_e.png"));
	r->roads.push_back(get_graphics_path("road_se.png"));

	r->rivers.clear();
	r->rivers.push_back(get_graphics_path("river_n.png"));
	r->rivers.push_back(get_graphics_path("river_w.png"));
	r->rivers.push_back(get_graphics_path("river_e.png"));
	r->rivers.push_back(get_graphics_path("river_s.png"));
	r->rivers.push_back(get_graphics_path("river.png"));

	r->empty_unit = get_graphics_path("empty.png");
	r->city = get_graphics_path("city.png");
	r->food_icon = get_graphics_path("food_icon.png");
	r->prod_icon = get_graphics_path("prod_icon.png");
	r->comm_icon = get_graphics_path("comm_icon.png");
	r->irrigation = get_graphics_path("irrigation.png");
	r->mine = get_graphics_path("mine.png");
	r->village_image = get_graphics_path("village.png");
}

std::string get_beige_bg_file()
{
	return get_graphics_path("beige_bg.png");
}

