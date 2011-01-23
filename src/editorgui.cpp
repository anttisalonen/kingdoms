#include "editorgui.h"

editorgui::editorgui(SDL_Surface* screen_, map& mm, pompelmous& rr,
		const gui_resource_files& resfiles,
		const TTF_Font& font_,
		const std::string& ruleset_name)
	: mapview(screen_, mm, rr, resfiles, font_),
	ew(screen, data, res, ruleset_name)
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

