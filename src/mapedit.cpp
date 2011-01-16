#include <stdlib.h>

#include "sdl-utils.h"

#include "pompelmous.h"
#include "parse_rules.h"
#include "gui-resources.h"
#include "editorgui.h"

#include "config.h"

void run_editor()
{
	SDL_Surface* screen = SDL_SetVideoMode(1024, 768, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (!screen) {
		fprintf(stderr, "Unable to set %dx%d video: %s\n", 1024, 768, SDL_GetError());
		return;
	}
	SDL_WM_SetCaption("Kingdoms Map Editor", NULL);
	TTF_Font* font = TTF_OpenFont(KINGDOMS_GFXDIR "DejaVuSans.ttf", 12);
	if(!font) {
		fprintf(stderr, "Could not open font: %s\n", TTF_GetError());
		return;
	}
	const int map_x = 80;
	const int map_y = 60;
	const int road_moves = 3;
	const unsigned int food_eaten_per_citizen = 2;
	const int num_turns = 300;
	resource_configuration resconf = parse_terrain_config(KINGDOMS_RULESDIR "terrain.txt");
	resource_map rmap = parse_resource_config(KINGDOMS_RULESDIR "resources.txt");
	unit_configuration_map uconfmap = parse_unit_config(KINGDOMS_RULESDIR "units.txt");
	advance_map amap = parse_advance_config(KINGDOMS_RULESDIR "discoveries.txt");
	city_improv_map cimap = parse_city_improv_config(KINGDOMS_RULESDIR "improvs.txt");
	std::vector<civilization*> civs;
	civs = parse_civs_config(KINGDOMS_RULESDIR "civs.txt");
	map m(map_x, map_y, resconf, rmap);
	pompelmous r(uconfmap, amap, cimap, &m, road_moves,
			food_eaten_per_citizen, num_turns);
	gui_resource_files grr;
	fetch_gui_resource_files(&grr);
	editorgui v(screen, r.get_map(), r, grr, *font);
	bool running = true;
	while(running) {
		v.display();
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					running = false;
				default:
					break;
			}
		}
	}
	for(unsigned int i = 0; i < civs.size(); i++) {
		delete civs[i];
	}
	TTF_CloseFont(font);
}

int main()
{
	if(sdl_init_all())
		exit(1);
	try {
		run_editor();
	}
	catch (boost::archive::archive_exception& e) {
		printf("boost::archive::archive_exception: %s (code %d).\n",
				e.what(), e.code);
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

