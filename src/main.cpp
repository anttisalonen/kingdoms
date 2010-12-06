#include <stdlib.h>
#include <unistd.h>

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

std::vector<std::vector<std::string> > parser(const std::string& filepath, unsigned int num_fields)
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
		if(value.size())
			values.push_back(value);
		if(values.size() == num_fields) {
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

round* parse_configs(const std::string& confpath, const std::string& terrainfile,
	       const std::string& unitfile, const std::string& civfile,
	       const std::string& discoveryfile, const std::string& improvfile,
	       resource_configuration* resconf,
	       std::vector<civilization*>* civs)
{
	typedef std::vector<std::vector<std::string> > parse_result;
	parse_result terrains = parser(confpath + terrainfile, 8);
	parse_result units = parser(confpath + unitfile, 6);
	parse_result pcivs = parser(confpath + civfile, 4);
	parse_result discoveries = parser(confpath + discoveryfile, 6);
	parse_result improvs = parser(confpath + improvfile, 4);

	if(terrains.size() >= (int)num_terrain_types) {
		fprintf(stderr, "Warning: more than %d parsed terrains will be ignored in %s.\n",
				num_terrain_types, std::string(confpath + terrainfile).c_str());
	}
	for(unsigned int i = 0; i < std::min<unsigned int>(num_terrain_types, terrains.size()); i++) {
		resconf->resource_name[i] = terrains[i][0];
		resconf->terrain_food_values[i] = stoi(terrains[i][1]);
		resconf->terrain_prod_values[i] = stoi(terrains[i][2]);
		resconf->terrain_comm_values[i] = stoi(terrains[i][3]);
		resconf->terrain_type[i] = stoi(terrains[i][4]);
		resconf->temperature[i] = stoi(terrains[i][5]);
		resconf->humidity[i] = stoi(terrains[i][6]);
		resconf->found_city[i] = get_flag(terrains[i][7], 0);
	}

	for(unsigned int i = 0; i < pcivs.size(); i++) {
		civs->push_back(new civilization(pcivs[i][0], i, 
					color(stoi(pcivs[i][1]),
						stoi(pcivs[i][2]),
						stoi(pcivs[i][3])),
					NULL, false));
	}

	advance_map amap;
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
	
	city_improv_map cimap;
	for(unsigned int i = 0; i < improvs.size(); i++) {
		city_improvement a;
		a.improv_id = i + 1;
		a.improv_name = improvs[i][0];
		a.cost = stoi(improvs[i][1]);
		a.needed_advance = stoi(discoveries[i][2]);
		a.barracks = get_flag(discoveries[i][3], 0);
		cimap.insert(std::make_pair(a.improv_id, a));
	}

	unit_configuration_map uconfmap;
	for(unsigned int i = 0; i < units.size(); i++) {
		unit_configuration u;
		u.unit_name = units[i][0];
		u.max_moves = stoi(units[i][1]);
		u.max_strength = stoi(units[i][2]);
		u.production_cost = stoi(units[i][3]);
		u.needed_advance = stoi(units[i][4]);
		u.settler = get_flag(units[i][5], 0);
		uconfmap.insert(std::make_pair(i, u));
	}
	round* r = new round(uconfmap, amap, cimap);
	return r;
}

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

int run(bool observer)
{
	const int map_x = 80;
	const int map_y = 60;

	resource_configuration resconf;
	resconf.city_food_bonus = 1;
	resconf.city_prod_bonus = 1;
	resconf.city_comm_bonus = 1;

	std::vector<civilization*> civs;
	round* r = parse_configs("share/", "terrain.txt", "units.txt", "civs.txt",
			"discoveries.txt", "improvs.txt", &resconf,
			&civs);
	if(!r) {
		fprintf(stderr, "Could not parse the configuration.\n");
		return 1;
	}
	map m(map_x, map_y, resconf);
	for(unsigned int i = 0; i < civs.size(); i++) {
		civs[i]->set_map(&m);
	}

	std::vector<coord> starting_places = m.get_starting_places(2);
	civs[0]->add_unit(0, starting_places[0].x, starting_places[0].y, (*(r->uconfmap.find(0))).second);
	civs[0]->add_unit(1, starting_places[0].x, starting_places[0].y, (*(r->uconfmap.find(1))).second);
	civs[1]->add_unit(0, starting_places[1].x, starting_places[1].y, (*(r->uconfmap.find(0))).second);
	civs[1]->add_unit(1, starting_places[1].x, starting_places[1].y, (*(r->uconfmap.find(1))).second);

	r->add_civilization(civs[0]);
	r->add_civilization(civs[1]);

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
		ais.insert(std::make_pair(0, ai(m, *r, r->civs[0])));
	ais.insert(std::make_pair(1, ai(m, *r, r->civs[1])));
	gui g(1024, 768, m, *r, terrain_files, unit_files, "share/empty.png", 
			"share/city.png", *font,
			"share/food_icon.png",
			"share/prod_icon.png",
			"share/comm_icon.png",
			observer ? &ais.find(0)->second : NULL, civs[0]);
	g.display();
	g.init_turn();
	while(running) {
		if(r->current_civ_id() == (int)civs[0]->civ_id) {
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
			std::map<unsigned int, ai>::iterator ait = ais.find(r->current_civ_id());
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
	TTF_CloseFont(font);
	delete r;
	for(unsigned int i = 0; i < civs.size(); i++) {
		delete civs[i];
	}
	return 0;
}

int main(int argc, char **argv)
{
	bool observer = false;
	srand(time(NULL));
	if(argc > 1 && !strcmp(argv[1], "-o")) {
		observer = true;
	}
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	if(!IMG_Init(IMG_INIT_PNG)) {
		fprintf(stderr, "Unable to init SDL_image: %s\n", IMG_GetError());
	}
	if(!TTF_Init()) {
		fprintf(stderr, "Unable to init SDL_ttf: %s\n", TTF_GetError());
	}
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	try {
		run(observer);
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

