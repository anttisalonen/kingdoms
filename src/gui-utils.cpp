#include "gui-utils.h"

int button::draw_surface(SDL_Surface* screen, SDL_Surface* surf)
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

texture_button::texture_button(const rect& dim_, SDL_Surface* surf_, 
		boost::function<int()> onclick_)
	: button(dim_, onclick_),
	surf(surf_)
{
}

texture_button::~texture_button()
{
}

int texture_button::draw(SDL_Surface* screen)
{
	return draw_surface(screen, surf);
}

plain_button::plain_button(const rect& dim_, const char* text, const TTF_Font* font,
		const color& bg_col, const color& text_col, 
		boost::function<int()> onclick_)
	: button(dim_, onclick_),
	surf(make_label(text, font, dim_.w, dim_.h, bg_col, text_col))
{
}

plain_button::~plain_button()
{
	SDL_FreeSurface(surf);
}

int plain_button::draw(SDL_Surface* screen)
{
	return draw_surface(screen, surf);
}

SDL_Surface* make_label(const char* text, const TTF_Font* font, 
		int w, int h, const color& bg_col, const color& text_col)
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

int draw_rect(int x, int y, int w, int h, const color& c, 
		int border_width, SDL_Surface* screen)
{
	Uint32 sdl_col = SDL_MapRGB(screen->format, c.r, c.g, c.b);
	if(border_width == 0) {
		SDL_Rect dest;
		dest.x = x;
		dest.y = y;
		dest.w = w;
		dest.h = h;
		return SDL_FillRect(screen, &dest, sdl_col);
	}
	else {
		SDL_Rect dest;
		dest.x = x;
		dest.y = y;
		dest.w = w;
		dest.h = border_width;
		if(SDL_FillRect(screen, &dest, sdl_col))
			return -1;
		dest.w = border_width;
		dest.h = h;
		if(SDL_FillRect(screen, &dest, sdl_col))
			return -1;
		dest.x = x + w - border_width;
		dest.y = y;
		if(SDL_FillRect(screen, &dest, sdl_col))
			return -1;
		dest.x = x;
		dest.y = y + h - border_width;
		dest.w = w;
		dest.h = border_width;
		return SDL_FillRect(screen, &dest, sdl_col);
	}
}

int draw_image(int x, int y, const SDL_Surface* tile, SDL_Surface* screen)
{
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;

	if(SDL_BlitSurface((SDL_Surface*)tile, NULL, screen, &dest)) {
		fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

int draw_road_overlays(const map& m, 
		int x, int y, int xpos, int ypos, 
		const tileset& terrains,
		SDL_Surface* screen)
{
	SDL_Rect dest;
	dest.x = xpos;
	dest.y = ypos;
	for(int i = 0; i < 3; i++) {
		for(int j = 0; j < 3; j++) {
			if((m.get_improvements_on(x + i - 1, y + j - 1) & improv_road) &&
				terrains.road_overlays[3 * i + j] != NULL) {
				if(SDL_BlitSurface(terrains.road_overlays[3 * i + j], NULL, screen, &dest)) {
					fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
					return 1;
				}
			}
		}
	}
	return 0;
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
	int im = m.get_improvements_on(x, y);
	SDL_Surface* imp_surf = NULL;
	if(im & improv_irrigation)
		imp_surf = terrains.irrigation_overlay;
	if(im & improv_mine)
		imp_surf = terrains.mine_overlay;
	if(imp_surf) {
		if(SDL_BlitSurface(imp_surf, NULL, screen, &dest)) {
			fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
			return 1;
		}
	}
	if(im & improv_road) {
		if(draw_road_overlays(m, x, y, xpos, ypos, terrains, screen))
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
	}
	return 0;
}

gui_resources::gui_resources(const TTF_Font& f, int tile_w, int tile_h, 
		SDL_Surface* food_, SDL_Surface* prod_, SDL_Surface* comm_)
	: terrains(tile_w, tile_h),
       	font(f),
	food_icon(food_),
	prod_icon(prod_),
	comm_icon(comm_)
{
}

gui_resources::~gui_resources()
{
	for(UnitImageMap::iterator it = unit_images.begin();
			it != unit_images.end();
			++it) {
		SDL_FreeSurface(it->second);
	}
}

gui_data::gui_data(map& mm, pompelmous& rr)
	: m(mm),
	r(rr)
{
}

tileset::tileset(int w, int h)
	: tile_w(w),
	tile_h(h)
{
}

SDL_Surface* gui_resources::get_unit_tile(const unit& u, const color& c)
{
	if(u.uconf_id < 0 || u.uconf_id >= (int)plain_unit_images.size()) {
		fprintf(stderr, "Image for Unit ID %d not loaded\n", u.uconf_id);
		return NULL;
	}
	SDL_Surface* surf;
	UnitImageMap::const_iterator it = unit_images.find(std::make_pair(u.uconf_id,
			       c));
	if(it == unit_images.end()) {
		// load image
		SDL_Surface* plain = plain_unit_images[u.uconf_id];
		if(!plain) {
			fprintf(stderr, "%s: could not load image file for unit '%s'.\n",
					__func__, u.uconf.unit_name.c_str());
			return NULL;
		}
		SDL_Surface* result = SDL_DisplayFormatAlpha(plain);
		sdl_change_pixel_color(result, color(0, 255, 255), c);
		unit_images.insert(std::make_pair(std::make_pair(u.uconf_id, c),
					result));
		surf = result;
	}
	else {
		surf = it->second;
	}
	return surf;
}

button::button(const rect& dim_, boost::function<int()> onclick_)
	: dim(dim_),
	onclick(onclick_)
{
}

int check_button_click(const std::list<button*>& buttons,
		const SDL_Event& ev)
{
	if(ev.type != SDL_MOUSEBUTTONDOWN)
		return 0;
	for(std::list<button*>::const_iterator it = buttons.begin();
			it != buttons.end();
			++it) {
		if(in_rectangle((*it)->dim, ev.button.x, ev.button.y)) {
			if((*it)->onclick) {
				return (*it)->onclick();
			}
			else
				return 0;
		}
	}
	return 0;
}

window::window(SDL_Surface* screen_, int w, int h, gui_data& data_, gui_resources& res_)
	: screen(screen_),
	screen_w(w),
	screen_h(h),
	data(data_),
	res(res_)
{
}

void window::add_subwindow(window* w)
{
	subwindows.push_back(w);
}

int window::draw()
{
	if(subwindows.empty()) {
		return draw_window();
	}
	else {
		std::for_each(subwindows.begin(), subwindows.end(), std::mem_fun(&window::draw));
		return 0;
	}
}

int window::handle_input(const SDL_Event& ev)
{
	if(subwindows.empty()) {
		return handle_window_input(ev);
	}
	else {
		for(std::list<window*>::iterator it = subwindows.begin();
				it != subwindows.end();
				++it) {
			int p = (*it)->handle_input(ev);
			if(p) {
				delete *it;
				subwindows.erase(it--);
			}
		}
		return 0;
	}
}

int window::process(int ms)
{
	int ret = process_window(ms);
	if(ret)
		return ret;
	std::for_each(subwindows.begin(), subwindows.end(), 
			std::bind2nd(std::mem_fun(&window::process), ms));
	return 0;
}

void window::init_turn()
{
	init_window_turn();
	std::for_each(subwindows.begin(), subwindows.end(), std::mem_fun(&window::init_turn));
}

