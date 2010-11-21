#include <stdlib.h>
#include <unistd.h>

#include <list>
#include <vector>
#include <map>
#include <utility>

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"

bool in_bounds(int a, int b, int c)
{
	return a <= b && b <= c;
}

template<typename N>
N clamp(N min, N val, N max)
{
	if(val < min)
		return min;
	if(val > max)
		return max;
	return val;
}

struct color {
	int r;
	int g;
	int b;
	color(int r_, int g_, int b_);
	bool operator<(const color& oth) const;
};

color::color(int r_, int g_, int b_)
	: r(r_),
	g(g_),
	b(b_)
{
}

bool color::operator<(const color& oth) const
{
	if(r < oth.r)
		return true;
	if(r > oth.r)
		return false;
	if(g < oth.g)
		return true;
	if(g > oth.g)
		return false;
	return b < oth.b;
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
		unit(int uid, int x, int y, const color& c_);
		~unit();
		int unit_id;
		int xpos;
		int ypos;
		color c;
};

unit::unit(int uid, int x, int y, const color& c_)
	: unit_id(uid),
	xpos(x),
	ypos(y),
	c(c_)
{
}

unit::~unit()
{
}

class civilization {
	public:
		civilization(const char* name, const color& c_);
		~civilization();
		void add_unit(int uid, int x, int y);

		const char* civname;
		color col;
		std::list<unit*> units;
};

civilization::civilization(const char* name, const color& c_)
	: civname(name),
	col(c_)
{
}

civilization::~civilization()
{
	while(!units.empty()) {
		unit* u = units.back();
		delete u;
		units.pop_back();
	}
}

void civilization::add_unit(int uid, int x, int y)
{
	units.push_back(new unit(uid, x, y, col));
}

