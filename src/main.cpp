#include <stdlib.h>
#include <unistd.h>

#include <list>
#include <vector>
#include <map>
#include <utility>

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

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

template<typename N>
class buf2d {
	public:
		buf2d(int x, int y);
		~buf2d();
		const N* get(int x, int y) const;
		void set(int x, int y, const N& val);
		const int size_x;
		const int size_y;
	private:
		int get_index(int x, int y) const;
		N* data;
};

template<typename N>
buf2d<N>::buf2d(int x, int y)
	: size_x(x),
	size_y(y)
{
	this->data = new N[x * y];
}

template<typename N>
buf2d<N>::~buf2d()
{
	delete[] this->data;
}

template<typename N>
int buf2d<N>::get_index(int x, int y) const
{
	return y * size_x + x;
}

template<typename N>
void buf2d<N>::set(int x, int y, const N& val)
{
	if(!in_bounds(0, x, size_x - 1) || !in_bounds(0, y, size_y - 1))
		return;
	data[get_index(x, y)] = val;
}

template<typename N>
const N* buf2d<N>::get(int x, int y) const
{
	if(!in_bounds(0, x, size_x - 1) || !in_bounds(0, y, size_y - 1))
		return NULL;
	return &data[get_index(x, y)];
}

class unit_configuration {
	public:
		const char* unit_name;
		unsigned int max_moves;
};

typedef std::map<int, unit_configuration*> unit_configuration_map;

class unit
{
	public:
		unit(int uid, int x, int y, const color& c_);
		~unit();
		void refill_moves(unsigned int m);
		int unit_id;
		int xpos;
		int ypos;
		color c;
		unsigned int moves;
};

unit::unit(int uid, int x, int y, const color& c_)
	: unit_id(uid),
	xpos(x),
	ypos(y),
	c(c_),
	moves(0)
{
}

unit::~unit()
{
}

void unit::refill_moves(unsigned int m)
{
	moves = m;
}

class fog_of_war {
	public:
		fog_of_war(int x, int y);
		void reveal(int x, int y, int radius);
		void shade(int x, int y, int radius);
		char get_value(int x, int y) const;
	private:
		int get_refcount(int x, int y) const;
		int get_raw(int x, int y) const;
		void set_value(int x, int y, int val);
		void up_refcount(int x, int y);
		void down_refcount(int x, int y);
		buf2d<int> fog;
};

fog_of_war::fog_of_war(int x, int y)
	: fog(buf2d<int>(x, y))
{
}

void fog_of_war::reveal(int x, int y, int radius)
{
	for(int i = x - radius; i <= x + radius; i++) {
		for(int j = y - radius; j <= y + radius; j++) {
			up_refcount(i, j);
			set_value(i, j, 2);
		}
	}
}

void fog_of_war::shade(int x, int y, int radius)
{
	for(int i = x - radius; i <= x + radius; i++) {
		for(int j = y - radius; j <= y + radius; j++) {
			down_refcount(i, j);
			if(get_refcount(i, j) == 0)
				set_value(i, j, 1);
		}
	}
}

char fog_of_war::get_value(int x, int y) const
{
	return get_raw(x, y) & 3;
}

int fog_of_war::get_refcount(int x, int y) const
{
	return get_raw(x, y) >> 2;
}

int fog_of_war::get_raw(int x, int y) const
{
	const int* i = fog.get(x, y);
	if(!i)
		return 0;
	return *i;
}

void fog_of_war::up_refcount(int x, int y)
{
	int i = get_refcount(x, y);
	int v = get_value(x, y);
	fog.set(x, y, ((i + 1) << 2) | v);
}

void fog_of_war::down_refcount(int x, int y)
{
	int i = get_refcount(x, y);
	if(i == 0)
		return;
	int v = get_value(x, y);
	fog.set(x, y, ((i - 1) << 2) | v);
}

void fog_of_war::set_value(int x, int y, int val)
{
	int i = get_raw(x, y);
	i &= ~3;
	fog.set(x, y, i | val); 
}

class map {
	public:
		map(int x, int y);
		int get_data(int x, int y) const;
		int size_x() const;
		int size_y() const;
	private:
		int get_index(int x, int y) const;
		buf2d<int> data;
};

