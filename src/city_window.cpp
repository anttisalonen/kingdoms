#include "city_window.h"

#include <boost/bind.hpp>

city_window::city_window(SDL_Surface* screen_, int x, int y, gui_data& data_, gui_resources& res_, city* c_,
		ai* ai_, civilization* myciv_)
	: window(screen_, x, y, data_, res_),
	c(c_),
	myciv(myciv_),
	internal_ai(ai_)
{
	buttons.push_back(new plain_button(rect(screen_w * 0.30, screen_h * 0.1, screen_w * 0.40, screen_h * 0.08),
				c->cityname.c_str(), &res.font, color(200, 200, 200), color(0, 0, 0),
				NULL));
	buttons.push_back(new plain_button(rect(screen_w * 0.75, screen_h * 0.8, screen_w * 0.15, screen_h * 0.08),
				"Exit", &res.font, color(128, 60, 60), color(0, 0, 0),
			       	boost::bind(&city_window::on_exit, this)));
	buttons.push_back(new plain_button(rect(screen_w * 0.75, screen_h * 0.6, screen_w * 0.15, screen_h * 0.08),
				"Change production", &res.font, color(128, 60, 60), color(0, 0, 0),
				boost::bind(&city_window::change_production, this)));

	// create "buttons" for unit icons
	rect unit_box = rect(screen_w * 0.8, screen_h * 0.1, screen_w * 0.4, screen_h * 0.4);
	int unit_x = unit_box.x;
	int unit_y = unit_box.y;
	rect unit_coord = rect(unit_x, unit_y, res.terrains.tile_w, res.terrains.tile_h);
	for(std::map<unsigned int, unit*>::const_iterator uit = data.r.civs[c->civ_id]->units.begin();
			uit != data.r.civs[c->civ_id]->units.end();
			++uit) {
		unit* u = uit->second;
		if(u->xpos == c->xpos && u->ypos == c->ypos) {
			SDL_Surface* unit_tile = res.get_unit_tile(*u,
					data.r.civs[u->civ_id]->col);

			buttons.push_back(new texture_button(unit_coord, 
						unit_tile,
						boost::bind(&city_window::on_unit, this, u)));
			unit_coord.x += unit_coord.w;
			if(unit_coord.x + unit_coord.w >= unit_box.w + unit_box.w) {
				unit_coord.x = unit_box.x;
				unit_coord.y += unit_coord.h;
				if(unit_coord.y + unit_coord.h >= unit_box.h + unit_box.h)
					break;
			}
		}
	}
}

city_window::~city_window()
{
	while(!buttons.empty()) {
		delete buttons.back();
		buttons.pop_back();
	}
}

int city_window::change_production()
{
	if(internal_ai)
		return 0;
	rect option_rect = rect(screen_w * 0.30, screen_h * 0.1, screen_w * 0.40, screen_h * 0.08);
	for(unit_configuration_map::const_iterator it = data.r.uconfmap.begin();
			it != data.r.uconfmap.end();
			++it) {
		if(!myciv->can_build_unit(it->second, *c))
			continue;
		change_prod_buttons.push_back(new plain_button(option_rect,
				it->second.unit_name.c_str(), &res.font, color(128, 128, 128), color(0, 0, 0),
				boost::bind(&city_window::choose_unit_production, this, *it)));
		option_rect.y += screen_h * 0.09;
	}
	for(city_improv_map::const_iterator it = data.r.cimap.begin();
			it != data.r.cimap.end();
			++it) {
		if(!myciv->can_build_improvement(it->second, *c))
			continue;
		change_prod_buttons.push_back(new plain_button(option_rect,
				it->second.improv_name.c_str(), &res.font, color(160, 160, 160), color(0, 0, 0),
				boost::bind(&city_window::choose_improv_production, this, *it)));
		option_rect.y += screen_h * 0.09;
	}
	return 0;
}

int city_window::choose_unit_production(const std::pair<int, unit_configuration>& u)
{
	c->set_unit_production(u.first);
	return 1;
}

int city_window::choose_improv_production(const std::pair<unsigned int, city_improvement>& i)
{
	c->set_improv_production(i.first);
	return 1;
}

int city_window::on_exit()
{
	return 1;
}

int city_window::on_unit(unit* u)
{
	if(!internal_ai)
		u->wake_up();
	return 0;
}

int city_window::draw_city_resources_screen(int xpos, int ypos)
{
	// draw terrain
	for(int i = -2; i <= 2; i++) {
		int x = xpos + (i + 2) * res.terrains.tile_w;
		for(int j = -2; j <= 2; j++) {
			int y = ypos + (j + 2) * res.terrains.tile_h;
			if(abs(i) != 2 || abs(j) != 2) {
				int xp = data.m.wrap_x(c->xpos + i);
				int yp = data.m.wrap_y(c->ypos + j);
				if(in_bounds(0, xp, data.m.size_x() - 1) &&
				   in_bounds(0, yp, data.m.size_y() - 1) && myciv->fog_at(xp, yp)) {
					draw_terrain_tile(xp, yp, x, y, false,
							data.m, res.terrains, screen);
				}
			}
		}
	}

	// draw the city itself
	draw_image(xpos + res.terrains.tile_w * 2,
		   ypos + res.terrains.tile_h * 2,
		   res.city_images[c->civ_id], screen);

	// draw resources and boxes on resource coords
	const std::list<coord>& resource_coords = c->get_resource_coords();
	for(std::list<coord>::const_iterator it = resource_coords.begin();
			it != resource_coords.end();
			++it) {
		int tile_x = xpos + res.terrains.tile_w * (it->x + 2);
		int tile_y = ypos + res.terrains.tile_h * (it->y + 2);

		draw_rect(tile_x, tile_y,
			  res.terrains.tile_w, res.terrains.tile_h, color(255, 255, 255),
			  1, screen);

		int tile_xcoord = data.m.wrap_x(c->xpos + it->x);
		int tile_ycoord = data.m.wrap_y(c->ypos + it->y);
		int food, prod, comm;
		data.m.get_resources_on_spot(tile_xcoord, tile_ycoord,
				&food, &prod, &comm);
		for(int i = 0; i < food; i++)
			draw_image(tile_x + i * res.terrains.tile_w / (food * 2),
				   tile_y, res.food_icon, screen);
		for(int i = 0; i < prod; i++)
			draw_image(tile_x + i * res.terrains.tile_w / (prod * 2),
				   tile_y + 8, res.prod_icon, screen);
		for(int i = 0; i < comm; i++)
			draw_image(tile_x + i * res.terrains.tile_w / (comm * 2),
				   tile_y + 16, res.comm_icon, screen);
	}

	return 0;
}

