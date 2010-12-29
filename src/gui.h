#ifndef GUI_H
#define GUI_H

#include <list>
#include <vector>
#include <map>

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

#include "color.h"
#include "utils.h"
#include "sdl-utils.h"
#include "buf2d.h"
#include "civ.h"
#include "rect.h"
#include "main_window.h"
#include "city_window.h"
#include "ai.h"

class gui
{
	public:
		gui(int x, int y, SDL_Surface* screen, const map& mm, pompelmous& rr,
				const std::vector<std::string>& terrain_files,
				const std::vector<std::string>& unit_files,
				const char* default_unit_file,
				const char* cityfile,
				const TTF_Font& font_,
				const char* food_icon_name,
				const char* prod_icon_name,
				const char* curr_icon_name,
				const char* irrigation_name,
				const char* mine_name,
				const std::vector<const char*>& road_names,
				ai* ai_,
				civilization* myciv_);
		~gui();
		int display();
		int handle_input(const SDL_Event& ev);
		int process(int ms);
		void init_turn();
	private:
		void show_city_window(city* c);
		const int screen_w;
		const int screen_h;
		SDL_Surface* screen;
		gui_data data;
		gui_resources res;
		main_window mw;
		city_window* cw;
};

#endif

