#include <stdlib.h>
#include <unistd.h>

#include <vector>
#include <utility>

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"

bool in_bounds(int a, int b, int c)
{
	return a <= b && b <= c;
}

class map {
	public:
		map(int x, int y);
		~map();
		int get_data(int x, int y) const;
		const int size_x;
		const int size_y;
	private:
		int get_index(int x, int y) const;
		int *data;
};

map::map(int x, int y)
	: size_x(x),
	size_y(y)
{
	this->data = new int[x * y];
	for(int i = 0; i < y; i++) {
		for(int j = 0; j < x; j++) {
			data[get_index(j, i)] = (i * j) % 5 % 2;
		}
	}
}

map::~map()
{
	delete[] this->data;
}

int map::get_index(int x, int y) const
{
	return y * size_x + x;
}

int map::get_data(int x, int y) const
{
	if(!in_bounds(0, x, size_x - 1) || !in_bounds(0, y, size_y - 1))
		return -1;
	return data[get_index(x, y)];
}

struct camera {
	int cam_x;
	int cam_y;
};

class gui
{
	public:
		gui(int x, int y, const map& mm, const std::vector<const char*>& terrain_files);
		~gui();
		int display() const;
		int handle_keydown(SDLKey k);
		camera cam;
	private:
		int show_terrain_image(int x, int y, int xpos, int ypos) const;
		const map& m;
		SDL_Surface* screen;
		std::vector<SDL_Surface*> terrains;
		const int tile_w;
		const int tile_h;
		const int screen_w;
		const int screen_h;
		const int total_tiles_x;
		const int total_tiles_y;
};

gui::gui(int x, int y, const map& mm, const std::vector<const char*>& terrain_files)
	: m(mm),
	tile_w(32),
	tile_h(32),
	screen_w(x),
	screen_h(y),
	total_tiles_x((screen_w + tile_w - 1) / tile_w),
	total_tiles_y((screen_h + tile_h - 1) / tile_h)
{
	screen = SDL_SetVideoMode(x, y, 32, SDL_SWSURFACE);
	if (!screen) {
		fprintf(stderr, "Unable to set %dx%d video: %s\n", x, y, SDL_GetError());
		return;
	}
	if(screen->locked) {
		fprintf(stderr, "screen is locked\n");
		SDL_UnlockSurface(screen);
	}
	terrains.resize(terrain_files.size());
	for(unsigned int i = 0; i < terrain_files.size(); i++) {
		terrains[i] = IMG_Load(terrain_files[i]);
		if(!terrains[i]) {
			fprintf(stderr, "Unable to load terrain '%s': %s\n", terrain_files[i],
				       	IMG_GetError());
		}
	}
	cam.cam_x = cam.cam_y = 0;
}

gui::~gui()
{
	for(unsigned int i = 0; i < terrains.size(); i++) {
		SDL_FreeSurface(terrains[i]);
	}
}

int gui::show_terrain_image(int x, int y, int xpos, int ypos) const
{
	SDL_Rect dest;
	dest.x = xpos * tile_w;
	dest.y = ypos * tile_h;
	int val = m.get_data(x, y);
	if(val < 0 || val >= (int)terrains.size()) {
		fprintf(stderr, "Terrain at %d not loaded at (%d, %d)\n", val,
				x, y);
		return 1;
	}
	if(SDL_BlitSurface(terrains[val], NULL, screen, &dest)) {
		fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

int gui::display() const
{
	int imax = std::min(cam.cam_y + total_tiles_y, m.size_y);
	int jmax = std::min(cam.cam_x + total_tiles_x, m.size_x);
	for(int i = cam.cam_y, y = 0; i < imax; i++, y++) {
		for(int j = cam.cam_x, x = 0; j < jmax; j++, x++) {
			if(show_terrain_image(j, i, x, y)) {
				return 1;
			}
		}
	}
	if(SDL_Flip(screen)) {
		fprintf(stderr, "Unable to flip: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

int gui::handle_keydown(SDLKey k)
{
	bool redraw = false;
	switch(k) {
		case SDLK_LEFT:
			if(cam.cam_x > 0)
				cam.cam_x--, redraw = true;
			break;
		case SDLK_RIGHT:
			if(cam.cam_x < m.size_x - total_tiles_x - 1)
				cam.cam_x++, redraw = true;
			break;
		case SDLK_UP:
			if(cam.cam_y > 0)
				cam.cam_y--, redraw = true;
			break;
		case SDLK_DOWN:
			if(cam.cam_y < m.size_y - total_tiles_y - 1)
				cam.cam_y++, redraw = true;
			break;
		case SDLK_ESCAPE:
		case SDLK_q:
			return 1;
		default:
			break;
	}
	if(redraw)
		display();
	return 0;
}

int run()
{
	std::vector<const char*> terrain_files;
	terrain_files.push_back("share/terrain1.png");
	terrain_files.push_back("share/terrain2.png");
	map m(64, 64);
	gui g(640, 480, m, terrain_files);
	g.display();
	while(1) {
		SDL_Event event;
		SDL_WaitEvent(&event);
		switch(event.type) {
			case SDL_KEYDOWN:
				if(g.handle_keydown(event.key.keysym.sym))
					return 0;
				break;
			case SDL_QUIT:
				return 0;
			default:
				break;
		}
	}
}

int main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	if(!IMG_Init(IMG_INIT_PNG)) {
		fprintf(stderr, "Unable to init SDL_Image: %s\n", IMG_GetError());
	}
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	run();

	SDL_Quit();
	return 0;
}


