#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include "main_window.h"

class editor_window : public main_window {
	public:
		editor_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_);
		~editor_window();
		int handle_window_input(const SDL_Event& ev);
	protected:
		void draw_sidebar();
	private:
		int handle_input_gui_mod(const SDL_Event& ev);
		int handle_mouse_down(const SDL_Event& ev);
		int handle_mousemotion(const SDL_Event& ev);
		void modify_map(int x, int y);
		int on_terrain_button(int val);
		int on_size_button(int val);
		int chosen_terrain;
		int brush_size;
		const int sidebar_terrain_xstart;
		const int sidebar_terrain_ystart;
		std::list<button*> sidebar_buttons;
};

#endif
