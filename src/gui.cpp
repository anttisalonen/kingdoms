#include "gui.h"

gui::gui(SDL_Surface* screen_, map& mm, pompelmous& rr,
		const gui_resource_files& resfiles,
		const TTF_Font& font_,
		ai* ai_,
		civilization* myciv_)
	: mapview(screen_, mm, rr, resfiles, font_),
	gw(screen, data, res, ai_, myciv_)
{
}

gui::~gui()
{
}

int gui::display()
{
	gw.draw();
	return 0;
}

int gui::handle_input(const SDL_Event& ev)
{
	gw.draw();
	return gw.handle_input(ev);
}

int gui::process(int ms)
{
	return gw.process(ms);
}

void gui::init_turn()
{
	gw.init_turn();
}

void gui::handle_action(const visible_move_action& a)
{
	gw.handle_action(a);
}

