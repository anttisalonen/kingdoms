#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include "main_window.h"

enum tool_type {
	editor_tool_terrain,
	editor_tool_resource,
	editor_tool_startpos,
};

class editor_window : public main_window {
	public:
		editor_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_);
		~editor_window();
		int handle_window_input(const SDL_Event& ev);
		bool draw_starting_positions();
	protected:
		void draw_sidebar();
	private:
		int handle_input_gui_mod(const SDL_Event& ev);
		int handle_mouse_down(const SDL_Event& ev);
		int handle_mousemotion(const SDL_Event& ev);
		void modify_map(int x, int y);
		int on_terrain_button(int val);
		int on_resource_button(resource_map::const_iterator it);
		int on_size_button(int val);
		int on_save(const std::string& s);
		int on_new_map(const widget_window* w);
		void set_terrain(int x, int y, int terr);
		void add_load_map_subwindow();
		int on_load_map(const std::string& s, const widget_window* w);
		int on_startposition_button();
		int on_civ_startpos(int civid, const widget_window* w);
		widget_window* standard_popup(int win_width, int win_height) const;
		tool_type current_tool;
		int current_tool_index;
		int chosen_terrain;
		unsigned int chosen_resource;
		int brush_size;
		const int sidebar_terrain_xstart;
		const int sidebar_terrain_ystart;
		const int sidebar_startpos_button_width;
		int sidebar_resource_ystart;
		int sidebar_brush_size_ystart;
		int sidebar_startpos_ystart;
		std::list<button*> sidebar_buttons;
		std::string saved_filename;
};

#endif
