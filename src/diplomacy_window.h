#ifndef DIPLOMACY_WINDOW_H
#define DIPLOMACY_WINDOW_H

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

class diplomacy_window : public window {
	public:
		diplomacy_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_, 
				civilization* myciv_, int other_civ_id_);
		~diplomacy_window();
		int handle_window_input(const SDL_Event& ev);
		int handle_mousedown(const SDL_Event& ev);
		int handle_keydown(SDLKey k);
		int draw_window();
	private:
		int on_war();
		int on_peace();
		civilization* myciv;
		int other_civ_id;
		std::list<button*> buttons;
};

#endif
