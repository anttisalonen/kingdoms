#include <stdlib.h>
#include <unistd.h>

#include <list>
#include <vector>
#include <map>
#include <utility>

#include "color.h"
#include "utils.h"
#include "sdl-utils.h"
#include "buf2d.h"
#include "civ.h"
#include "gui.h"

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

int run()
{
	const int map_x = 64;
	const int map_y = 32;
	map m(map_x, map_y);

	civilization* civ1 = new civilization("Babylonians", 0, color(255, 0, 0), m);
	civilization* civ2 = new civilization("Egyptians", 1, color(255, 255, 0), m);

	unit_configuration settlers_conf;
	unit_configuration warrior_conf;
	settlers_conf.max_moves = 1;
	settlers_conf.unit_name = "Settlers";
	settlers_conf.settler = true;
	warrior_conf.max_moves = 1;
	warrior_conf.unit_name = "Warrior";
	warrior_conf.settler = false;
	unit_configuration_map uconfmap;
	uconfmap.insert(std::make_pair(0, &settlers_conf));
	uconfmap.insert(std::make_pair(1, &warrior_conf));
	resource_configuration resconf;

	resconf.terrain_food_values[0] = 1; resconf.terrain_prod_values[0] = 0; resconf.terrain_comm_values[0] = 2;
	resconf.terrain_food_values[1] = 2; resconf.terrain_prod_values[1] = 0; resconf.terrain_comm_values[1] = 1;
	resconf.city_food_bonus = 1;
	resconf.city_prod_bonus = 1;
	resconf.city_comm_bonus = 1;

	round r(uconfmap, resconf);
	civ1->add_unit(0, 1, 1);
	civ1->add_unit(1, 2, 2);
	civ2->add_unit(0, 7, 6);
	civ2->add_unit(1, 7, 7);

	r.add_civilization(civ1);
	r.add_civilization(civ2);
	std::list<unit*>::iterator current_unit = (*r.current_civ)->units.begin();

	std::vector<const char*> terrain_files;
	std::vector<const char*> unit_files;
	terrain_files.push_back("share/terrain1.png");
	terrain_files.push_back("share/terrain2.png");
	unit_files.push_back("share/settlers.png");
	unit_files.push_back("share/warrior.png");

	bool running = true;

	TTF_Font* font;
	font = TTF_OpenFont("share/DejaVuSans.ttf", 12);
	if(!font) {
		fprintf(stderr, "Could not open font: %s\n", TTF_GetError());
	}

	gui g(1024, 768, m, r, terrain_files, unit_files, "share/city.png", *font,
			"share/food_icon.png",
			"share/prod_icon.png",
			"share/comm_icon.png");
	g.display(*current_unit);
	while(running) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					if(g.handle_input(event, current_unit))
						running = false;
					break;
				case SDL_QUIT:
					running = false;
				default:
					break;
			}
		}
		SDL_Delay(50);
		g.process(50, *current_unit);
	}
	TTF_CloseFont(font);
	delete civ2;
	delete civ1;
	return 0;
}

int main(int argc, char **argv)
{
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
	run();

	TTF_Quit();
	SDL_Quit();
	return 0;
}

