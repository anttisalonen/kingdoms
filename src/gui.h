#ifndef GUI_H
#define GUI_H

#include <vector>

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

#include "color.h"
#include "utils.h"
#include "sdl-utils.h"
#include "pompelmous.h"
#include "rect.h"
#include "game_window.h"
#include "ai.h"
#include "mapview.h"

class gui : public mapview, public action_listener
{
	public:
		gui(SDL_Surface* screen, map& mm, pompelmous& rr,
				const gui_resource_files& resfiles,
				const TTF_Font& font_,
				ai* ai_,
				civilization* myciv_,
				const std::string& ruleset_name);
		~gui();
		int display();
		int handle_input(const SDL_Event& ev);
		int process(int ms);
		void init_turn();
		void handle_action(const visible_move_action& a);
	private:
		game_window gw;
};

#endif

