#include "gui.h"

gui::gui(int x, int y, SDL_Surface* screen_, const map& mm, pompelmous& rr,
	       	const std::vector<std::string>& terrain_files,
		const std::vector<std::string>& unit_files,
		const std::vector<std::string>& resource_files,
		const char* default_unit_file,
		const char* city_file,
		const TTF_Font& font_,
		const char* food_icon_name,
		const char* prod_icon_name,
		const char* curr_icon_name,
		const char* irrigation_name,
		const char* mine_name,
		const std::vector<const char*>& road_names,
		ai* ai_,
		civilization* myciv_)
	: screen_w(x),
	screen_h(y),
	screen(screen_),
	data(gui_data(mm, rr)),
	res(font_, 32, 32, sdl_load_image(food_icon_name), 
			sdl_load_image(prod_icon_name), 
			sdl_load_image(curr_icon_name)),
	mw(screen, x, y, data, res, ai_, myciv_)
{
	// terrain files
	res.terrains.textures.resize(terrain_files.size());
	for(unsigned int i = 0; i < terrain_files.size(); i++) {
		res.terrains.textures[i] = sdl_load_image(terrain_files[i].c_str());
	}

	// terrain overlays
	res.terrains.irrigation_overlay = sdl_load_image(irrigation_name);
	res.terrains.mine_overlay = sdl_load_image(mine_name);
	for(unsigned int i = 0; i < 9; i++) {
		res.terrains.road_overlays[i] = road_names.size() > i ? 
			sdl_load_image(road_names[i]) : NULL;
	}

	// unit images
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

	// city images (one for each civ)
	res.city_images.resize(rr.civs.size());
	for(unsigned int i = 0; i < rr.civs.size(); i++) {
		SDL_Surface* plain = sdl_load_image(city_file);
		res.city_images[i] = SDL_DisplayFormat(plain);
		SDL_FreeSurface(plain);
		sdl_change_pixel_color(res.city_images[i], color(0, 255, 255), data.r.civs[i]->col);
	}

	// resource images
	for(unsigned int i = 0; i < resource_files.size(); i++) {
		res.resource_images[i + 1] = sdl_load_image(resource_files[i].c_str());
	}
}

gui::~gui()
{
	SDL_FreeSurface(res.terrains.irrigation_overlay);
	SDL_FreeSurface(res.terrains.mine_overlay);
	for(unsigned int i = 0; i < 9; i++) {
		if(res.terrains.road_overlays[i])
			SDL_FreeSurface(res.terrains.road_overlays[i]);
	}
	for(unsigned int i = 0; i < res.terrains.textures.size(); i++) {
		SDL_FreeSurface(res.terrains.textures[i]);
	}
	for(unsigned int i = 0; i < res.city_images.size(); i++) {
		SDL_FreeSurface(res.city_images[i]);
	}
	for(unsigned int i = 0; i < res.plain_unit_images.size(); i++) {
		SDL_FreeSurface(res.plain_unit_images[i]);
	}
	while(!res.resource_images.empty()) {
		SDL_FreeSurface(res.resource_images.begin()->second);
		res.resource_images.erase(res.resource_images.begin());
	}
	SDL_FreeSurface(res.food_icon);
	SDL_FreeSurface(res.prod_icon);
	SDL_FreeSurface(res.comm_icon);
}

int gui::display()
{
	mw.draw();
	return 0;
}

int gui::handle_input(const SDL_Event& ev)
{
	mw.draw();
	return mw.handle_input(ev);
}

int gui::process(int ms)
{
	return mw.process(ms);
}

void gui::init_turn()
{
	mw.init_turn();
}

void gui::handle_action(const visible_move_action& a)
{
	mw.handle_action(a);
}

