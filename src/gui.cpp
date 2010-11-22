#include "gui.h"
#include <functional>
#include <algorithm>

tileset::tileset(int w, int h)
	: tile_w(w),
	tile_h(h)
{
}

int draw_terrain_tile(int x, int y, int xpos, int ypos, bool shade,
		const map& m, 
		const tileset& terrains,
		SDL_Surface* screen)
{
	SDL_Rect dest;
	dest.x = xpos;
	dest.y = ypos;
	int val = m.get_data(x, y);
	if(val < 0 || val >= (int)terrains.textures.size()) {
		fprintf(stderr, "Terrain at %d not loaded at (%d, %d)\n", val,
				x, y);
		return 1;
	}
	if(SDL_BlitSurface(terrains.textures[val], NULL, screen, &dest)) {
		fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
		return 1;
	}
	if(shade) {
		color c(0, 0, 0);
		for(int i = dest.y; i < dest.y + terrains.tile_h; i++) {
			for(int j = dest.x; j < dest.x + terrains.tile_w; j++) {
				if((i % 2) == (j % 2)) {
					sdl_put_pixel(screen, j, i, c);
				}
			}
		}
		SDL_UpdateRect(screen, dest.x, dest.y, terrains.tile_w, terrains.tile_h);
	}
	return 0;
}

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
	return draw_text(screen, &res.font, (*data.r.current_civ)->civname, 10, sidebar_size * tile_h / 2 + 40, 255, 255, 255);
}

int main_window::draw_unit_info(const unit* current_unit) const
{
	if(!current_unit)
		return 0;
	const unit_configuration* uconf = data.r.get_unit_configuration(current_unit->unit_id);
	if(!uconf)
		return 1;
	draw_text(screen, &res.font, uconf->unit_name, 10, sidebar_size * tile_h / 2 + 60, 255, 255, 255);
	char buf[256];
	buf[255] = '\0';
	snprintf(buf, 255, "Moves: %-2d/%2d", current_unit->moves, uconf->max_moves);
	draw_text(screen, &res.font, buf, 10, sidebar_size * tile_h / 2 + 80, 255, 255, 255);
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
		SDL_Rect dest;
		dest.x = (x + sidebar_size - cam.cam_x) * tile_w;
		dest.y = (y - cam.cam_y) * tile_h;

		if(SDL_BlitSurface((SDL_Surface*)surf, NULL, screen, &dest)) {
			fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
			return 1;
		}
	}
	return 0;
}

int main_window::draw_city(const city& c) const
{
	return draw_tile(res.city_images[c.civ_id], c.xpos, c.ypos);
}

