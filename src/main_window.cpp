#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "main_window.h"
#include "map-astar.h"
#include "city_window.h"
#include "diplomacy_window.h"
#include "serialize.h"

main_window::main_window(SDL_Surface* screen_, int x, int y, gui_data& data_, gui_resources& res_,
		ai* ai_, civilization* myciv_)
	: window(screen_, x, y, data_, res_),
	tile_w(32),
	tile_h(32),
	cam_total_tiles_x((screen_w + tile_w - 1) / tile_w),
	cam_total_tiles_y((screen_h + tile_h - 1) / tile_h),
	sidebar_size(4),
	current_unit(myciv_->units.end()),
	blink_unit(false),
	timer(0),
	myciv(myciv_),
	mouse_down_sqx(-1),
	mouse_down_sqy(-1),
	internal_ai(ai_)
{
	cam.cam_x = cam.cam_y = 0;
}

main_window::~main_window()
{
}

void main_window::get_next_free_unit()
{
	if(myciv->units.empty())
		return;
	std::map<unsigned int, unit*>::const_iterator uit = current_unit;
	for(++current_unit;
			current_unit != myciv->units.end();
			++current_unit) {
		if(current_unit->second->idle()) {
			try_center_camera_to_unit(current_unit->second);
			return;
		}
	}

	// run through the first half
	for(current_unit = myciv->units.begin();
			current_unit != uit;
			++current_unit) {
		if(current_unit->second->idle()) {
			try_center_camera_to_unit(current_unit->second);
			return;
		}
	}
	current_unit = myciv->units.end();
}

