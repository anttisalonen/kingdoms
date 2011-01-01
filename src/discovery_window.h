#ifndef DISCOVERY_WINDOW_H
#define DISCOVERY_WINDOW_H

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

class discovery_window : public window {
	public:
		discovery_window(SDL_Surface* screen_, int x, int y, gui_data& data_, gui_resources& res_, 
				civilization* myciv_, unsigned int discovered_id_);
		~discovery_window();
		int handle_window_input(const SDL_Event& ev);
		int handle_mousedown(const SDL_Event& ev);
		int handle_keydown(SDLKey k);
		int draw_window();
	private:
		int on_button(const advance_map::const_iterator& it);
		civilization* myciv;
		std::list<button*> buttons;
		unsigned int discovered_id;
};

#endif
