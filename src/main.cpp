#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <list>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>
#include <fstream>
#include <sstream>

#include <boost/algorithm/string.hpp>

#include "color.h"
#include "utils.h"
#include "sdl-utils.h"
#include "buf2d.h"
#include "civ.h"
#include "gui.h"
#include "ai.h"

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

std::vector<std::vector<std::string> > parser(const std::string& filepath, 
		unsigned int num_fields, bool vararg = false)
{
	using namespace std;
	ifstream ifs(filepath.c_str(), ifstream::in);
	int linenum = 0;
	vector<vector<string> > results;
	while(ifs.good()) {
		linenum++;
		string line;
		getline(ifs, line);
		bool quoting = false;
		bool filling = false;
		bool escaping = false;
		vector<string> values;
		string value;
		for(unsigned int i = 0; i < line.length(); ++i) {
			if(escaping) {
				value += line[i];
				escaping = false;
			}
			else if(((!quoting && (line[i] == ' ' || line[i] == '\t')) || (quoting && line[i] == '"')) 
					&& filling) {
				values.push_back(value);
				value = "";
				quoting = false;
				filling = false;
				escaping = false;
			}
			else if(line[i] == '"') {
				filling = true;
				quoting = true;
			}
			else if(!quoting && line[i] == '#') {
				break;
			}
			else if(line[i] == '\\') {
				escaping = true;
			}
			else if(quoting || !(line[i] == ' ' || line[i] == '\t')) {
				filling = true;
				value += line[i];
			}
		}
		// add last parsed token to values
		if(value.size())
			values.push_back(value);

		if(values.size() == num_fields || (vararg && values.size() > num_fields)) {
			results.push_back(values);
		}
		else if(values.size() != 0) {
			fprintf(stderr, "Warning: parsing %s - line %d: parsed %d "
					"tokens - wanted %d:\n",
					filepath.c_str(),
					linenum, values.size(), num_fields);
			for(unsigned int i = 0; i < values.size(); i++) {
				fprintf(stderr, "\t%s\n", values[i].c_str());
			}
		}
	}
	ifs.close();
	return results;
}

int stoi(const std::string& s)
{
	int res = 0;
	std::stringstream(s) >> res;
	return res;
}

bool get_flag(const std::string& s, unsigned int i)
{
	if(s.length() <= i)
		return false;
	else
		return s[i] != '0';
}

typedef std::vector<std::vector<std::string> > parse_result;

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

std::vector<civilization*> parse_civs_config(const std::string& fp)
{
	std::vector<civilization*> civs;
	parse_result pcivs = parser(fp, 5, true);
	for(unsigned int i = 0; i < pcivs.size(); i++) {
		civs.push_back(new civilization(pcivs[i][0], i, 
					color(stoi(pcivs[i][1]),
						stoi(pcivs[i][2]),
						stoi(pcivs[i][3])),
					NULL, false, pcivs[i].begin() + 4, 
					pcivs[i].end(), NULL));
	}
	return civs;
}

unit_configuration_map parse_unit_config(const std::string& fp)
{
	parse_result units = parser(fp, 6);
	unit_configuration_map uconfmap;
	for(unsigned int i = 0; i < units.size(); i++) {
		unit_configuration u;
		u.unit_name = units[i][0];
		u.max_moves = stoi(units[i][1]);
		u.max_strength = stoi(units[i][2]);
		u.production_cost = stoi(units[i][3]);
		u.needed_advance = stoi(units[i][4]);
		u.settler = get_flag(units[i][5], 0);
		u.worker = get_flag(units[i][5], 1);
		uconfmap.insert(std::make_pair(i, u));
	}
	return uconfmap;
}

advance_map parse_advance_config(const std::string& fp)
{
	advance_map amap;
	parse_result discoveries = parser(fp, 6);
	for(unsigned int i = 0; i < discoveries.size(); i++) {
		advance a;
		a.advance_id = i + 1;
		a.advance_name = discoveries[i][0];
		a.cost = stoi(discoveries[i][1]);
		for(int j = 0; j < 4; j++) {
			a.needed_advances[j] = stoi(discoveries[i][j + 2]);
		}
		amap.insert(std::make_pair(a.advance_id, a));
	}
	return amap;
}

