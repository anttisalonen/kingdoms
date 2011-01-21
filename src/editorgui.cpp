#include "editorgui.h"

editorgui::editorgui(SDL_Surface* screen_, map& mm, pompelmous& rr,
		const gui_resource_files& resfiles,
		const TTF_Font& font_)
	: mapview(screen_, mm, rr, resfiles, font_),
	ew(screen, data, res)
{
}

editorgui::~editorgui()
{
}

int editorgui::display()
{
	ew.draw();
	return 0;
}

int editorgui::handle_input(const SDL_Event& ev)
{
	return ew.handle_input(ev);
}

bool editorgui::is_quitting() const
{
	return ew.is_quitting();
}