class map {
	public:
		map(int x, int y);
		~map();
		int get_data(int x, int y) const;
		int try_move_unit(SDLKey k, unit* u);
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

class round
{
	public:
		round();
		void add_civilization(civilization* civ);
		bool next_civ();
	private:
		std::vector<civilization*> civs;
		std::vector<civilization*>::iterator current;
};

round::round()
{
	current = civs.begin();
}

void round::add_civilization(civilization* civ)
{
	civs.push_back(civ);
}

bool round::next_civ()
{
	if(civs.empty())
		return false;
	current++;
	if(current == civs.end()) {
		current = civs.begin();
		return true;
	}
	return false;
}

struct camera {
	int cam_x;
	int cam_y;
};

class gui
{
	typedef std::map<std::pair<int, color>, SDL_Surface*> UnitImageMap;
	public:
		gui(int x, int y, map& mm, std::vector<civilization*>& civs,
				const std::vector<const char*>& terrain_files,
				const std::vector<const char*>& unit_files);
		~gui();
		int display();
		int handle_keydown(SDLKey k, unit* current_unit);
		int handle_mousemotion(int x, int y);
		int process(int ms, unit* current_unit);
		camera cam;
	private:
		int show_terrain_image(int x, int y, int xpos, int ypos) const;
		int draw_main_map();
		int draw_sidebar() const;
		int draw_unit(const unit& u);
		color get_minimap_color(int x, int y) const;
		int try_move_camera(bool left, bool right, bool up, bool down);
		void center_camera_to_unit(unit* u);
		int try_center_camera_to_unit(unit* u);
		map& m;
		std::vector<civilization*>& civilizations;
		SDL_Surface* screen;
		std::vector<SDL_Surface*> terrains;
		std::vector<SDL_Surface*> plain_unit_images;
		UnitImageMap unit_images;
		const int tile_w;
		const int tile_h;
		const int screen_w;
		const int screen_h;
		const int cam_total_tiles_x;
		const int cam_total_tiles_y;
		const int sidebar_size;
		int timer;
		unit* blink_unit;
};

gui::gui(int x, int y, map& mm, std::vector<civilization*>& civs,
	       	const std::vector<const char*>& terrain_files,
		const std::vector<const char*>& unit_files)
	: m(mm),
	civilizations(civs),
	tile_w(32),
	tile_h(32),
	screen_w(x),
	screen_h(y),
	cam_total_tiles_x((screen_w + tile_w - 1) / tile_w),
	cam_total_tiles_y((screen_h + tile_h - 1) / tile_h),
	sidebar_size(4),
	timer(0),
	blink_unit(NULL)
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
	plain_unit_images.resize(unit_files.size());
	for(unsigned int i = 0; i < unit_files.size(); i++) {
		plain_unit_images[i] = sdl_load_image(unit_files[i]);
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

int gui::display()
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

int gui::draw_unit(const unit& u)
{
	if(u.unit_id < 0 || u.unit_id >= (int)plain_unit_images.size()) {
		fprintf(stderr, "Image for Unit ID %d not loaded\n", u.unit_id);
		return 1;
	}
	if(in_bounds(cam.cam_x, u.xpos, cam.cam_x + cam_total_tiles_x) &&
	   in_bounds(cam.cam_y, u.ypos, cam.cam_y + cam_total_tiles_y)) {
		SDL_Rect dest;
		dest.x = (u.xpos + sidebar_size - cam.cam_x) * tile_w;
		dest.y = (u.ypos - cam.cam_y) * tile_h;
		SDL_Surface* surf = NULL;
		UnitImageMap::const_iterator it = unit_images.find(std::make_pair(u.unit_id, u.c));
		if(it == unit_images.end()) {
			// load image
			SDL_Surface* plain = plain_unit_images[u.unit_id];
			SDL_Surface* result = SDL_DisplayFormat(plain);
			sdl_change_pixel_color(result, color(0, 255, 255), u.c);
			unit_images.insert(std::make_pair(std::make_pair(u.unit_id, u.c), result));
			surf = result;
		}
		else
			surf = it->second;

		if(SDL_BlitSurface(surf, NULL, screen, &dest)) {
			fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
			return 1;
		}
	}
	return 0;
}

int gui::draw_main_map()
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
	for(int i = 0; i < (int)civilizations.size(); i++) {
		for(std::list<unit*>::const_iterator it = civilizations[i]->units.begin(); 
		    it != civilizations[i]->units.end();
		    ++it) {
			if(blink_unit == *it)
				continue;
			if(draw_unit(**it)) {
				return 1;
			}
		}
	}
	return 0;
}

int gui::try_move_camera(bool left, bool right, bool up, bool down)
{
	bool redraw = false;
	if(left) {
		if(cam.cam_x > 0)
			cam.cam_x--, redraw = true;
	}
	else if(right) {
		if(cam.cam_x < m.size_x - cam_total_tiles_x)
			cam.cam_x++, redraw = true;
	}
	if(up) {
		if(cam.cam_y > 0)
			cam.cam_y--, redraw = true;
	}
	else if(down) {
		if(cam.cam_y < m.size_y - cam_total_tiles_y)
			cam.cam_y++, redraw = true;
	}
	if(redraw)
		display();
	return redraw;
}

int map::try_move_unit(SDLKey k, unit* u)
{
	if(!u)
		return 0;
	int chx = 0, chy = 0;
	switch(k) {
		case SDLK_KP4:
			chx = -1;
			break;
		case SDLK_KP6:
			chx = 1;
			break;
		case SDLK_KP8:
			chy = -1;
			break;
		case SDLK_KP2:
			chy = 1;
			break;
		case SDLK_KP1:
			chx = -1;
			chy = 1;
			break;
		case SDLK_KP3:
			chx = 1;
			chy = 1;
			break;
		case SDLK_KP7:
			chx = -1;
			chy = -1;
			break;
		case SDLK_KP9:
			chx = 1;
			chy = -1;
			break;
		default:
			break;
	}
	if(chx || chy) {
		if(get_data(u->xpos + chx, u->ypos + chy) > 0) {
			u->xpos += chx;
			u->ypos += chy;
			return 1;
		}
	}
	return 0;
}

void gui::center_camera_to_unit(unit* u)
{
	cam.cam_x = clamp(0, u->xpos - (-sidebar_size + cam_total_tiles_x) / 2, m.size_x - cam_total_tiles_x);
	cam.cam_y = clamp(0, u->ypos - cam_total_tiles_y / 2, m.size_y - cam_total_tiles_y);
}

int gui::try_center_camera_to_unit(unit* u)
{
	const int border = 3;
	if(!in_bounds(cam.cam_x + border, u->xpos, cam.cam_x - sidebar_size + cam_total_tiles_x - border) ||
	   !in_bounds(cam.cam_y + border, u->ypos, cam.cam_y + cam_total_tiles_y - border)) {
		center_camera_to_unit(u);
		return true;
	}
	return false;
}

int gui::handle_keydown(SDLKey k, unit* current_unit)
{
	if(k == SDLK_ESCAPE || k == SDLK_q)
		return 1;
	else if(k == SDLK_LEFT || k == SDLK_RIGHT || k == SDLK_UP || k == SDLK_DOWN)
		try_move_camera(k == SDLK_LEFT, k == SDLK_RIGHT, k == SDLK_UP, k == SDLK_DOWN);
	else if(current_unit != NULL) {
		if(k == SDLK_c) {
			center_camera_to_unit(current_unit);
			display();
		}
		else if(m.try_move_unit(k, current_unit)) {
			try_center_camera_to_unit(current_unit);
			display();
		}
	}
	return 0;
}

color gui::get_minimap_color(int x, int y) const
{
	if((x >= cam.cam_x && x <= cam.cam_x + cam_total_tiles_x - 1) &&
	   (y >= cam.cam_y && y <= cam.cam_y + cam_total_tiles_y - 1) &&
	   (x == cam.cam_x || x == cam.cam_x + cam_total_tiles_x - 1 ||
	    y == cam.cam_y || y == cam.cam_y + cam_total_tiles_y - 1))
		return color(255, 255, 255);
	int val = m.get_data(x, y);
	if(val < 0 || val >= (int)terrains.size()) {
		fprintf(stderr, "Terrain at %d not loaded at (%d, %d)\n", val,
				x, y);
		return color(0, 0, 0);
	}
	return sdl_get_pixel(terrains[val], 16, 16);
}

int gui::handle_mousemotion(int x, int y)
{
	const int border = tile_w;
	try_move_camera(x >= sidebar_size * tile_w && x < sidebar_size * tile_w + border,
			x > screen_w - border,
			y < border,
			y > screen_h - border);
	return 0;
}

int gui::process(int ms, unit* current_unit)
{
	int old_timer = timer;
	timer += ms;
	unit* old_blink_unit = blink_unit;
	if(timer % 1000 > 700) {
			blink_unit = current_unit;
	}
	else {
		blink_unit = NULL;
	}
	if(blink_unit != old_blink_unit)
		display();
	if(old_timer / 200 != timer / 200) {
		int x, y;
		SDL_GetMouseState(&x, &y);
		handle_mousemotion(x, y);
	}
	return 0;
}

int run()
{
	std::vector<civilization*> civilizations;
	civilization* civ1 = new civilization("Babylonians", color(255, 0, 0));
	civ1->add_unit(0, 1, 1);
	unit* current_unit = civ1->units.front();
	civilizations.push_back(civ1);

	std::vector<const char*> terrain_files;
	std::vector<const char*> unit_files;
	terrain_files.push_back("share/terrain1.png");
	terrain_files.push_back("share/terrain2.png");
	unit_files.push_back("share/settlers.png");
	bool running = true;

	map m(64, 32);
	gui g(1024, 768, m, civilizations, terrain_files, unit_files);
	g.display();
	while(running) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					if(g.handle_keydown(event.key.keysym.sym, current_unit))
						running = false;
					break;
				case SDL_QUIT:
					running = false;
				default:
					break;
			}
		}
		SDL_Delay(50);
		g.process(50, current_unit);
	}
	delete civ1;
	civilizations.pop_back();
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

