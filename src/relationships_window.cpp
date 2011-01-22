#include "relationships_window.h"
#include "diplomacy_window.h"

#include <boost/bind.hpp>

#define MATH_PI 3.1415926535

const int relationships_window::button_width = 192;
const int relationships_window::button_height = 24;;
relationships_window::relationships_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_,
		ai* ai_, civilization* myciv_)
	: window(screen_, data_, res_),
	myciv(myciv_),
	internal_ai(ai_)
{
	buttons.push_back(new plain_button(rect(screen->w * 0.75, screen->h * 0.8, screen->w * 0.15, screen->h * 0.08),
				"Exit", &res.font, color(128, 60, 60), color(0, 0, 0),
			       	boost::bind(&relationships_window::on_exit, this)));

	for(std::vector<civilization*>::const_iterator it = data.r.civs.begin();
			it != data.r.civs.end();
			++it) {
		if(myciv->get_relationship_to_civ((*it)->civ_id) != relationship_unknown)
			shown_civs.push_back(*it);
	}

	if(shown_civs.empty())
		return;
	
	int civnum = 0;
	for(std::list<civilization*>::const_iterator it = shown_civs.begin();
			it != shown_civs.end();
			++it) {
		if(myciv->get_relationship_to_civ((*it)->civ_id) != relationship_unknown) {
			float xp, yp;
			button_angle(civnum, &xp, &yp);
			rect civ_box = rect(xp, yp, button_width, button_height);
			buttons.push_back(new plain_button(civ_box,
						(*it)->civname.c_str(), &res.font,
						(*it)->col, color(255, 255, 255),
						boost::bind(&relationships_window::on_civilization, this, *it)));
			civnum++;
		}
	}
}

relationships_window::~relationships_window()
{
	while(!buttons.empty()) {
		delete buttons.back();
		buttons.pop_back();
	}
}

void relationships_window::button_angle(int civnum, float* xp, float* yp) const
{
	float ang_diff = MATH_PI * 2 / shown_civs.size();
	float ang = -MATH_PI * 0.5f + ang_diff * civnum;
	*xp = screen->w * 0.5 + cos(ang) * screen->w * 0.35f - button_width / 2;
	*yp = screen->h * 0.5 + sin(ang) * screen->h * 0.35f - button_height / 2;
}

int relationships_window::on_exit()
{
	return 1;
}

int relationships_window::on_civilization(const civilization* civ)
{
	if(civ->civ_id == myciv->civ_id)
		return 0;

	if(!internal_ai) {
		add_subwindow(new diplomacy_window(screen, data, res, myciv,
					civ->civ_id));
	}
	return 0;
}

int relationships_window::draw_window()
{
	// background
	draw_plain_rectangle(screen, screen->w * 0.05f,
			screen->h * 0.05f,
			screen->w * 0.90f,
			screen->h * 0.90f, color(255, 255, 255));

	// relationship lines
	int civnum1 = 0;
	for(std::list<civilization*>::const_iterator it = shown_civs.begin();
			it != shown_civs.end();
			++it) {
		int civnum2 = civnum1;
		for(std::list<civilization*>::const_iterator it2 = it;
				it2 != shown_civs.end();
				++it2) {
			if((*it)->civ_id != (*it2)->civ_id) {
				float xp1, yp1, xp2, yp2;
				button_angle(civnum1, &xp1, &yp1);
				button_angle(civnum2, &xp2, &yp2);
				xp1 += button_width / 2;
				xp2 += button_width / 2;
				yp1 += button_height / 2;
				yp2 += button_height / 2;
				switch((*it)->get_relationship_to_civ((*it2)->civ_id)) {
					case relationship_peace:
						draw_line(screen, xp1, yp1, xp2, yp2, color(0, 200, 0));
						break;
					case relationship_war:
						draw_line(screen, xp1, yp1, xp2, yp2, color(255, 0, 0));
						break;
					default:
						break;
				}
			}
			civnum2++;
		}
		civnum1++;
	}

	// buttons
	std::for_each(buttons.begin(),
			buttons.end(),
			std::bind2nd(std::mem_fun(&button::draw), screen));

	// final flip
	if(SDL_Flip(screen)) {
		fprintf(stderr, "Unable to flip: %s\n", SDL_GetError());
	}
	return 0;
}

int relationships_window::handle_window_input(const SDL_Event& ev)
{
	switch(ev.type) {
		case SDL_KEYDOWN:
			return handle_keydown(ev.key.keysym.sym, ev.key.keysym.mod);
		case SDL_MOUSEBUTTONDOWN:
			return handle_mousedown(ev);
		default:
			return 0;
	}
}

int relationships_window::handle_mousedown(const SDL_Event& ev)
{
	return check_button_click(buttons, ev);
}

int relationships_window::handle_keydown(SDLKey k, SDLMod mod)
{
	if(k == SDLK_ESCAPE || k == SDLK_RETURN || k == SDLK_KP_ENTER)
		return 1;
	return 0;
}


