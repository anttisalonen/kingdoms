#ifndef EDITORGUI_H
#define EDITORGUI_H

#include "mapview.h"
#include "editor_window.h"

class editorgui : public mapview
{
	public:
		editorgui(SDL_Surface* screen, map& mm, pompelmous& rr,
				const gui_resource_files& resfiles,
				const TTF_Font& font_);
		~editorgui();
		int display();
		int handle_input(const SDL_Event& ev);
	private:
		editor_window ew;
};

#endif