int main_window::draw_unit(const unit& u)
{
	if(u.unit_id < 0 || u.unit_id >= (int)res.plain_unit_images.size()) {
		fprintf(stderr, "Image for Unit ID %d not loaded\n", u.unit_id);
		return 1;
	}
	if(!(in_bounds(cam.cam_x, u.xpos, cam.cam_x + cam_total_tiles_x) &&
 	     in_bounds(cam.cam_y, u.ypos, cam.cam_y + cam_total_tiles_y))) {
		return 0;
	}
	SDL_Surface* surf = NULL;
	color& c = data.r.civs[u.civ_id]->col;
	UnitImageMap::const_iterator it = res.unit_images.find(std::make_pair(u.unit_id,
			       c));
	if(it == res.unit_images.end()) {
		// load image
		SDL_Surface* plain = res.plain_unit_images[u.unit_id];
		SDL_Surface* result = SDL_DisplayFormat(plain);
		sdl_change_pixel_color(result, color(0, 255, 255), c);
		res.unit_images.insert(std::make_pair(std::make_pair(u.unit_id, c),
					result));
		surf = result;
	}
	else {
		surf = it->second;
	}

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
			if((*data.r.current_civ)->fog_at((*it)->xpos, (*it)->ypos) == 2) {
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
		default:
			return 0;
	}
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
		current_unit_it = (*data.r.current_civ)->units.begin();
		set_current_unit(*current_unit_it);
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
				current_unit_it = (*data.r.current_civ)->units.erase(current_unit_it);
				unit_changed = true;
			}
		}
		else {
			int chx, chy;
			numpad_to_move(k, &chx, &chy);
			if((chx || chy) && ((*data.r.current_civ)->try_move_unit(*current_unit_it, chx, chy))) {
				if((*current_unit_it)->moves == 0) {
					// no moves left
					current_unit_it++;
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

template<typename T>
button<T>::button(const rect& dim_, SDL_Surface* surf_, int(T::* cb)())
	: dim(dim_),
	surf(surf_),
	onclick(cb)
{
}

template<typename T>
int button<T>::draw(SDL_Surface* screen) const
{
	SDL_Rect dest;
	dest.x = dim.x;
	dest.y = dim.y;

	if(SDL_BlitSurface(surf, NULL, screen, &dest)) {
		fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

SDL_Surface* make_label(const char* text, const TTF_Font* font, int w, int h, const color& bg_col, const color& text_col)
{
	SDL_Surface* button_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, rmask, gmask, bmask, 0);
	if(!button_surf)
		return NULL;
	Uint32 button_bg_col = SDL_MapRGB(button_surf->format, bg_col.r, bg_col.g, bg_col.b);
	SDL_FillRect(button_surf, NULL, button_bg_col);
	SDL_Color text_sdl_col = {text_col.r, text_col.g, text_col.b};
	SDL_Surface* button_text = TTF_RenderUTF8_Blended((TTF_Font*)font, text, text_sdl_col);
	if(button_text) {
		SDL_Rect pos;
		pos.x = w / 2 - button_text->w / 2;
		pos.y = h / 2 - button_text->h / 2;
		if(SDL_BlitSurface(button_text, NULL, button_surf, &pos)) {
			fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
			SDL_FreeSurface(button_surf);
			button_surf = NULL;
		}
		SDL_FreeSurface(button_text);
	}
	else {
		SDL_FreeSurface(button_surf);
		button_surf = NULL;
	}
	return button_surf;
}

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
	label_surf = make_label(c->cityname, &res.font, name_rect.w, name_rect.h, color(200, 200, 200), color(0, 0, 0));
	button_surf = make_label("Exit", &res.font, exit_rect.w, exit_rect.h, color(128, 60, 60), color(0, 0, 0));
	buttons.push_back(new button<city_window>(name_rect,
				label_surf, NULL));
	buttons.push_back(new button<city_window>(exit_rect,
				button_surf, &city_window::on_exit));
}

city_window::~city_window()
{
	while(!buttons.empty()) {
		delete buttons.back();
		buttons.pop_back();
	}
	SDL_FreeSurface(button_surf);
	SDL_FreeSurface(label_surf);
}

int city_window::on_exit()
{
	return 1;
}

int city_window::draw_city_resources_screen(int xpos, int ypos)
{
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

	// buttons
	std::for_each(buttons.begin(),
			buttons.end(),
			std::bind2nd(std::mem_fun(&button<city_window>::draw), screen));

	// city resources screen
	draw_city_resources_screen(screen_w * 0.3, screen_h * 0.2);

	// final flip
	if(SDL_Flip(screen)) {
		fprintf(stderr, "Unable to flip: %s\n", SDL_GetError());
	}
	return 0;
}

int city_window::handle_input(const SDL_Event& ev)
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

int city_window::handle_mousedown(const SDL_Event& ev)
{
	for(std::list<button<city_window>*>::iterator it = buttons.begin();
			it != buttons.end();
			++it) {
		if(in_rectangle((*it)->dim, ev.button.x, ev.button.y)) {
			if((*it)->onclick) {
				city_window_fun cb = (*it)->onclick;
				return CALL_MEMBER_FUN(*this, cb)();
			}
			else
				return 0;
		}
	}
	return 0;
}

int city_window::handle_keydown(SDLKey k, SDLMod mod)
{
	if(k == SDLK_ESCAPE || k == SDLK_RETURN || k == SDLK_KP_ENTER)
		return 1;
	return 0;
}

gui_resources::gui_resources(const TTF_Font& f, int tile_w, int tile_h)
	: terrains(tile_w, tile_h),
       	font(f)
{
}

gui_data::gui_data(map& mm, round& rr)
	: m(mm),
	r(rr)
{
}

gui::gui(int x, int y, map& mm, round& rr,
	       	const std::vector<const char*>& terrain_files,
		const std::vector<const char*>& unit_files,
		const char* city_file,
		const TTF_Font& font_)
	: screen_w(x),
	screen_h(y),
	data(gui_data(mm, rr)),
	res(font_, 32, 32),
	screen(SDL_SetVideoMode(x, y, 32, SDL_SWSURFACE)),
	mw(screen, x, y, data, res),
	cw(NULL)
{
	if (!screen) {
		fprintf(stderr, "Unable to set %dx%d video: %s\n", x, y, SDL_GetError());
		return;
	}
	res.terrains.textures.resize(terrain_files.size());
	for(unsigned int i = 0; i < terrain_files.size(); i++) {
		res.terrains.textures[i] = sdl_load_image(terrain_files[i]);
	}
	res.plain_unit_images.resize(unit_files.size());
	for(unsigned int i = 0; i < unit_files.size(); i++) {
		res.plain_unit_images[i] = sdl_load_image(unit_files[i]);
	}
	res.city_images.resize(rr.civs.size());
	for(unsigned int i = 0; i < rr.civs.size(); i++) {
		SDL_Surface* plain = sdl_load_image(city_file);
		res.city_images[i] = SDL_DisplayFormat(plain);
		SDL_FreeSurface(plain);
		sdl_change_pixel_color(res.city_images[i], color(0, 255, 255), data.r.civs[i]->col);
	}
}

gui::~gui()
{
	for(unsigned int i = 0; i < res.terrains.textures.size(); i++) {
		SDL_FreeSurface(res.terrains.textures[i]);
	}
}

int gui::display(const unit* current_unit)
{
	if(!cw)
		mw.set_current_unit(current_unit);
	mw.draw();
	if(cw)
		cw->draw();
	return 0;
}

int gui::handle_input(const SDL_Event& ev, std::list<unit*>::iterator& current_unit)
{
	if(cw) {
		int ret = cw->handle_input(ev);
		if(ret) {
			delete cw;
			cw = NULL;
		}
		return 0;
	}
	else {
		mw.set_current_unit(*current_unit);
		city* nc = NULL;
		int ret;
		ret = mw.handle_input(ev, current_unit, &nc);
		if(!ret && nc) {
			cw = new city_window(screen, screen_w, screen_h, data, res, nc);
		}
		return ret;
	}
}

int gui::process(int ms, const unit* u)
{
	if(!cw)
		return mw.process(ms);
	else
		return cw->draw();
}

