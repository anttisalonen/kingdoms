#include "gui.h"

gui::gui(int x, int y, map& mm, round& rr,
	       	const std::vector<std::string>& terrain_files,
		const std::vector<std::string>& unit_files,
		const char* default_unit_file,
		const char* city_file,
		const TTF_Font& font_,
		const char* food_icon_name,
		const char* prod_icon_name,
		const char* curr_icon_name,
		civilization* myciv_)
	: screen_w(x),
	screen_h(y),
	screen(SDL_SetVideoMode(x, y, 32, SDL_HWSURFACE | SDL_DOUBLEBUF)),
	data(gui_data(mm, rr)),
	res(font_, 32, 32, sdl_load_image(food_icon_name), 
			sdl_load_image(prod_icon_name), 
			sdl_load_image(curr_icon_name)),
	mw(screen, x, y, data, res, myciv_),
	cw(NULL),
	myciv(myciv_)
{
	if (!screen) {
		fprintf(stderr, "Unable to set %dx%d video: %s\n", x, y, SDL_GetError());
		return;
	}
	res.terrains.textures.resize(terrain_files.size());
	for(unsigned int i = 0; i < terrain_files.size(); i++) {
		res.terrains.textures[i] = sdl_load_image(terrain_files[i].c_str());
	}
	res.plain_unit_images.resize(rr.uconfmap.size());
	for(unsigned int i = 0; i < unit_files.size(); i++) {
		res.plain_unit_images[i] = sdl_load_image(unit_files[i].c_str());
	}
	if(unit_files.size() < rr.uconfmap.size()) {
		if(default_unit_file) {
			for(unsigned int i = unit_files.size(); i < rr.uconfmap.size(); i++) {
				const unit_configuration* uconf = rr.get_unit_configuration(i);
				if(uconf) {
					fprintf(stderr, "Note: no graphics available for Unit %s - using the default.\n",
							uconf->unit_name.c_str());
					res.plain_unit_images[i] = sdl_load_image(default_unit_file);
				}
				else {
					fprintf(stderr, "Warning: no unit configuration for ID %d.\n",
							i);
				}
			}
		}
		else {
			fprintf(stderr, "Warning: units with ID >= %d have no graphics, and no default image file is defined.\n",
					unit_files.size());
		}
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
	for(unsigned int i = 0; i < res.city_images.size(); i++) {
		SDL_FreeSurface(res.city_images[i]);
	}
	for(unsigned int i = 0; i < res.plain_unit_images.size(); i++) {
		SDL_FreeSurface(res.plain_unit_images[i]);
	}
	SDL_FreeSurface(res.food_icon);
	SDL_FreeSurface(res.prod_icon);
	SDL_FreeSurface(res.comm_icon);
}

int gui::display()
{
	mw.draw();
	if(cw)
		cw->draw();
	return 0;
}

int gui::handle_input(const SDL_Event& ev)
{
	if(cw) {
		int ret = cw->handle_input(ev);
		if(ret) {
			delete cw;
			cw = NULL;
			mw.draw();
		}
		return 0;
	}
	else {
		mw.draw();
		city* nc = NULL;
		int ret;
		ret = mw.handle_input(ev, &nc);
		if(!ret && nc) {
			cw = new city_window(screen, screen_w, screen_h, data, res, nc,
					myciv);
		}
		return ret;
	}
}

int gui::process(int ms)
{
	if(!cw)
		return mw.process(ms);
	else
		return cw->draw();
}

void gui::init_turn()
{
	mw.init_turn();
}
