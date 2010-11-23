#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include <vector>
#include <stdlib.h>

#include <boost/function.hpp>

#include "SDL/SDL.h"

#include "color.h"
#include "sdl-utils.h"
#include "rect.h"
#include "civ.h"

struct tileset {
	tileset(int w, int h);
	std::vector<SDL_Surface*> textures;
	const int tile_w;
	const int tile_h;
};

typedef std::map<std::pair<int, color>, SDL_Surface*> UnitImageMap;

struct gui_resources {
	gui_resources(const TTF_Font& f, int tile_w, int tile_h,
			SDL_Surface* food_, SDL_Surface* prod_,
			SDL_Surface* comm_);
	tileset terrains;
	std::vector<SDL_Surface*> plain_unit_images;
	UnitImageMap unit_images;
	const TTF_Font& font;
	std::vector<SDL_Surface*> city_images;
	SDL_Surface* food_icon;
	SDL_Surface* prod_icon;
	SDL_Surface* comm_icon;
	SDL_Surface* get_unit_tile(const unit& u, const color& c);
};

class gui_data {
	public:
		gui_data(map& mm, round& rr);
		map& m;
		round& r;
		unit* current_unit;
};

class button {
	public:
		button(const rect& dim_, SDL_Surface* surf_, boost::function<int()> onclick_);
		int draw(SDL_Surface* screen) const;
		rect dim;
		SDL_Surface* surf;
		boost::function<int()>onclick;
};

int draw_rect(int x, int y, int w, int h, const color& c, 
		int border_width, SDL_Surface* screen);
int draw_image(int x, int y, const SDL_Surface* tile, SDL_Surface* screen);
int draw_terrain_tile(int x, int y, int xpos, int ypos, bool shade,
		const map& m, 
		const tileset& terrains,
		SDL_Surface* screen);
SDL_Surface* make_label(const char* text, const TTF_Font* font, int w, int h, const color& bg_col, const color& text_col);
int check_button_click(const std::list<button*>& buttons,
		const SDL_Event& ev);

#endif
