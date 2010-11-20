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

struct color {
	int r;
	int g;
	int b;
	color(int r_, int g_, int b_);
};

color::color(int r_, int g_, int b_)
	: r(r_),
	g(g_),
	b(b_)
{
}

/* doesn't update the screen. lock must be held.
 * snippet from: http://www.libsdl.org/intro.en/usingvideo.html */
void sdl_put_pixel(SDL_Surface* screen, int x, int y, const color& c)
{
	Uint32 color = SDL_MapRGB(screen->format, c.r, c.g, c.b);

	switch (screen->format->BytesPerPixel) {
		case 1: { /* Assuming 8-bpp */
				Uint8 *bufp;

				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
				*bufp = color;
			}
			break;

		case 2: { /* Probably 15-bpp or 16-bpp */
				Uint16 *bufp;

				bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
				*bufp = color;
			}
			break;

		case 3: { /* Slow 24-bpp mode, usually not used */
				Uint8 *bufp;

				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
				*(bufp+screen->format->Rshift/8) = c.r;
				*(bufp+screen->format->Gshift/8) = c.g;
				*(bufp+screen->format->Bshift/8) = c.b;
			}
			break;

		case 4: { /* Probably 32-bpp */
				Uint32 *bufp;

				bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
				*bufp = color;
			}
			break;
	}
}

color sdl_get_pixel(SDL_Surface* screen, int x, int y)
{
	Uint8 r, g, b;
	Uint32 pixel;
	switch (screen->format->BytesPerPixel) {
		case 1: { /* Assuming 8-bpp */
				Uint8 *bufp;
				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
				pixel = *bufp;
			}
			break;

		case 2: { /* Probably 15-bpp or 16-bpp */
				Uint16 *bufp;
				bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
				pixel = *bufp;
			}
			break;

		case 4: { /* Probably 32-bpp */
				Uint32 *bufp;
				bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
				pixel = *bufp;
			}
			break;
	}
	SDL_GetRGB(pixel, screen->format, &r, &g, &b);
	return color(r, g, b);
}

void sdl_change_pixel_color(SDL_Surface* screen, const color& src, const color& dst)
{
	if(screen->format->BytesPerPixel != 4)
		return;
	Uint32 src_color = SDL_MapRGB(screen->format, src.r, src.g, src.b);
	Uint32 dst_color = SDL_MapRGB(screen->format, dst.r, dst.g, dst.b);
	for(int i = 0; i < screen->h; i++) {
		for(int j = 0; j < screen->w; j++) {
			Uint32 *bufp = (Uint32*)screen->pixels + i * screen->pitch / 4 + j;
			if(*bufp == src_color)
				*bufp = dst_color;
		}
	}
}

SDL_Surface* sdl_load_image(const char* filename)
{
	SDL_Surface* img = IMG_Load(filename);
	if(!img) {
		fprintf(stderr, "Unable to load image '%s': %s\n", filename,
				IMG_GetError());
		return NULL;
	}
	else {
		SDL_Surface* opt = SDL_DisplayFormat(img);
		SDL_FreeSurface(img);
		if(!opt) {
			fprintf(stderr, "Unable to convert image '%s': %s\n",
					filename, SDL_GetError());
			return NULL;
		}
		else {
			return opt;
		}
	}
}

class unit
{
	public:
		unit(int uid, int x, int y, const color& c);
		~unit();
		int unit_id;
		int xpos;
		int ypos;
};

unit::unit(int uid, int x, int y, const color& c)
	: unit_id(uid),
	xpos(x),
	ypos(y)
{
	printf("Constructing unit\n");
}

unit::~unit()
{
	printf("Destructing unit\n");
}

class map {
	public:
		map(int x, int y, const std::vector<unit*>& units_);
		~map();
		int get_data(int x, int y) const;
		const int size_x;
		const int size_y;
	private:
		int get_index(int x, int y) const;
		int *data;
	public:
		std::vector<unit*> units;
};

