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

enum action_type {
	action_give_up,
	action_eot,
	action_unit_action,
	action_city_action,
	action_none,
};

enum unit_action_type {
	action_move_unit,
	action_found_city,
	action_skip,
	action_fortify,
};

struct action {
	action(action_type t);
	action_type type;
	union {
		struct {
			unit_action_type uatype;
			unit* u;
			union {
				struct {
					int chx;
					int chy;
				} move_pos;
			} unit_action_data;
		} unit_data;
	} data;
};

action unit_action(unit_action_type t, unit* u);

class main_window {
	public:
		main_window(SDL_Surface* screen_, int x, int y, gui_data& data_, gui_resources& res_);
		~main_window();
		void set_current_unit(const unit* u);
		int draw();
		int process(int ms);
		int handle_input(const SDL_Event& ev, std::list<unit*>::iterator& current_unit_it, city** c);
		void get_next_free_unit(std::list<unit*>::iterator& current_unit_it) const;
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
		int handle_civ_messages(std::list<msg>* messages);
		bool try_move_unit(unit* u, int chx, int chy);
		action input_to_action(const SDL_Event& ev, const std::list<unit*>::iterator& current_unit_it);
		bool perform_action(const action& a);
		void handle_input_gui_mod(const SDL_Event& ev, std::list<unit*>::iterator& current_unit_it, city** c);
		void handle_successful_action(const action& a, std::list<unit*>::iterator& current_unit_it, city** c);
		void update_view(std::list<unit*>::iterator& current_unit_it);
		int try_choose_with_mouse(const SDL_Event& ev, std::list<unit*>::iterator& current_unit_it, city** c);
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
