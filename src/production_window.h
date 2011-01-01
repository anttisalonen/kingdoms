#ifndef PRODUCTION_WINDOW_H
#define PRODUCTION_WINDOW_H

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

class production_window : public window {
	typedef int(production_window::*production_window_fun)();
	public:
		production_window(SDL_Surface* screen_, int x, int y,
				gui_data& data_, gui_resources& res_, city* c_,
				civilization* myciv_, const rect& bg_, 
				const color& bgcol_,
				const std::string& text_, bool allow_zoom_);
		~production_window();
		int handle_window_input(const SDL_Event& ev);
		int draw_window();
	private:
		int handle_production_input(const SDL_Event& ev);
		int change_production(int num);
		int choose_unit_production(const std::pair<int, unit_configuration>& u);
		int choose_improv_production(const std::pair<unsigned int, city_improvement>& i);
		int zoom_to_city();
		city* c;
		std::list<button*> change_prod_buttons;
		civilization* myciv;
		rect bg;
		color bgcol;
		std::string text;
		std::vector<std::string> text_lines;
		bool allow_zoom;
		bool zoomed_in;
};


#endif
