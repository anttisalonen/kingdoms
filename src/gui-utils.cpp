#include "gui-utils.h"

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

gui_data::gui_data(map& mm, round& rr)
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
	if(u.unit_id < 0 || u.unit_id >= (int)plain_unit_images.size()) {
		fprintf(stderr, "Image for Unit ID %d not loaded\n", u.unit_id);
		return NULL;
	}
	SDL_Surface* surf;
	UnitImageMap::const_iterator it = unit_images.find(std::make_pair(u.unit_id,
			       c));
	if(it == unit_images.end()) {
		// load image
		SDL_Surface* plain = plain_unit_images[u.unit_id];
		SDL_Surface* result = SDL_DisplayFormat(plain);
		sdl_change_pixel_color(result, color(0, 255, 255), c);
		unit_images.insert(std::make_pair(std::make_pair(u.unit_id, c),
					result));
		surf = result;
	}
	else {
		surf = it->second;
	}
	return surf;
}

button::button(const rect& dim_, SDL_Surface* surf_, boost::function<int()> onclick_)
	: dim(dim_),
	surf(surf_),
	onclick(onclick_)
{
}

int button::draw(SDL_Surface* screen) const
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


