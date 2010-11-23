#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <list>

#include "color.h"
#include "gui-utils.h"
#include "civ.h"
#include "rect.h"

struct camera {
	int cam_x;
	int cam_y;
};

class main_window {
	public:
		main_window(SDL_Surface* screen_, int x, int y, gui_data& data_, gui_resources& res_);
		~main_window();
		void set_current_unit(const unit* u);
		int draw();
		int process(int ms);
		int handle_input(const SDL_Event& ev, std::list<unit*>::iterator& current_unit_it, city** c);
	private:
		int draw_main_map();
		int draw_sidebar() const;
		int draw_unit(const unit& u);
		color get_minimap_color(int x, int y) const;
		int draw_minimap() const;
		int draw_civ_info() const;
		int draw_unit_info(const unit* current_unit) const;
		int draw_eot() const;
		int clear_sidebar() const;
		int clear_main_map() const;
		int draw_tile(const SDL_Surface* surf, int x, int y) const;
		int draw_city(const city& c) const;
		int show_terrain_image(int x, int y, int xpos, int ypos, bool shade) const;
		int handle_mousemotion(int x, int y);
		int try_move_camera(bool left, bool right, bool up, bool down);
		void center_camera_to_unit(unit* u);
		int try_center_camera_to_unit(unit* u);
		void numpad_to_move(SDLKey k, int* chx, int* chy) const;
		int handle_keydown(SDLKey k, SDLMod mod, std::list<unit*>::iterator& current_unit_it, city** c);
		int handle_mousedown(const SDL_Event& ev, city** c);
		void get_next_free_unit(std::list<unit*>::iterator& current_unit_it) const;
		int handle_civ_messages(std::list<msg>* messages);
		SDL_Surface* screen;
		const int screen_w;
		const int screen_h;
		gui_data& data;
		gui_resources& res;
		const int tile_w;
		const int tile_h;
		const int cam_total_tiles_x;
		const int cam_total_tiles_y;
		const int sidebar_size;
		camera cam;
		const unit* current_unit;
		const unit* blink_unit;
		int timer;
};


#endif
