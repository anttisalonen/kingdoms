#include "main_window.h"

main_window::main_window(SDL_Surface* screen_, int x, int y, gui_data& data_, gui_resources& res_)
	: screen(screen_),
	screen_w(x),
	screen_h(y),
	data(data_),
	res(res_),
	tile_w(32),
	tile_h(32),
	cam_total_tiles_x((screen_w + tile_w - 1) / tile_w),
	cam_total_tiles_y((screen_h + tile_h - 1) / tile_h),
	sidebar_size(4),
	current_unit(NULL),
	blink_unit(NULL),
	timer(0)
{
	cam.cam_x = cam.cam_y = 0;
}

main_window::~main_window()
{
}

void main_window::get_next_free_unit(std::list<unit*>::iterator& current_unit_it) const
{
	std::list<unit*>::iterator uit = current_unit_it;
	for(++current_unit_it;
			current_unit_it != (*data.r.current_civ)->units.end();
			++current_unit_it) {
		if((*current_unit_it)->moves > 0 && !(*current_unit_it)->fortified)
			return;
	}

	// run through the first half
	for(current_unit_it = (*data.r.current_civ)->units.begin();
			current_unit_it != uit;
			++current_unit_it) {
		if((*current_unit_it)->moves > 0 && !(*current_unit_it)->fortified)
			return;
	}
	current_unit_it = (*data.r.current_civ)->units.end();
}

void main_window::set_current_unit(const unit* u)
{
	current_unit = u;
	blink_unit = NULL;
}

int main_window::draw()
{
	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0) {
			return 1;
		}
	}
	draw_sidebar();
	if (SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}
	clear_main_map();
	draw_main_map();
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
	if(current_unit)
		draw_unit_info(current_unit);
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
			if((c.r == 255 && c.g == 255 && c.b == 255) || (*data.r.current_civ)->fog_at(x, y) > 0) {
				sdl_put_pixel(screen, j, i, c);
			}
		}
	}
	SDL_UpdateRect(screen, 0, 0, minimap_w, minimap_h);

	return 0;
}

int main_window::draw_civ_info() const
{
	draw_text(screen, &res.font, (*data.r.current_civ)->civname, 10, sidebar_size * tile_h / 2 + 40, 255, 255, 255);
	char buf[256];
	buf[255] = '\0';
	snprintf(buf, 255, "Gold: %d", (*data.r.current_civ)->gold);
	draw_text(screen, &res.font, buf, 10, sidebar_size * tile_h / 2 + 60, 255, 255, 255);
	return 0;
}

int main_window::draw_unit_info(const unit* u) const
{
	if(!u)
		return 0;
	const unit_configuration* uconf = data.r.get_unit_configuration(u->unit_id);
	if(!uconf)
		return 1;
	draw_text(screen, &res.font, uconf->unit_name, 10, sidebar_size * tile_h / 2 + 80, 255, 255, 255);
	char buf[256];
	buf[255] = '\0';
	snprintf(buf, 255, "Moves: %-2d/%2d", u->moves, uconf->max_moves);
	draw_text(screen, &res.font, buf, 10, sidebar_size * tile_h / 2 + 100, 255, 255, 255);
	return 0;
}

int main_window::draw_eot() const
{
	return draw_text(screen, &res.font, "End of turn", 10, screen_h - 100, 255, 255, 255);
}

int main_window::draw_tile(const SDL_Surface* surf, int x, int y) const
{
	if(in_bounds(cam.cam_x, x, cam.cam_x + cam_total_tiles_x) &&
	   in_bounds(cam.cam_y, y, cam.cam_y + cam_total_tiles_y)) {
		return draw_image((x + sidebar_size - cam.cam_x) * tile_w,
				(y - cam.cam_y) * tile_h,
				surf, screen);
	}
	return 0;
}

int main_window::draw_city(const city& c) const
{
	return draw_tile(res.city_images[c.civ_id], c.xpos, c.ypos);
}

int main_window::draw_unit(const unit& u)
{
	if(!(in_bounds(cam.cam_x, u.xpos, cam.cam_x + cam_total_tiles_x) &&
 	     in_bounds(cam.cam_y, u.ypos, cam.cam_y + cam_total_tiles_y))) {
		return 0;
	}
	SDL_Surface* surf = res.get_unit_tile(u, data.r.civs[u.civ_id]->col);
	return draw_tile(surf, u.xpos, u.ypos);
}

