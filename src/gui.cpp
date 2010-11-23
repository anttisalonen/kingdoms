#include "gui.h"

gui::gui(int x, int y, map& mm, round& rr,
	       	const std::vector<const char*>& terrain_files,
		const std::vector<const char*>& unit_files,
		const char* city_file,
		const TTF_Font& font_,
		const char* food_icon_name,
		const char* prod_icon_name,
		const char* curr_icon_name)
	: screen_w(x),
	screen_h(y),
	screen(SDL_SetVideoMode(x, y, 32, SDL_SWSURFACE)),
	data(gui_data(mm, rr)),
	res(font_, 32, 32, sdl_load_image(food_icon_name), 
			sdl_load_image(prod_icon_name), 
			sdl_load_image(curr_icon_name)),
	mw(screen, x, y, data, res),
	cw(NULL)
{
	if (!screen) {
		fprintf(stderr, "Unable to set %dx%d video: %s\n", x, y, SDL_GetError());
		return;
	}
	res.terrains.textures.resize(terrain_files.size());
	for(unsigned int i = 0; i < terrain_files.size(); i++) {
		res.terrains.textures[i] = sdl_load_image(terrain_files[i]);
	}
	res.plain_unit_images.resize(unit_files.size());
	for(unsigned int i = 0; i < unit_files.size(); i++) {
		res.plain_unit_images[i] = sdl_load_image(unit_files[i]);
	}
	res.city_images.resize(rr.civs.size());
	for(unsigned int i = 0; i < rr.civs.size(); i++) {
		SDL_Surface* plain = sdl_load_image(city_file);
		res.city_images[i] = SDL_DisplayFormat(plain);
		SDL_FreeSurface(plain);
		sdl_change_pixel_color(res.city_images[i], color(0, 255, 255), data.r.civs[i]->col);
	}
}

gui::~gui()
{
	for(unsigned int i = 0; i < res.terrains.textures.size(); i++) {
		SDL_FreeSurface(res.terrains.textures[i]);
	}
}

int gui::display(const unit* current_unit)
{
	if(!cw)
		mw.set_current_unit(current_unit);
	mw.draw();
	if(cw)
		cw->draw();
	return 0;
}

int gui::handle_input(const SDL_Event& ev, std::list<unit*>::iterator& current_unit)
{
	if(cw) {
		int ret = cw->handle_input(ev);
		if(ret) {
			delete cw;
			cw = NULL;
		}
		return 0;
	}
	else {
		if(current_unit != (*data.r.current_civ)->units.end())
			mw.set_current_unit(*current_unit);
		else {
			mw.draw();
			mw.set_current_unit(NULL);
		}
		city* nc = NULL;
		int ret;
		ret = mw.handle_input(ev, current_unit, &nc);
		if(!ret && nc) {
			cw = new city_window(screen, screen_w, screen_h, data, res, nc);
		}
		return ret;
	}
}

int gui::process(int ms, const unit* u)
{
	if(!cw)
		return mw.process(ms);
	else
		return cw->draw();
}

