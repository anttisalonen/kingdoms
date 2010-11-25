#ifndef CITY_WINDOW_H
#define CITY_WINDOW_H

#include <list>
#include <vector>

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

#include "color.h"
#include "utils.h"
#include "gui-utils.h"
#include "civ.h"
#include "rect.h"

class city_window {
	typedef int(city_window::*city_window_fun)();
	public:
		city_window(SDL_Surface* screen_, int x, int y, gui_data& data_, gui_resources& res_, city* c_);
		~city_window();
		int handle_input(const SDL_Event& ev);
		int draw();
	private:
		int handle_keydown(SDLKey k, SDLMod mod);
		int handle_mousedown(const SDL_Event& ev);
		int handle_production_input(const SDL_Event& ev);
		int change_production();
		int on_exit();
		int on_unit(unit* u);
		int draw_city_resources_screen(int xpos, int ypos);
		int choose_unit_production(const std::pair<int, unit_configuration*>& u);
		int choose_improv_production(const std::pair<unsigned int, city_improvement*>& i);
		SDL_Surface* screen;
		const int screen_w;
		const int screen_h;
		gui_data& data;
		gui_resources& res;
		city* c;
		std::list<button*> buttons;
		SDL_Surface* label_surf;
		SDL_Surface* button_surf;
		SDL_Surface* change_prod_surf;
		std::vector<SDL_Surface*> unit_tiles;

		std::list<button*> change_prod_buttons;
		std::vector<SDL_Surface*> change_prod_labels;
};


#endif
