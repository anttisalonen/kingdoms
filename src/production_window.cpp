#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

#include "production_window.h"
#include "city_window.h"

production_window::production_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_, city* c_,
		civilization* myciv_, const rect& bg_, const color& bgcol_, const std::string& text_, bool allow_zoom_)
	: window(screen_, data_, res_),
	c(c_),
	myciv(myciv_),
	bg(bg_),
	bgcol(bgcol_),
	text(text_),
	allow_zoom(allow_zoom_),
	zoomed_in(false)
{
	boost::split(text_lines, text, boost::is_any_of("\n"));
	change_production(0);
}

production_window::~production_window()
{
}

int production_window::zoom_to_city()
{
	add_subwindow(new city_window(screen, data, res, c,
				NULL, myciv));
	zoomed_in = true;
	return 0;
}

int production_window::choose_unit_production(const std::pair<int, unit_configuration>& u)
{
	c->set_unit_production(u.first);
	return 1;
}

int production_window::choose_improv_production(const std::pair<unsigned int, city_improvement>& i)
{
	c->set_improv_production(i.first);
	return 1;
}

int production_window::draw_window()
{
	if(zoomed_in)
		return 0;

	// background
	draw_plain_rectangle(screen, bg.x, bg.y, bg.w, bg.h, bgcol);

	// text
	int text_y = bg.y + 10;
	for(std::vector<std::string>::const_iterator it = text_lines.begin();
			it != text_lines.end();
			++it) {
		if(!it->empty())
			draw_text(screen, &res.font, it->c_str(), bg.x + 10, text_y, 0, 0, 0);
		text_y += 14;
	}

	// production choice buttons
	std::for_each(change_prod_buttons.begin(),
			change_prod_buttons.end(),
			std::bind2nd(std::mem_fun(&button::draw), screen));

	// final flip
	if(SDL_Flip(screen)) {
		fprintf(stderr, "Unable to flip: %s\n", SDL_GetError());
	}
	return 0;
}

int production_window::change_production(int num)
{
	const int button_dist_y = 46;
	const int button_height = 38;
	rect option_rect = rect(bg.x + 10, bg.y + 30 + (text_lines.size() - 1) * 14, bg.w - 40, button_height);
	int listed = 0;
	change_prod_buttons.clear();
	if(allow_zoom) {
		change_prod_buttons.push_back(new plain_button(option_rect,
					"Zoom to city", &res.font, color(200, 200, 200), color(0, 0, 0),
					boost::bind(&production_window::zoom_to_city, this)));
		option_rect.y += button_dist_y;
		listed++;
	}
	if(num) {
		change_prod_buttons.push_back(new plain_button(option_rect,
					"back", &res.font, color(128, 128, 128), color(0, 0, 0),
					boost::bind(&production_window::change_production, this, 0)));
		option_rect.y += button_dist_y;
	}
	for(unit_configuration_map::const_iterator it = data.r.uconfmap.begin();
			it != data.r.uconfmap.end();
			++it) {
		if(!myciv->can_build_unit(it->second, *c))
			continue;
		if(num > listed++) {
			continue;
		}
		if(option_rect.y + 2 * button_dist_y > bg.y + bg.h - 20) {
			change_prod_buttons.push_back(new plain_button(option_rect,
						"more", &res.font, color(128, 128, 128), color(0, 0, 0),
						boost::bind(&production_window::change_production, this, listed - 1)));
			return 0;
		}
		char buf[256];
		buf[255] = '\0';
		if(it->second.max_strength)
			snprintf(buf, 255, "%s - Strength: %d (Cost: %d)", it->second.unit_name.c_str(),
					it->second.max_strength,
					it->second.production_cost);
		else
			snprintf(buf, 255, "%s (Cost: %d)", it->second.unit_name.c_str(),
					it->second.production_cost);
		change_prod_buttons.push_back(new plain_button(option_rect,
				buf, &res.font, color(160, 160, 160), color(0, 0, 0),
				boost::bind(&production_window::choose_unit_production, this, *it)));
		option_rect.y += button_dist_y;
	}
	for(city_improv_map::const_iterator it = data.r.cimap.begin();
			it != data.r.cimap.end();
			++it) {
		if(!myciv->can_build_improvement(it->second, *c))
			continue;
		if(num > listed++) {
			continue;
		}
		if(option_rect.y + 2 * button_dist_y > bg.y + bg.h - 20) {
			change_prod_buttons.push_back(new plain_button(option_rect,
						"more", &res.font, color(128, 128, 128), color(0, 0, 0),
						boost::bind(&production_window::change_production, this, listed - 1)));
			return 0;
		}
		char buf[256];
		buf[255] = '\0';
		snprintf(buf, 255, "%s (Cost: %d)", it->second.improv_name.c_str(),
				it->second.cost);
		change_prod_buttons.push_back(new plain_button(option_rect,
				buf, &res.font, color(200, 200, 200), color(0, 0, 0),
				boost::bind(&production_window::choose_improv_production, this, *it)));
		option_rect.y += button_dist_y;
	}
	return 0;
}

int production_window::handle_window_input(const SDL_Event& ev)
{
	if(zoomed_in && num_subwindows() == 0) {
		// zoomed window closed
		return 1;
	}
	if(ev.type == SDL_MOUSEBUTTONDOWN) {
		return check_button_click(change_prod_buttons, ev);
	}
	else if(ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
		return 1;
	}
	return 0;
}

