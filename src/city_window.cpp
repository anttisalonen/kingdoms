#include "city_window.h"

#include <boost/bind.hpp>

city_window::city_window(SDL_Surface* screen_, int x, int y, gui_data& data_, gui_resources& res_, city* c_)
	: screen(screen_),
	screen_w(x),
	screen_h(y),
	data(data_),
	res(res_),
	c(c_)
{
	rect name_rect = rect(screen_w * 0.30, screen_h * 0.1, screen_w * 0.40, screen_h * 0.08);
	rect exit_rect = rect(screen_w * 0.75, screen_h * 0.8, screen_w * 0.15, screen_h * 0.08);
	rect change_prod_rect = rect(screen_w * 0.75, screen_h * 0.6, screen_w * 0.15, screen_h * 0.08);
	label_surf = make_label(c->cityname, &res.font, name_rect.w, name_rect.h, color(200, 200, 200), color(0, 0, 0));
	button_surf = make_label("Exit", &res.font, exit_rect.w, exit_rect.h, color(128, 60, 60), color(0, 0, 0));
	change_prod_surf = make_label("Change production", &res.font, change_prod_rect.w, 
			change_prod_rect.h, color(128, 60, 60), color(0, 0, 0));
	buttons.push_back(new button(name_rect,
				label_surf, NULL));
	buttons.push_back(new button(exit_rect,
				button_surf, boost::bind(&city_window::on_exit, this)));
	buttons.push_back(new button(change_prod_rect,
				change_prod_surf, boost::bind(&city_window::change_production, this)));

	// create "buttons" for unit icons
	rect unit_box = rect(screen_w * 0.5, screen_h * 0.1, screen_w * 0.4, screen_h * 0.4);
	int unit_x = unit_box.x;
	int unit_y = unit_box.y;
	rect unit_coord = rect(unit_x, unit_y, res.terrains.tile_w, res.terrains.tile_h);
	for(std::list<unit*>::const_iterator uit = data.r.civs[c->civ_id]->units.begin();
			uit != data.r.civs[c->civ_id]->units.end();
			++uit) {
		if((*uit)->xpos == c->xpos && (*uit)->ypos == c->ypos) {
			SDL_Surface* unit_tile = res.get_unit_tile(**uit,
					data.r.civs[(*uit)->civ_id]->col);
			unit_tiles.push_back(unit_tile);

			buttons.push_back(new button(unit_coord, 
						unit_tile,
						boost::bind(&city_window::on_unit, this, *uit)));
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
	while(!unit_tiles.empty()) {
		SDL_FreeSurface(unit_tiles.back());
		unit_tiles.pop_back();
	}
	while(!buttons.empty()) {
		delete buttons.back();
		buttons.pop_back();
	}
	SDL_FreeSurface(change_prod_surf);
	SDL_FreeSurface(button_surf);
	SDL_FreeSurface(label_surf);
}

int city_window::change_production()
{
	rect option_rect = rect(screen_w * 0.30, screen_h * 0.1, screen_w * 0.40, screen_h * 0.08);
	for(unit_configuration_map::const_iterator it = data.r.uconfmap.begin();
			it != data.r.uconfmap.end();
			++it) {
		SDL_Surface* button_surf = make_label(it->second->unit_name, 
				&res.font, option_rect.w, option_rect.h, color(128, 128, 128), color(0, 0, 0));
		change_prod_labels.push_back(button_surf);
		change_prod_buttons.push_back(new button(option_rect,
				button_surf, boost::bind(&city_window::choose_production, this, *it)));
		option_rect.y += screen_h * 0.09;
	}
	return 0;
}

int city_window::choose_production(const std::pair<int, unit_configuration*>& u)
{
	c->current_production_unit_id = u.first;
	return 1;
}

int city_window::on_exit()
{
	return 1;
}

int city_window::on_unit(unit* u)
{
	u->fortified = false;
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
				int xp = c->xpos + i;
				int yp = c->ypos + j;
				if(in_bounds(0, xp, data.m.size_x() - 1) &&
				   in_bounds(0, yp, data.m.size_y() - 1) && (*data.r.current_civ)->fog_at(xp, yp)) {
					draw_terrain_tile(c->xpos + i, c->ypos + j, x, y, false,
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
	for(std::list<coord>::const_iterator it = c->resource_coords.begin();
			it != c->resource_coords.end();
			++it) {
		int tile_x = xpos + res.terrains.tile_w * (it->x + 2);
		int tile_y = ypos + res.terrains.tile_h * (it->y + 2);

		draw_rect(tile_x, tile_y,
			  res.terrains.tile_w, res.terrains.tile_h, color(255, 255, 255),
			  1, screen);

		int tile_xcoord = c->xpos + it->x;
		int tile_ycoord = c->ypos + it->y;
		int food, prod, comm;
		data.m.get_resources_by_terrain(data.m.get_data(tile_xcoord,
				tile_ycoord), it->x == 0 && it->y == 0, &food,
				&prod, &comm);
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

int city_window::draw()
{
	SDL_Rect rect;
	rect.x = screen_w * 0.05f;
	rect.y = screen_h * 0.05f;
	rect.w = screen_w * 0.90f;
	rect.h = screen_h * 0.90f;
	Uint32 bgcol = SDL_MapRGB(screen->format, 255, 255, 255);

	// background
	SDL_FillRect(screen, &rect, bgcol);

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
		unit_configuration_map::const_iterator it = data.r.uconfmap.find(c->current_production_unit_id);
		if(it == data.r.uconfmap.end()) {
			snprintf(buf, 63, "Production: %d per turn - not producing", prod);
		}
		else {
			snprintf(buf, 63, "Production: %d/turn - %s - %d/%d", 
					prod, it->second->unit_name, 
					c->stored_prod, 
					it->second->production_cost);
		}
		draw_text(screen, &res.font, buf, screen_w * 0.3, screen_h * 0.70, 0, 0, 0);
	}
	snprintf(buf, 63, "Commerce: %d/turn", comm);
	draw_text(screen, &res.font, buf, screen_w * 0.3, screen_h * 0.80, 0, 0, 0);

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

int city_window::handle_input(const SDL_Event& ev)
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
			while(!change_prod_labels.empty()) {
				SDL_FreeSurface(change_prod_labels.back());
				change_prod_labels.pop_back();
			}
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


