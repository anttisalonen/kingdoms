#include "mapview.h"

mapview::mapview(SDL_Surface* screen_, map& mm, pompelmous& rr,
		const gui_resource_files& resfiles,
		const TTF_Font& font_)
	: screen(screen_),
	data(gui_data(mm, rr)),
	res(font_, 32, 32, sdl_load_image(resfiles.food_icon.c_str()), 
			sdl_load_image(resfiles.prod_icon.c_str()), 
			sdl_load_image(resfiles.comm_icon.c_str()))
{
	// terrain files
	res.terrains.textures.resize(resfiles.terrains.size());
	for(unsigned int i = 0; i < resfiles.terrains.size(); i++) {
		res.terrains.textures[i] = sdl_load_image(resfiles.terrains[i].c_str());
	}

	// terrain overlays
	res.terrains.irrigation_overlay = sdl_load_image(resfiles.irrigation.c_str());
	res.terrains.mine_overlay = sdl_load_image(resfiles.mine.c_str());
	for(unsigned int i = 0; i < 9; i++) {
		res.terrains.road_overlays[i] = resfiles.roads.size() > i ? 
			sdl_load_image(resfiles.roads[i].c_str()) : NULL;
	}

	for(unsigned int i = 0; i < 5; i++) {
		res.terrains.river_overlays[i] = resfiles.rivers.size() > i ? 
			sdl_load_image(resfiles.rivers[i].c_str()) : NULL;
	}

	// unit images
	res.plain_unit_images.resize(rr.uconfmap.size());
	for(unsigned int i = 0; i < resfiles.units.size(); i++) {
		res.plain_unit_images[i] = sdl_load_image(resfiles.units[i].c_str());
	}
	if(resfiles.units.size() < rr.uconfmap.size()) {
		if(!resfiles.empty_unit.empty()) {
			for(unsigned int i = resfiles.units.size(); i < rr.uconfmap.size(); i++) {
				const unit_configuration* uconf = rr.get_unit_configuration(i);
				if(uconf) {
					fprintf(stderr, "Note: no graphics available for Unit %s - using the default.\n",
							uconf->unit_name.c_str());
					res.plain_unit_images[i] = sdl_load_image(resfiles.empty_unit.c_str());
				}
				else {
					fprintf(stderr, "Warning: no unit configuration for ID %d.\n",
							i);
				}
			}
		}
		else {
			fprintf(stderr, "Warning: units with ID >= %lu have no graphics, and no default image file is defined.\n",
					resfiles.units.size());
		}
	}

	// city images (one for each civ)
	res.city_images.resize(rr.civs.size());
	for(unsigned int i = 0; i < rr.civs.size(); i++) {
		SDL_Surface* plain = sdl_load_image(resfiles.city.c_str());
		res.city_images[i] = SDL_DisplayFormat(plain);
		SDL_FreeSurface(plain);
		sdl_change_pixel_color(res.city_images[i], color(0, 255, 255), data.r.civs[i]->col);
	}

	// resource images
	for(unsigned int i = 0; i < resfiles.resources.size(); i++) {
		res.resource_images[i + 1] = sdl_load_image(resfiles.resources[i].c_str());
	}
}

mapview::~mapview()
{
	SDL_FreeSurface(res.terrains.irrigation_overlay);
	SDL_FreeSurface(res.terrains.mine_overlay);
	for(unsigned int i = 0; i < 9; i++) {
		if(res.terrains.road_overlays[i])
			SDL_FreeSurface(res.terrains.road_overlays[i]);
	}
	for(unsigned int i = 0; i < 5; i++) {
		SDL_FreeSurface(res.terrains.river_overlays[i]);
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

int mapview::display()
{
	return 0;
}

int mapview::handle_input(const SDL_Event& ev)
{
	return 1;
}