city_improv_map parse_city_improv_config(const std::string& fp)
{
	parse_result improvs = parser(fp, 7);
	city_improv_map cimap;
	for(unsigned int i = 0; i < improvs.size(); i++) {
		city_improvement a;
		a.improv_id = i + 1;
		a.improv_name = improvs[i][0];
		a.cost = stoi(improvs[i][1]);
		a.needed_advance = stoi(improvs[i][2]);
		a.comm_bonus = stoi(improvs[i][3]);
		a.culture = stoi(improvs[i][4]);
		a.happiness = stoi(improvs[i][5]);
		a.barracks = get_flag(improvs[i][6], 0);
		a.granary = get_flag(improvs[i][6], 1);
		cimap.insert(std::make_pair(a.improv_id, a));
	}
	return cimap;
}

resource_configuration parse_resource_config(const std::string& fp)
{
	resource_configuration resconf;
	resconf.city_food_bonus = 1;
	resconf.city_prod_bonus = 1;
	resconf.city_comm_bonus = 0; // compensated by road
	resconf.irrigation_needed_turns = 3;
	resconf.mine_needed_turns = 4;
	resconf.road_needed_turns = 2;
	parse_result terrains = parser(fp, 8);

	if(terrains.size() >= (int)num_terrain_types) {
		fprintf(stderr, "Warning: more than %d parsed terrains will be ignored in %s.\n",
				num_terrain_types, std::string(fp).c_str());
	}
	for(unsigned int i = 0; i < std::min<unsigned int>(num_terrain_types, terrains.size()); i++) {
		resconf.resource_name[i] = terrains[i][0];
		resconf.terrain_food_values[i] = stoi(terrains[i][1]);
		resconf.terrain_prod_values[i] = stoi(terrains[i][2]);
		resconf.terrain_comm_values[i] = stoi(terrains[i][3]);
		resconf.terrain_type[i] = stoi(terrains[i][4]);
		resconf.temperature[i] = stoi(terrains[i][5]);
		resconf.humidity[i] = stoi(terrains[i][6]);
		resconf.found_city[i] = get_flag(terrains[i][7], 0);
		resconf.irrigatable[i] = get_flag(terrains[i][7], 1);
		resconf.mineable[i] = get_flag(terrains[i][7], 2);
		resconf.roadable[i] = get_flag(terrains[i][7], 3);
	}
	return resconf;
}

government_map parse_government_config(const std::string& fp)
{
	government_map govmap;
	parse_result governments = parser(fp, 5);

	for(unsigned int i = 0; i < governments.size(); i++) {
		government gov(i + 1, governments[i][0]);
		gov.needed_advance = stoi(governments[i][1]);
		gov.free_units = stoi(governments[i][2]);
		gov.unit_cost = stoi(governments[i][3]);
		govmap.insert(std::make_pair(gov.gov_id, gov));
	}
	return govmap;
}