int city_window::draw_window()
{
	// background
	draw_plain_rectangle(screen, screen_w * 0.05f,
			screen_h * 0.05f,
			screen_w * 0.90f,
			screen_h * 0.90f, color(255, 255, 255));

	// buttons (including residing units)
	std::for_each(buttons.begin(),
			buttons.end(),
			std::bind2nd(std::mem_fun(&button::draw), screen));

	// city resources screen
	draw_city_resources_screen(screen_w * 0.3, screen_h * 0.2);

	// statistics
	int food, prod, comm;
	total_resources(*c, data.m, &food, &prod, &comm);
	char buf[64];
	buf[63] = '\0';
	snprintf(buf, 63, "Food: %d/turn (Total: %d)", food, c->stored_food);
	draw_text(screen, &res.font, buf, screen_w * 0.3, screen_h * 0.60, 0, 0, 0);
	{
		const char* prod_tgt = NULL;
		int prod_cost = 0;
		if(c->production.producing_unit) {
			unit_configuration_map::const_iterator it = data.r.uconfmap.find(c->production.current_production_id);
			if(it != data.r.uconfmap.end()) {
				prod_tgt = it->second.unit_name.c_str();
				prod_cost = it->second.production_cost;
			}
		}
		else {
			city_improv_map::const_iterator it = data.r.cimap.find(c->production.current_production_id);
			if(it != data.r.cimap.end()) {
				prod_tgt = it->second.improv_name.c_str();
				prod_cost = it->second.cost;
			}
		}
		if(!prod_tgt) {
			snprintf(buf, 63, "Production: %d per turn - not producing", prod);
		}
		else {
			snprintf(buf, 63, "Production: %d/turn - %s - %d/%d", 
					prod, prod_tgt,
					c->stored_prod, 
					prod_cost);
		}
		draw_text(screen, &res.font, buf, screen_w * 0.3, screen_h * 0.66, 0, 0, 0);
	}
	snprintf(buf, 63, "Commerce: %d/turn", comm);
	draw_text(screen, &res.font, buf, screen_w * 0.3, screen_h * 0.72, 0, 0, 0);
	snprintf(buf, 63, "City Size: %d", c->get_city_size());
	draw_text(screen, &res.font, buf, screen_w * 0.3, screen_h * 0.78, 0, 0, 0);
	snprintf(buf, 63, "Culture: %d (Level %d)", c->accum_culture, 
			c->culture_level);
	draw_text(screen, &res.font, buf, screen_w * 0.3, screen_h * 0.84, 0, 0, 0);

	// built city improvements
	int improv_y = screen_w * 0.05 + 10;
	for(std::set<unsigned int>::const_iterator it = c->built_improvements.begin();
			it != c->built_improvements.end();
			++it) {
		city_improv_map::const_iterator ciit = data.r.cimap.find(*it);
		if(ciit != data.r.cimap.end()) {
			snprintf(buf, 63, "%s", ciit->second.improv_name.c_str());
		}
		else {
			snprintf(buf, 63, "<unknown>");
		}
		draw_text(screen, &res.font, buf, screen_w * 0.10, improv_y, 0, 0, 0);
		improv_y += 20;
	}

	// production choice buttons if any
	std::for_each(change_prod_buttons.begin(),
			change_prod_buttons.end(),
			std::bind2nd(std::mem_fun(&button::draw), screen));

	// final flip
	if(SDL_Flip(screen)) {
		fprintf(stderr, "Unable to flip: %s\n", SDL_GetError());
	}
	return 0;
}

int city_window::handle_production_input(const SDL_Event& ev)
{
	if(ev.type == SDL_MOUSEBUTTONDOWN) {
		return check_button_click(change_prod_buttons, ev);
	}
	return 0;
}

int city_window::handle_window_input(const SDL_Event& ev)
{
	if(change_prod_buttons.size() == 0) {
		switch(ev.type) {
			case SDL_KEYDOWN:
				return handle_keydown(ev.key.keysym.sym, ev.key.keysym.mod);
			case SDL_MOUSEBUTTONDOWN:
				return handle_mousedown(ev);
			default:
				return 0;
		}
	}
	else {
		if(handle_production_input(ev)) {
			while(!change_prod_buttons.empty()) {
				delete change_prod_buttons.back();
				change_prod_buttons.pop_back();
			}
		}
		return 0;
	}
}

int city_window::handle_mousedown(const SDL_Event& ev)
{
	return check_button_click(buttons, ev);
}

int city_window::handle_keydown(SDLKey k, SDLMod mod)
{
	if(k == SDLK_ESCAPE || k == SDLK_RETURN || k == SDLK_KP_ENTER)
		return 1;
	return 0;
}


