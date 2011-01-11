#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include <vector>
#include <stdlib.h>

#include <boost/function.hpp>

#include "SDL/SDL.h"

#include "color.h"
#include "sdl-utils.h"
#include "rect.h"
#include "pompelmous.h"

struct tileset {
	tileset(int w, int h);
	std::vector<SDL_Surface*> textures;
	SDL_Surface* irrigation_overlay;
	SDL_Surface* mine_overlay;
	SDL_Surface* road_overlays[9];
	const int tile_w;
	const int tile_h;
};

typedef std::map<std::pair<int, color>, SDL_Surface*> UnitImageMap;

struct gui_resources {
	gui_resources(const TTF_Font& f, int tile_w, int tile_h,
			SDL_Surface* food_, SDL_Surface* prod_,
			SDL_Surface* comm_);
	~gui_resources();
	tileset terrains;
	std::vector<SDL_Surface*> plain_unit_images;
	UnitImageMap unit_images;
	const TTF_Font& font;
	std::vector<SDL_Surface*> city_images;
	std::map<unsigned int, SDL_Surface*> resource_images;
	SDL_Surface* food_icon;
	SDL_Surface* prod_icon;
	SDL_Surface* comm_icon;
	SDL_Surface* get_unit_tile(const unit& u, const color& c);
};

class gui_data {
	public:
		gui_data(const map& mm, pompelmous& rr);
		const map& m;
		pompelmous& r;
};

class button {
	public:
		button(const rect& dim_, boost::function<int()> onclick_);
		~button() { }
		virtual int draw(SDL_Surface* screen) = 0;
		rect dim;
		boost::function<int()>onclick;
	protected:
		int draw_surface(SDL_Surface* screen, SDL_Surface* surf);
};

class texture_button : public button {
	public:
		texture_button(const rect& dim_, SDL_Surface* surf_, 
				boost::function<int()> onclick_);
		~texture_button();
		int draw(SDL_Surface* screen);
	private:
		SDL_Surface* surf;
};

class plain_button : public button {
	public:
		plain_button(const rect& dim_, const char* text, const TTF_Font* font,
				const color& bg_col, const color& text_col, 
				boost::function<int()> onclick_);
		~plain_button();
		int draw(SDL_Surface* screen);
	private:
		SDL_Surface* surf;
};

class window {
	public:
		window(SDL_Surface* screen_, int w, int h, gui_data& data_, gui_resources& res_);
		virtual ~window() { } 
		int draw();
		int process(int ms);
		void init_turn();
		int handle_input(const SDL_Event& ev);
		void add_subwindow(window* w);
	protected:
		virtual int handle_window_input(const SDL_Event& ev) = 0;
		virtual int draw_window() = 0;
		virtual int process_window(int ms) { return 0; }
		virtual void init_window_turn() { }
		int num_subwindows() const;
		SDL_Surface* screen;
		const int screen_w;
		const int screen_h;
		gui_data& data;
		gui_resources& res;
	private:
		std::list<window*> subwindows;
};

int draw_rect(int x, int y, int w, int h, const color& c, 
		int border_width, SDL_Surface* screen);
int draw_image(int x, int y, const SDL_Surface* tile, SDL_Surface* screen);
int draw_terrain_tile(int x, int y, int xpos, int ypos, bool shade,
		const map& m, 
		const tileset& terrains,
		const std::map<unsigned int, SDL_Surface*>& resource_images,
		bool draw_improvements,
		bool draw_resources,
		std::set<unsigned int>* researched_advances,
		SDL_Surface* screen);
SDL_Surface* make_label(const char* text, const TTF_Font* font, int w, int h, const color& bg_col, const color& text_col);
int check_button_click(const std::list<button*>& buttons,
		const SDL_Event& ev);

#endif
