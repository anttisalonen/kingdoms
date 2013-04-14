#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include "main_window.h"

enum tool_type {
	editor_tool_terrain,
	editor_tool_resource,
	editor_tool_startpos,
	editor_tool_river,
};

class editor_window : public main_window {
	public:
		editor_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_,
				const std::string& ruleset_name_);
		~editor_window();
		int handle_window_input(const SDL_Event& ev);
		bool draw_starting_positions();
		bool is_quitting() const;
	protected:
		void draw_sidebar();
	private:
		int handle_input_gui_mod(const SDL_Event& ev);
		int handle_mouse_down(const SDL_Event& ev);
		int handle_mousemotion(const SDL_Event& ev);
		void modify_map(int x, int y, bool remove);
		int on_terrain_button(int val);
		int on_resource_button(unsigned int res_id);
		int on_size_button(int val);
		int on_river_button();
		int on_save(const std::string& s);
		int on_new_map(const widget_window* w);
		void set_terrain(int x, int y, int terr);
		void add_load_map_subwindow();
		int on_load_map(const std::string& s, const widget_window* w);
		int on_startposition_button();
		void add_quit_confirm_subwindow();
		int confirm_quit(const widget_window* w);
		int on_civ_startpos(int civid, const widget_window* w);
		widget_window* standard_popup(int win_width, int win_height) const;
		bool mouse_tile_moved() const;
		int on_coastal_protection_button();
		void save_old_mousepos();
		tool_type current_tool;
		int current_tool_index;
		int brush_size;
		const int sidebar_terrain_xstart;
		const int sidebar_terrain_ystart;
		const int sidebar_startpos_button_width;
		const int sidebar_coastal_protection_button_width;
		int sidebar_resource_ystart;
		int sidebar_river_ystart;
		int sidebar_brush_size_ystart;
		int sidebar_startpos_ystart;
		int sidebar_coastal_protection_ystart;
		std::list<button*> sidebar_buttons;
		std::string saved_filename;
		bool quitting;
		int old_mouse_sqx;
		int old_mouse_sqy;
		bool coastal_protection;
		std::string ruleset_name;
};

#endif