int main_window::draw_main_map()
{
	int imax = std::min(cam.cam_y + cam_total_tiles_y, data.m.size_y());
	int jmax = std::min(cam.cam_x + cam_total_tiles_x, data.m.size_x());
	for(int i = cam.cam_y, y = 0; i < imax; i++, y++) {
		for(int j = cam.cam_x, x = sidebar_size; j < jmax; j++, x++) {
			char fog = (*data.r.current_civ)->fog_at(j, i);
			if(fog > 0) {
				if(show_terrain_image(j, i, x, y, fog == 1)) {
					return 1;
				}
			}
		}
	}
	for(std::vector<civilization*>::iterator cit = data.r.civs.begin();
			cit != data.r.civs.end();
			++cit) {
		for(std::list<city*>::const_iterator it = (*cit)->cities.begin();
				it != (*cit)->cities.end();
				++it) {
			if((*data.r.current_civ)->fog_at((*it)->xpos, (*it)->ypos) > 0) {
				if(draw_city(**it)) {
					return 1;
				}
			}
		}
		for(std::list<unit*>::const_iterator it = (*cit)->units.begin(); 
				it != (*cit)->units.end();
				++it) {
			if(blink_unit == *it)
				continue;
			if((*data.r.current_civ)->fog_at((*it)->xpos, (*it)->ypos) == 2) {
				if(draw_unit(**it)) {
					return 1;
				}
			}
		}
	}
	return 0;
}

int main_window::show_terrain_image(int x, int y, int xpos, int ypos, bool shade) const
{
	return draw_terrain_tile(x, y, xpos * tile_w, ypos * tile_h, shade,
			data.m, res.terrains, screen);
}

