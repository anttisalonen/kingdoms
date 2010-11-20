#include <stdlib.h>
#include <unistd.h>

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
			data[get_index(j, i)] = 1;
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
		gui(int x, int y);
		~gui();
		int display(const map& m);
		camera cam;
	private:
		int show_terrain_image(int x, int y, int val);
		SDL_Surface* screen;
		SDL_Surface* terrain1;
		const int tile_w;
		const int tile_h;
		const int screen_w;
		const int screen_h;
};

gui::gui(int x, int y)
	: tile_w(32),
	tile_h(32),
	screen_w(x),
	screen_h(y)
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
	terrain1 = IMG_Load("share/terrain1.png");
	if (!terrain1) {
		fprintf(stderr, "Unable to load terrain1: %s\n", IMG_GetError());
	}
	cam.cam_x = cam.cam_y = 0;
}

gui::~gui()
{
	SDL_FreeSurface(terrain1);
}

int gui::show_terrain_image(int x, int y, int val)
{
	SDL_Rect dest;
	dest.x = x * tile_w;
	dest.y = y * tile_h;
	if(SDL_BlitSurface(terrain1, NULL, screen, &dest)) {
		fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

int gui::display(const map& m)
{
	for(int i = cam.cam_y; i < m.size_y; i++) {
		for(int j = cam.cam_x; j < m.size_x; j++) {
			if(show_terrain_image(i, j, m.get_data(i, j))) {
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

int run()
{
	gui g(640, 480);
	map m(20, 20);

	g.display(m);
	sleep(2);
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
	run();

	SDL_Quit();
	return 0;
}