int run(bool observer, bool use_gui)
{
	const int map_x = 80;
	const int map_y = 60;

	if(!use_gui)
		observer = true;

	std::vector<civilization*> civs = parse_civs_config("share/civs.txt");
	unit_configuration_map uconfmap = parse_unit_config("share/units.txt");
	advance_map amap = parse_advance_config("share/discoveries.txt");
	city_improv_map cimap = parse_city_improv_config("share/improvs.txt");
	resource_configuration resconf = parse_resource_config("share/terrain.txt");
	government_map govmap = parse_government_config("share/governments.txt");
	map m(map_x, map_y, resconf);
	for(unsigned int i = 0; i < civs.size(); i++) {
		civs[i]->set_map(&m);
		civs[i]->set_government(&govmap.begin()->second);
	}
	round r(uconfmap, amap, cimap, m);

	std::vector<coord> starting_places = m.get_starting_places(civs.size());
	for(unsigned int i = 0; i < civs.size(); i++) {
		// settler
		civs[i]->add_unit(0, starting_places[i].x, starting_places[i].y, (*(r.uconfmap.find(0))).second);
		// warrior
		civs[i]->add_unit(2, starting_places[i].x, starting_places[i].y, (*(r.uconfmap.find(2))).second);
		r.add_civilization(civs[i]);
	}

	std::vector<std::string> terrain_files = get_file_list("share/", "share/terrain-gfx.txt");
	std::vector<std::string> unit_files = get_file_list("share/", "share/units-gfx.txt");

	bool running = true;

	TTF_Font* font;
	font = TTF_OpenFont("share/DejaVuSans.ttf", 12);
	if(!font) {
		fprintf(stderr, "Could not open font: %s\n", TTF_GetError());
	}

	std::map<unsigned int, ai> ais;
	if(observer)
		ais.insert(std::make_pair(0, ai(m, r, r.civs[0])));
	for(unsigned int i = 1; i < civs.size(); i++)
		ais.insert(std::make_pair(i, ai(m, r, r.civs[i])));
	if(use_gui) {
		std::vector<const char*> road_images;
		road_images.push_back("share/road_nw.png");
		road_images.push_back("share/road_w.png");
		road_images.push_back("share/road_sw.png");
		road_images.push_back("share/road_n.png");
		road_images.push_back("share/road.png");
		road_images.push_back("share/road_s.png");
		road_images.push_back("share/road_ne.png");
		road_images.push_back("share/road_e.png");
		road_images.push_back("share/road_se.png");
		gui g(1024, 768, m, r, terrain_files, unit_files, "share/empty.png", 
				"share/city.png", *font,
				"share/food_icon.png",
				"share/prod_icon.png",
				"share/comm_icon.png",
				"share/irrigation.png",
				"share/mine.png",
				road_images,
				observer ? &ais.find(0)->second : NULL, civs[0]);
		g.display();
		g.init_turn();
		while(running) {
			if(r.current_civ_id() == (int)civs[0]->civ_id) {
				SDL_Event event;
				while(SDL_PollEvent(&event)) {
					switch(event.type) {
						case SDL_KEYDOWN:
						case SDL_MOUSEBUTTONDOWN:
						case SDL_MOUSEBUTTONUP:
							if(g.handle_input(event))
								running = false;
							break;
						case SDL_QUIT:
							running = false;
						default:
							break;
					}
				}
				SDL_Delay(50);
				g.process(50);
			}
			else {
				std::map<unsigned int, ai>::iterator ait = ais.find(r.current_civ_id());
				if(ait != ais.end()) {
					if(ait->second.play())
						running = false;
					else
						g.init_turn();
				}
				else {
					running = false;
				}
			}
		}
	}
	else {
		while(1) {
			std::map<unsigned int, ai>::iterator ait = ais.find(r.current_civ_id());
			if(ait != ais.end()) {
				ait->second.play();
			}
		}
	}
	TTF_CloseFont(font);
	for(unsigned int i = 0; i < civs.size(); i++) {
		delete civs[i];
	}
	return 0;
}

int main(int argc, char **argv)
{
	bool observer = false;
	bool gui = true;
	int seed = 0;
	int c;
	bool succ = true;
	while((c = getopt(argc, argv, "oxs:")) != -1) {
		switch(c) {
			case 'o':
				observer = true;
				break;
			case 'x':
				gui = false;
				break;
			case 's':
				seed = atoi(optarg);
				break;
			case '?':
			default:
				fprintf(stderr, "Unrecognized option: -%c\n",
						optopt);
				succ = false;
				break;
		}
	}
	if(!succ)
		exit(2);
	if(seed)
		srand(seed);
	else {
		seed = time(NULL);
		printf("Seed: %d\n", seed);
		srand(seed);
	}
	int sdl_flags = SDL_INIT_EVERYTHING;
	if(!gui)
		sdl_flags |= SDL_INIT_NOPARACHUTE;
	if (SDL_Init(sdl_flags) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	if(!gui)
		signal(SIGINT, SIG_DFL);
	if(!IMG_Init(IMG_INIT_PNG)) {
		fprintf(stderr, "Unable to init SDL_image: %s\n", IMG_GetError());
	}
	if(!TTF_Init()) {
		fprintf(stderr, "Unable to init SDL_ttf: %s\n", TTF_GetError());
	}
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	try {
		run(observer, gui);
	}
	catch (std::exception& e) {
		printf("std::exception: %s\n", e.what());
	}
	catch(...) {
		printf("Unknown exception.\n");
	}

	TTF_Quit();
	SDL_Quit();
	return 0;
}

