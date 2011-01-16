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
		main_window(SDL_Surface* screen_, gui_data& data_,
				gui_resources& res_,
				int sidebar_size_);
		virtual ~main_window();
		virtual int draw_window();
		virtual int process(int ms);
		virtual int handle_window_input(const SDL_Event& ev);
	protected:
		virtual char fog_on_tile(int x, int y) const;
		virtual bool city_info_available(const city& c) const;
		virtual bool can_draw_unit(const unit* u) const;
		virtual const std::set<unsigned int>* discovered_advances() const;
		virtual void post_draw();
		virtual void draw_sidebar();
		void handle_input_gui_mod(const SDL_Event& ev);
		int draw_main_map();
		int draw_unit(const unit* u);
		void clear_sidebar();
		color get_minimap_color(int x, int y) const;
		int draw_minimap() const;
		int clear_main_map() const;
		int draw_tile(const SDL_Surface* surf, int x, int y) const;
		int draw_city(const city& c) const;
		int test_draw_border(int x, int y, int xpos, int ypos);
		int show_terrain_image(int x, int y, int xpos, int ypos,
				bool draw_improvements, bool draw_resources,
				bool check_resource_discovered, bool shade);
		int handle_mouse_up(const SDL_Event& ev);
		int handle_mouse_down(const SDL_Event& ev);
		int draw_complete_tile(int x, int y, int shx, int shy,
				bool terrain, bool resources,
				bool improvements, bool borders,
				boost::function<bool(const unit*)> unit_predicate,
				bool cities);
		int handle_mousemotion(int x, int y);
		int try_move_camera(bool left, bool right, bool up, bool down);
		void center_camera_at(int x, int y);
		void center_camera_to_unit(const unit* u);
		int try_center_camera_at(int x, int y);
		int try_center_camera_to_unit(const unit* u);
		void mouse_coord_to_tiles(int mx, int my, int* sqx, int* sqy);
		int tile_xcoord_to_pixel(int x) const;
		int tile_ycoord_to_pixel(int y) const;
		int tile_visible(int x, int y) const;
		const int tile_w;
		const int tile_h;
		const int cam_total_tiles_x;
		const int cam_total_tiles_y;
		camera cam;
		int mouse_down_sqx;
		int mouse_down_sqy;
		const int sidebar_size;
};

#endif