int main_window::draw_window()
{
	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0) {
			return 1;
		}
	}
	draw_sidebar();
	clear_main_map();
	draw_main_map();
	if (SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}
	if(SDL_Flip(screen)) {
		fprintf(stderr, "Unable to flip: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

int main_window::clear_main_map() const
{
	SDL_Rect dest;
	dest.x = sidebar_size * tile_w;
	dest.y = 0;
	dest.w = screen_w - sidebar_size * tile_w;
	dest.h = screen_h;
	Uint32 color = SDL_MapRGB(screen->format, 0, 0, 0);
	SDL_FillRect(screen, &dest, color);
	return 0;
}

int main_window::clear_sidebar() const
{
	SDL_Rect dest;
	dest.x = 0;
	dest.y = 0;
	dest.w = sidebar_size * tile_w;
	dest.h = screen_h;
	Uint32 color = SDL_MapRGB(screen->format, 0, 0, 0);
	SDL_FillRect(screen, &dest, color);
	return 0;
}

int main_window::draw_sidebar() const
{
	clear_sidebar();
	draw_minimap();
	draw_civ_info();
	if(!internal_ai && current_unit != myciv->units.end())
		draw_unit_info();
	else
		draw_eot();
	return 0;
}

int main_window::draw_minimap() const
{
	const int minimap_w = sidebar_size * tile_w;
	const int minimap_h = sidebar_size * tile_h / 2;
	for(int i = 0; i < minimap_h; i++) {
		int y = i * data.m.size_y() / minimap_h;
		for(int j = 0; j < minimap_w; j++) {
			int x = j * data.m.size_x() / minimap_w;
			color c = get_minimap_color(x, y);
			if((c.r == 255 && c.g == 255 && c.b == 255) || 
					internal_ai || 
					myciv->fog_at(x, y) > 0) {
				sdl_put_pixel(screen, j, i, c);
			}
		}
	}
	SDL_UpdateRect(screen, 0, 0, minimap_w, minimap_h);

	return 0;
}

int main_window::draw_civ_info() const
{
	draw_text(screen, &res.font, myciv->civname.c_str(), 10, sidebar_size * tile_h / 2 + 40, 255, 255, 255);
	char buf[256];
	buf[255] = '\0';
	snprintf(buf, 255, "Gold: %d", myciv->gold);
	draw_text(screen, &res.font, buf, 10, sidebar_size * tile_h / 2 + 60, 255, 255, 255);
	int lux = 10 - myciv->alloc_gold - myciv->alloc_science;
	snprintf(buf, 255, "%d/%d/%d", 
			myciv->alloc_gold * 10,
			myciv->alloc_science * 10,
			lux);
	draw_text(screen, &res.font, buf, 10, sidebar_size * tile_h / 2 + 80, 255, 255, 255);
	return 0;
}

int main_window::draw_unit_info() const
{
	if(current_unit == myciv->units.end())
		return 0;
	const unit_configuration* uconf = data.r.get_unit_configuration((current_unit->second)->uconf_id);
	if(!uconf)
		return 1;
	draw_text(screen, &res.font, uconf->unit_name.c_str(), 10, sidebar_size * tile_h / 2 + 100, 255, 255, 255);
	char buf[256];
	buf[255] = '\0';
	snprintf(buf, 255, "Moves: %-2d/%2d", current_unit->second->num_moves(), uconf->max_moves);
	draw_text(screen, &res.font, buf, 10, sidebar_size * tile_h / 2 + 120, 255, 255, 255);
	if(current_unit->second->strength) {
		snprintf(buf, 255, "Unit strength:");
		draw_text(screen, &res.font, buf, 10, sidebar_size * tile_h / 2 + 140, 255, 255, 255);
		if(current_unit->second->strength % 10) {
			snprintf(buf, 255, "%d.%d/%d", current_unit->second->strength / 10, current_unit->second->strength % 10,
					uconf->max_strength);
		}
		else {
			snprintf(buf, 255, "%d/%d", current_unit->second->strength / 10,
					uconf->max_strength);
		}
		draw_text(screen, &res.font, buf, 10, sidebar_size * tile_h / 2 + 160, 255, 255, 255);
	}
	return 0;
}

int main_window::draw_eot() const
{
	return draw_text(screen, &res.font, "End of turn", 10, screen_h - 100, 255, 255, 255);
}

int main_window::draw_tile(const SDL_Surface* surf, int x, int y) const
{
	if(tile_visible(x, y)) {
		return draw_image(tile_xcoord_to_pixel(x),
				tile_ycoord_to_pixel(y),
				surf, screen);
	}
	return 0;
}

int main_window::draw_city(const city& c) const
{
	if(!tile_visible(c.xpos, c.ypos))
		return 0;

	if(draw_tile(res.city_images[c.civ_id], c.xpos, c.ypos))
		return 1;
	if(!tile_visible(c.xpos + 1, c.ypos) ||
	   !tile_visible(c.xpos - 1, c.ypos))
		return 0;
	char buf[64];
	snprintf(buf, 63, "%d %s", c.get_city_size(),
			c.cityname.c_str());
	buf[63] = '\0';
	return draw_text(screen, &res.font, buf, tile_xcoord_to_pixel(c.xpos) + tile_h / 2,
			tile_ycoord_to_pixel(c.ypos) + tile_w,
			255, 255, 255, true);
}

int main_window::tile_ycoord_to_pixel(int y) const
{
	return data.m.wrap_y(y - cam.cam_y) * tile_h;
}

int main_window::tile_xcoord_to_pixel(int x) const
{
	return data.m.wrap_x(x + sidebar_size - cam.cam_x) * tile_w;
}

int main_window::tile_visible(int x, int y) const
{
	bool vis_no_wrap = (in_bounds(cam.cam_x, x, cam.cam_x + cam_total_tiles_x) &&
			in_bounds(cam.cam_y, y, cam.cam_y + cam_total_tiles_y));
	if(vis_no_wrap)
		return vis_no_wrap;
	else // handle X wrapping
		return (in_bounds(cam.cam_x, x + data.m.size_x(), cam.cam_x + cam_total_tiles_x) &&
			in_bounds(cam.cam_y, y, cam.cam_y + cam_total_tiles_y));
}

int main_window::draw_line_by_sq(const coord& c1, const coord& c2, int r, int g, int b)
{
	coord start(tile_xcoord_to_pixel(c1.x) + tile_w / 2,
			tile_ycoord_to_pixel(c1.y) + tile_h / 2);
	coord end(tile_xcoord_to_pixel(c2.x) + tile_w / 2,
			tile_ycoord_to_pixel(c2.y) + tile_h / 2);
	draw_line(screen, start.x, start.y, end.x, end.y, color(r, g, b));
	return 0;
}

int main_window::draw_unit(const unit* u)
{
	if(!tile_visible(u->xpos, u->ypos)) {
		return 0;
	}
	if(u->carried() && (current_unit == myciv->units.end() || current_unit->second != u)) {
		return 0;
	}
	SDL_Surface* surf = res.get_unit_tile(*u, data.r.civs[u->civ_id]->col);
	return draw_tile(surf, u->xpos, u->ypos);
}

int main_window::draw_complete_tile(int x, int y, int shx, int shy, bool terrain,
		bool improvements, bool borders,
		boost::function<bool(const unit*)> unit_predicate,
		bool cities)
{
	char fog = myciv->fog_at(x, y);
	if(fog == 0 && !internal_ai)
		return 0;
	if(terrain) {
		if(show_terrain_image(x, y, shx, shy, improvements, !internal_ai && fog == 1))
			return 1;
	}
	if(borders) {
		if(test_draw_border(x, y, shx, shy))
			return 1;
	}
	const std::list<unit*>& units = data.m.units_on_spot(x, y);
	for(std::list<unit*>::const_iterator it = units.begin();
			it != units.end(); ++it) {
		if((internal_ai || fog == 2) && unit_predicate(*it)) {
			if(draw_unit(*it))
				return 1;
		}
	}
	if(cities) {
		city* c = data.m.city_on_spot(x, y);
		if(c) {
			if(draw_city(*c))
				return 1;
		}
	}
	return 0;
}

bool main_window::draw_gui_unit(const unit* u) const
{
	if(current_unit == myciv->units.end())
	       return true;
	else
		return u != current_unit->second &&
			(u->xpos != current_unit->second->xpos ||
			 u->ypos != current_unit->second->ypos);
}

int main_window::draw_main_map()
{
	int imax = cam.cam_y + cam_total_tiles_y;
	int jmax = cam.cam_x + cam_total_tiles_x;
	// bottom-up in y dimension for correct rendering of city names
	for(int i = imax - 1, y = cam_total_tiles_y - 1; i >= cam.cam_y; i--, y--) {
		for(int j = cam.cam_x, x = sidebar_size; j < jmax; j++, x++) {
			if(draw_complete_tile(data.m.wrap_x(j),
						data.m.wrap_y(i),
						x, y,
						true, true, true,
						boost::bind(&main_window::draw_gui_unit,
							this, boost::lambda::_1),
						true))
				return 1;
		}
	}

	if(!internal_ai && current_unit != myciv->units.end() && !blink_unit) {
		if(draw_unit(current_unit->second))
			return 1;
	}
	if(!path_to_draw.empty()) {
		std::list<coord>::const_iterator cit = path_to_draw.begin();
		std::list<coord>::const_iterator cit2 = path_to_draw.begin();
		cit2++;
		while(cit2 != path_to_draw.end()) {
			draw_line_by_sq(*cit, *cit2, 255, 255, 255);
			cit++;
			cit2++;
		}
	}
	return 0;
}

int main_window::show_terrain_image(int x, int y, int xpos, int ypos,
		bool draw_improvements, bool shade)
{
	return draw_terrain_tile(x, y, xpos * tile_w, ypos * tile_h, shade,
			data.m, res.terrains, draw_improvements, screen);
}

int main_window::test_draw_border(int x, int y, int xpos, int ypos)
{
	int this_owner = data.m.get_land_owner(x, y);
	int w_owner = data.m.get_land_owner(x - 1, y);
	int e_owner = data.m.get_land_owner(x + 1, y);
	int n_owner = data.m.get_land_owner(x,     y - 1);
	int s_owner = data.m.get_land_owner(x,     y + 1);
	for(int i = 0; i < 4; i++) {
		int oth, sx, sy, ex, ey;
		switch(i) {
			case 0:
				oth = w_owner;
				sx = 0; sy = 0; ex = 0; ey = 1;
				break;
			case 1:
				oth = e_owner;
				sx = 1; sy = 0; ex = 1; ey = 1;
				break;
			case 2:
				oth = n_owner;
				sx = 0; sy = 0; ex = 1; ey = 0;
				break;
			default:
				oth = s_owner;
				sx = 0; sy = 1; ex = 1; ey = 1;
				break;
		}
		if(this_owner != oth) {
			const color& col = this_owner == -1 ? data.r.civs[oth]->col :
				data.r.civs[this_owner]->col;
			draw_line(screen, (xpos + sx) * tile_w, (ypos + sy) * tile_h, 
					(xpos + ex) * tile_w, (ypos + ey) * tile_h,
					col);
		}
	}
	return 0;
}

color main_window::get_minimap_color(int x, int y) const
{
	// minimap rectangle - no wrapping
	if((x >= cam.cam_x && x <= cam.cam_x + cam_total_tiles_x - 1) &&
	   (y >= cam.cam_y && y <= cam.cam_y + cam_total_tiles_y - 1) &&
	   (x == cam.cam_x || x == cam.cam_x + cam_total_tiles_x - 1 ||
	    y == cam.cam_y || y == cam.cam_y + cam_total_tiles_y - 1))
		return color(255, 255, 255);

	// minimap rectangle - wrapped in X
	if((cam.cam_x > data.m.wrap_x(cam.cam_x + cam_total_tiles_x - 1) && 
	    data.m.wrap_x(x) <= data.m.wrap_x(cam.cam_x + cam_total_tiles_x - 1)) &&
	   (y >= cam.cam_y && y <= cam.cam_y + cam_total_tiles_y - 1) &&
	   (x == cam.cam_x || data.m.wrap_x(x) == data.m.wrap_x(cam.cam_x + cam_total_tiles_x - 1) ||
	    y == cam.cam_y || y == cam.cam_y + cam_total_tiles_y - 1))
		return color(255, 255, 255);

	int val = data.m.get_data(x, y);
	if(val < 0 || val >= (int)res.terrains.textures.size()) {
		fprintf(stderr, "Terrain at %d not loaded at (%d, %d)\n", val,
				x, y);
		return color(0, 0, 0);
	}
	return sdl_get_pixel(res.terrains.textures[val], 16, 16);
}

int main_window::try_move_camera(bool left, bool right, bool up, bool down)
{
	bool redraw = false;
	if(left) {
		if(cam.cam_x > 0 || data.m.x_wrapped()) {
			cam.cam_x = data.m.wrap_x(cam.cam_x - 1);
			redraw = true;
		}
	}
	else if(right) {
		if(cam.cam_x < data.m.size_x() - cam_total_tiles_x || data.m.x_wrapped()) {
			cam.cam_x = data.m.wrap_x(cam.cam_x + 1);
			redraw = true;
		}
	}
	if(up) {
		if(cam.cam_y > 0 || data.m.y_wrapped()) {
			cam.cam_y = data.m.wrap_y(cam.cam_y - 1);
			redraw = true;
		}
	}
	else if(down) {
		if(cam.cam_y < data.m.size_y() - cam_total_tiles_y || data.m.y_wrapped()) {
			cam.cam_y = data.m.wrap_y(cam.cam_y + 1);
			redraw = true;
		}
	}
	if(redraw) {
		draw();
	}
	return redraw;
}

void main_window::center_camera_to_unit(unit* u)
{
	cam.cam_x = clamp(0, data.m.wrap_x(u->xpos - (-sidebar_size + cam_total_tiles_x) / 2), 
			data.m.size_x() - (data.m.x_wrapped() ? 0 : cam_total_tiles_x));
	cam.cam_y = clamp(0, data.m.wrap_y(u->ypos - cam_total_tiles_y / 2), 
			data.m.size_y() - (data.m.y_wrapped() ? 0 : cam_total_tiles_y));
}

int main_window::try_center_camera_to_unit(unit* u)
{
	const int border = 3;
	if(!in_bounds(cam.cam_x + border, u->xpos, cam.cam_x - sidebar_size + cam_total_tiles_x - border) ||
	   !in_bounds(cam.cam_y + border, u->ypos, cam.cam_y + cam_total_tiles_y - border)) {
		center_camera_to_unit(u);
		return true;
	}
	return false;
}

int main_window::process(int ms)
{
	if(internal_ai)
		return 0;
	int old_timer = timer;
	timer += ms;
	bool old_blink_unit = blink_unit;

	if(timer % 1000 < 300) {
		blink_unit = true;
	}
	else {
		blink_unit = false;
	}
	if(blink_unit != old_blink_unit) {
		draw();
	}
	if(old_timer / 200 != timer / 200) {
		int x, y;
		SDL_GetMouseState(&x, &y);
		handle_mousemotion(x, y);
	}
	handle_civ_messages(&myciv->messages);
	return 0;
}

int main_window::handle_mousemotion(int x, int y)
{
	const int border = tile_w;
	try_move_camera(x >= sidebar_size * tile_w && x < sidebar_size * tile_w + border,
			x > screen_w - border,
			y < border,
			y > screen_h - border);
	check_line_drawing(x, y);
	return 0;
}

int main_window::check_line_drawing(int x, int y)
{
	if(current_unit == myciv->units.end())
		return 0;
	if(mouse_down_sqx >= 0) {
		int curr_sqx, curr_sqy;
		mouse_coord_to_tiles(x, y, &curr_sqx, &curr_sqy);
		coord curr(curr_sqx, curr_sqy);
		if(curr_sqx != mouse_down_sqx || curr_sqy != mouse_down_sqy) {
			if(path_to_draw.empty() || 
					path_to_draw.back() != curr) {
				path_to_draw = map_astar(*myciv, *current_unit->second, 
						false,
						coord(current_unit->second->xpos,
							current_unit->second->ypos),
						curr);
				mouse_down_sqx = curr_sqx;
				mouse_down_sqy = curr_sqy;
			}
		}
	}
	return 0;
}

void main_window::numpad_to_move(SDLKey k, int* chx, int* chy) const
{
	*chx = 0; *chy = 0;
	switch(k) {
		case SDLK_KP4:
			*chx = -1;
			break;
		case SDLK_KP6:
			*chx = 1;
			break;
		case SDLK_KP8:
			*chy = -1;
			break;
		case SDLK_KP2:
			*chy = 1;
			break;
		case SDLK_KP1:
			*chx = -1;
			*chy = 1;
			break;
		case SDLK_KP3:
			*chx = 1;
			*chy = 1;
			break;
		case SDLK_KP7:
			*chx = -1;
			*chy = -1;
			break;
		case SDLK_KP9:
			*chx = 1;
			*chy = -1;
			break;
		default:
			break;
	}
}

action main_window::input_to_action(const SDL_Event& ev)
{
	switch(ev.type) {
		case SDL_QUIT:
			return action(action_give_up);
		case SDL_KEYDOWN:
			{
				SDLKey k = ev.key.keysym.sym;
				if(k == SDLK_ESCAPE || k == SDLK_q)
					return action(action_give_up);
				else if((k == SDLK_RETURN || k == SDLK_KP_ENTER) && 
						(current_unit == myciv->units.end() || (ev.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)))) {
					return action(action_eot);
				}
				else if(current_unit != myciv->units.end()) {
					if(k == SDLK_b) {
						return unit_action(action_found_city, current_unit->second);
					}
					else if(k == SDLK_SPACE) {
						return unit_action(action_skip, current_unit->second);
					}
					else if(k == SDLK_i) {
						return improve_unit_action(current_unit->second, improv_irrigation);
					}
					else if(k == SDLK_m) {
						return improve_unit_action(current_unit->second, improv_mine);
					}
					else if(k == SDLK_r) {
						return improve_unit_action(current_unit->second, improv_road);
					}
					else if(k == SDLK_f) {
						return unit_action(action_fortify, current_unit->second);
					}
					else if(k == SDLK_l) {
						return unit_action(action_load, current_unit->second);
					}
					else if(k == SDLK_u) {
						return unit_action(action_unload, current_unit->second);
					}
					else if(k == SDLK_s && (ev.key.keysym.mod & KMOD_CTRL)) {
						printf("Saving.\n");
						save_game(data.r);
						return action_none;
					}
					else {
						int chx, chy;
						numpad_to_move(k, &chx, &chy);
						if(chx || chy) {
							return move_unit_action(current_unit->second, chx, chy);
						}
					}
				}
			}
			break;
		default:
			break;
	}
	return action_none;
}

action main_window::observer_action(const SDL_Event& ev)
{
	switch(ev.type) {
		case SDL_QUIT:
			return action(action_give_up);
		case SDL_KEYDOWN:
			{
				SDLKey k = ev.key.keysym.sym;
				if(k == SDLK_ESCAPE || k == SDLK_q) {
					return action(action_give_up);
				}
				else if(k == SDLK_RETURN || k == SDLK_KP_ENTER) {
					internal_ai->play();
				}
				else if(k == SDLK_s && (ev.key.keysym.mod & KMOD_CTRL)) {
					printf("Saving.\n");
					save_game(data.r);
					return action_none;
				}
			}
		default:
			break;
	}
	return action_none;
}

void main_window::handle_successful_action(const action& a, city** c)
{
	switch(a.type) {
		case action_eot:
			// end of turn for this civ
			get_next_free_unit();
			break;
		case action_unit_action:
			switch(a.data.unit_data.uatype) {
				case action_move_unit:
					if(current_unit != myciv->units.end()) {
						if(current_unit->second->num_moves() == 0 && 
								current_unit->second->num_road_moves() == 0) {
							current_unit = myciv->units.end();
						}
					}
					break;
				case action_found_city:
					current_unit = myciv->units.end();
					*c = myciv->cities.rbegin()->second;
					// fall through
				case action_improvement:
				case action_skip:
				case action_fortify:
				case action_load:
				case action_unload:
					get_next_free_unit();
					break;
				default:
					break;
			}
		default:
			break;
	}
}

void main_window::handle_input_gui_mod(const SDL_Event& ev, city** c)
{
	switch(ev.type) {
		case SDL_KEYDOWN:
			{
				SDLKey k = ev.key.keysym.sym;
				if(k == SDLK_LEFT || k == SDLK_RIGHT || k == SDLK_UP || k == SDLK_DOWN) {
					try_move_camera(k == SDLK_LEFT, k == SDLK_RIGHT, k == SDLK_UP, k == SDLK_DOWN);
				}
				if(!internal_ai && current_unit != myciv->units.end()) {
					if(k == SDLK_c) {
						center_camera_to_unit(current_unit->second);
					}
					else if(k == SDLK_w) {
						std::map<unsigned int, unit*>::const_iterator old_it = current_unit;
						get_next_free_unit();
						if(current_unit == myciv->units.end())
							current_unit = old_it;
					}
				}
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			handle_mouse_down(ev, c);
			break;
		case SDL_MOUSEBUTTONUP:
			handle_mouse_up(ev);
			break;
		default:
			break;
	}
}

void main_window::update_view()
{
	blink_unit = true;
	draw();
}

int main_window::handle_window_input(const SDL_Event& ev)
{
	city* c = NULL;
	action a = internal_ai ? observer_action(ev) : input_to_action(ev);
	if(a.type != action_none) {
		// save the iterator - performing an action may destroy
		// the current unit
		bool already_begin = current_unit == myciv->units.begin();
		if(!already_begin) {
			current_unit--;
		}
		bool success = data.r.perform_action(myciv->civ_id, a);
		if(!already_begin) {
			current_unit++;
		}
		else {
			current_unit = myciv->units.begin();
		}
		if(success) {
			handle_successful_action(a, &c);
		}
		else {
			printf("Unable to perform action.\n");
		}
	}
	else {
		handle_input_gui_mod(ev, &c);
	}
	if(!internal_ai && current_unit == myciv->units.end()) {
		get_next_free_unit();
	}
	update_view();
	if(c) {
		add_subwindow(new city_window(screen, screen_w, screen_h, data, res, c,
					internal_ai, myciv));
	}
	return a.type == action_give_up;
}

int main_window::handle_civ_messages(std::list<msg>* messages)
{
	while(!messages->empty()) {
		msg& m = messages->front();
		switch(m.type) {
			case msg_new_unit:
				{
					unit_configuration_map::const_iterator it = data.r.uconfmap.find(m.msg_data.city_prod_data.prod_id);
					if(it != data.r.uconfmap.end()) {
						printf("New unit '%s' produced.\n",
								it->second.unit_name.c_str());
					}
				}
				break;
			case msg_civ_discovery:
				printf("Discovered civilization '%s'.\n",
						data.r.civs[m.msg_data.discovered_civ_id]->civname.c_str());
				add_subwindow(new diplomacy_window(screen, screen_w, screen_h, data, res, myciv,
							m.msg_data.discovered_civ_id));
				break;
			case msg_new_advance:
				{
					unsigned int adv_id = m.msg_data.new_advance_id;
					advance_map::const_iterator it = data.r.amap.find(adv_id);
					if(it != data.r.amap.end()) {
						printf("Discovered advance '%s'.\n",
								it->second.advance_name.c_str());
					}
					it = data.r.amap.find(myciv->research_goal_id);
					if(it != data.r.amap.end()) {
						printf("Now researching '%s'.\n",
								it->second.advance_name.c_str());
					}
				}
				break;
			case msg_new_city_improv:
				{
					city_improv_map::const_iterator it = data.r.cimap.find(m.msg_data.city_prod_data.prod_id);
					if(it != data.r.cimap.end()) {
						printf("New improvement '%s' built.\n",
								it->second.improv_name.c_str());
					}
				}
				break;
			case msg_unit_disbanded:
				printf("Unit disbanded.\n");
				break;
			default:
				printf("Unknown message received: %d\n",
						m.type);
				break;
		}
		messages->pop_front();
	}
	return 0;
}

void main_window::mouse_coord_to_tiles(int mx, int my, int* sqx, int* sqy)
{
	*sqx = (mx - sidebar_size * tile_w) / tile_w;
	*sqy = my / tile_h;
	if(*sqx >= 0) {
		*sqx = data.m.wrap_x(*sqx + cam.cam_x);
		*sqy = data.m.wrap_y(*sqy + cam.cam_y);
	}
	else {
		*sqx = -1;
		*sqy = -1;
	}
}

int main_window::handle_mouse_up(const SDL_Event& ev)
{
	path_to_draw.clear();
	mouse_down_sqx = mouse_down_sqy = -1;
	return 0;
}

int main_window::handle_mouse_down(const SDL_Event& ev, city** c)
{
	path_to_draw.clear();
	mouse_coord_to_tiles(ev.button.x, ev.button.y, &mouse_down_sqx, &mouse_down_sqy);
	if(mouse_down_sqx >= 0)
		try_choose_with_mouse(c);
	return 0;
}

int main_window::try_choose_with_mouse(city** c)
{
	// choose city
	for(std::map<unsigned int, city*>::const_iterator it = myciv->cities.begin();
			it != myciv->cities.end();
			++it) {
		city *cn = it->second;
		if(cn->xpos == mouse_down_sqx && cn->ypos == mouse_down_sqy) {
			*c = cn;
			break;
		}
	}

	// if no city chosen, choose unit
	if(!*c && !internal_ai) {
		for(std::map<unsigned int, unit*>::iterator it = myciv->units.begin();
				it != myciv->units.end();
				++it) {
			unit* u = it->second;
			if(u->xpos == mouse_down_sqx && u->ypos == mouse_down_sqy) {
				u->wake_up();
				if(u->num_moves() > 0 || u->num_road_moves() > 0) {
					current_unit = it;
					blink_unit = false;
				}
			}
		}
	}
	return 0;
}

void main_window::init_turn()
{
	draw_window();
	if(internal_ai) {
		if(data.r.get_round_number() == 0 && myciv->units.begin() != myciv->units.end())
			try_center_camera_to_unit(myciv->units.begin()->second);
		return;
	}
	else {
		if(data.r.get_round_number() == 0) {
			current_unit = myciv->units.begin();
			try_center_camera_to_unit(current_unit->second);
		}
		else {
			get_next_free_unit();
		}
	}
}

bool main_window::unit_not_at(int x, int y, const unit* u) const
{
	return u->xpos != x && u->ypos != y;
}

void main_window::handle_action(const visible_move_action& a)
{
	if(abs(a.change.x) > 1 || abs(a.change.y) > 1)
		return;
	int newx = data.m.wrap_x(a.u->xpos + a.change.x);
	int newy = data.m.wrap_y(a.u->ypos + a.change.y);
	if((myciv->fog_at(a.u->xpos, a.u->ypos) != 2 || myciv->fog_at(newx, newy) != 2) &&
			!internal_ai)
		return;
	if(!tile_visible(a.u->xpos, a.u->ypos) || !tile_visible(newx, newy)) {
		return;
	}
	std::vector<coord> redrawable_tiles;
	redrawable_tiles.push_back(coord(a.u->xpos, a.u->ypos));
	redrawable_tiles.push_back(coord(newx, newy));
	if(a.u->xpos != newx || a.u->ypos != newy) {
		redrawable_tiles.push_back(coord(a.u->xpos, newy));
		redrawable_tiles.push_back(coord(newx, a.u->ypos));
	}
	SDL_Surface* surf = res.get_unit_tile(*a.u, data.r.civs[a.u->civ_id]->col);
	float xpos = tile_xcoord_to_pixel(a.u->xpos);
	float ypos = tile_ycoord_to_pixel(a.u->ypos);
	const int steps = 30;
	float xdiff = tile_w * a.change.x / steps;
	float ydiff = tile_h * a.change.y / steps;
	for(int i = 0; i < steps; i++) {
		for(std::vector<coord>::const_iterator it = redrawable_tiles.begin();
				it != redrawable_tiles.end();
				++it) {
			if(tile_visible(it->x, it->y)) {
				int shx = data.m.wrap_x(it->x - cam.cam_x + sidebar_size);
				int shy = data.m.wrap_y(it->y - cam.cam_y);
				draw_complete_tile(it->x, it->y, shx, shy,
						true, true, true,
						boost::bind(&main_window::unit_not_at, 
							this,
							a.u->xpos, a.u->ypos, 
							boost::lambda::_1), 
						true);
			}
		}
		draw_image((int)xpos, (int)ypos, surf, screen);
		xpos += xdiff;
		ypos += ydiff;
		if(SDL_Flip(screen)) {
			fprintf(stderr, "Unable to flip: %s\n", SDL_GetError());
			return;
		}
	}
}

