#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <list>

#include "color.h"
#include "gui-utils.h"
#include "civ.h"
#include "rect.h"
#include "ai.h"

struct camera {
	int cam_x;
	int cam_y;
};

class main_window : public window {
	public:
		main_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_,
				ai* ai_, civilization* myciv_);
		~main_window();
		int draw_window();
		int process(int ms);
		int handle_window_input(const SDL_Event& ev);
		void init_turn();
		void handle_action(const visible_move_action& a);
	private:
		void get_next_free_unit();
		int draw_main_map();
		int draw_sidebar() const;
		int draw_unit(const unit* u);
		color get_minimap_color(int x, int y) const;
		int draw_minimap() const;
		int draw_civ_info() const;
		int draw_unit_info() const;
		int draw_eot() const;
		int clear_sidebar() const;
		int clear_main_map() const;
		int draw_tile(const SDL_Surface* surf, int x, int y) const;
		int draw_city(const city& c) const;
		int test_draw_border(int x, int y, int xpos, int ypos);
		int show_terrain_image(int x, int y, int xpos, int ypos,
				bool draw_improvements, bool draw_resources,
				bool check_resource_discovered, bool shade);
		int handle_mousemotion(int x, int y);
		int try_move_camera(bool left, bool right, bool up, bool down);
		void center_camera_at(int x, int y);
		void center_camera_to_unit(const unit* u);
		int try_center_camera_at(int x, int y);
		int try_center_camera_to_unit(const unit* u);
		void numpad_to_move(SDLKey k, int* chx, int* chy) const;
		int handle_civ_messages(std::list<msg>* messages);
		bool try_move_unit(unit* u, int chx, int chy);
		action input_to_action(const SDL_Event& ev);
		action observer_action(const SDL_Event& ev);
		void handle_input_gui_mod(const SDL_Event& ev, city** c);
		void handle_successful_action(const action& a, city** c);
		void update_view();
		int handle_mouse_down(const SDL_Event& ev, city** c);
		int check_line_drawing(int x, int y);
		int handle_mouse_up(const SDL_Event& ev);
		int try_choose_with_mouse(city** c);
		void mouse_coord_to_tiles(int mx, int my, int* sqx, int* sqy);
		int draw_line_by_sq(const coord& c1, const coord& c2, int r, int g, int b);
		int tile_xcoord_to_pixel(int x) const;
		int tile_ycoord_to_pixel(int y) const;
		int tile_visible(int x, int y) const;
		bool unit_not_at(int x, int y, const unit* u) const;
		int draw_complete_tile(int x, int y, int shx, int shy,
				bool terrain, bool resources,
				bool improvements, bool borders,
				boost::function<bool(const unit*)> unit_predicate,
				bool cities);
		bool draw_gui_unit(const unit* u) const;
		void handle_action_mouse_down(const SDL_Event& ev);
		action handle_action_mouse_up(const SDL_Event& ev);
		void check_unit_movement_orders();
		int try_perform_action(const action& a, city** c);
		void update_tile_info(int x, int y);
		void display_tile_info() const;
		std::string unit_strength_info_string(const unit* u) const;
		bool write_unit_info(const unit* u, int* written_lines) const;
		void add_gui_msg(const std::string& s);
		void update_action_buttons();
		int unit_wait();
		int unit_center();
		int unit_skip();
		int unit_fortify();
		int unit_found_city();
		int unit_improve(improvement_type i);
		int unit_load();
		int unit_unload();
		void draw_overlays();
		void draw_action_buttons();
		void clear_action_buttons();
		rect action_button_dim(int num) const;
		const int tile_w;
		const int tile_h;
		const int cam_total_tiles_x;
		const int cam_total_tiles_y;
		const int sidebar_size;
		camera cam;
		std::map<unsigned int, unit*>::const_iterator current_unit;
		std::map<unsigned int, std::list<coord> > unit_movement_orders;
		bool blink_unit;
		int timer;
		civilization* myciv;
		int mouse_down_sqx;
		int mouse_down_sqy;
		std::list<coord> path_to_draw;
		ai* internal_ai;
		coord sidebar_info_display;
		std::list<std::string> gui_msg_queue;
		std::list<button*> action_buttons;
		action action_button_action;
};


#endif