map::map(int x, int y)
	: data(buf2d<int>(x, y))
{
	for(int i = 0; i < y; i++) {
		for(int j = 0; j < x; j++) {
			data.set(j, i, rand() % 2);
		}
	}
}

int map::get_data(int x, int y) const
{
	const int* v = data.get(x, y);
	if(!v)
		return -1;
	return *v;
}

int map::size_x() const
{
	return data.size_x;
}

int map::size_y() const
{
	return data.size_y;
}

class civilization {
	public:
		civilization(const char* name, const color& c_, const map& m_);
		~civilization();
		void add_unit(int uid, int x, int y);
		int try_move_unit(unit* u, int chx, int chy);
		void refill_moves(const unit_configuration_map& uconfmap);
		char fog_at(int x, int y) const;

		const char* civname;
		color col;
		std::list<unit*> units;
		const map& m;
		fog_of_war fog;
};

civilization::civilization(const char* name, const color& c_, const map& m_)
	: civname(name),
	col(c_),
	m(m_),
	fog(fog_of_war(m_.size_x(), m_.size_y()))
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
	fog.reveal(x, y, 1);
}

void civilization::refill_moves(const unit_configuration_map& uconfmap)
{
	for(std::list<unit*>::iterator uit = units.begin();
		uit != units.end();
		++uit) {
		unit_configuration_map::const_iterator max_moves_it = uconfmap.find((*uit)->unit_id);
		int max_moves;
		if(max_moves_it == uconfmap.end())
			max_moves = 0;
		else
			max_moves = max_moves_it->second->max_moves;
		(*uit)->refill_moves(max_moves);
	}
}

int civilization::try_move_unit(unit* u, int chx, int chy)
{
	if((!chx && !chy) || !u || !u->moves)
		return 0;
	if(m.get_data(u->xpos + chx, u->ypos + chy) > 0) {
		fog.shade(u->xpos, u->ypos, 1);
		u->xpos += chx;
		u->ypos += chy;
		u->moves--;
		fog.reveal(u->xpos, u->ypos, 1);
		return 1;
	}
	return 0;
}

char civilization::fog_at(int x, int y) const
{
	return fog.get_value(x, y);
}

class round
{
	public:
		round(const unit_configuration_map& uconfmap_);
		void add_civilization(civilization* civ);
		bool next_civ();
		std::vector<civilization*> civs;
		std::vector<civilization*>::iterator current_civ;
		const unit_configuration* get_unit_configuration(int uid) const;
	private:
		const unit_configuration_map& uconfmap;
		void refill_moves();
};

round::round(const unit_configuration_map& uconfmap_)
	: uconfmap(uconfmap_)
{
	current_civ = civs.begin();
}

void round::add_civilization(civilization* civ)
{
	civs.push_back(civ);
	current_civ = civs.begin();
	refill_moves();
}

void round::refill_moves()
{
	for(std::vector<civilization*>::iterator it = civs.begin();
	    it != civs.end();
	    ++it) {
		(*it)->refill_moves(uconfmap);
	}
}

bool round::next_civ()
{
	if(civs.empty())
		return false;
	current_civ++;
	if(current_civ == civs.end()) {
		current_civ = civs.begin();
		refill_moves();
		return true;
	}
	return false;
}

const unit_configuration* round::get_unit_configuration(int uid) const
{
	unit_configuration_map::const_iterator it = uconfmap.find(uid);
	if(it == uconfmap.end())
		return NULL;
	return it->second;
}

struct camera {
	int cam_x;
	int cam_y;
};