color main_window::get_minimap_color(int x, int y) const
{
	if((x >= cam.cam_x && x <= cam.cam_x + cam_total_tiles_x - 1) &&
	   (y >= cam.cam_y && y <= cam.cam_y + cam_total_tiles_y - 1) &&
	   (x == cam.cam_x || x == cam.cam_x + cam_total_tiles_x - 1 ||
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
		if(cam.cam_x > 0)
			cam.cam_x--, redraw = true;
	}
	else if(right) {
		if(cam.cam_x < data.m.size_x() - cam_total_tiles_x)
			cam.cam_x++, redraw = true;
	}
	if(up) {
		if(cam.cam_y > 0)
			cam.cam_y--, redraw = true;
	}
	else if(down) {
		if(cam.cam_y < data.m.size_y() - cam_total_tiles_y)
			cam.cam_y++, redraw = true;
	}
	if(redraw) {
		draw();
	}
	return redraw;
}

void main_window::center_camera_to_unit(unit* u)
{
	cam.cam_x = clamp(0, u->xpos - (-sidebar_size + cam_total_tiles_x) / 2, data.m.size_x() - cam_total_tiles_x);
	cam.cam_y = clamp(0, u->ypos - cam_total_tiles_y / 2, data.m.size_y() - cam_total_tiles_y);
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
	int old_timer = timer;
	timer += ms;
	const unit* old_blink_unit = blink_unit;
	if(timer % 1000 < 300) {
		blink_unit = current_unit;
	}
	else {
		blink_unit = NULL;
	}
	if(blink_unit != old_blink_unit)
		draw();
	if(old_timer / 200 != timer / 200) {
		int x, y;
		SDL_GetMouseState(&x, &y);
		handle_mousemotion(x, y);
	}
	return 0;
}

int main_window::handle_mousemotion(int x, int y)
{
	const int border = tile_w;
	try_move_camera(x >= sidebar_size * tile_w && x < sidebar_size * tile_w + border,
			x > screen_w - border,
			y < border,
			y > screen_h - border);
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

int main_window::handle_input(const SDL_Event& ev, std::list<unit*>::iterator& current_unit_it, city** c)
{
	switch(ev.type) {
		case SDL_QUIT:
			return 1;
		case SDL_KEYDOWN:
			return handle_keydown(ev.key.keysym.sym, ev.key.keysym.mod, current_unit_it, c);
		case SDL_MOUSEBUTTONDOWN:
			return handle_mousedown(ev, current_unit_it, c);
		default:
			return 0;
	}
}

int main_window::handle_civ_messages(std::list<msg>* messages)
{
	while(!messages->empty()) {
		msg& m = messages->front();
		switch(m.type) {
			case msg_new_unit:
				printf("New unit '%s' produced.\n",
						m.msg_data.new_unit->uconf.unit_name);
				break;
			case msg_civ_discovery:
				printf("Discovered civilization '%s'.\n",
						data.r.civs[m.msg_data.discovered_civ_id]->civname);
				break;
			default:
				break;
		}
		messages->pop_front();
	}
	return 0;
}

int main_window::handle_keydown(SDLKey k, SDLMod mod, std::list<unit*>::iterator& current_unit_it, city** c)
{
	if(k == SDLK_ESCAPE || k == SDLK_q)
		return 1;
	else if(k == SDLK_LEFT || k == SDLK_RIGHT || k == SDLK_UP || k == SDLK_DOWN) {
		try_move_camera(k == SDLK_LEFT, k == SDLK_RIGHT, k == SDLK_UP, k == SDLK_DOWN);
	}
	else if((k == SDLK_RETURN || k == SDLK_KP_ENTER) && 
			(current_unit_it == (*data.r.current_civ)->units.end() || (mod & (KMOD_LSHIFT | KMOD_RSHIFT)))) {
		// end of turn for this civ
		data.r.next_civ();
		handle_civ_messages(&(*data.r.current_civ)->messages);
		current_unit_it = (*data.r.current_civ)->units.end();
		get_next_free_unit(current_unit_it);
		if(current_unit_it != (*data.r.current_civ)->units.end())
			set_current_unit(*current_unit_it);
		else
			set_current_unit(NULL);
		draw();
	}
	else if(current_unit_it != (*data.r.current_civ)->units.end()) {
		bool unit_changed = false;
		if(k == SDLK_c) {
			center_camera_to_unit(*current_unit_it);
			draw();
		}
		else if(k == SDLK_b) {
			const unit_configuration* uconf = data.r.get_unit_configuration((*current_unit_it)->unit_id);
			bool can_build = uconf == NULL ? false : uconf->settler;
			if(can_build) {
				*c = (*data.r.current_civ)->add_city("city name", (*current_unit_it)->xpos,
						(*current_unit_it)->ypos);
				set_default_city_production(*c, data.r.uconfmap);
				current_unit_it = (*data.r.current_civ)->units.erase(current_unit_it);
				unit_changed = true;
			}
		}
		else if(k == SDLK_SPACE) {
			(*current_unit_it)->moves = 0;
			get_next_free_unit(current_unit_it);
			unit_changed = true;
		}
		else if(k == SDLK_f) {
			(*current_unit_it)->fortified = true;
			get_next_free_unit(current_unit_it);
			unit_changed = true;
		}
		else if(k == SDLK_w) {
			std::list<unit*>::iterator old_it = current_unit_it;
			get_next_free_unit(current_unit_it);
			if(current_unit_it == (*data.r.current_civ)->units.end())
				current_unit_it = old_it;
			else
				unit_changed = true;
		}
		else {
			int chx, chy;
			numpad_to_move(k, &chx, &chy);
			if((chx || chy) && ((*data.r.current_civ)->try_move_unit(*current_unit_it, chx, chy))) {
				if((*current_unit_it)->moves == 0) {
					// no moves left
					get_next_free_unit(current_unit_it);
					unit_changed = true;
				}
			}
		}
		if(unit_changed) {
			if(current_unit_it != (*data.r.current_civ)->units.end()) {
				try_center_camera_to_unit(*current_unit_it);
				set_current_unit(*current_unit_it);
				draw();
			}
			else {
				set_current_unit(NULL);
				draw();
			}
		}
	}
	return 0;
}

int main_window::handle_mousedown(const SDL_Event& ev, std::list<unit*>::iterator& current_unit_it, city** c)
{
	int sq_x = (ev.button.x - sidebar_size * tile_w) / tile_w;
	int sq_y = ev.button.y / tile_h;
	if(sq_x >= 0) {
		sq_x += cam.cam_x;
		sq_y += cam.cam_y;

		// choose city
		for(std::list<city*>::const_iterator it = (*data.r.current_civ)->cities.begin();
				it != (*data.r.current_civ)->cities.end();
				++it) {
			if((*it)->xpos == sq_x && (*it)->ypos == sq_y) {
				*c = *it;
				break;
			}
		}

		// if no city chosen, choose unit
		if(!*c) {
			for(std::list<unit*>::iterator it = (*data.r.current_civ)->units.begin();
					it != (*data.r.current_civ)->units.end();
					++it) {
				if((*it)->xpos == sq_x && (*it)->ypos == sq_y) {
					(*it)->fortified = false;
					if((*it)->moves > 0) {
						current_unit_it = it;
						set_current_unit(*it);
					}
				}
			}
		}
	}
	return 0;
}


