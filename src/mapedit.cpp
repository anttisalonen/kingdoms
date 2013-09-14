#include <stdlib.h>
#include <unistd.h>

#include "sdl-utils.h"

#include "pompelmous.h"
#include "parse_rules.h"
#include "gui-resources.h"
#include "editorgui.h"

#include "paths.h"

static std::string ruleset_name;

void run_editor()
{
	SDL_Surface* screen = SDL_SetVideoMode(1024, 768, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (!screen) {
		fprintf(stderr, "Unable to set %dx%d video: %s\n", 1024, 768, SDL_GetError());
		return;
	}
	SDL_WM_SetCaption("Kingdoms Map Editor", NULL);
	TTF_Font* font = TTF_OpenFont(get_graphics_path("DejaVuSans.ttf").c_str(), 12);
	if(!font) {
		fprintf(stderr, "Could not open font: %s\n", TTF_GetError());
		return;
	}
	const int map_x = 80;
	const int map_y = 60;
	const int road_moves = 3;
	const unsigned int food_eaten_per_citizen = 2;
	const int num_turns = 300;
	const int anarchy_period = 1;
	resource_configuration resconf;
	resource_map rmap;
	unit_configuration_map uconfmap;
	advance_map amap;
	city_improv_map cimap;
	government_map govmap;
	std::vector<civilization*> civs;
	get_configuration(ruleset_name, &civs, &uconfmap, &amap, &cimap,
			&resconf, &govmap, &rmap);
	map m(map_x, map_y, resconf, rmap);
	pompelmous r(uconfmap, amap, cimap, govmap, &m, road_moves,
			food_eaten_per_citizen, anarchy_period, num_turns);
	for(unsigned int i = 0; i < civs.size(); i++) {
		civs[i]->set_map(&m);
		civs[i]->set_government(&govmap.begin()->second);
		r.add_civilization(civs[i]);
	}
	gui_resource_files grr;
	fetch_gui_resource_files(ruleset_name, &grr);
	editorgui v(screen, r.get_map(), r, grr, *font, ruleset_name);
	bool running = true;
	while(running) {
		v.display();
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
				case SDL_MOUSEMOTION:
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					if(v.handle_input(event)) {
						running = false;
					}
					break;
				case SDL_QUIT:
					running = false;
				default:
					break;
			}
		}
		if(v.is_quitting())
			running = false;
		SDL_Delay(20);
	}
	for(unsigned int i = 0; i < civs.size(); i++) {
		delete civs[i];
	}
	TTF_CloseFont(font);
}

void usage(const char* pn)
{
	fprintf(stderr, "Usage: %s [-r <ruleset name>]\n\n",
			pn);
	fprintf(stderr, "\t-r ruleset: use custom ruleset\n");
}

int main(int argc, char** argv)
{
	int c;
	ruleset_name = "default";
	while((c = getopt(argc, argv, "r:h")) != -1) {
		switch(c) {
			case 'r':
				ruleset_name = std::string(optarg);
				break;
			case 'h':
				usage(argv[0]);
				exit(2);
				break;
			case '?':
			default:
				fprintf(stderr, "Unrecognized option: -%c\n",
						optopt);
				exit(2);
		}
	}
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

