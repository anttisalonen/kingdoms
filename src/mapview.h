#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <vector>

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

#include "color.h"
#include "utils.h"
#include "sdl-utils.h"
#include "gui-utils.h"
#include "pompelmous.h"
#include "rect.h"
#include "gui-resources.h"

class mapview
{
	public:
		mapview(SDL_Surface* screen, map& mm, pompelmous& rr,
				const gui_resource_files& resfiles,
				const TTF_Font& font_);
		virtual ~mapview();
		virtual int display();
		virtual int handle_input(const SDL_Event& ev);
	protected:
		SDL_Surface* screen;
		gui_data data;
		gui_resources res;
};

#endif

