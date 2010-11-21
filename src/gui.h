#ifndef GUI_H
#define GUI_H

#include <list>
#include <vector>
#include <map>

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

#include "color.h"
#include "utils.h"
#include "sdl-utils.h"
#include "buf2d.h"
#include "civ.h"

struct camera {
	int cam_x;
	int cam_y;
};

class gui
{
	typedef std::map<std::pair<int, color>, SDL_Surface*> UnitImageMap;
	public:
		gui(int x, int y, map& mm, round& rr,
				const std::vector<const char*>& terrain_files,
				const std::vector<const char*>& unit_files,
				const TTF_Font& font_);
		~gui();
		int display(const unit* current_unit);
		int redraw_main_map();
		int handle_keydown(SDLKey k, SDLMod mod, std::list<unit*>::iterator& current_unit);
		int handle_mousemotion(int x, int y);
		int process(int ms, const unit* current_unit);
		camera cam;
	private:
		int show_terrain_image(int x, int y, int xpos, int ypos, bool shade) const;
		int draw_main_map();
		int draw_sidebar(const unit* current_unit) const;
		int draw_unit(const unit& u);
		color get_minimap_color(int x, int y) const;
		int try_move_camera(bool left, bool right, bool up, bool down);
		void center_camera_to_unit(unit* u);
		int try_center_camera_to_unit(unit* u);
		int draw_minimap() const;
		int draw_civ_info() const;
		int draw_unit_info(const unit* current_unit) const;
		int draw_eot() const;
		int draw_text(const char* str, int x, int y, int r, int g, int b) const;
		int clear_sidebar() const;
		int clear_main_map() const;
		void numpad_to_move(SDLKey k, int* chx, int* chy) const;
		map& m;
		round& r;
		SDL_Surface* screen;
		std::vector<SDL_Surface*> terrains;
		std::vector<SDL_Surface*> plain_unit_images;
		UnitImageMap unit_images;
		const int tile_w;
		const int tile_h;
		const int screen_w;
		const int screen_h;
		const int cam_total_tiles_x;
		const int cam_total_tiles_y;
		const int sidebar_size;
		int timer;
		const unit* blink_unit;
		const TTF_Font& font;
};

#endif

