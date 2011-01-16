#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include "main_window.h"

class editor_window : public main_window {
	public:
		editor_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_);
};

#endif
