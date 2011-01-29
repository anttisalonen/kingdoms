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
#include "ai.h"
#include "production_window.h"

class city_window : public kingdoms_window {
	public:
		city_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_, city* c_,
				ai* ai_, civilization* myciv_);
		~city_window();
		int handle_window_input(const SDL_Event& ev);
		int draw_window();
	private:
		int handle_keydown(SDLKey k, SDLMod mod);
		int handle_mousedown(const SDL_Event& ev);
		int change_production();
		int on_exit();
		int on_unit(unit* u);
		int draw_city_resources_screen(int xpos, int ypos);
		city* c;
		std::list<button*> buttons;
		std::vector<SDL_Surface*> change_prod_labels;
		civilization* myciv;
		ai* internal_ai;
};


#endif