class gui
{
	typedef std::map<std::pair<int, color>, SDL_Surface*> UnitImageMap;
	public:
		gui(int x, int y, map& mm, round& rr,
				const std::vector<const char*>& terrain_files,
				const std::vector<const char*>& unit_files,
				const TTF_Font& font_);
		~gui();
		int display(const unit* current_unit);
		int redraw_main_map();
		int handle_keydown(SDLKey k, SDLMod mod, std::list<unit*>::iterator& current_unit);
		int handle_mousemotion(int x, int y);
		int process(int ms, const unit* current_unit);
		camera cam;
	private:
		int show_terrain_image(int x, int y, int xpos, int ypos, bool shade) const;
		int draw_main_map();
		int draw_sidebar(const unit* current_unit) const;
		int draw_unit(const unit& u);
		color get_minimap_color(int x, int y) const;
		int try_move_camera(bool left, bool right, bool up, bool down);
		void center_camera_to_unit(unit* u);
		int try_center_camera_to_unit(unit* u);
		int draw_minimap() const;
		int draw_civ_info() const;
		int draw_unit_info(const unit* current_unit) const;
		int draw_eot() const;
		int draw_text(const char* str, int x, int y, int r, int g, int b) const;
		int clear_sidebar() const;
		int clear_main_map() const;
		void numpad_to_move(SDLKey k, int* chx, int* chy) const;
		map& m;
		round& r;
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
		const unit* blink_unit;
		const TTF_Font& font;
};

gui::gui(int x, int y, map& mm, round& rr,
	       	const std::vector<const char*>& terrain_files,
		const std::vector<const char*>& unit_files,
		const TTF_Font& font_)
	: m(mm),
	r(rr),
	tile_w(32),
	tile_h(32),
	screen_w(x),
	screen_h(y),
	cam_total_tiles_x((screen_w + tile_w - 1) / tile_w),
	cam_total_tiles_y((screen_h + tile_h - 1) / tile_h),
	sidebar_size(4),
	timer(0),
	blink_unit(NULL),
	font(font_)
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

int gui::show_terrain_image(int x, int y, int xpos, int ypos, bool shade) const
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
	if(shade) {
		color c(0, 0, 0);
		for(int i = dest.y; i < dest.y + tile_h; i++) {
			for(int j = dest.x; j < dest.x + tile_w; j++) {
				if((i % 2) == (j % 2)) {
					sdl_put_pixel(screen, j, i, c);
				}
			}
		}
		SDL_UpdateRect(screen, dest.x, dest.y, tile_w, tile_h);
	}
	return 0;
}

int gui::display(const unit* current_unit)
{
	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0) {
			return 1;
		}
	}
	draw_sidebar(current_unit);
	if (SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}
	return redraw_main_map();
}

