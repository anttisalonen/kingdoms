#include "diplomacy_window.h"

#include <boost/bind.hpp>

diplomacy_window::diplomacy_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_,
		civilization* myciv_, int other_civ_id_)
	: kingdoms_window(screen_, data_, res_),
	myciv(myciv_),
	other_civ_id(other_civ_id_)
{
	const std::string& othercivname = data.r.civs[other_civ_id]->civname;
	switch(myciv->get_relationship_to_civ(other_civ_id)) {
		case relationship_war:
			greetings.push_back(std::string("Hello, leader of the " + myciv->civname + "."));
			greetings.push_back(std::string("Have you come to beg for mercy?"));
			buttons.push_back(new plain_button(rect(screen->w * 0.20, screen->h * 0.65, screen->w * 0.55, screen->h * 0.10),
						"Yes, let's make peace.", &res.font, color(120, 120, 255), color(0, 0, 0),
						boost::bind(&diplomacy_window::on_peace, this)));
			buttons.push_back(new plain_button(rect(screen->w * 0.20, screen->h * 0.77, screen->w * 0.55, screen->h * 0.10),
						"Oh, never mind.", &res.font, color(120, 120, 255), color(0, 0, 0),
						boost::bind(&diplomacy_window::on_exit, this)));
			break;
		case relationship_peace:
			greetings.push_back(std::string("Hello, leader of the " + myciv->civname + "."));
			greetings.push_back(std::string("I, the leader of the mighty " + othercivname + ", welcome you."));
			greetings.push_back(std::string("I don't have much time for chit-chat. So what's it gonna be?"));
			buttons.push_back(new plain_button(rect(screen->w * 0.20, screen->h * 0.65, screen->w * 0.55, screen->h * 0.10),
						"War!", &res.font, color(200, 0, 0), color(0, 0, 0),
						boost::bind(&diplomacy_window::on_war, this)));
			buttons.push_back(new plain_button(rect(screen->w * 0.20, screen->h * 0.77, screen->w * 0.55, screen->h * 0.10),
						"Peace", &res.font, color(120, 120, 255), color(0, 0, 0),
						boost::bind(&diplomacy_window::on_peace, this)));
			break;
		case relationship_unknown:
		default:
			greetings.push_back(std::string("Hello, leader of the " + myciv->civname + ", you've stumbled upon"));
			greetings.push_back(std::string("me, the leader of the mighty " + othercivname + "! I welcome you."));
			greetings.push_back(std::string("I don't have much time for chit-chat. So what's it gonna be?"));
			buttons.push_back(new plain_button(rect(screen->w * 0.20, screen->h * 0.65, screen->w * 0.55, screen->h * 0.10),
						"War!", &res.font, color(200, 0, 0), color(0, 0, 0),
						boost::bind(&diplomacy_window::on_war, this)));
			buttons.push_back(new plain_button(rect(screen->w * 0.20, screen->h * 0.77, screen->w * 0.55, screen->h * 0.10),
						"Peace", &res.font, color(120, 120, 255), color(0, 0, 0),
						boost::bind(&diplomacy_window::on_peace, this)));
			break;
	}
}

diplomacy_window::~diplomacy_window()
{
}

int diplomacy_window::draw_window()
{
	// background
	draw_plain_rectangle(screen, screen->w * 0.05f,
			screen->h * 0.05f,
			screen->w * 0.90f,
			screen->h * 0.90f, color(255, 255, 255));
	
	int greet_y = screen->h * 0.20f;
	for(unsigned int i = 0; i < greetings.size(); i++) {
		draw_text(screen, &res.font, greetings[i].c_str(), screen->w * 0.2, greet_y, 0, 0, 0);
		greet_y += 30;
	}
	std::for_each(buttons.begin(),
			buttons.end(),
			std::bind2nd(std::mem_fun(&button::draw), screen));
	return SDL_Flip(screen);
}

int diplomacy_window::on_war()
{
	data.r.declare_war_between(myciv->civ_id, other_civ_id);
	return 1;
}

int diplomacy_window::on_peace()
{
	bool had_peace = myciv->get_relationship_to_civ(other_civ_id) == relationship_peace;
	greetings.clear();
	buttons.clear();
	buttons.push_back(new plain_button(rect(screen->w * 0.20, screen->h * 0.77, screen->w * 0.55, screen->h * 0.10),
				"Exit", &res.font, color(120, 120, 255), color(0, 0, 0),
				boost::bind(&diplomacy_window::on_exit, this)));
	if(data.r.suggest_peace(myciv->civ_id, other_civ_id)) {
		if(had_peace)
			return 1;
		else
			greetings.push_back(std::string("Yup, then peace it is. War's stupid anyway."));
	}
	else {
		greetings.push_back(std::string("Ha! Ha! No way!"));
	}
	return 0;
}

int diplomacy_window::on_exit()
{
	return 1;
}

int diplomacy_window::handle_window_input(const SDL_Event& ev)
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

int diplomacy_window::handle_mousedown(const SDL_Event& ev)
{
	return check_button_click(buttons, ev);
}

int diplomacy_window::handle_keydown(SDLKey k)
{
	if(k == SDLK_ESCAPE)
		return 1;
	return 0;
}


