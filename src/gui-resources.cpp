#include <fstream>
#include <boost/algorithm/string.hpp>
#include "parse_rules.h"
#include "gui-resources.h"
#include "config.h"

std::vector<std::string> get_file_list(const std::string& prefix, const std::string& filepath)
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
			results.push_back(line.insert(0, prefix));
	}
	ifs.close();
	return results;
}

void fetch_gui_resource_files(const std::string& ruleset_name, gui_resource_files* r)
{
	std::string prefix = get_rules_path(ruleset_name);
	r->terrains = get_file_list(KINGDOMS_GFXDIR, prefix + "terrain-gfx.txt");
	r->units = get_file_list(KINGDOMS_GFXDIR, prefix + "units-gfx.txt");
	r->resources = get_file_list(KINGDOMS_GFXDIR, prefix + "resources-gfx.txt");

	r->roads.clear();
	r->roads.push_back(KINGDOMS_GFXDIR "road_nw.png");
	r->roads.push_back(KINGDOMS_GFXDIR "road_w.png");
	r->roads.push_back(KINGDOMS_GFXDIR "road_sw.png");
	r->roads.push_back(KINGDOMS_GFXDIR "road_n.png");
	r->roads.push_back(KINGDOMS_GFXDIR "road.png");
	r->roads.push_back(KINGDOMS_GFXDIR "road_s.png");
	r->roads.push_back(KINGDOMS_GFXDIR "road_ne.png");
	r->roads.push_back(KINGDOMS_GFXDIR "road_e.png");
	r->roads.push_back(KINGDOMS_GFXDIR "road_se.png");

	r->rivers.clear();
	r->rivers.push_back(KINGDOMS_GFXDIR "river_n.png");
	r->rivers.push_back(KINGDOMS_GFXDIR "river_w.png");
	r->rivers.push_back(KINGDOMS_GFXDIR "river_e.png");
	r->rivers.push_back(KINGDOMS_GFXDIR "river_s.png");
	r->rivers.push_back(KINGDOMS_GFXDIR "river.png");

	r->empty_unit = KINGDOMS_GFXDIR "empty.png";
	r->city = KINGDOMS_GFXDIR "city.png";
	r->food_icon = KINGDOMS_GFXDIR "food_icon.png";
	r->prod_icon = KINGDOMS_GFXDIR "prod_icon.png";
	r->comm_icon = KINGDOMS_GFXDIR "comm_icon.png";
	r->irrigation = KINGDOMS_GFXDIR "irrigation.png";
	r->mine = KINGDOMS_GFXDIR "mine.png";
}

