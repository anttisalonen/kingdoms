#ifndef RELATIONSHIPS_WINDOW_H
#define RELATIONSHIPS_WINDOW_H

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

class relationships_window : public kingdoms_window {
	public:
		relationships_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_,
				ai* ai_, civilization* myciv_);
		~relationships_window();
		int handle_window_input(const SDL_Event& ev);
		int draw_window();
	private:
		int handle_keydown(SDLKey k, SDLMod mod);
		int handle_mousedown(const SDL_Event& ev);
		int on_exit();
		int on_civilization(const civilization* civ);
		void button_angle(int civnum, float* xp, float* yp) const;
		std::list<button*> buttons;
		std::list<civilization*> shown_civs;
		civilization* myciv;
		ai* internal_ai;
		static const int button_width;
		static const int button_height;
};


#endif