int gui::redraw_main_map()
{
	clear_main_map();
	draw_main_map();
	if(SDL_Flip(screen)) {
		fprintf(stderr, "Unable to flip: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

int gui::clear_main_map() const
{
	SDL_Rect dest;
	dest.x = sidebar_size * tile_w;
	dest.y = 0;
	dest.w = screen_w - sidebar_size * tile_w;
	dest.h = screen_h;
	Uint32 color = SDL_MapRGB(screen->format, 0, 0, 0);
	SDL_FillRect(screen, &dest, color);
	return 0;
}

int gui::clear_sidebar() const
{
	SDL_Rect dest;
	dest.x = 0;
	dest.y = 0;
	dest.w = sidebar_size * tile_w;
	dest.h = screen_h;
	Uint32 color = SDL_MapRGB(screen->format, 0, 0, 0);
	SDL_FillRect(screen, &dest, color);
	return 0;
}

int gui::draw_sidebar(const unit* current_unit) const
{
	clear_sidebar();
	draw_minimap();
	draw_civ_info();
	if(current_unit)
		draw_unit_info(current_unit);
	else
		draw_eot();
	return 0;
}

int gui::draw_minimap() const
{
	const int minimap_w = sidebar_size * tile_w;
	const int minimap_h = sidebar_size * tile_h / 2;
	for(int i = 0; i < minimap_h; i++) {
		int y = i * m.size_y() / minimap_h;
		for(int j = 0; j < minimap_w; j++) {
			int x = j * m.size_x() / minimap_w;
			color c = get_minimap_color(x, y);
			if((c.r == 255 && c.g == 255 && c.b == 255) || (*r.current_civ)->fog_at(x, y) > 0) {
				sdl_put_pixel(screen, j, i, c);
			}
		}
	}
	SDL_UpdateRect(screen, 0, 0, minimap_w, minimap_h);

	return 0;
}

int gui::draw_civ_info() const
{
	return draw_text((*r.current_civ)->civname, 10, sidebar_size * tile_h / 2 + 40, 255, 255, 255);
}

int gui::draw_unit_info(const unit* current_unit) const
{
	if(!current_unit)
		return 0;
	const unit_configuration* uconf = r.get_unit_configuration(current_unit->unit_id);
	if(!uconf)
		return 1;
	draw_text(uconf->unit_name, 10, sidebar_size * tile_h / 2 + 60, 255, 255, 255);
	char buf[256];
	buf[255] = '\0';
	snprintf(buf, 255, "Moves: %-2d/%2d", current_unit->moves, uconf->max_moves);
	draw_text(buf, 10, sidebar_size * tile_h / 2 + 80, 255, 255, 255);
	return 0;
}

int gui::draw_text(const char* str, int x, int y, int r, int g, int b) const
{
	if(!str)
		return 0;
	SDL_Surface* text;
	SDL_Color color = {r, g, b};
	text = TTF_RenderUTF8_Blended((TTF_Font*)&font, str, color);
	if(!text) {
		fprintf(stderr, "Could not render text: %s\n",
				TTF_GetError());
		return 1;
	}
	else {
		SDL_Rect dest;
		dest.x = x;
		dest.y = y;
		SDL_BlitSurface(text, NULL, screen, &dest);
		SDL_FreeSurface(text);
		return 0;
	}
}

int gui::draw_eot() const
{
	return draw_text("End of turn", 10, screen_h - 100, 255, 255, 255);
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
	int imax = std::min(cam.cam_y + cam_total_tiles_y, m.size_y());
	int jmax = std::min(cam.cam_x + cam_total_tiles_x, m.size_x());
	for(int i = cam.cam_y, y = 0; i < imax; i++, y++) {
		for(int j = cam.cam_x, x = sidebar_size; j < jmax; j++, x++) {
			char fog = (*r.current_civ)->fog_at(j, i);
			if(fog > 0) {
				if(show_terrain_image(j, i, x, y, fog == 1)) {
					return 1;
				}
			}
		}
	}
	for(std::vector<civilization*>::iterator cit = r.civs.begin();
			cit != r.civs.end();
			++cit) {
		for(std::list<unit*>::const_iterator it = (*cit)->units.begin(); 
				it != (*cit)->units.end();
				++it) {
			if(blink_unit == *it)
				continue;
			if((*r.current_civ)->fog_at((*it)->xpos, (*it)->ypos) == 2) {
				if(draw_unit(**it)) {
					return 1;
				}
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
		if(cam.cam_x < m.size_x() - cam_total_tiles_x)
			cam.cam_x++, redraw = true;
	}
	if(up) {
		if(cam.cam_y > 0)
			cam.cam_y--, redraw = true;
	}
	else if(down) {
		if(cam.cam_y < m.size_y() - cam_total_tiles_y)
			cam.cam_y++, redraw = true;
	}
	if(redraw) {
		redraw_main_map();
	}
	return redraw;
}

void gui::center_camera_to_unit(unit* u)
{
	cam.cam_x = clamp(0, u->xpos - (-sidebar_size + cam_total_tiles_x) / 2, m.size_x() - cam_total_tiles_x);
	cam.cam_y = clamp(0, u->ypos - cam_total_tiles_y / 2, m.size_y() - cam_total_tiles_y);
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

void gui::numpad_to_move(SDLKey k, int* chx, int* chy) const
{
	*chx = 0; *chy = 0;
	switch(k) {
		case SDLK_KP4:
			*chx = -1;
			break;
		case SDLK_KP6:
			*chx = 1;
			break;
		case SDLK_KP8:
			*chy = -1;
			break;
		case SDLK_KP2:
			*chy = 1;
			break;
		case SDLK_KP1:
			*chx = -1;
			*chy = 1;
			break;
		case SDLK_KP3:
			*chx = 1;
			*chy = 1;
			break;
		case SDLK_KP7:
			*chx = -1;
			*chy = -1;
			break;
		case SDLK_KP9:
			*chx = 1;
			*chy = -1;
			break;
		default:
			break;
	}
}

int gui::handle_keydown(SDLKey k, SDLMod mod, std::list<unit*>::iterator& current_unit)
{
	if(k == SDLK_ESCAPE || k == SDLK_q)
		return 1;
	else if(k == SDLK_LEFT || k == SDLK_RIGHT || k == SDLK_UP || k == SDLK_DOWN) {
		try_move_camera(k == SDLK_LEFT, k == SDLK_RIGHT, k == SDLK_UP, k == SDLK_DOWN);
	}
	else if((k == SDLK_RETURN || k == SDLK_KP_ENTER) && 
			(current_unit == (*r.current_civ)->units.end() || (mod & (KMOD_LSHIFT | KMOD_RSHIFT)))) {
		// end of turn for this civ
		r.next_civ();
		current_unit = (*r.current_civ)->units.begin();
		display(*current_unit);
	}
	else if(current_unit != (*r.current_civ)->units.end()) {
		if(k == SDLK_c) {
			center_camera_to_unit(*current_unit);
			redraw_main_map();
		}
		else {
			int chx, chy;
			numpad_to_move(k, &chx, &chy);
			if((chx || chy) && ((*r.current_civ)->try_move_unit(*current_unit, chx, chy))) {
				if((*current_unit)->moves == 0) {
					// no moves left
					current_unit++;
				}
				if(current_unit != (*r.current_civ)->units.end()) {
					try_center_camera_to_unit(*current_unit);
					display(*current_unit);
				}
				else {
					display(NULL);
				}
			}
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

int gui::process(int ms, const unit* current_unit)
{
	int old_timer = timer;
	timer += ms;
	const unit* old_blink_unit = blink_unit;
	if(timer % 1000 < 300) {
			blink_unit = current_unit;
	}
	else {
		blink_unit = NULL;
	}
	if(blink_unit != old_blink_unit)
		redraw_main_map();
	if(old_timer / 200 != timer / 200) {
		int x, y;
		SDL_GetMouseState(&x, &y);
		handle_mousemotion(x, y);
	}
	return 0;
}

int run()
{
	const int map_x = 64;
	const int map_y = 32;
	map m(map_x, map_y);
	civilization* civ1 = new civilization("Babylonians", color(255, 0, 0), m);
	civilization* civ2 = new civilization("Egyptians", color(255, 255, 0), m);
	unit_configuration settlers_conf;
	settlers_conf.max_moves = 1;
	settlers_conf.unit_name = "Settlers";
	unit_configuration_map uconfmap;
	uconfmap.insert(std::make_pair(0, &settlers_conf));
	round r(uconfmap);
	civ1->add_unit(0, 1, 1);
	civ1->add_unit(0, 2, 2);
	civ2->add_unit(0, 7, 6);
	civ2->add_unit(0, 7, 7);
	r.add_civilization(civ1);
	r.add_civilization(civ2);
	std::list<unit*>::iterator current_unit = (*r.current_civ)->units.begin();

	std::vector<const char*> terrain_files;
	std::vector<const char*> unit_files;
	terrain_files.push_back("share/terrain1.png");
	terrain_files.push_back("share/terrain2.png");
	unit_files.push_back("share/settlers.png");
	bool running = true;
	TTF_Font* font;
	font = TTF_OpenFont("share/DejaVuSans.ttf", 12);
	if(!font) {
		fprintf(stderr, "Could not open font: %s\n", TTF_GetError());
	}

	gui g(1024, 768, m, r, terrain_files, unit_files, *font);
	g.display(*current_unit);
	while(running) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					if(g.handle_keydown(event.key.keysym.sym, event.key.keysym.mod,
								current_unit))
						running = false;
					break;
				case SDL_QUIT:
					running = false;
				default:
					break;
			}
		}
		SDL_Delay(50);
		g.process(50, *current_unit);
	}
	TTF_CloseFont(font);
	delete civ2;
	delete civ1;
	return 0;
}

int main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	if(!IMG_Init(IMG_INIT_PNG)) {
		fprintf(stderr, "Unable to init SDL_image: %s\n", IMG_GetError());
	}
	if(!TTF_Init()) {
		fprintf(stderr, "Unable to init SDL_ttf: %s\n", TTF_GetError());
	}
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	run();

	TTF_Quit();
	SDL_Quit();
	return 0;
}

