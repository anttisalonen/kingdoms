#include "discovery_window.h"

#include <boost/bind.hpp>

discovery_window::discovery_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_,
		civilization* myciv_, unsigned int discovered_id_)
	: window(screen_, data_, res_),
	myciv(myciv_),
	discovered_id(discovered_id_)
{
	const float button_dist_y = 0.07f;
	rect option_rect = rect(screen->w * 0.30, screen->h * 0.4, screen->w * 0.38, screen->h * 0.06);
	for(advance_map::const_iterator it = data.r.amap.begin();
			it != data.r.amap.end();
			++it) {
		if(myciv->allowed_research_goal(it)) {
			buttons.push_back(new plain_button(option_rect,
						it->second.advance_name.c_str(), &res.font, 
						color(70, 150, 255), 
						color(0, 0, 0),
						boost::bind(&discovery_window::on_button, this, it)));
			option_rect.y += screen->h * button_dist_y;
			if(option_rect.y > screen->h * 0.8) {
				// that's too much
				break;
			}
		}
	}
}

discovery_window::~discovery_window()
{
}

int discovery_window::draw_window()
{
	// background
	draw_plain_rectangle(screen, screen->w * 0.05f,
			screen->h * 0.05f,
			screen->w * 0.90f,
			screen->h * 0.90f, color(255, 255, 255));
	const std::string* discovered = NULL;
	if(discovered_id != 0) {
		advance_map::const_iterator it = data.r.amap.find(discovered_id);
		if(it != data.r.amap.end()) {
			discovered = &it->second.advance_name;
		}
	}
	char question[256];
	question[255] = '\0';
	if(discovered)
		sprintf(question, "Our wise men have discovered %s! What should we concentrate on next, sire?",
				discovered->c_str());
	else
		sprintf(question, "What should our wise men research, sire? Pick one...");

	draw_text(screen, &res.font, question, screen->w * 0.2, screen->h * 0.2, 0, 0, 0);
	std::for_each(buttons.begin(),
			buttons.end(),
			std::bind2nd(std::mem_fun(&button::draw), screen));
	return SDL_Flip(screen);
}

int discovery_window::on_button(const advance_map::const_iterator& it)
{
	myciv->research_goal_id = it->first;
	return 1;
}

int discovery_window::handle_window_input(const SDL_Event& ev)
{
	switch(ev.type) {
		case SDL_KEYDOWN:
			return handle_keydown(ev.key.keysym.sym);
		case SDL_MOUSEBUTTONDOWN:
			return handle_mousedown(ev);
		default:
			return 0;
	}
}

int discovery_window::handle_mousedown(const SDL_Event& ev)
{
	return check_button_click(buttons, ev);
}

int discovery_window::handle_keydown(SDLKey k)
{
	if(k == SDLK_ESCAPE)
		return 1;
	return 0;
}