map::map(int x, int y, const std::vector<unit*>& units_)
	: size_x(x),
	size_y(y),
	units(units_)
{
	this->data = new int[x * y];
	for(int i = 0; i < y; i++) {
		for(int j = 0; j < x; j++) {
			data[get_index(j, i)] = rand() % 2;
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
		gui(int x, int y, const map& mm, const std::vector<const char*>& terrain_files,
				const std::vector<const char*>& unit_files);
		~gui();
		int display() const;
		int handle_keydown(SDLKey k);
		camera cam;
	private:
		int show_terrain_image(int x, int y, int xpos, int ypos) const;
		int draw_main_map() const;
		int draw_sidebar() const;
		int draw_unit(const unit& u) const;
		color get_minimap_color(int x, int y) const;
		const map& m;
		SDL_Surface* screen;
		std::vector<SDL_Surface*> terrains;
		std::vector<SDL_Surface*> unit_images;
		const int tile_w;
		const int tile_h;
		const int screen_w;
		const int screen_h;
		const int cam_total_tiles_x;
		const int cam_total_tiles_y;
		const int sidebar_size;
};

gui::gui(int x, int y, const map& mm, const std::vector<const char*>& terrain_files,
		const std::vector<const char*>& unit_files)
	: m(mm),
	tile_w(32),
	tile_h(32),
	screen_w(x),
	screen_h(y),
	cam_total_tiles_x((screen_w + tile_w - 1) / tile_w),
	cam_total_tiles_y((screen_h + tile_h - 1) / tile_h),
	sidebar_size(4)
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
		terrains[i] = sdl_load_image(terrain_files[i]);
	}
	unit_images.resize(unit_files.size());
	for(unsigned int i = 0; i < unit_files.size(); i++) {
		unit_images[i] = sdl_load_image(unit_files[i]);
		sdl_change_pixel_color(unit_images[i], color(0, 255, 255), color(255, 0, 0));
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
	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0) {
			return 1;
		}
	}
	draw_sidebar();
	if (SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}
	draw_main_map();
	if(SDL_Flip(screen)) {
		fprintf(stderr, "Unable to flip: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

int gui::draw_sidebar() const
{
	const int minimap_w = sidebar_size * tile_w;
	const int minimap_h = sidebar_size * tile_h / 2;
	for(int i = 0; i < minimap_h; i++) {
		int y = i * m.size_y / minimap_h;
		for(int j = 0; j < minimap_w; j++) {
			int x = j * m.size_x / minimap_w;
			sdl_put_pixel(screen, j, i, get_minimap_color(x, y));
		}
	}
	SDL_UpdateRect(screen, 0, 0, minimap_w, minimap_h);
	return 0;
}

int gui::draw_unit(const unit& u) const
{
	if(u.unit_id < 0 || u.unit_id >= (int)unit_images.size()) {
		fprintf(stderr, "Image for Unit ID %d not loaded\n", u.unit_id);
		return 1;
	}
	if(in_bounds(cam.cam_x, u.xpos, cam.cam_x + cam_total_tiles_x) &&
	   in_bounds(cam.cam_y, u.ypos, cam.cam_y + cam_total_tiles_y)) {
		SDL_Rect dest;
		dest.x = (u.xpos + sidebar_size - cam.cam_x) * tile_w;
		dest.y = (u.ypos - cam.cam_y) * tile_h;
		if(SDL_BlitSurface(unit_images[u.unit_id], NULL, screen, &dest)) {
			fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
			return 1;
		}
	}
	return 0;
}

int gui::draw_main_map() const
{
	int imax = std::min(cam.cam_y + cam_total_tiles_y, m.size_y);
	int jmax = std::min(cam.cam_x + cam_total_tiles_x, m.size_x);
	for(int i = cam.cam_y, y = 0; i < imax; i++, y++) {
		for(int j = cam.cam_x, x = sidebar_size; j < jmax; j++, x++) {
			if(show_terrain_image(j, i, x, y)) {
				return 1;
			}
		}
	}
	for(std::vector<unit*>::const_iterator it = m.units.begin(); it != m.units.end(); ++it) {
		if(draw_unit(**it)) {
			return 1;
		}
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
			if(cam.cam_x < m.size_x - cam_total_tiles_x - 1)
				cam.cam_x++, redraw = true;
			break;
		case SDLK_UP:
			if(cam.cam_y > 0)
				cam.cam_y--, redraw = true;
			break;
		case SDLK_DOWN:
			if(cam.cam_y < m.size_y - cam_total_tiles_y - 1)
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

color gui::get_minimap_color(int x, int y) const
{
	if((x >= cam.cam_x && x <= cam.cam_x + cam_total_tiles_x) &&
	   (y >= cam.cam_y && y <= cam.cam_y + cam_total_tiles_y) &&
	   (x == cam.cam_x || x == cam.cam_x + cam_total_tiles_x ||
	    y == cam.cam_y || y == cam.cam_y + cam_total_tiles_y))
		return color(255, 255, 255);
	int val = m.get_data(x, y);
	if(val < 0 || val >= (int)terrains.size()) {
		fprintf(stderr, "Terrain at %d not loaded at (%d, %d)\n", val,
				x, y);
		return color(0, 0, 0);
	}
	return sdl_get_pixel(terrains[val], 16, 16);
}

int run()
{
	std::vector<unit*> units;
	units.push_back(new unit(0, 1, 1, color(255, 0, 0)));

	std::vector<const char*> terrain_files;
	std::vector<const char*> unit_files;
	terrain_files.push_back("share/terrain1.png");
	terrain_files.push_back("share/terrain2.png");
	unit_files.push_back("share/settlers.png");
	bool running = true;

	map m(64, 32, units);
	gui g(640, 480, m, terrain_files, unit_files);
	g.display();
	while(running) {
		SDL_Event event;
		SDL_WaitEvent(&event);
		switch(event.type) {
			case SDL_KEYDOWN:
				if(g.handle_keydown(event.key.keysym.sym))
					running = false;
				break;
			case SDL_QUIT:
				running = false;
			default:
				break;
		}
	}
	return 0;
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

